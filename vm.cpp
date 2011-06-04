#include "env.h"
#include "binding.h"
#include "value_helpers.h"
#include "assert.h"
#include "lambda.h"
#include "compile.h"
#include "gc.h"
#include "err.h"

#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct bytecode_jump {
	bytecode_t m_bytecode;
	void (*m_func)(vm_t *, unsigned long, value_t *);
} bytecode_jump_t;


char const *g_opcode_print[] =  {
    "OP_PUSH",
    "OP_BIND",
    "OP_BINDF",
    "OP_CATCH",
    "OP_DUP",
    "OP_LOAD",
    "OP_LOADF",
    "OP_CALL",
    "OP_LAMBDA",
    "OP_BINDGF",
    "OP_BINDG",
    "OP_IFNILJMP",
    "OP_RET",
};



vm_t *vm_create(unsigned long p_stack_size)
{
	vm_t *vm = (vm_t *)malloc(sizeof(vm_t));
	vm->m_heap = NULL;
	vm->m_static_heap = NULL;
	vm->m_sp = 0;
	vm->m_bp = 0;
	vm->m_ev = 1;
	vm->m_ip = 0;
	vm->m_stack = (value_t **)malloc(sizeof(value_t *) * p_stack_size);

	// CANNOT create any value_t's until the above is initialized

	vm->m_kernel_env = value_create_environment(vm, NULL);
	vm->m_user_env = value_create_environment(vm, vm->m_kernel_env);
	vm->m_current_env = (value_t **)malloc(sizeof(value_t *) * p_stack_size);
	vm->m_current_env[0] = vm->m_user_env;

	return vm;
}

void vm_destroy(vm_t *p_vm)
{

	p_vm->m_heap = NULL;
	p_vm->m_current_env[0] = NULL;
	p_vm->m_ev = 1;
	gc_shutdown(p_vm);
	free(p_vm->m_stack);
	free(p_vm->m_current_env);
	free(p_vm);
}




