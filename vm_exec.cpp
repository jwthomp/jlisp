#include "vm_exec.h"

#include "value_helpers.h"
#include "err.h"
#include "assert.h"
#include "lambda.h"
#include "symbols.h"
#include "compile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#define NUM_HARDWARE_THREADS 1

typedef struct vm_exec_proc_s
{
	int m_hardware_thread_affinity;
	int m_count;
	value_t *m_vm_list;
	pthread_t m_pid;
	struct vm_exec_proc_s *m_next;
} vm_exec_proc_t;


static pthread_key_t g_thread_key;
static vm_exec_proc_t *g_exec_procs = NULL;
bool g_step_debug = false;
value_t *g_static_heap;
value_t *g_symbol_table;

void vm_exec_proc_create();
void *exec_proc_loop(void *arg);

void vm_exec_init()
{
	voidobj = value_create(NULL, VT_VOID, 0, true);
	g_static_heap = NULL;
	g_symbol_table = voidobj;
	g_exec_procs = NULL;

	pthread_key_create(&g_thread_key, NULL);
	
	int pid_count = NUM_HARDWARE_THREADS;
	for (int i = 0; i < pid_count; i++) {
		vm_exec_proc_create();
	}
}

void vm_exec_wait()
{
	void *data;
	pthread_join(g_exec_procs->m_pid, &data);
}

value_t *vm_current_vm()
{
	vm_exec_proc_t *exec_proc = (vm_exec_proc_t *)pthread_getspecific(g_thread_key);
	return exec_proc->m_vm_list;
}

void vm_exec_proc_create()
{
printf("START\n");
	vm_exec_proc_t *exec_proc = (vm_exec_proc_t *)malloc(sizeof(vm_exec_proc_t));
	exec_proc->m_hardware_thread_affinity = -1;
	exec_proc->m_vm_list = nil;
	exec_proc->m_pid = 0;
	exec_proc->m_next = g_exec_procs;
	exec_proc->m_count = 0;
	g_exec_procs = exec_proc;

	// Spawn this bad boy
	int ret = pthread_create(&exec_proc->m_pid, NULL, exec_proc_loop, exec_proc);
	assert(ret == 0);
}

void *exec_proc_loop(void *arg)
{
	vm_exec_proc_t *exec_proc = (vm_exec_proc_t *)arg;
	while(exec_proc->m_pid == 0) {
		usleep(100000);
	}

	pthread_setspecific(g_thread_key, exec_proc);

	while(1) {
		vm_exec(NULL, 0, true);

		value_t *vm_val = exec_proc->m_vm_list;
		vm_t *vm = *(vm_t **)vm_val->m_data; 
//		printf("RES: "); value_print(vm, vm->m_stack[vm->m_sp - 1]); printf("\n");
		//vm_print_stack(vm);
		// We exited so we need to remove the current vm
		exec_proc->m_vm_list = exec_proc->m_vm_list->m_next_symbol;
	}

	printf("proc done\n");

	return NULL;
}


void vm_exec_proc_destroy(value_t *p_proc)
{
	// Not implementing yet
	assert(!"Not implementing yet");
}

void vm_exec_add_vm(value_t *p_vm)
{
	p_vm->m_next_symbol = g_exec_procs->m_vm_list;
	g_exec_procs->m_vm_list = p_vm;
}

void vm_exec_remove_vm(vm_t *p_vm)
{
}


value_t *vm_store_dyn_value(vm_t *p_vm, value_t *p_sym, value_t *p_value, value_t *p_sym_val)
{
	value_t *val = p_vm->m_exec_stack[p_vm->m_exp - 1];
	vm_state_t *vm_state = (vm_state_t *)cdr(p_vm, val)->m_data;

	vm_state->m_dynval = value_create_cons(p_vm, value_create_cons(p_vm, p_sym, p_value), vm_state->m_dynval);
	return vm_state->m_dynval;
}

void vm_remove_dyn_value(vm_t *p_vm, value_t *p_sym_val)
{
	value_t *sym = car(p_vm, p_sym_val);
	value_t *value = cdr(p_vm, p_sym_val);

	value_t *dyn_val = sym->m_cons[1];
	value_t *last = NULL;
	while(dyn_val != voidobj) {
		if (dyn_val == cdr(p_vm, p_sym_val)) {
			if (last == NULL) {
				sym->m_cons[1] = cdr(p_vm, dyn_val);
			} else {
				last->m_cons[1] = cdr(p_vm, dyn_val);
			}
			return;
		}

		last = dyn_val;
		dyn_val = cdr(p_vm, dyn_val);
	}
}

