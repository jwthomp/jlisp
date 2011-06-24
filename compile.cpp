#include "value.h"
#include "value_helpers.h"
#include "assert.h"
#include "vm.h"
#include "gc.h"
#include "lambda.h"
#include "err.h"
#include "reader.h"

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

bool is_form_macro(vm_t *p_vm, value_t *p_value)
{
	value_t *v_car = p_value->m_cons[0];
	
	if (is_symbol(p_vm, v_car) == false) {
		return false;
	}

	value_t *v_env = p_vm->m_current_env[p_vm->m_ev -1];
	verify(v_env && is_environment(p_vm, v_env), "is_form_macro: env is null or not an environment\n");
	environment_t *env = (environment_t *)v_env->m_data;

	value_t *v_fbindings = env->m_function_bindings;
	if (v_fbindings == NULL) {
		return false;
	}

	verify(v_fbindings && is_binding(p_vm, v_fbindings), "is_form_macro: fbindings are null or not a binding\n");

	value_t *v_bind = binding_find(p_vm, v_fbindings, v_car);
	if (v_bind == NULL) {
		return false;
	}

	verify(is_binding(p_vm, v_bind), "is_form_macro: found binding is not a binding\n");

	binding_t *bind = (binding_t *)v_bind->m_data;

	if (bind && bind->m_value && is_closure(p_vm, bind->m_value)) {
		value_t *v_lambda = bind->m_value->m_cons[1];
		verify(v_lambda && is_lambda(p_vm, v_lambda), "is_form_macro: found binding value is not a lambda\n");
		lambda_t *l = (lambda_t *)v_lambda->m_data;
		if (l->m_is_macro) {
			return true;
		}
	} else if (bind && bind->m_value && is_ifunc(p_vm, bind->m_value)) {
		value_t *ifunc = bind->m_value;
		if (*(bool *)&ifunc->m_data[sizeof(vm_func_t) + 4] == true) {
			return true;
		}
	}

	// See if 
	return false;
}

