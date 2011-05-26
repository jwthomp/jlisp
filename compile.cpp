#include "value.h"
#include "value_helpers.h"
#include "assert.h"
#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_POOL_SIZE 256
#define MAX_BYTECODE_SIZE 1024

int compile_form(value_t *p_form, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index, bool p_function);


void push_opcode(opcode_e p_opcode, int p_arg, bytecode_t *p_bytecode, int *p_bytecode_index)
{
	printf("push opcode: %s %u\n", g_opcode_print[p_opcode], p_arg);
	(p_bytecode)[*p_bytecode_index].m_opcode = p_opcode;
	(p_bytecode)[*p_bytecode_index].m_value = p_arg;
	(*p_bytecode_index)++;
}

int push_pool(value_t *p_value, value_t **p_pool, int *p_pool_index)
{
	printf("push pool: "); value_print(p_value); printf("\n");
	(p_pool)[*p_pool_index] = p_value;
	(*p_pool_index)++;
	return (*p_pool_index) - 1;
}


int assemble(opcode_e p_opcode, void *p_arg, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index)
{
	switch (p_opcode) {
		case OP_CALL:
			push_opcode(OP_CALL, (int)p_arg, p_bytecode, p_bytecode_index);
			break;
		case OP_LOAD:
		{
			int index = push_pool((value_t *)p_arg, p_pool, p_pool_index);
			push_opcode(OP_LOAD, index, p_bytecode, p_bytecode_index);
			break;
		}
		case OP_LOADF:
		{
			int index = push_pool((value_t *)p_arg, p_pool, p_pool_index);
			push_opcode(OP_LOADF, index, p_bytecode, p_bytecode_index);
			break;
		}
		case OP_PUSH:
		{
			int index = push_pool((value_t *)p_arg, p_pool, p_pool_index);
			push_opcode(OP_PUSH, index, p_bytecode, p_bytecode_index);
			break;
		}
	}


	return 0;
}

int compile_args(value_t *p_form, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index)
{
	printf("Compile args: "); value_print(p_form); printf("\n");

	// Just run through cons until we get to a nil and compile_form them
	// Return how many there were
	value_t *val = p_form;
	int n_args = 0;
	while(val && val->m_cons[0]) {
		n_args++;
		compile_form(val->m_cons[0], p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
		val = val->m_cons[1];
	}
		


	return 0;
}

int compile_function(value_t *p_form, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index)
{
	printf("Compile function: "); value_print(p_form); printf("\n");

	assert(p_form && is_cons(p_form));
	value_t *func = p_form->m_cons[0];
	value_t *args = p_form->m_cons[1];

	compile_form(func, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, true);
	int argc = compile_args(args, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	assemble(OP_CALL, (int *)argc, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
}

int compile_form(value_t *p_form, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index, bool p_function)
{
	printf("Compile form: "); value_print(p_form); printf("\n");

	if (is_cons(p_form)) {
		compile_function(p_form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else if (is_symbol(p_form)) {
		opcode_e opcode = p_function ? OP_LOADF : OP_LOAD;
		assemble(opcode, p_form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else {
		assemble(OP_PUSH, p_form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	}
}

value_t *compile(vm_t *p_vm, value_t *p_parameters, value_t *p_body)
{
	value_t *pool[MAX_POOL_SIZE];
	bytecode_t bytecode[MAX_BYTECODE_SIZE];
	int pool_index = 0;
	int bytecode_index = 0;

	assert(p_parameters == NULL || is_cons(p_parameters));

	value_t *param = p_parameters;
	while(param) {
		assert(is_symbol(param->m_cons[0]));
		param = param->m_cons[1];
	}

	assert(p_body && is_cons(p_body));
	value_t *val = p_body;
	while(val) {
		compile_form(val->m_cons[0], p_vm, (bytecode_t *)bytecode, &bytecode_index, (value_t **)pool, &pool_index, false);
		val = val->m_cons[1];
	}

		printf("op: %lu %lu %p\n", bytecode[0].m_opcode, (unsigned long)bytecode_index, &bytecode_index);

	bytecode_t *bc_allocd = (bytecode_t *)malloc(sizeof(bytecode_t) * bytecode_index);
	memcpy(bc_allocd, bytecode, sizeof(bytecode_t) * bytecode_index);
	value_t *bytecode_final = value_create_bytecode(bc_allocd, bytecode_index);
	value_t *pool_final = value_create_pool(pool, pool_index);

	return value_create_lambda(p_parameters, bytecode_final, pool_final);
}

value_t * make_closure(vm_t *p_vm, value_t *p_lambda)
{
	value_t *env = value_create_environment(p_vm->m_current_env);
	return value_create_closure(env, p_lambda);
}

value_t *execute(vm_t *p_vm, value_t *p_closure)
{
	return p_closure;
}

value_t * eval(vm_t *p_vm, value_t *p_form)
{
	value_t *lambda = compile(p_vm, NULL, list(p_form));
	 printf("lambda: "); value_print(lambda); printf("\n");

	value_t *closure =  make_closure(p_vm, lambda);
	return execute(p_vm, closure);
}
