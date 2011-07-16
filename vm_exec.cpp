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
	while(dyn_val != p_vm->voidobj) {
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


	printf("push_exec: ip: %d exp: %lu\n", p_vm->m_ip, p_vm->m_exp);
assert(p_vm->m_exp < 128);

	///////////////////////////////
	// Push new environment
	// MODIFIES: EP
	//////////////////////////////
	value_t *env = value_create_environment(p_vm, p_closure->m_cons[0]);
	vm_push_env(p_vm, env);
	p_vm->m_ip = 0;
}

void vm_pop_exec_state(vm_t *p_vm)
{
	value_t *val = p_vm->m_exec_stack[p_vm->m_exp - 1];
	vm_state_t *vm_state = (vm_state_t *)cdr(p_vm, val)->m_data;
	p_vm->m_bp = vm_state->m_bp;
	p_vm->m_ip = vm_state->m_ip;
	p_vm->m_exp--;

	vm_pop_env(p_vm);

	// Restore any dynamic bindings
	// Search for any value we added and remove it if it still exists
	value_t *dyn_store = vm_state->m_dynval;
	while(dyn_store != p_vm->nil) {
		// (sym . val)
		value_t *sym_val = car(p_vm, dyn_store);
		vm_remove_dyn_value(p_vm, sym_val);

		dyn_store = cdr(p_vm, dyn_store);
	}

	printf("pop_exec: ip: %d exp: %lu\n", p_vm->m_ip, p_vm->m_exp);
}

value_t *vm_peek_exec_state_closure(vm_t *p_vm)
{
	return car(p_vm, p_vm->m_exec_stack[p_vm->m_exp - 1]);
}