void vm_push_exec_state(vm_t *p_vm, value_t *p_closure)
{
	value_t *state = value_create_vm_state(p_vm);
	p_vm->m_exec_stack[p_vm->m_exp++] = value_create_cons(p_vm, p_closure, state);


//	printf("push_exec: ip: %d bp: %lu exp: %lu\n", p_vm->m_ip, p_vm->m_bp, p_vm->m_exp);
//value_print(p_vm, p_closure);
assert(p_vm->m_exp < 128);

	///////////////////////////////
	// Push new environment
	// MODIFIES: EP
	//////////////////////////////
	if (is_ifunc(p_closure) == false) {
		value_t *env = value_create_environment(p_vm, p_closure->m_cons[0]);
		vm_push_env(p_vm, env);
	}
	p_vm->m_ip = 0;
}

void vm_pop_exec_state(vm_t *p_vm)
{
	value_t *val = p_vm->m_exec_stack[p_vm->m_exp - 1];
	vm_state_t *vm_state = (vm_state_t *)cdr(p_vm, val)->m_data;
	p_vm->m_bp = vm_state->m_bp;
	p_vm->m_ip = vm_state->m_ip;
	p_vm->m_exp--;

	value_t *closure = car(p_vm, val);

	if (is_ifunc(closure) == false) {
		vm_pop_env(p_vm);
	}

	// Restore any dynamic bindings
	// Search for any value we added and remove it if it still exists
	value_t *dyn_store = vm_state->m_dynval;
	while(dyn_store != nil) {
		// (sym . val)
		value_t *sym_val = car(p_vm, dyn_store);
		vm_remove_dyn_value(p_vm, sym_val);

		dyn_store = cdr(p_vm, dyn_store);
	}

//	printf("pop_exec: ip: %d exp: %lu\n", p_vm->m_ip, p_vm->m_exp);
}

value_t *vm_peek_exec_state_closure(vm_t *p_vm)
{
	return car(p_vm, p_vm->m_exec_stack[p_vm->m_exp - 1]);
}



