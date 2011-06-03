#include "value.h"
#include "value_helpers.h"
#include "assert.h"
#include "vm.h"
#include "gc.h"
#include "lambda.h"
#include "err.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_POOL_SIZE 256
#define MAX_BYTECODE_SIZE 1024

value_t *compile(vm_t *p_vm, value_t *p_parameters, value_t *p_body);

void compile_form(value_t *p_form, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index, bool p_function);

value_t * eval(vm_t *p_vm, value_t * p_form);

bool is_macro(vm_t *p_vm, value_t *p_value)
{
	value_t *v_car = p_value->m_cons[0];
	
	if (v_car->m_type != VT_SYMBOL) {
		return false;
	}

	value_t *v_env = p_vm->m_current_env[p_vm->m_ev -1];
	verify(v_env && v_env->m_type == VT_ENVIRONMENT, "is_macro: env is null or not an environment\n");
	environment_t *env = (environment_t *)v_env->m_data;

	value_t *v_fbindings = env->m_function_bindings;
	if (v_fbindings == NULL) {
		return false;
	}

	verify(v_fbindings && v_fbindings->m_type == VT_BINDING, "is_macro: fbindings are null or not a binding\n");

	value_t *v_bind = binding_find(v_fbindings, v_car);
	if (v_bind == NULL) {
		return false;
	}

	verify(v_bind->m_type == VT_BINDING, "is_macro: found binding is not a binding\n");

	binding_t *bind = (binding_t *)v_bind->m_data;

	if (bind && bind->m_value && (bind->m_value->m_type == VT_CLOSURE)) {
		value_t *v_lambda = bind->m_value->m_cons[1];
		verify(v_lambda && v_lambda->m_type == VT_LAMBDA, "is_macro: found binding value is not a lambda\n");
		lambda_t *l = (lambda_t *)v_lambda->m_data;
		if (l->m_is_macro) {
			return true;
		}
	}

	// See if 
	return false;
}

int macro_expand_1(vm_t *p_vm, value_t **p_value)
{
	if (is_cons(*p_value) && is_macro(p_vm, *p_value)) {
		(*p_value)->m_cons[0]->m_type = VT_MACRO;
		*p_value = eval(p_vm, *p_value);
		return 1;
printf("Found macro! "); value_print((*p_value)->m_cons[0]); printf("\n");
	} else {
		return 0;
	}
}

void macro_expand(vm_t *p_vm, value_t **p_value)
{
	while(macro_expand_1(p_vm, p_value) == 1) { }
	return;
}

void push_opcode(opcode_e p_opcode, int p_arg, bytecode_t *p_bytecode, int *p_bytecode_index)
{
	printf("push opcode(%d): %s %u\n", *p_bytecode_index, g_opcode_print[p_opcode], p_arg);
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
		case OP_CATCH:
		{
			push_opcode(OP_CATCH, (int)p_arg, p_bytecode, p_bytecode_index);
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
		case OP_DUP:
			verify(false, "op_dup are not supported. Write the code");
			break;
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

printf("cf: "); value_print(args); printf("\n");

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

	} else if ((func->m_type == VT_SYMBOL) && !strcmp("unwind-protect", func->m_data)) {
		value_t * protect_form = args->m_cons[0];
		value_t * cleanup_form = args->m_cons[1]->m_cons[0];

		assemble(OP_CATCH, (int *)0, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
		int old_index = *p_bytecode_index - 1;

		compile_form(protect_form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);

		int save_index = *p_bytecode_index;
		assemble(OP_CATCH, (int *)(save_index - old_index), p_vm, p_bytecode, &old_index, p_pool, p_pool_index);
printf("catch: %d\n", save_index - old_index);
		*p_bytecode_index = save_index;

		compile_form(cleanup_form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);

		// return
		assemble(OP_RET, (int *)0, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);


	} else if ((func->m_type == VT_SYMBOL) && !strcmp("cond", func->m_data)) {
		while(args->m_cons[0]) {
			value_t * test = args->m_cons[0]->m_cons[0];
			value_t * if_true = args->m_cons[0]->m_cons[1]->m_cons[0];

			// compile test
			compile_form(test, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);

			// Ifniljump 2
			assemble(OP_IFNILJMP, (int *)3, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

			int old_index = *p_bytecode_index - 1;

			// compile_form body
			compile_form(if_true, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);

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

	} else if ((func->m_type == VT_SYMBOL) && !strcmp("defmacro", func->m_data)) {
		value_t *sym = args->m_cons[0];
		value_t *largs = args->m_cons[1]->m_cons[0];
		value_t *body = args->m_cons[1]->m_cons[1]->m_cons[0];

		// compile args, body
		value_t *lambda = compile(p_vm, largs, list(p_vm, body));

		// Flag this lambda as a macro
		((lambda_t *)lambda->m_data)->m_is_macro = true;

		// lambda
		assemble(OP_LAMBDA, lambda, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

		// bindf symbol
		assemble(OP_BINDGF, sym, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);


	} else if ((func->m_type == VT_SYMBOL) && !strcmp("defun", func->m_data)) {
		//printf("defun: "); value_print(args); printf("\n");
		value_t *sym = args->m_cons[0];
		value_t *body = args->m_cons[1]->m_cons[1]->m_cons[0];
		value_t *largs = nil;

		if (args->m_cons[1] != NULL && (args->m_cons[1] != nil) && args->m_cons[1]->m_cons[0] != NULL) {
			largs = args->m_cons[1]->m_cons[0];
		}

printf("largs: "); value_print(largs); printf("\n");
printf("body: "); value_print(body); printf("\n");

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

void compile_form(value_t *p_form, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index, bool p_function)
{
	printf("Compile form: "); value_print(p_form); printf("\n");

	// If it's a cons, do a macroexpand in case this is a macro
	if (is_cons(p_form)) {
		macro_expand(p_vm, &p_form);
	}

	// If this was a macro, we need to do checks again here since the form may have changed
	if (is_cons(p_form)) {
		compile_function(p_form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else if (is_symbol(p_form) || (p_form->m_type == VT_MACRO)) {
		// Change type back to symbol
		p_form->m_type = VT_SYMBOL;
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

printf("POOL: %p\n", pool);

printf("nil: %p param: %p -> ", nil, p_parameters); value_print(p_parameters); printf("\n");

	assert(p_parameters == NULL || p_parameters == nil || is_cons(p_parameters));

	value_t *param = p_parameters;
	while(param && (param != nil) && param->m_cons[0]) {
		assert(is_symbol(param->m_cons[0]));
		param = param->m_cons[1];
	}

	assert(p_body != NULL);
	verify(p_body != nil, "compile: p_body is nil\n");
	verify(is_cons(p_body), "compile: p_body is not a cons\n");

	value_t *val = p_body;
	while(val && val != nil) {
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

printf("POOL OUT: %p\n", pool);
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