// Looks for closure on the stack and starts executing bytecode
void vm_exec(vm_t *p_vm, unsigned long p_return_on_exp)
{
//printf("VM_EXEC2\n");

	if (p_vm->m_exp == 0) {
		return;
	}

	// Get pointer to closure
	//value_t *closure = p_vm->m_stack[p_vm->m_sp - 1];
	value_t *closure = vm_peek_exec_state_closure(p_vm);

//printf(" closure: "); value_print(p_vm, closure); printf("\n");

	assert(closure && is_closure(p_vm, closure) && (closure)->m_cons[1]);

	////////////////////////////////
	// Get lambda from closure
	/////////////////////////////////
	lambda_t *l = (lambda_t *)(closure->m_cons[1]->m_data);

	/////////////////////////////////////
	// Bind parameters
	////////////////////////////////////
	value_t *p = l->m_parameters;
	assert(is_null(p_vm, p) == true);

	///////////////////////////
	// Start executing
	////////////////////////////
	while(p_vm->m_exp > p_return_on_exp) {
		value_t *c = vm_peek_exec_state_closure(p_vm);
		assert(is_closure(p_vm, c) == true);
		l = (lambda_t *)(c->m_cons[1]->m_data);
		bytecode_t *bc = &((bytecode_t *)l->m_bytecode->m_data)[p_vm->m_ip];
		unsigned long p_arg = bc->m_value;
		value_t *p_pool = l->m_pool;

		if (g_debug_display == true) {
			printf("ip: %d] sp: %ld] ep: %ld bp: %ld] exp: %ld] %s %ld\n", p_vm->m_ip, p_vm->m_sp, p_vm->m_ev, p_vm->m_bp, p_vm->m_exp, g_opcode_print[bc->m_opcode], bc->m_value);
//vm_print_stack(p_vm);
		}

		////////////////////////////
		// Handle OP CODES
		////////////////////////////
		switch (bc->m_opcode) {
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
				if (is_symbol_dynamic(p_vm, sym)) {
					p_vm->m_stack[p_vm->m_sp++] = car(p_vm, sym->m_cons[1]);
					p_vm->m_ip++;
				} else {
					value_t *b = environment_binding_find(p_vm, ((value_t **)p_pool->m_data)[p_arg], false);

					verify(b && is_binding(p_vm, b), "The variable %s is unbound.\n", 
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

				if (is_symbol_function_dynamic(p_vm, sym)) {
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
				if (is_ifunc(p_vm, call_closure)) {
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
				if (is_ifunc(p_vm, call_closure)) {
					vm_func_t func = *(vm_func_t *)call_closure->m_data;
					p_vm->m_bp = p_vm->m_sp - 1 - p_arg;
printf("bp: %lu = sp: %lu - 1 - p_arg %lu\n", p_vm->m_bp, p_vm->m_sp, p_arg);
					func(p_vm);
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

				value_t *dyn_store = p_vm->nil;

//printf("params2: "); value_print(p_vm, call_params); printf("\n");

				int bp_offset = 0;
				while(call_params && call_params != p_vm->nil && call_params->m_cons[0]) {
					if (strcmp(call_params->m_cons[0]->m_cons[0]->m_data, "&REST")) {
						value_t *stack_val = p_vm->m_stack[p_vm->m_bp + 1 + bp_offset];
						value_t *sym = call_params->m_cons[0];

//printf("binding: %d ", call_params->m_cons[0]->m_type); value_print(p_vm, call_params->m_cons[0]); printf (" to: "); value_print(p_vm, stack_val); printf("\n");
//printf("binding symbol: %s\n", sym->m_cons[0]->m_data);

						bind_internal(p_vm, sym, stack_val, false, false);
			
						if (is_symbol_dynamic(p_vm, sym) == true) {
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

				if (top == p_vm->nil) {
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

//printf("A ip: %d\n", p_vm->m_ip);
#if 0
				if (p_vm->m_bp == return_bp) {
					p_vm->m_ip = -1;
				} else {
					// Inspect the closure to get the number of arguments
					value_t *ret_closure = p_vm->m_stack[p_vm->m_bp];

					int args = 0;
					if (is_ifunc(p_vm, ret_closure)) {
						vm_func_t func = *(vm_func_t *)ret_closure->m_data;
						args = ret_closure->m_data[sizeof(vm_func_t)];
					} else {
						args = (int)ret_closure->m_cons[2];
					}


printf("args: %d\n", args);
vm_print_stack(p_vm);

					value_t *previous_vm_state = p_vm->m_stack[p_vm->m_bp + 1 + args];
	
					vm_state_t *vm_state = (vm_state_t *)previous_vm_state->m_data;
					p_vm->m_bp = vm_state->m_bp;
	
					p_vm->m_ip = vm_state->m_ip + 1;

/*
				// Restore any dynamic bindings
				// Search for any value we added and remove it if it still exists
				while(dyn_store != p_vm->nil) {
					// (sym . val)
					value_t *sym_val = car(p_vm, dyn_store);
					vm_remove_dyn_value(p_vm, sym_val);

					dyn_store = cdr(p_vm, dyn_store);
				}

				printf("sp: %lu bp: %lu\n", p_vm->m_sp, p_vm->m_bp);
				vm_print_stack(p_vm);
				printf("----\n");


				// Consume the stack back through the function name
				unsigned long cur_bp = p_vm->m_bp;
				vm_state_pop(p_vm);

				p_vm->m_stack[cur_bp - 2] = p_vm->m_stack[p_vm->m_sp - 1];
				p_vm->m_sp = cur_bp - 1;

				p_vm->m_bp = old_bp;
				p_vm->m_ip =  old_ip;

				printf("sp: %lu bp: %lu\n", p_vm->m_sp, p_vm->m_bp);
				vm_print_stack(p_vm);
				printf("---------------------------\n");

				vm_pop_env(p_vm);

				if (g_debug_display == true) {
					printf("--vm_exec\n");
					vm_print_stack(p_vm);
				}

*/
				}
#endif

				break;
			}
			case OP_UPDATE:
			{
				value_t *sym = ((value_t **)p_pool->m_data)[p_arg];

				if (sym->m_cons[1] != p_vm->voidobj) {
					sym->m_cons[1]->m_cons[0] = p_vm->m_stack[p_vm->m_sp - 1];
				} else {
					value_t *b = environment_binding_find(p_vm, sym, false);

					verify(b && is_binding(p_vm, b), "op_load: Binding lookup failed: %s\n", 
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



