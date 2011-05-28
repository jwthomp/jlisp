#include "env.h"
#include "binding.h"
#include "value_helpers.h"
#include "assert.h"
#include "lambda.h"
#include "compile.h"

#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct bytecode_jump {
	bytecode_t m_bytecode;
	void (*m_func)(vm_t *, unsigned long, value_t *);
} bytecode_jump_t;


void op_push(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_bind(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_bindgf(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_bindf(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_print(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_dup(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_load(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_loadf(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_call(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_lambda(vm_t *, unsigned long p_arg, value_t *p_pool);


static bytecode_jump_t g_bytecode[] = {
	{ {OP_PUSH, 0}, &op_push },
	{ {OP_BIND, 0}, &op_bind },
	{ {OP_BINDF, 0}, &op_bind },
	{ {OP_PRINT, 0}, &op_print },
	{ {OP_DUP, 0}, &op_dup },
	{ {OP_LOAD, 0}, &op_load },
	{ {OP_LOADF, 0}, &op_loadf },
	{ {OP_CALL, 0}, &op_call },
	{ {OP_LAMBDA, 0}, &op_lambda },
	{ {OP_BINDGF, 0}, &op_bindgf },
};


vm_t *vm_create(unsigned long p_stack_size)
{
	vm_t *vm = (vm_t *)malloc(sizeof(vm_t));
	vm->m_kernel_env = value_create_environment(NULL);
	vm->m_user_env = value_create_environment(vm->m_kernel_env);
	vm->m_stack = (value_t **)malloc(sizeof(value_t *) * p_stack_size);
	vm->m_current_env = (value_t **)malloc(sizeof(value_t *) * p_stack_size);
	vm->m_current_env[0] = vm->m_user_env;
	vm->m_sp = 0;
	vm->m_bp = 0;
	vm->m_ev = 1;
}

void vm_destroy(vm_t *p_vm)
{
	free(p_vm->m_stack);
	free(p_vm->m_current_env);
	free(p_vm);
}

void exec_instruction(vm_t *p_vm, bytecode_t p_bc, value_t *p_pool)
{
	g_bytecode[p_bc.m_opcode].m_func(p_vm, p_bc.m_value, p_pool);
}


void op_push(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	p_vm->m_stack[p_vm->m_sp++] = ((value_t **)p_pool->m_data)[p_arg];
}

void op_bind(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
	binding_t *binding = binding_create( sym, p_vm->m_stack[p_vm->m_sp - 1]);
	binding->m_next = ((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_bindings;
	((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_bindings = binding;
}

void op_bindf(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
	binding_t *binding = binding_create( sym, p_vm->m_stack[p_vm->m_sp - 1]);
	binding->m_next = ((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_function_bindings;
	((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_function_bindings = binding;
}

void op_bindgf(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
	binding_t *binding = binding_create( sym, p_vm->m_stack[p_vm->m_sp - 1]);
	binding->m_next = ((environment_t *)p_vm->m_user_env->m_data)->m_function_bindings;
	((environment_t *)p_vm->m_user_env->m_data)->m_function_bindings = binding;
}

void op_print(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	printf("\nVAL: ");
	value_print(p_vm->m_stack[p_vm->m_sp - 1]);
	printf("\n");
}

void op_dup(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	// Get value from stack
	value_t *v = p_vm->m_stack[p_vm->m_bp + p_arg];

	// Push it onto the stack
	p_vm->m_stack[p_vm->m_sp++] = v;
}

void op_load(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	binding_t *b = environment_binding_find(p_vm, ((value_t **)p_pool->m_data)[p_arg], false);

	printf("op_load: key: "); value_print(((value_t **)p_pool->m_data)[p_arg]); printf("\n");

	// Push it onto the stack
	p_vm->m_stack[p_vm->m_sp++] = b->m_value;
}

void op_loadf(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	printf("loadf: "); value_print(((value_t **)p_pool->m_data)[p_arg]); printf("\n");
	printf("ev: %lu\n", p_vm->m_ev);

	binding_t *b = environment_binding_find(p_vm, ((value_t **)p_pool->m_data)[p_arg], true);


	assert(b != NULL);

	// Push it onto the stack
	p_vm->m_stack[p_vm->m_sp++] = b->m_value;
}

void op_lambda(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	// get value from pool
	value_t *lambda = ((value_t **)p_pool->m_data)[p_arg];
	value_t *closure = make_closure(p_vm, lambda);
	p_vm->m_stack[p_vm->m_sp++] = closure;
}

void op_call(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	// Store current bp & sp
	int old_bp = p_vm->m_bp;

	// Set bp to the name of the function we are calling
	p_vm->m_bp = p_vm->m_sp - (1 + p_arg);


	value_t *func_val = p_vm->m_stack[p_vm->m_bp];


	value_t *ret = 0;
	if (func_val->m_type == VT_INTERNAL_FUNCTION) {
		vm_func_t func = *(vm_func_t *)func_val->m_data;
		ret = func(p_vm);
	} else if (func_val->m_type == VT_CLOSURE) {
		vm_exec(p_vm, func_val, p_arg);
		ret = p_vm->m_stack[p_vm->m_sp - 1];
	}  else {
		printf("ERROR: UNKNOWN FUNCTION TYPE\n");
		*(int *)0;
	}

	// Consume the stack back through the function name
	p_vm->m_sp = p_vm->m_bp;
	p_vm->m_stack[p_vm->m_sp++] = ret;
	p_vm->m_bp = old_bp;
}

// TODO - Bind calls should replace existing bindings
void vm_bind(vm_t *p_vm, char *p_symbol, value_t *p_value)
{
	binding_t *binding = binding_create(value_create_symbol(p_symbol), p_value);
	binding->m_next = ((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_bindings;
	((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_bindings = binding;
}

// TODO - Bind calls should replace existing bindings
void vm_bindf(vm_t *p_vm, char *p_symbol, vm_func_t p_func)
{
	binding_t *binding = binding_create(value_create_symbol(p_symbol),
										value_create_internal_func(p_func)); 
	binding->m_next = ((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_function_bindings;
	((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_function_bindings = binding;
}

void vm_bindf(vm_t *p_vm, char *p_symbol, value_t *p_code)
{
	binding_t *binding = binding_create(value_create_symbol(p_symbol), p_code);
	binding->m_next = ((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_function_bindings;
	((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_function_bindings = binding;
}

// Pull the top two elements off the stack and replace them with a cons value
void vm_cons(vm_t *p_vm)
{
	value_t *car = p_vm->m_stack[p_vm->m_sp - 2];
	value_t *cdr = p_vm->m_stack[p_vm->m_sp - 1];
	value_t *cons = value_create_cons(car, cdr);

	p_vm->m_stack[p_vm->m_sp - 2] = cons;
	p_vm->m_sp -= 1;
}

void vm_list(vm_t *p_vm, int p_args)
{
	value_t *nil = value_create_cons(NULL, NULL);
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
	// Store current bp & sp
	int old_bp = p_vm->m_bp;

	// Set bp to the name of the function we are calling
	p_vm->m_bp = p_vm->m_sp - p_nargs;

printf("obp: %d nbp: %d sp: %d nargs: %d\n", old_bp, p_vm->m_bp, p_vm->m_sp, p_nargs);

    assert(p_closure && p_closure->m_type == VT_CLOSURE && p_closure->m_cons[1]);
    lambda_t *l = (lambda_t *)(p_closure->m_cons[1]->m_data);

	value_t *env = value_create_environment(p_closure->m_cons[0]);
	vm_push_env(p_vm, env);

	// Bind parameters
	value_t *p = l->m_parameters;


	int bp_offset = 0;
	while(p && p->m_cons[0]) {
		value_t *stack_val = p_vm->m_stack[p_vm->m_bp + bp_offset];

printf("binding: "); value_print(p->m_cons[0]); printf ("to: "); value_print(stack_val); printf("\n");

		vm_bind(p_vm, p->m_cons[0]->m_data, stack_val);

		bp_offset++;
		p = p->m_cons[1];
	}


	for(unsigned long i = 0; i < (l->m_bytecode->m_size / sizeof(bytecode_t *)); i++) {
		exec_instruction(p_vm, ((bytecode_t *)l->m_bytecode->m_data)[i], l->m_pool);
	}


	// Consume the stack back through the function name
	p_vm->m_stack[p_vm->m_bp] = p_vm->m_stack[p_vm->m_sp - 1];
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_bp = old_bp;

	vm_pop_env(p_vm);

}



