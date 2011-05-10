#include "env.h"

#include "vm.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct bytecode_jump {
	bytecode_t m_bytecode;
	void (*m_func)(vm_t *, unsigned long);
} bytecode_jump_t;


void push(vm_t *, unsigned long p_arg);

static bytecode_jump_t g_bytecode[] = {
	{ {0, 0}, &push }
};


vm_t *vm_create(unsigned long p_stack_size)
{
	vm_t *vm = (vm_t *)malloc(sizeof(vm_t));
	vm->m_kernel_env = environment_create(NULL);
	vm->m_user_env = environment_create(vm->m_kernel_env);
	vm->m_current_env = vm->m_user_env;
	vm->m_stack = (value_t *)malloc(sizeof(value_t) * p_stack_size);
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
		exec_instruction(p_vm, p_bc[i]);
	}
}

void push(vm_t *p_vm, unsigned long p_arg)
{
	printf("PUSH: %lu\n", p_arg);
}
