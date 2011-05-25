#include "env.h"
#include "binding.h"
#include "value_helpers.h"

#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct bytecode_jump {
	bytecode_t m_bytecode;
	void (*m_func)(vm_t *, void *);
} bytecode_jump_t;


void op_push(vm_t *, void * p_arg);
void op_push_env(vm_t *, void * p_arg);
void op_pop_env(vm_t *, void * p_arg);
void op_bind(vm_t *, void * p_arg);
void op_bindf(vm_t *, void * p_arg);
void op_print(vm_t *, void * p_arg);
void op_dup(vm_t *, void * p_arg);
void op_load(vm_t *, void * p_arg);
void op_loadf(vm_t *, void * p_arg);
void op_call(vm_t *, void * p_arg);

static bytecode_jump_t g_bytecode[] = {
	{ {OP_PUSH, 0}, &op_push },
	{ {OP_PUSH_ENV, 0}, &op_push_env },
	{ {OP_POP_ENV, 0}, &op_pop_env },
	{ {OP_BIND, 0}, &op_bind },
	{ {OP_BINDF, 0}, &op_bind },
	{ {OP_PRINT, 0}, &op_print },
	{ {OP_DUP, 0}, &op_dup },
	{ {OP_LOAD, 0}, &op_load },
	{ {OP_LOADF, 0}, &op_loadf },
	{ {OP_CALL, 0}, &op_call },
};


vm_t *vm_create(unsigned long p_stack_size)
{
	vm_t *vm = (vm_t *)malloc(sizeof(vm_t));
	vm->m_kernel_env = environment_create(NULL);
	vm->m_user_env = environment_create(vm->m_kernel_env);
	vm->m_current_env = vm->m_user_env;
	vm->m_stack = (value_t **)malloc(sizeof(value_t *) * p_stack_size);
	vm->m_sp = 0;
	vm->m_bp = 0;
}

void vm_destroy(vm_t *p_vm)
{
	// Walk up the environments and destroy them

	environment_t *env = p_vm->m_current_env;
	while(env) {
		environment_t *env_parent = env->m_parent;
		environment_destroy(env);
		env = env_parent;
	}

	free(p_vm->m_stack);
}

void exec_instruction(vm_t *p_vm, bytecode_t p_bc)
{
	g_bytecode[p_bc.opcode].m_func(p_vm, p_bc.argument);
}

void vm_exec(vm_t *p_vm, bytecode_t *p_bc, unsigned long p_size)
{
	for(unsigned long i = 0; i < p_size; i++) {
printf("ins: %lu\n", i);
		exec_instruction(p_vm, p_bc[i]);
	}
}

void op_push(vm_t *p_vm, void * p_arg)
{
	p_vm->m_stack[p_vm->m_sp++] = (value_t *)p_arg;
}

void op_push_env(vm_t *p_vm, void * p_arg)
{
	environment_t *env = environment_create(p_vm->m_current_env);
	p_vm->m_current_env = env;
}

void op_pop_env(vm_t *p_vm, void * p_arg)
{
	environment_t *env = p_vm->m_current_env;
	p_vm->m_current_env = p_vm->m_current_env->m_parent;
	environment_destroy(env);
}

void op_bind(vm_t *p_vm, void * p_arg)
{
	binding_t *binding = binding_create( p_vm->m_stack[p_vm->m_sp - 2],
								p_vm->m_stack[p_vm->m_sp - 1]);
	binding->m_next = p_vm->m_current_env->m_bindings;
	p_vm->m_current_env->m_bindings = binding;
}

void op_bindf(vm_t *p_vm, void * p_arg)
{
	binding_t *binding = binding_create( p_vm->m_stack[p_vm->m_sp - 2],
								p_vm->m_stack[p_vm->m_sp - 1]);
	binding->m_next = p_vm->m_current_env->m_function_bindings;
	p_vm->m_current_env->m_function_bindings = binding;
}

void op_print(vm_t *p_vm, void * p_arg)
{
	printf("\nVAL: ");
	value_print(p_vm->m_stack[p_vm->m_sp - 1]);
	printf("\n");
}

void op_dup(vm_t *p_vm, void * p_arg)
{
	// Get value from stack
	value_t *v = p_vm->m_stack[p_vm->m_bp + (int)p_arg];

	printf("\nDUP: ");
	value_print(v);
	printf("\n");

	// Push it
	op_push(p_vm, (void *)v);
}

void op_load(vm_t *p_vm, void * p_arg)
{
	binding_t *b = binding_find(p_vm->m_current_env->m_bindings, (value_t *)p_arg);
	op_push(p_vm, (void *)b->m_value);
}

void op_loadf(vm_t *p_vm, void * p_arg)
{
	binding_t *b = binding_find(p_vm->m_current_env->m_function_bindings, (value_t *)p_arg);
	op_push(p_vm, (void *)b->m_value);
}

void op_call(vm_t *p_vm, void * p_arg)
{
	// Store current bp & sp
	int old_bp = p_vm->m_bp;

	// Set bp to the name of the function we are calling
	p_vm->m_bp = p_vm->m_sp - (1 + (int)p_arg);


	value_t *func_val = p_vm->m_stack[p_vm->m_bp];


	value_t *ret = 0;
	if (func_val->m_type == VT_INTERNAL_FUNCTION) {
		vm_func_t func = *(vm_func_t *)func_val->m_data;
		ret = func(p_vm);
	} else if (func_val->m_type == VT_BYTECODE) {
		vm_exec(p_vm, (bytecode_t *)func_val->m_data, func_val->m_size / sizeof(bytecode_t));
		ret = p_vm->m_stack[p_vm->m_sp - 1];
	}  else {
		printf("ERROR: UNKNOWN FUNCTION TYPE\n");
		*(int *)0;
	}

	// Consume the stack back through the function name
	p_vm->m_sp = p_vm->m_bp;
	op_push(p_vm, ret);
	p_vm->m_bp = old_bp;
}

// TODO - Bind calls should replace existing bindings
void vm_bind(vm_t *p_vm, char *p_symbol, value_t *p_value)
{
	binding_t *binding = binding_create(value_create_symbol(p_symbol), p_value);
	binding->m_next = p_vm->m_current_env->m_function_bindings;
	p_vm->m_current_env->m_bindings = binding;
}

// TODO - Bind calls should replace existing bindings
void vm_bindf(vm_t *p_vm, char *p_symbol, vm_func_t p_func)
{
	binding_t *binding = binding_create(value_create_symbol(p_symbol),
										value_create_internal_func(p_func)); 
	binding->m_next = p_vm->m_current_env->m_function_bindings;
	p_vm->m_current_env->m_function_bindings = binding;
}

void vm_bindf(vm_t *p_vm, char *p_symbol, value_t *p_code)
{
	binding_t *binding = binding_create(value_create_symbol(p_symbol), p_code);
	binding->m_next = p_vm->m_current_env->m_function_bindings;
	p_vm->m_current_env->m_function_bindings = binding;
}

// Pull the top two elements off the stack and replace them with a cons value
void vm_cons(vm_t *p_vm)
{
	value_t *car = p_vm->m_stack[p_vm->m_sp - 2];
	value_t *cdr = p_vm->m_stack[p_vm->m_sp - 1];
	value_t *cons = value_create_cons(car, cdr);

	printf("Cons VAL: ");
	value_print(cons);
	printf("\n");

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
	printf("Push VAL: ");
	value_print(p_value);
	printf("\n");

	p_vm->m_stack[p_vm->m_sp++] = p_value;
}


