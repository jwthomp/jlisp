#include "env.h"
#include "binding.h"
#include "value_helpers.h"
#include "assert.h"
#include "lambda.h"
#include "compile.h"
#include "gc.h"

#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct bytecode_jump {
	bytecode_t m_bytecode;
	void (*m_func)(vm_t *, unsigned long, value_t *);
} bytecode_jump_t;


void op_push(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_bind(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_bindg(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_bindgf(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_bindf(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_print(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_dup(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_load(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_loadf(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_call(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_lambda(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_ifniljmp(vm_t *, unsigned long p_arg, value_t *p_pool);
void op_ret(vm_t *, unsigned long p_arg, value_t *p_pool);

char const *g_opcode_print[] =  {
    "OP_PUSH",
    "OP_BIND",
    "OP_BINDF",
    "OP_PRINT",
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
	{ {OP_BINDG, 0}, &op_bindg },
	{ {OP_IFNILJMP, 0}, &op_ifniljmp },
	{ {OP_RET, 0}, &op_ret },
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
printf("free vm: %p\n", p_vm);
	free(p_vm->m_stack);
	free(p_vm->m_current_env);
	free(p_vm);
}

void exec_instruction(vm_t *p_vm, bytecode_t p_bc, value_t *p_pool)
{
	printf("exec instruction: %d -> %s -- %lu > ", p_vm->m_ip, g_opcode_print[p_bc.m_opcode], p_bc.m_value);
	switch(p_bc.m_opcode) {
		case OP_LOADF:
		case OP_LOAD:
		case OP_PUSH:
			value_print(((value_t **)p_pool->m_data)[p_bc.m_value]);
			break;
		default:
			break;
	}

	printf("\n");

	g_bytecode[p_bc.m_opcode].m_func(p_vm, p_bc.m_value, p_pool);
}


void op_push(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	p_vm->m_stack[p_vm->m_sp++] = ((value_t **)p_pool->m_data)[p_arg];
	p_vm->m_ip++;
}

void op_bind(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
	value_t *binding = value_create_binding(p_vm, sym, p_vm->m_stack[p_vm->m_sp - 1]);
	binding_t *bind = (binding_t *)binding->m_data;

	bind->m_next = ((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_bindings;
assert((bind->m_next == NULL) || (bind->m_next->m_type == VT_BINDING));
assert(binding->m_type == VT_BINDING);
	((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_bindings = binding;
	p_vm->m_ip++;
}

void op_bindf(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
	value_t *binding = value_create_binding(p_vm, sym, p_vm->m_stack[p_vm->m_sp - 1]);
	binding_t *bind = (binding_t *)binding->m_data;

	bind->m_next = ((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_function_bindings;
assert((bind->m_next == NULL) || (bind->m_next->m_type == VT_BINDING));
assert(binding->m_type == VT_BINDING);
	((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_function_bindings = binding;
	p_vm->m_ip++;
}

void op_bindg(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
	value_t *binding = value_create_binding(p_vm, sym, p_vm->m_stack[p_vm->m_sp - 1]);
	binding_t *bind = (binding_t *)binding->m_data;
	bind->m_next = ((environment_t *)p_vm->m_user_env->m_data)->m_bindings;
assert((bind->m_next == NULL) || (bind->m_next->m_type == VT_BINDING));
assert(binding->m_type == VT_BINDING);
	((environment_t *)p_vm->m_user_env->m_data)->m_bindings = binding;
	p_vm->m_ip++;
}

void op_bindgf(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	value_t *sym = ((value_t **)p_pool->m_data)[p_arg];
	value_t *binding = value_create_binding(p_vm, sym, p_vm->m_stack[p_vm->m_sp - 1]);
	binding_t *bind = (binding_t *)binding->m_data;
	bind->m_next = ((environment_t *)p_vm->m_user_env->m_data)->m_function_bindings;
assert((bind->m_next == NULL) || (bind->m_next->m_type == VT_BINDING));
assert(binding->m_type == VT_BINDING);
	((environment_t *)p_vm->m_user_env->m_data)->m_function_bindings = binding;
	p_vm->m_ip++;
}

void op_print(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	printf("\nVAL: ");
	value_print(p_vm->m_stack[p_vm->m_sp - 1]);
	printf("\n");
	p_vm->m_ip++;
}

void op_dup(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	// Get value from stack
	value_t *v = p_vm->m_stack[p_vm->m_bp + p_arg];

	// Push it onto the stack
	p_vm->m_stack[p_vm->m_sp++] = v;
	p_vm->m_ip++;
}

void op_load(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	value_t *b = environment_binding_find(p_vm, ((value_t **)p_pool->m_data)[p_arg], false);

	assert(b && b->m_type == VT_BINDING);

//	printf("op_load: key: %d '", ((value_t **)p_pool->m_data)[p_arg]->m_type); value_print(((value_t **)p_pool->m_data)[p_arg]); printf("'\n");


	// Push it onto the stack
	binding_t *bind = (binding_t *)b->m_data;

	p_vm->m_stack[p_vm->m_sp++] = bind->m_value;
	p_vm->m_ip++;
}

void op_loadf(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	//printf("loadf: "); value_print(((value_t **)p_pool->m_data)[p_arg]); printf("\n");
	//printf("ev: %lu\n", p_vm->m_ev);

	value_t *b = environment_binding_find(p_vm, ((value_t **)p_pool->m_data)[p_arg], true);


	assert(b != NULL);

	// Push it onto the stack
	binding_t *bind = (binding_t *)b->m_data;
	p_vm->m_stack[p_vm->m_sp++] = bind->m_value;
	p_vm->m_ip++;
}

void op_lambda(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	// get value from pool
	value_t *lambda = ((value_t **)p_pool->m_data)[p_arg];

printf("op_lambda: bc: "); value_print(((lambda_t *)lambda->m_data)->m_bytecode); printf("\n");

	value_t *closure = make_closure(p_vm, lambda);
	p_vm->m_stack[p_vm->m_sp++] = closure;
	p_vm->m_ip++;
}

void op_ret(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	p_vm->m_ip = -1;
}

void op_ifniljmp(vm_t *p_vm, unsigned long p_arg, value_t *p_pool)
{
	value_t *top = p_vm->m_stack[p_vm->m_sp - 1];

//printf("ifniljmp: "); value_print(top); printf("\n");

	if (top->m_type == VT_SYMBOL && (!strcmp(top->m_data, "nil"))) {
		//printf("op_ifniljmp: %d\n", (int)p_arg);
		p_vm->m_ip += (int)p_arg;
	}  else { 
		p_vm->m_ip++;
	}
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
		assert(!"ERROR: UNKNOWN FUNCTION TYPE\n");
	}

	// Consume the stack back through the function name
	p_vm->m_sp = p_vm->m_bp;
	p_vm->m_stack[p_vm->m_sp++] = ret;
	p_vm->m_bp = old_bp;
	p_vm->m_ip++;
}

// TODO - Bind calls should replace existing bindings
void vm_bind(vm_t *p_vm, char *p_symbol, value_t *p_value)
{
	value_t *binding = value_create_binding(p_vm, value_create_symbol(p_vm, p_symbol), p_value);
	binding_t *bind = (binding_t *)binding->m_data;
	bind->m_next = ((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_bindings;
	((environment_t *)p_vm->m_current_env[p_vm->m_ev - 1]->m_data)->m_bindings = binding;
}

// TODO - Bind calls should replace existing bindings
void vm_bindf(vm_t *p_vm, char *p_symbol, vm_func_t p_func)
{
	value_t *binding = value_create_binding(p_vm, value_create_symbol(p_vm, p_symbol), value_create_internal_func(p_vm, p_func));

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
	value_t *nil = value_create_cons(p_vm, NULL, NULL);
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

//printf("obp: %d nbp: %lu sp: %lu nargs: %d\n", old_bp, p_vm->m_bp, p_vm->m_sp, p_nargs);


    assert(p_closure && p_closure->m_type == VT_CLOSURE && p_closure->m_cons[1]);
    lambda_t *l = (lambda_t *)(p_closure->m_cons[1]->m_data);

	value_t *env = value_create_environment(p_vm, p_closure->m_cons[0]);
	vm_push_env(p_vm, env);

	vm_push(p_vm, p_closure);

	// Bind parameters
	value_t *p = l->m_parameters;


	int bp_offset = 0;
	while(p && p->m_cons[0]) {
		value_t *stack_val = p_vm->m_stack[p_vm->m_bp + bp_offset];

//printf("binding: "); value_print(p->m_cons[0]); printf ("to: "); value_print(stack_val); printf("\n");

		vm_bind(p_vm, p->m_cons[0]->m_data, stack_val);

		bp_offset++;
		p = p->m_cons[1];
	}

	int old_ip = p_vm->m_ip;
	p_vm->m_ip = 0;

printf("active pool: %p\n", l->m_pool);

	while(p_vm->m_ip != -1) {
		exec_instruction(p_vm, 
				((bytecode_t *)l->m_bytecode->m_data)[p_vm->m_ip], 
					l->m_pool);
	}

printf("left active pool\n");


	// Consume the stack back through the function name
	p_vm->m_stack[p_vm->m_bp] = p_vm->m_stack[p_vm->m_sp - 1];
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_bp = old_bp;
	p_vm->m_ip =  old_ip;

	vm_pop_env(p_vm);

}



