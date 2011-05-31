#include "value.h"
#include "value_helpers.h"
#include "assert.h"
#include "vm.h"
#include "gc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_POOL_SIZE 256
#define MAX_BYTECODE_SIZE 1024

value_t *compile(vm_t *p_vm, value_t *p_parameters, value_t *p_body);

int compile_form(value_t *p_form, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index, bool p_function);


void push_opcode(opcode_e p_opcode, int p_arg, bytecode_t *p_bytecode, int *p_bytecode_index)
{
	//printf("push opcode: %s %u\n", g_opcode_print[p_opcode], p_arg);
	(p_bytecode)[*p_bytecode_index].m_opcode = p_opcode;
	(p_bytecode)[*p_bytecode_index].m_value = p_arg;
	(*p_bytecode_index)++;
}

int push_pool(value_t *p_value, value_t **p_pool, int *p_pool_index)
{
	//printf("push pool: "); value_print(p_value); printf("\n");
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
			push_opcode(OP_CALL, (long)p_arg, p_bytecode, p_bytecode_index);
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
		case OP_BIND:
		{
			int index = push_pool((value_t *)p_arg, p_pool, p_pool_index);
			push_opcode(OP_BIND, index, p_bytecode, p_bytecode_index);
			break;
		}
		case OP_BINDGF:
		{
			int index = push_pool((value_t *)p_arg, p_pool, p_pool_index);
			push_opcode(OP_BINDGF, index, p_bytecode, p_bytecode_index);
			break;
		}
		case OP_BINDG:
		{
			int index = push_pool((value_t *)p_arg, p_pool, p_pool_index);
			push_opcode(OP_BINDG, index, p_bytecode, p_bytecode_index);
			break;
		}
		case OP_BINDF:
		{
			int index = push_pool((value_t *)p_arg, p_pool, p_pool_index);
			push_opcode(OP_BINDF, index, p_bytecode, p_bytecode_index);
			break;
		}
		case OP_IFNILJMP:
		{
			push_opcode(OP_IFNILJMP, (int)p_arg, p_bytecode, p_bytecode_index);
			break;
		}
		case OP_RET:
		{
			push_opcode(OP_RET, (int)p_arg, p_bytecode, p_bytecode_index);
			break;
		}
		case OP_LAMBDA:
		{
			int index = push_pool((value_t *)p_arg, p_pool, p_pool_index);
//printf("pushed lambda to index: %d\n", index);
			push_opcode(OP_LAMBDA, index, p_bytecode, p_bytecode_index);
			break;
		}
	}


	return 0;
}