// TODO - Bind calls should replace existing bindings
void vm_bind(vm_t *p_vm, char *p_symbol, value_t *p_value)
{

	value_t *binding = value_create_binding(p_vm, value_create_symbol(p_vm, p_symbol), p_value);
	binding_t *bind = (binding_t *)binding->m_data;

printf("binding "); value_print(bind->m_key); printf(" to "); value_print(bind->m_value); printf("\n");

	bind->m_next = ((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_bindings;
	((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_bindings = binding;
}

// TODO - Bind calls should replace existing bindings
void vm_bindf(vm_t *p_vm, char *p_symbol, vm_func_t p_func, unsigned long p_param_count)
{
	value_t *binding = value_create_binding(p_vm, value_create_symbol(p_vm, p_symbol), value_create_internal_func(p_vm, p_func, p_param_count));

	assert(binding != NULL);

	binding_t *bind = (binding_t *)binding->m_data;

	assert(bind != NULL);
	assert(p_vm->m_current_env[p_vm->m_ev - 1]);
	assert(p_vm->m_current_env[p_vm->m_ev - 1]->m_data);

	bind->m_next = ((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_function_bindings;

	((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_function_bindings = binding;
}

void vm_bindf(vm_t *p_vm, char *p_symbol, value_t *p_code)
{
	value_t *binding = value_create_binding(p_vm, value_create_symbol(p_vm, p_symbol), p_code);
	binding_t *bind = (binding_t *)binding->m_data;
	bind->m_next = ((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_function_bindings;
	((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_function_bindings = binding;
}

// Pull the top two elements off the stack and replace them with a cons value
void vm_cons(vm_t *p_vm)
{
	value_t *car = p_vm->m_stack[p_vm->m_sp - 2];
	value_t *cdr = p_vm->m_stack[p_vm->m_sp - 1];
	value_t *cons = value_create_cons(p_vm, car, cdr);

	p_vm->m_stack[p_vm->m_sp - 2] = cons;
	p_vm->m_sp -= 1;
}

void vm_list(vm_t *p_vm, int p_args)
{
	vm_push(p_vm, nil);

	for(int i = p_args; i > 0; i--) {
		vm_cons(p_vm);
	}

}

void vm_push(vm_t *p_vm, value_t *p_value)
{
	p_vm->m_stack[p_vm->m_sp++] = p_value;
}

void vm_push_env(vm_t *p_vm, value_t *p_env)
{
	p_vm->m_current_env[p_vm->m_ev++] = p_env;
}

void vm_pop_env(vm_t *p_vm)
{
	p_vm->m_ev--;
}

void vm_exec(vm_t *p_vm, value_t *p_closure, int p_nargs)
{

	if (p_closure->m_type == VT_INTERNAL_FUNCTION) {
		vm_func_t func = *(vm_func_t *)p_closure->m_data;

		int func_arg_count = p_closure->m_data[sizeof(vm_func_t)];
		// Condense remaining args into a list
		if (func_arg_count < 0) {
			func_arg_count = -func_arg_count;
			for( ; p_nargs >= func_arg_count; p_nargs--) {
				vm_cons(p_vm);
			}
			p_nargs++;
		}


		verify(p_nargs == func_arg_count, "Mismatched ifunc arg count: %d != %d\n", p_nargs, func_arg_count);
		
		p_vm->m_stack[p_vm->m_sp++] = func(p_vm);
		return;
	}

	// Store current bp & sp
	int old_bp = p_vm->m_bp;

	// Set bp to the name of the function we are calling
	p_vm->m_bp = p_vm->m_sp - p_nargs;

	int old_ip = p_vm->m_ip;
	p_vm->m_ip = 0;


printf("obp: %d nbp: %lu sp: %lu nargs: %d\n", old_bp, p_vm->m_bp, p_vm->m_sp, p_nargs);


    assert(p_closure && p_closure->m_type == VT_CLOSURE && p_closure->m_cons[1]);
    lambda_t *l = (lambda_t *)(p_closure->m_cons[1]->m_data);

	value_t *env = value_create_environment(p_vm, p_closure->m_cons[0]);
	vm_push_env(p_vm, env);

	vm_push(p_vm, p_closure);

	// Bind parameters
	value_t *p = l->m_parameters;

	int func_arg_count = (int)p_closure->m_cons[2];
	// Condense remaining args into a list
	if (func_arg_count < 0) {
		func_arg_count = -func_arg_count;
		for( ; p_nargs >= func_arg_count; p_nargs--) {
			vm_cons(p_vm);
		}
		p_nargs++;
	}

	int bp_offset = 0;
	while(p && p != nil && p->m_cons[0]) {
		if (strcmp(p->m_cons[0]->m_data, "&rest")) {
			value_t *stack_val = p_vm->m_stack[p_vm->m_bp + bp_offset];

printf("binding: "); value_print(p->m_cons[0]); printf ("to: "); value_print(stack_val); printf("\n");

			vm_bind(p_vm, p->m_cons[0]->m_data, stack_val);

			bp_offset++;
		}
		p = p->m_cons[1];
	}

	verify(p_nargs == bp_offset, "Mismatched arg count: %d != %d\n", p_nargs, bp_offset);

printf("active pool: %p\n", l->m_pool);

	while(p_vm->m_ip != -1) {
		bytecode_t *bc = &((bytecode_t *)l->m_bytecode->m_data)[p_vm->m_ip];
		unsigned long p_arg = bc->m_value;
		value_t *p_pool = l->m_pool;

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
				value_t *binding = value_create_binding(p_vm, sym, p_vm->m_stack[p_vm->m_sp - 1]);
				binding_t *bind = (binding_t *)binding->m_data;

				bind->m_next = ((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_bindings;
				assert((bind->m_next == NULL) || (bind->m_next->m_type == VT_BINDING));
				assert(binding->m_type == VT_BINDING);

				((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_bindings = binding;
				p_vm->m_ip++;
				break;
			}
			case OP_BINDF:
			{
				value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
				value_t *binding = value_create_binding(p_vm, sym, p_vm->m_stack[p_vm->m_sp - 1]);
				binding_t *bind = (binding_t *)binding->m_data;

				assert((bind->m_next == NULL) || (bind->m_next->m_type == VT_BINDING));
				assert(binding->m_type == VT_BINDING);

				bind->m_next = ((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_function_bindings;
				((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_function_bindings = binding;
				p_vm->m_ip++;
				break;
			}
			case OP_CATCH:
			{
				//printf("op_catch arg: %d\n", (int)p_arg);

				int ip = (int)p_arg;

				if (p_arg == -1) {
//printf("pop'd handler\n");
					pop_handler_stack();
					p_vm->m_ip++;
					return;
				}

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
				value_t *b = environment_binding_find(p_vm, ((value_t **)p_pool->m_data)[p_arg], false);

				verify(b && b->m_type == VT_BINDING, "op_load: Binding lookup failed: %s\n", 
					(char *)((value_t **)p_pool->m_data)[p_arg]->m_data);

//	printf("op_load: key: %d '", ((value_t **)p_pool->m_data)[p_arg]->m_type); value_print(((value_t **)p_pool->m_data)[p_arg]); printf("'\n");


				// Push it onto the stack
				binding_t *bind = (binding_t *)b->m_data;

				p_vm->m_stack[p_vm->m_sp++] = bind->m_value;
				p_vm->m_ip++;
				break;
			}
			case OP_LOADF:
			{
	//printf("loadf: "); value_print(((value_t **)p_pool->m_data)[p_arg]); printf("\n");
	//printf("ev: %lu\n", p_vm->m_ev);

				value_t *b = environment_binding_find(p_vm, ((value_t **)p_pool->m_data)[p_arg], true);


//printf("sym: %s\n", (char *)((value_t **)p_pool->m_data)[p_arg]->m_data);
				verify(b != NULL, "op_loadf: Could not find function binding for symbol: %s\n", 
					(char *)((value_t **)p_pool->m_data)[p_arg]->m_data);

				// Push it onto the stack
				binding_t *bind = (binding_t *)b->m_data;
				p_vm->m_stack[p_vm->m_sp++] = bind->m_value;
				p_vm->m_ip++;
				break;
			}
			case OP_CALL:
			{
				// Store current bp & sp
				int old_bp = p_vm->m_bp;

				// Set bp to the name of the function we are calling
				p_vm->m_bp = p_vm->m_sp - (1 + p_arg);


				value_t *func_val = p_vm->m_stack[p_vm->m_bp];


				value_t *ret = 0;
				if (func_val->m_type == VT_INTERNAL_FUNCTION) {
					vm_exec(p_vm, func_val, p_arg);
					ret = p_vm->m_stack[p_vm->m_sp - 1];
				} else if (func_val->m_type == VT_CLOSURE) {
					vm_exec(p_vm, func_val, p_arg);
					ret = p_vm->m_stack[p_vm->m_sp - 1];
				}  else {
					verify(false, "ERROR: UNKNOWN FUNCTION TYPE: %d\n", func_val->m_type);
				}

				// Consume the stack back through the function name
				p_vm->m_sp = p_vm->m_bp;
				p_vm->m_stack[p_vm->m_sp++] = ret;
				p_vm->m_bp = old_bp;
				p_vm->m_ip++;
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
				value_t *binding = value_create_binding(p_vm, sym, p_vm->m_stack[p_vm->m_sp - 1]);
				binding_t *bind = (binding_t *)binding->m_data;
				bind->m_next = ((environment_t *)p_vm->m_user_env->m_data)->m_bindings;
			assert((bind->m_next == NULL) || (bind->m_next->m_type == VT_BINDING));
			assert(binding->m_type == VT_BINDING);
				((environment_t *)p_vm->m_user_env->m_data)->m_bindings = binding;
				p_vm->m_ip++;
				break;
			}
			case OP_BINDGF:
			{
				value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
				value_t *binding = value_create_binding(p_vm, sym, p_vm->m_stack[p_vm->m_sp - 1]);
				binding_t *bind = (binding_t *)binding->m_data;
				bind->m_next = ((environment_t *)p_vm->m_user_env->m_data)->m_function_bindings;
				assert((bind->m_next == NULL) || (bind->m_next->m_type == VT_BINDING));
				assert(binding->m_type == VT_BINDING);
				((environment_t *)p_vm->m_user_env->m_data)->m_function_bindings = binding;
				p_vm->m_ip++;
				break;
			}
			case OP_IFNILJMP:
			{
				value_t *top = p_vm->m_stack[p_vm->m_sp - 1];

//printf("ifniljmp: "); value_print(top); printf("\n");

				if (top->m_type == VT_SYMBOL && (!strcmp(top->m_data, "nil"))) {
					//printf("op_ifniljmp: %d\n", (int)p_arg);
					p_vm->m_ip += (int)p_arg;
				}  else { 
					p_vm->m_ip++;
				}
			
				break;
			}
			case OP_RET:
			{
				p_vm->m_ip = -1;
				break;
			}
		}
	}

//printf("left active pool\n");


	// Consume the stack back through the function name
	p_vm->m_stack[p_vm->m_bp] = p_vm->m_stack[p_vm->m_sp - 1];
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_bp = old_bp;
	p_vm->m_ip =  old_ip;

	vm_pop_env(p_vm);

}



