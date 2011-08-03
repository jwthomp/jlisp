#include "value.h"
#include "value_helpers.h"
#include "assert.h"
#include "vm.h"
#include "vm_exec.h"
#include "gc.h"
#include "lambda.h"
#include "err.h"
#include "reader.h"
#include "symbols.h"

#include "compile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_POOL_SIZE 256
#define MAX_BYTECODE_SIZE 1024

//value_t *compile(vm_t *p_vm, value_t *p_parameters, value_t *p_body);

void compile_form(value_t *p_form, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index, bool p_function);

value_t * eval(vm_t *p_vm, value_t * p_form);

bool is_form_macro(vm_t *p_vm, value_t *p_value)
{
	value_t *v_car = p_value->m_cons[0];

//printf("is_form_macro"); value_print(p_vm, v_car); printf("\n");
	
	if (is_symbol(v_car) == false) {
		return false;
	}

	value_t *func;

	if (v_car->m_cons[2] != voidobj) {
		func = car(p_vm, v_car->m_cons[2]);
	} else {
		value_t *v_bind = environment_binding_find(p_vm, v_car, true);

		if (v_bind == NULL) {
			return false;
		}

		verify(is_binding(v_bind), "is_form_macro: found binding is not a binding\n");

		binding_t *bind = (binding_t *)v_bind->m_data;
		func = bind->m_value;
	}

	if (is_closure(func)) {
		value_t *v_lambda = func->m_cons[1];
		verify(v_lambda && is_lambda(v_lambda), "is_form_macro: found binding value is not a lambda\n");
		lambda_t *l = (lambda_t *)v_lambda->m_data;
		if (l->m_is_macro) {
			return true;
		}
	} else if (is_ifunc(func)) {
		if (*(bool *)&func->m_data[sizeof(vm_func_t) + 4] == true) {
			return true;
		}
	}

		

	// See if 
	return false;
}

int macro_expand_1(vm_t *p_vm, value_t **p_value)
{
printf("macro_expand_1: "); value_print(p_vm, *p_value); printf("\n");
	if (is_cons(*p_value) && is_form_macro(p_vm, *p_value)) {
printf("Found macro, compiling\n");
		(*p_value)->m_cons[0]->m_type = VT_MACRO;
		eval(p_vm, *p_value, false);
		vm_exec(p_vm, p_vm->m_exp - 1, false);
		*p_value = p_vm->m_stack[p_vm->m_sp - 1];

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
		case OP_BINDD:
		case OP_BINDDF:
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
			push_opcode(p_opcode, (int)(long)p_arg, p_bytecode, p_bytecode_index);
			break;
		}
		case OP_SET_PROC_STATUS:
		case OP_DUP:
			verify(false, "op_dup are not supported. Write the code");
			break;
	}


	return 0;
}