int compile_args(value_t *p_form, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index)
{
	//printf("Compile args: "); value_print(p_form); printf("\n");

	// Just run through cons until we get to a nil and compile_form them
	// Return how many there were
	value_t *val = p_form;
	int n_args = 0;
	while(val && val->m_cons[0]) {
		n_args++;
		compile_form(val->m_cons[0], p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
		val = val->m_cons[1];
	}
		


	return n_args;
}

void compile_function(value_t *p_form, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index)
{
	//printf("Compile function: "); value_print(p_form); printf("\n");

	assert(p_form && is_cons(p_form));
	value_t *func = p_form->m_cons[0];
	value_t *args = p_form->m_cons[1];

	if ((func->m_type == VT_SYMBOL) && !strcmp("quote", func->m_data)) {
		
		assert(args->m_cons[1] && args->m_cons[1]->m_cons[0] == NULL);

//printf("quote: %d", args->m_cons[0]->m_type); value_print(args->m_cons[0]); printf("\n");

		// is of form (args . nil) so only push car
		assemble(OP_PUSH, args->m_cons[0], p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else if ((func->m_type == VT_SYMBOL) && !strcmp("setq", func->m_data)) {
		assert(args->m_cons[0] && args->m_cons[1] && args->m_cons[1]->m_cons[0]);
//		assert(args->m_cons[1]->m_cons[1] == NULL);

		value_t *sym = args->m_cons[0];
		value_t *form = args->m_cons[1]->m_cons[0];

		compile_form(form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
		assemble(OP_BIND, sym, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else if ((func->m_type == VT_SYMBOL) && !strcmp("cond", func->m_data)) {
		while(args->m_cons[0]) {
			value_t * test = args->m_cons[0]->m_cons[0];
			value_t * if_true = args->m_cons[0]->m_cons[1]->m_cons[0];

			// compile test
			compile_form(test, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);

			// Call test
//			assemble(OP_CALL, (int *)0, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

			// Ifniljump 2
			assemble(OP_IFNILJMP, (int *)3, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

			int old_index = *p_bytecode_index - 1;

			// compile_form body
			compile_form(if_true, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
//			assemble(OP_CALL, (int *)0, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

			// return
			assemble(OP_RET, (int *)0, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

			int save_index = *p_bytecode_index;
			assemble(OP_IFNILJMP, (int *)(save_index - old_index), p_vm, p_bytecode, &old_index, p_pool, p_pool_index);
			*p_bytecode_index = save_index;
			

			// Next test
			args = args->m_cons[1];
		}
//printf("cond: "); value_print(args->m_cons[0]); printf("\n");
	} else if ((func->m_type == VT_SYMBOL) && !strcmp("defparameter", func->m_data)) {
		assert(args->m_cons[0] && args->m_cons[1] && args->m_cons[1]->m_cons[0]);

		value_t *sym = args->m_cons[0];
		value_t *form = args->m_cons[1]->m_cons[0];

		compile_form(form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
		assemble(OP_BINDG, sym, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

	} else if ((func->m_type == VT_SYMBOL) && !strcmp("defun", func->m_data)) {
		//printf("defun: "); value_print(args); printf("\n");
		value_t *sym = args->m_cons[0];
		value_t *largs = args->m_cons[1]->m_cons[0];
		value_t *body = args->m_cons[1]->m_cons[1]->m_cons[0];

		// compile args, body
		value_t *lambda = compile(p_vm, largs, list(p_vm, body));

		// lambda
		assemble(OP_LAMBDA, lambda, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

		// bindf symbol
		assemble(OP_BINDGF, sym, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

	} else if ((func->m_type == VT_SYMBOL) && !strcmp("lambda", func->m_data)) {
//		printf("lambda: "); value_print(args->m_cons[1]->m_cons[0]); printf("\n");
		value_t *largs = args->m_cons[0];
		value_t *body = args->m_cons[1]->m_cons[0];
		value_t *lambda = compile(p_vm, largs, list(p_vm, body));
//printf("lambda: "); value_print(lambda); printf("\n");
		assemble(OP_LAMBDA, lambda, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else {
		compile_form(func, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, true);
		int argc = compile_args(args, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
		assemble(OP_CALL, (int *)argc, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	}
}

int compile_form(value_t *p_form, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index, bool p_function)
{
	//printf("Compile form: "); value_print(p_form); printf("\n");

	if (is_cons(p_form)) {
		compile_function(p_form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else if (is_symbol(p_form)) {
//printf("cf: sym\n");
		opcode_e opcode = p_function ? OP_LOADF : OP_LOAD;
		assemble(opcode, p_form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else {
//printf("cf: num: %d\n", p_form->m_type);
		assemble(OP_PUSH, p_form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	}
}

value_t *compile(vm_t *p_vm, value_t *p_parameters, value_t *p_body)
{
	value_t *pool[MAX_POOL_SIZE];
	bytecode_t bytecode[MAX_BYTECODE_SIZE];
	int pool_index = 0;
	int bytecode_index = 0;

//printf("POOL: %p\n", pool);

	assert(p_parameters == NULL || is_cons(p_parameters));

	value_t *param = p_parameters;
	while(param && param->m_cons[0]) {
		assert(is_symbol(param->m_cons[0]));
		param = param->m_cons[1];
	}

	assert(p_body && is_cons(p_body));
	value_t *val = p_body;
	while(val) {
		compile_form(val->m_cons[0], p_vm, (bytecode_t *)bytecode, &bytecode_index, (value_t **)pool, &pool_index, false);
		val = val->m_cons[1];
	}

	bytecode_t bc = {OP_RET, 0};
	bytecode[bytecode_index++] = bc;

	bytecode_t *bc_allocd = (bytecode_t *)malloc(sizeof(bytecode_t) * bytecode_index);
	memcpy(bc_allocd, bytecode, sizeof(bytecode_t) * bytecode_index);
	value_t *bytecode_final = value_create_bytecode(p_vm, bc_allocd, bytecode_index);
	free(bc_allocd);
	value_t *pool_final = value_create_pool(p_vm, pool, pool_index);

//printf("POOL OUT: %p\n", pool);
	return value_create_lambda(p_vm, p_parameters, bytecode_final, pool_final);
}

value_t * make_closure(vm_t *p_vm, value_t *p_lambda)
{
	value_t *env = value_create_environment(p_vm, p_vm->m_current_env[p_vm->m_ev - 1]);
	return value_create_closure(p_vm, env, p_lambda);
}

value_t *execute(vm_t *p_vm, value_t *p_closure)
{
	// Push the env
	// pop the env
	return p_closure;
}

value_t * eval(vm_t *p_vm, value_t * p_form)
{
	value_t *lambda = compile(p_vm, NULL, list(p_vm, p_form));
	value_t *closure =  make_closure(p_vm, lambda);
	vm_exec(p_vm, closure, 0);

	return p_vm->m_stack[p_vm->m_sp - 1];
}
