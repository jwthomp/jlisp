#include "env.h"
#include "binding.h"

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
	binding_t *b = binding_find(p_vm->m_current_env->m_bindings, 
									p_vm->m_stack[p_vm->m_sp - 1]);
	op_push(p_vm, (void *)b->m_value);
}

void op_loadf(vm_t *p_vm, void * p_arg)
{
	binding_t *b = binding_find(p_vm->m_current_env->m_function_bindings, 
									p_vm->m_stack[p_vm->m_sp - 1]);
	op_push(p_vm, (void *)b->m_value);
}