// Looks for closure on the stack and starts executing bytecode
void vm_exec(vm_t *p_vmUNUSED, unsigned long p_return_on_exp, bool p_allow_preemption)
{
//printf("VM_EXEC2: ret: %lu  %s\n", p_return_on_exp, p_allow_preemption ? "Yes" : "No");
	vm_exec_proc_t *exec_proc;
	exec_proc = (vm_exec_proc_t *)pthread_getspecific(g_thread_key);

	while(exec_proc->m_vm_list == NULL) {
		usleep(300000);
	}

//printf("GOT A VM\n");

	value_t *vm_val = exec_proc->m_vm_list;

	vm_t *p_vm = *(vm_t **)vm_val->m_data; 
	

	if (p_vm->m_exp == 0) {
		return;
	}

	// Get pointer to closure
	//value_t *closure = p_vm->m_stack[p_vm->m_sp - 1];
	value_t *closure = vm_peek_exec_state_closure(p_vm);

//printf(" closure: "); value_print(p_vm, closure); printf("\n");

	assert(closure && is_closure(closure) && (closure)->m_cons[1]);

	////////////////////////////////
	// Get lambda from closure
	/////////////////////////////////
	lambda_t *l = (lambda_t *)(closure->m_cons[1]->m_data);

	/////////////////////////////////////
	// Bind parameters
	////////////////////////////////////
	value_t *p = l->m_parameters;
	assert(is_null(p) == true);

	///////////////////////////
	// Start executing
	////////////////////////////
	while(p_vm->m_exp > p_return_on_exp) {
		p_vm->m_count++;

		if (p_vm->m_count > 1) {
			p_vm->m_count = 0;
			// See if there is another vm, switch to it and continue
			if (exec_proc->m_vm_list->m_next_symbol) {
				value_t *current_vm = exec_proc->m_vm_list;
				p_vm = *(vm_t **)current_vm->m_next_symbol->m_data;
				exec_proc->m_vm_list = current_vm->m_next_symbol;
				value_t *head = exec_proc->m_vm_list;;
				current_vm->m_next_symbol = NULL;
				while (head->m_next_symbol) {
					head = head->m_next_symbol;
				}
				head->m_next_symbol = current_vm;
			}
		}


		value_t *c = vm_peek_exec_state_closure(p_vm);

		//////////////////////////////
		// If this is an ifunc then we can just call it now
		/////////////////////////////
		bytecode_t *bc = NULL;
		unsigned long bc_opcode = OP_RET;
		long bc_value = 0;
		unsigned long p_arg = 0;
		value_t *p_pool = NULL;
		if (is_ifunc(c) && p_vm->m_ip > 0) {
			bc_opcode = OP_RET;
		} else if (is_ifunc(c)) {
			vm_func_t func = *(vm_func_t *)c->m_data;
			func(p_vm);
			continue;
		} else {
			assert(is_closure(c) == true);
			l = (lambda_t *)(c->m_cons[1]->m_data);
			bc = &((bytecode_t *)l->m_bytecode->m_data)[p_vm->m_ip];
			p_arg = bc->m_value;
			p_pool = l->m_pool;
			bc_opcode = bc->m_opcode;
			bc_value = bc->m_value;

			if (g_debug_display == true) {
				printf("%p) ip: %d] sp: %ld] ep: %ld bp: %ld] exp: %ld] %s %ld\n", p_vm, p_vm->m_ip, p_vm->m_sp, p_vm->m_ev, p_vm->m_bp, p_vm->m_exp, g_opcode_print[bc->m_opcode], bc->m_value);
				vm_print_stack(p_vm);
			}
		}

		// do_step_debugger
		if (g_step_debug == true) {
			char input[256];
			input[0] = '\0';
			printf("ip: %d %s %ld [r]un [n]ext [s]tack [c]losure [v]m: ", p_vm->m_ip, g_opcode_print[bc_opcode], bc_value);
			while(gets(input) != NULL && strcmp(input, "n")) {
				if (!strcmp(input, "s")) {
					vm_print_stack(p_vm);
				} else if (!strcmp(input, "c")) { 
					printf("\n");
					value_print(p_vm, c);
					printf("\n");
				} else if (!strcmp(input, "r")) {
					g_step_debug = false;
					break;
				} else if (!strcmp(input, "v")) {
					printf("%p) ip: %d] sp: %ld] ep: %ld bp: %ld] exp: %ld] %s %ld\n", p_vm, p_vm->m_ip, p_vm->m_sp, p_vm->m_ev, p_vm->m_bp, p_vm->m_exp, g_opcode_print[bc->m_opcode], bc->m_value);
				}

				printf("ip: %d %s %ld [r]un [n]ext [s]tack [c]losure [v]m: ", 
						p_vm->m_ip, g_opcode_print[bc_opcode], bc_value);
			}
		}

		////////////////////////////
		// Handle OP CODES
		////////////////////////////
		switch (bc_opcode) {
			case OP_PUSH:
			{
				p_vm->m_stack[p_vm->m_sp++] = ((value_t **)l->m_pool->m_data)[bc->m_value];
				p_vm->m_ip++;
				break;
			}
			case OP_BIND:
			{
				value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
				value_t *value = p_vm->m_stack[p_vm->m_sp - 1];
				bind_internal(p_vm, sym, value, false, false);
				p_vm->m_ip++;
				break;
			}
			case OP_BINDF:
			{
				value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
				value_t *value = p_vm->m_stack[p_vm->m_sp - 1];
				bind_internal(p_vm, sym, value, true, false);
				p_vm->m_ip++;
				break;
			}
			case OP_BINDD:
			{
				value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
				value_t *value = p_vm->m_stack[p_vm->m_sp - 1];
				bind_internal(p_vm, sym, value, false, true);
				p_vm->m_ip++;
				break;
			}
			case OP_BINDDF:
			{
				value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
				value_t *value = p_vm->m_stack[p_vm->m_sp - 1];
				bind_internal(p_vm, sym, value, true, true);
				p_vm->m_ip++;
				break;
			}
			case OP_CATCH:
			{
				//printf("op_catch arg: %d\n", (int)p_arg);

				int ip = (int)p_arg;

				if (ip == -1) {
//printf("pop'd handler\n");
					pop_handler_stack();
					p_vm->m_ip++;
					return;
				}


//////// TODO: Need to put this on the stack somehow?
				int vm_bp = p_vm->m_bp;
				int vm_sp = p_vm->m_sp;
				int vm_ev = p_vm->m_ev;
				int vm_ip = p_vm->m_ip;
				value_t *vm_current_env = p_vm->m_current_env[p_vm->m_ev - 1];


				int i = setjmp(*push_handler_stack());
				if (i != 0) {
//printf("error in catch\n");
					p_vm->m_bp = vm_bp;
					p_vm->m_sp = vm_sp;
					p_vm->m_ev = vm_ev;
					p_vm->m_ip = vm_ip;
					p_vm->m_current_env[p_vm->m_ev - 1] = vm_current_env;

					pop_handler_stack();
					p_vm->m_ip = ip;
//printf("jumping to %d\n", p_vm->m_ip);
				} else {
					p_vm->m_ip++;
				}
				break;
			}
			case OP_DUP:
			{
				// Get value from stack
			
				value_t *v = p_vm->m_stack[p_vm->m_bp + p_arg];

				// Push it onto the stack
				p_vm->m_stack[p_vm->m_sp++] = v;
				p_vm->m_ip++;
				break;
			}
			case OP_LOAD:
			{
				value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
				if (is_symbol_dynamic(sym)) {
					p_vm->m_stack[p_vm->m_sp++] = car(p_vm, sym->m_cons[1]);
					p_vm->m_ip++;
				} else {
					value_t *b = environment_binding_find(p_vm, ((value_t **)p_pool->m_data)[p_arg], false);

					verify(b && is_binding(b), "The variable %s is unbound.\n", 
						(char *)((value_t **)p_pool->m_data)[p_arg]->m_cons[0]->m_data);

//	printf("op_load: key: %d '", ((value_t **)p_pool->m_data)[p_arg]->m_type); value_print(((value_t **)p_pool->m_data)[p_arg]); printf("'\n");


					// Push it onto the stack
					binding_t *bind = (binding_t *)b->m_data;

					p_vm->m_stack[p_vm->m_sp++] = bind->m_value;
					p_vm->m_ip++;
				}
				break;
			}
			case OP_LOADF:
			{
//	printf("loadf: "); value_print(p_vm, ((value_t **)p_pool->m_data)[p_arg]); printf("\n");
//	printf("ev: %lu\n", p_vm->m_ev);

				value_t *sym = ((value_t **)p_pool->m_data)[p_arg];

				if (is_symbol_function_dynamic(sym)) {
					p_vm->m_stack[p_vm->m_sp++] = car(p_vm, sym->m_cons[2]);
					p_vm->m_ip++;
				} else {
					value_t *b = environment_binding_find(p_vm, ((value_t **)p_pool->m_data)[p_arg], true);

					verify(b != NULL, "The variable %s is not bound to a function.\n", 
						(char *)((value_t **)p_pool->m_data)[p_arg]->m_cons[0]->m_data);

					// Push it onto the stack
					binding_t *bind = (binding_t *)b->m_data;
					p_vm->m_stack[p_vm->m_sp++] = bind->m_value;
					p_vm->m_ip++;
				}

				break;
			}
			case OP_CALL:
			{
				//////////////////////
				// Need to setup new bytecode to run
				//
				// Fix up parameters and bind them?
				// Save current ip, bp, and ev
				// Set new bp and ip
				// Continue executing
				// Ret is responsible for restoring ip, bp, and ev and updating stack
				//
				// NOTE: p_arg contains the number of args call is passing

				///////////////////////////////
				// Get the closure we are calling which will have been loaded and is right before the args
				//////////////////////////////
				value_t *call_closure = p_vm->m_stack[p_vm->m_sp - p_arg - 1];

				////////////////////////////////
				// Get lambda from closure
				/////////////////////////////////
				lambda_t *call_lambda = (lambda_t *)(call_closure->m_cons[1]->m_data);

//printf("params1: %d ", (int)call_closure->m_cons[2]); value_print(p_vm, call_lambda->m_parameters); printf("\n");

				////////////////////////////////
				// If supports &rest need to collapse extra args into one spot on cons on the stack
				///////////////////////////////
				long int func_arg_count = 0;
				if (is_ifunc(call_closure)) {
					vm_func_t func = *(vm_func_t *)call_closure->m_data;
					func_arg_count = call_closure->m_data[sizeof(vm_func_t)];
				} else {
					func_arg_count = (long int)(call_closure)->m_cons[2];
				}

				// Condense remaining args into a list
				if (func_arg_count < 0) {
					func_arg_count = -func_arg_count;
					for( ; p_arg > func_arg_count; p_arg--) {
						vm_cons(p_vm);
					}

				}


				//////////////////////////////
				// If this is an ifunc then we can just call it now
				/////////////////////////////
				if (is_ifunc(call_closure)) {
					vm_func_t func = *(vm_func_t *)call_closure->m_data;
					p_vm->m_ip++;
					p_vm->m_bp = p_vm->m_sp - 1 - p_arg;
					vm_push_exec_state(p_vm, call_closure);
//printf("bp: %lu = sp: %lu - 1 - p_arg %lu\n", p_vm->m_bp, p_vm->m_sp, p_arg);
					continue;
				}

				///////////////////////////
				// Put our current state onto the stack
				//////////////////////////
//printf("ip: %d\n", p_vm->m_ip);
				p_vm->m_ip++;
				vm_push_exec_state(p_vm, call_closure);
				
				//////////////////////////
				// Setup vm for new closure
				/////////////////////////
				// Store current bp & sp

				// Set bp to the closure we are calling
				p_vm->m_bp = p_vm->m_sp - 1 - p_arg;
				p_vm->m_ip = 0;
				


				/////////////////////////////
				// Bind all parameters
				///////////////////////////////
				value_t *call_params = call_lambda->m_parameters;

				value_t *dyn_store = nil;

//printf("params2: "); value_print(p_vm, call_params); printf("\n");

				int bp_offset = 0;
				while(call_params && call_params != nil && call_params->m_cons[0]) {
					if (strcmp(call_params->m_cons[0]->m_cons[0]->m_data, "&REST")) {
						value_t *stack_val = p_vm->m_stack[p_vm->m_bp + 1 + bp_offset];
						value_t *sym = call_params->m_cons[0];

//printf("binding: %d ", call_params->m_cons[0]->m_type); value_print(p_vm, call_params->m_cons[0]); printf (" to: "); value_print(p_vm, stack_val); printf("\n");
//printf("binding symbol: %s\n", sym->m_cons[0]->m_data);

						bind_internal(p_vm, sym, stack_val, false, false);
			
						if (is_symbol_dynamic(sym) == true) {
							dyn_store = vm_store_dyn_value(p_vm, sym, sym->m_cons[1], dyn_store);
						}


						bp_offset++;
					}
					call_params = call_params->m_cons[1];
				}

printf("p_arg: %lu bp_offset: %d\n", p_arg, bp_offset);

				verify(func_arg_count == bp_offset, "Mismatched arg count: %d != %d\n", p_arg, bp_offset);


				break;

			}
			case OP_LAMBDA:
			{
				// get value from pool
				value_t *lambda = ((value_t **)p_pool->m_data)[p_arg];

//printf("op_lambda: bc: "); value_print(((lambda_t *)lambda->m_data)->m_bytecode); printf("\n");

				value_t *closure = make_closure(p_vm, lambda);
				p_vm->m_stack[p_vm->m_sp++] = closure;
				p_vm->m_ip++;
				break;
			}
			case OP_BINDG:
			{
				value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
				value_t *value = p_vm->m_stack[p_vm->m_sp - 1];
				bind_internal(p_vm, sym, value, false, true);
				p_vm->m_ip++;
				break;
			}
			case OP_BINDGF:
			{
				value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
				value_t *value = p_vm->m_stack[p_vm->m_sp - 1];
				bind_internal(p_vm, sym, value, true, true);
				p_vm->m_ip++;
				break;
			}
			case OP_POP:
				p_vm->m_sp--;
				p_vm->m_ip++;
				break;
			case OP_JMP:
				p_vm->m_ip += (int)p_arg;
				break;
			case OP_IFNILJMP:
			{
				value_t *top = p_vm->m_stack[p_vm->m_sp - 1];

//printf("ifniljmp: "); value_print(top); printf("\n");

				if (top == nil) {
					//printf("op_ifniljmp: %d\n", (int)p_arg);
					p_vm->m_ip += (int)p_arg;
				}  else { 
					p_vm->m_ip++;
				}
			
				break;
			}
			case OP_RET:
			{
//printf("B ip: %d\n", p_vm->m_ip);

				// Move top of stack to bp
				if (p_vm->m_sp > 1) {
					p_vm->m_stack[p_vm->m_bp] = p_vm->m_stack[p_vm->m_sp - 1];
					p_vm->m_sp = p_vm->m_bp + 1;
				}
				vm_pop_exec_state(p_vm);


				break;
			}
			case OP_UPDATE:
			{
				value_t *sym = ((value_t **)p_pool->m_data)[p_arg];

				if (sym->m_cons[1] != voidobj) {
					sym->m_cons[1]->m_cons[0] = p_vm->m_stack[p_vm->m_sp - 1];
				} else {
					value_t *b = environment_binding_find(p_vm, sym, false);

					verify(b && is_binding(b), "op_load: Binding lookup failed: %s\n", 
						(char *)((value_t **)p_pool->m_data)[p_arg]->m_data);

					// Change value
					binding_t *bind = (binding_t *)b->m_data;
					bind->m_value = p_vm->m_stack[p_vm->m_sp - 1];
				}

				p_vm->m_ip++;
				break;
			}
		}

		if (g_debug_display == true) {
			printf("------\n");
			vm_print_stack(p_vm);
			printf("--ip: %d] sp: %ld] ep: %ld] bp: %ld] exp: %ld] %s %ld\n", p_vm->m_ip, p_vm->m_sp, p_vm->m_ev, p_vm->m_bp, p_vm->m_exp, g_opcode_print[bc->m_opcode], bc->m_value);
		}
	}
}