int macro_expand_1(vm_t *p_vm, value_t **p_value)
{
	if (is_cons(p_vm, *p_value) && is_form_macro(p_vm, *p_value)) {
//printf("Found macro, compiling\n");
		(*p_value)->m_cons[0]->m_type = VT_MACRO;
		*p_value = eval(p_vm, *p_value);

		return 1;
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
	if (g_debug_display == true) {
		printf("push opcode(%d): %s %d\n", *p_bytecode_index, g_opcode_print[p_opcode], p_arg);
	}
	(p_bytecode)[*p_bytecode_index].m_opcode = p_opcode;
	(p_bytecode)[*p_bytecode_index].m_value = p_arg;
	(*p_bytecode_index)++;


}

int push_pool(vm_t *p_vm, value_t *p_value, value_t **p_pool, int *p_pool_index)
{
	if (g_debug_display == true) {
		printf("push pool: "); value_print(p_vm, p_value); printf("\n");
	}
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
		case OP_LOADF:
		case OP_PUSH:
		case OP_UPDATE:
		case OP_BIND:
		case OP_BINDGF:
		case OP_BINDG:
		case OP_BINDF:
		case OP_LAMBDA:
		{
			int index = push_pool(p_vm, (value_t *)p_arg, p_pool, p_pool_index);
			push_opcode(p_opcode, index, p_bytecode, p_bytecode_index);
			break;
		}
		case OP_POP:
		case OP_JMP:
		case OP_IFNILJMP:
		case OP_CATCH:
		case OP_RET:
		{
			push_opcode(p_opcode, (int)p_arg, p_bytecode, p_bytecode_index);
			break;
		}
		case OP_BINDD:
		case OP_BINDDF:
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
//	printf("Compile args: "); value_print(p_form); printf("\n");

	// Just run through cons until we get to a nil and compile_form them
	// Return how many there were
	value_t *val = p_form;
	int n_args = 0;
	while(val != p_vm->nil && val->m_cons[0]) {
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
	//printf("Compile function: "); value_print(p_vm, p_form); printf("\n");

	assert(p_form && is_cons(p_vm, p_form));
	value_t *func = p_form->m_cons[0];
	value_t *args = p_form->m_cons[1];

//printf("cf: func: "); value_print(p_vm, func); printf("\n");
//printf("cf: args: "); value_print(p_vm, args); printf("\n");

	if (is_symbol(p_vm, func) && is_symbol_name("QUOTE", func)) {
		
//printf("quote: %d", args->m_cons[0]->m_type); value_print(args->m_cons[0]); printf("\n");

		// is of form (args . nil) so only push car
		assemble(OP_PUSH, args->m_cons[0], p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else if (is_symbol(p_vm, func) && is_symbol_name("DEFVAR", func)) {
		assert(args->m_cons[0] != p_vm->nil && args->m_cons[1] != p_vm->nil && args->m_cons[1]->m_cons[0] != p_vm->nil);

		value_t *sym = car(p_vm, args);
		value_t *form = cadr(p_vm, args);

		compile_form(form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
		assemble(OP_BIND, sym, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

	} else if (is_symbol(p_vm, func) && is_symbol_name("SETQ", func)) {
		assert(args->m_cons[0] != p_vm->nil && args->m_cons[1] != p_vm->nil && args->m_cons[1]->m_cons[0] != p_vm->nil);

		value_t *sym = car(p_vm, args);
		value_t *form = cadr(p_vm, args);

		compile_form(form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
		assemble(OP_UPDATE, sym, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

	} else if (is_symbol(p_vm, func) && is_symbol_name("UNWIND-PROTECT", func)) {
		value_t * protect_form = args->m_cons[0];
		value_t * cleanup_form = args->m_cons[1]->m_cons[0];

		assemble(OP_CATCH, (int *)0, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
		int old_index = *p_bytecode_index - 1;

		compile_form(protect_form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);

		int save_index = *p_bytecode_index;
		assemble(OP_CATCH, (int *)(save_index - old_index), p_vm, p_bytecode, &old_index, p_pool, p_pool_index);
//printf("catch: %d\n", save_index - old_index);
		*p_bytecode_index = save_index;

		compile_form(cleanup_form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);

		// return
		assemble(OP_RET, (int *)0, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);


	} else if (is_symbol(p_vm, func) && is_symbol_name("LOOP", func)) {
		int old_index = *p_bytecode_index;
		compile_form(car(p_vm, args), p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
		assemble(OP_POP, (int *)0, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
		assemble(OP_JMP, (int *)(old_index - *p_bytecode_index), p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else if (is_symbol(p_vm, func) && is_symbol_name("COND", func)) {
		while(args != p_vm->nil) {
assert(is_cons(p_vm, args->m_cons[0]));
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
	} else if (is_symbol(p_vm, func) && is_symbol_name("DEFPARAMETER", func)) {
		assert(args->m_cons[0] && args->m_cons[1] && args->m_cons[1]->m_cons[0]);

		value_t *sym = args->m_cons[0];
		value_t *form = args->m_cons[1]->m_cons[0];

		compile_form(form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
		assemble(OP_BINDG, sym, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

	} else if (is_symbol(p_vm, func) && is_symbol_name("DEFMACRO", func)) {
		value_t *sym = args->m_cons[0];
		value_t *largs = args->m_cons[1]->m_cons[0];
		value_t *body_list = args->m_cons[1]->m_cons[1];

		// compile args, body
		value_t *lambda = compile(p_vm, largs, body_list);

		// Flag this lambda as a macro
		((lambda_t *)lambda->m_data)->m_is_macro = true;

		// lambda
		assemble(OP_LAMBDA, lambda, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

		// bindf symbol
		assemble(OP_BINDGF, sym, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);


	} else if (is_symbol(p_vm, func) && is_symbol_name("DEFUN", func)) {
		// (defun func (args) body) ==> (bindgf func (lambda (args) body))
		//printf("defun: "); value_print(args); printf("\n");
		value_t *sym = args->m_cons[0];
		value_t *arg_list = args->m_cons[1]->m_cons[0];
		value_t *body_list = args->m_cons[1]->m_cons[1];

//printf("largs: "); value_print(arg_list); printf("\n");
//printf("body: "); value_print(body_list); printf("\n");

		value_t *lambda = compile(p_vm, arg_list, body_list);
		assemble(OP_LAMBDA, lambda, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
		
		// bindf symbol
		assemble(OP_BINDGF, sym, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);



	} else if (is_symbol(p_vm, func) && is_symbol_name("FUNCALL", func)) {
		value_t *closure = args->m_cons[0];
		compile_form(closure, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, true);
		assemble(OP_CALL, (int *)0, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else if (is_symbol(p_vm, func) && is_symbol_name("LAMBDA", func)) {
		value_t *largs = args->m_cons[0];
		value_t *body_list = args->m_cons[1];
		value_t *lambda = compile(p_vm, largs, body_list);
		assemble(OP_LAMBDA, lambda, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else {
		// Must do test on whether this is a macro prior to calling compile_form as that will
		// switch a symbol from type VT_MACRO to type VT_SYMBOL
		bool macro = is_macro(p_vm, func);

		compile_form(func, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, true);
		value_t * final_args = args;
		int argc = 1;
		if (macro) {
			assemble(OP_PUSH, (int *)final_args, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
		} else {
			argc = compile_args(final_args, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
		}
		assemble(OP_CALL, (int *)argc, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	}
}

void compile_form(value_t *p_form, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index, bool p_function)
{
	//printf("Compile form: "); value_print(p_vm, p_form); printf("\n");

	// If it's a cons, do a macroexpand in case this is a macro
	if (is_cons(p_vm, p_form)) {
		macro_expand(p_vm, &p_form);
	}

	//printf("Compile post macro form: "); value_print(p_vm, p_form); printf("\n");

	// If this was a macro, we need to do checks again here since the form may have changed
	if (is_cons(p_vm, p_form)) {
		compile_function(p_form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else if (is_symbol(p_vm, p_form) || (is_macro(p_vm, p_form))) {
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

// printf("POOL: %p\n", pool);

//printf("nil: %p param: %p -> ", p_vm->nil, p_parameters); value_print(p_parameters); printf("\n");

	assert(p_parameters == NULL || p_parameters == p_vm->nil || is_cons(p_vm, p_parameters));


	value_t *param = p_parameters;
	while((param != p_vm->nil) && param->m_cons[0]) {
		assert(is_symbol(p_vm, param->m_cons[0]));
		param = param->m_cons[1];
	}

	assert(p_body != NULL);
	verify(p_body != p_vm->nil, "compile: p_body is nil\n");
	verify(is_cons(p_vm, p_body), "compile: p_body is not a cons\n");

	value_t *val = p_body;
	while(val && val != p_vm->nil) {
//printf("compile!: "); value_print(val->m_cons[0]); printf("\n");
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

// printf("POOL OUT: %p\n", pool);
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
	value_t *lambda = compile(p_vm, p_vm->nil, list(p_vm, p_form));
	value_t *closure =  make_closure(p_vm, lambda);

	vm_exec(p_vm, &closure, 0);
	p_vm->m_stack[p_vm->m_sp - 2] = p_vm->m_stack[p_vm->m_sp - 1];
	p_vm->m_sp--;

	return p_vm->m_stack[p_vm->m_sp - 1];
}