void compile_quasi(value_t *p_form, vm_t *p_vm) {
#if 0
		if (p_vm->m_quasiquote_count > 0 && is_quasi(p_form) == false) {

			// Time to rebuild the form
			value_t *val = p_form->m_cons[0];
			vm_push(p_vm, val);
			compile_form(p_form->m_cons[1], p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
			vm_list(p_vm, 2);
		(*p_value)->m_cons[0]->m_type = VT_MACRO;
		eval(p_vm, *p_value, false);
		vm_exec(p_vm, p_vm->m_exp - 1, false);
		*p_value = p_vm->m_stack[p_vm->m_sp - 1];
		} else {
#endif
}

int compile_args(value_t *p_form, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index, bool p_is_macro)
{
//	printf("Compile args: "); value_print(p_vm, p_form); printf("\n");

	// Just run through cons until we get to a nil and compile_form them
	// Return how many there were
	value_t *val = p_form;
	int n_args = 0;
	while(val != nil && val->m_cons[0]) {
		n_args++;
		if (p_is_macro) {
			assemble(OP_PUSH, (int *)val->m_cons[0], p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
		} else {
			compile_form(val->m_cons[0], p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
		}
		val = val->m_cons[1];
	}

//	if (p_is_macro) {
//		assemble(OP_PUSH, (int *)nil, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
//	}

	return n_args;
}

bool is_quasi(vm_t *p_vm, value_t *p_form)
{
	value_t *func = p_form->m_cons[0];

printf("is_quasi: "); value_print(p_vm, func); printf("\n");

	if (is_symbol(func) == false) {
		return false;
	}

	if (is_symbol_name("QUASI-QUOTE", func) == true ||
		is_symbol_name("UNQUOTE", func) == true ||
		is_symbol_name("UNQUOTE-SPLICING", func)) {
		return true;
	}

	return false;
}

void compile_function(value_t *p_form, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index)
{
	printf("Compile function: "); value_print(p_vm, p_form); printf("\n");

	assert(p_form && is_cons(p_form));
	value_t *func = p_form->m_cons[0];
	value_t *args = p_form->m_cons[1];

printf("cf: func: "); value_print(p_vm, func); printf("\n");
printf("cf: args: "); value_print(p_vm, args); printf("\n");


	if (is_symbol(func) && is_symbol_name("QUOTE", func)) {
		
printf("QUOTE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
printf("quote: %s", valuetype_print(args->m_type)); value_print(p_vm, args); printf("\n");

		// is of form (args . nil) so only push car
		assemble(OP_PUSH, args->m_cons[0], p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else if (is_symbol(func) && is_symbol_name("QUASI-QUOTE", func)) {
//		value_t *form = car(p_vm, args);
//		p_vm->m_quasiquote_count++;
//		compile_form(form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
//		p_vm->m_quasiquote_count--;
		value_t *qt = value_create_symbol(p_vm, "QUOTE");
		value_t *ret = value_create_cons(p_vm, qt, args);
//		value_t *list = value_create_cons(p_vm, ret, nil);
		compile_form(ret, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
	} else if (is_symbol(func) && is_symbol_name("UNQUOTE", func)) {
		value_t *form = car(p_vm, args);
		verify(p_vm->m_quasiquote_count > 0, "Using UNQUOTE without corresponding QUASI-QUOTE\n");
		p_vm->m_quasiquote_count--;
		compile_form(form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
		p_vm->m_quasiquote_count++;
	} else if (is_symbol(func) && is_symbol_name("DEFVAR", func)) {
		assert(args->m_cons[0] != nil && args->m_cons[1] != nil && args->m_cons[1]->m_cons[0] != nil);

		value_t *sym = car(p_vm, args);
		value_t *form = cadr(p_vm, args);

		compile_form(form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
		assemble(OP_BIND, sym, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

	} else if (is_symbol(func) && is_symbol_name("SETQ", func)) {
		assert(args->m_cons[0] != nil && args->m_cons[1] != nil && args->m_cons[1]->m_cons[0] != nil);

		value_t *sym = car(p_vm, args);
		value_t *form = cadr(p_vm, args);

		compile_form(form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
		assemble(OP_UPDATE, sym, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

	} else if (is_symbol(func) && is_symbol_name("UNWIND-PROTECT", func)) {
		value_t * protect_form = args->m_cons[0];
		value_t * cleanup_form = args->m_cons[1]->m_cons[0];

		assemble(OP_CATCH, (int *)0, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
		int old_index = *p_bytecode_index - 1;

		compile_form(protect_form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);

		int save_index = *p_bytecode_index;
		assemble(OP_CATCH, (int *)(save_index - old_index), p_vm, p_bytecode, &old_index, p_pool, p_pool_index);
//printf("catch: %d\n", save_index - old_index);
		*p_bytecode_index = save_index;

		assemble(OP_CATCH, (int *)-1, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

		compile_form(cleanup_form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);


		// return
//		assemble(OP_RET, (int *)0, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);


	} else if (is_symbol(func) && is_symbol_name("LOOP", func)) {
		int old_index = *p_bytecode_index;
		compile_form(car(p_vm, args), p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
		assemble(OP_POP, (int *)0, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
		assemble(OP_JMP, (int *)(old_index - *p_bytecode_index), p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else if (is_symbol(func) && is_symbol_name("COND", func)) {
		while(args != nil) {
assert(is_cons(args->m_cons[0]));
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
	} else if (is_symbol(func) && is_symbol_name("DEFPARAMETER", func)) {
		assert(args->m_cons[0] && args->m_cons[1] && args->m_cons[1]->m_cons[0]);

		value_t *sym = args->m_cons[0];
		value_t *form = args->m_cons[1]->m_cons[0];

		compile_form(form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, false);
		assemble(OP_BINDD, sym, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);

	} else if (is_symbol(func) && is_symbol_name("DEFMACRO", func)) {
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


	} else if (is_symbol(func) && is_symbol_name("DEFUN", func)) {
		// (defun func (args) body) ==> (bindgf func (lambda (args) body))
		//printf("defun: "); value_print(args); printf("\n");
		value_t *sym = args->m_cons[0];
		value_t *arg_list = args->m_cons[1]->m_cons[0];
		value_t *body_list = args->m_cons[1]->m_cons[1];

//printf("largs: "); value_print(p_vm, arg_list); printf("\n");
//printf("body: "); value_print(p_vm, body_list); printf("\n");

		value_t *lambda = compile(p_vm, arg_list, body_list);
		assemble(OP_LAMBDA, lambda, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
		
		// bindf symbol
		assemble(OP_BINDDF, sym, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);



	} else if (is_symbol(func) && is_symbol_name("FUNCALL", func)) {
		value_t *closure = args->m_cons[0];
		compile_form(closure, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, true);
		assemble(OP_CALL, (int *)0, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else if (is_symbol(func) && is_symbol_name("LAMBDA", func)) {
		value_t *largs = args->m_cons[0];
		value_t *body_list = args->m_cons[1];
		value_t *lambda = compile(p_vm, largs, body_list);
		assemble(OP_LAMBDA, lambda, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else {
		// Must do test on whether this is a macro prior to calling compile_form as that will
		// switch a symbol from type VT_MACRO to type VT_SYMBOL
		bool macro = is_macro(func);

		compile_form(func, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, true);
		value_t * final_args = args;
		int argc = 0;
		argc = compile_args(final_args, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index, macro);
		assemble(OP_CALL, (int *)argc, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	}
}

void compile_form(value_t *p_form, vm_t *p_vm, 
					bytecode_t *p_bytecode, int *p_bytecode_index,
					value_t **p_pool, int *p_pool_index, bool p_function)
{
	printf("Compile form: "); value_print(p_vm, p_form); printf("\n");

	// If it's a cons, do a macroexpand in case this is a macro
	if (is_cons(p_form)) {
		macro_expand(p_vm, &p_form);
	}

	printf("Compile post macro form: "); value_print(p_vm, p_form); printf("\n");


	// If this was a macro, we need to do checks again here since the form may have changed
	if (is_cons(p_form) && (p_vm->m_quasiquote_count == 0 || is_quasi(p_vm, p_form) == true)) { 
		compile_function(p_form, p_vm, p_bytecode, p_bytecode_index, p_pool, p_pool_index);
	} else if (p_vm->m_quasiquote_count == 0 && (is_symbol(p_form) || (is_macro(p_form)))) {
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

 //printf("POOL: %p\n", pool);

//printf("nil: %p param: %p -> ", nil, p_parameters); value_print(p_vm, p_parameters); printf("\n");

	assert(p_parameters == NULL || p_parameters == nil || is_cons(p_parameters));


	value_t *param = p_parameters;
	while((param != nil) && param->m_cons[0]) {
		assert(is_symbol(param->m_cons[0]));
		param = param->m_cons[1];
	}

	assert(p_body != NULL);
	verify(p_body != nil, "compile: p_body is nil\n");
	verify(is_cons(p_body), "compile: p_body is not a cons\n");
//value_print(p_vm, p_body); printf("\n");

	value_t *val = p_body;
	while(val && val != nil) {
//printf("compile!: "); value_print(p_vm, val->m_cons[0]); printf("\n");
		compile_form(val->m_cons[0], p_vm, (bytecode_t *)bytecode, &bytecode_index, (value_t **)pool, &pool_index, false);
		val = val->m_cons[1];
	}

	bytecode_t bc = {OP_NOP, 0};
	bytecode[bytecode_index++] = bc;
	bc.m_opcode = OP_RET;
	bc.m_value = 0;
	bytecode[bytecode_index++] = bc;

	bytecode_t *bc_allocd = (bytecode_t *)malloc(sizeof(bytecode_t) * bytecode_index);
	memcpy(bc_allocd, bytecode, sizeof(bytecode_t) * bytecode_index);
	value_t *bytecode_final = value_create_bytecode(p_vm, bc_allocd, bytecode_index);
	free(bc_allocd);
	value_t *pool_final = value_create_pool(p_vm, pool, pool_index);

 //printf("POOL OUT: %p\n", pool);
	return value_create_lambda(p_vm, p_parameters, bytecode_final, pool_final, p_body);
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


value_t *optimize(vm_t *p_vm, value_t *p_lambda)
{
	// Tail recursion
	// Replace all OP_CALL/OP_RET's with OP_RETCALL
	lambda_t *l = (lambda_t *)p_lambda->m_data;
	assert(p_lambda->m_type == VT_LAMBDA);

	value_t *bc_val = l->m_bytecode;
	bytecode_t *bc = (bytecode_t *)bc_val->m_data;
	int instructions = bc_val->m_size / sizeof(bytecode_t);

	for (int i = 1; i < instructions; i++) {
		if (bc[i].m_opcode == OP_RET && bc[i - 1].m_opcode == OP_CALL) {
			bc[i - 1].m_opcode = OP_RETCALL;
			bc[i].m_opcode = OP_NOP;
		}
	}


	return p_lambda;
}


value_t * eval(vm_t *p_vm, value_t * p_form, bool p_pop)
{
	if (g_debug_display == true) {
		printf("eval: "); value_print(p_vm, p_form), printf("\n");
	}

	value_t *lambda = compile(p_vm, nil, list(p_vm, p_form));

	if (p_pop == true) {
		lambda_t *l = (lambda_t *)lambda->m_data;
		value_t *bc_val = l->m_bytecode;
		bytecode_t *bc = (bytecode_t *)bc_val->m_data;
		int instructions = bc_val->m_size / sizeof(bytecode_t);
		assert(bc[instructions - 2].m_opcode == OP_NOP);
		bc[instructions - 2].m_opcode = OP_POP;
	}
	

	lambda = optimize(p_vm, lambda);
	value_t *closure =  make_closure(p_vm, lambda);

	vm_push_exec_state(p_vm, closure);
	p_vm->m_ip = 0;
	p_vm->m_bp = p_vm->m_bp; // + 1;


	return p_vm->m_stack[p_vm->m_sp - 1];
}
