#include "env.h"
#include "binding.h"
#include "value_helpers.h"
#include "assert.h"
#include "lambda.h"
#include "compile.h"
#include "gc.h"
#include "err.h"
#include "symbols.h"
#include "vm_exec.h"
#include "message.h"

#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXEC_STACK_SIZE 4096 * 10 
#define C_STACK_SIZE 1024

typedef struct bytecode_jump {
	bytecode_t m_bytecode;
	void (*m_func)(vm_t *, unsigned long, value_t *);
} bytecode_jump_t;


char const *g_opcode_print[] =  {
    "OP_PUSH",
    "OP_BIND",
    "OP_BINDF",
    "OP_CATCH",
    "OP_DUP",
    "OP_LOAD",
    "OP_LOADF",
    "OP_CALL",
    "OP_LAMBDA",
    "OP_BINDGF",
    "OP_BINDG",
    "OP_IFNILJMP",
    "OP_RET",
	"OP_UPDATE",
	"OP_BINDD",
	"OP_BINDDF",
	"OP_JMP",
	"OP_POP",
	"OP_SET_PROC_STATUS",
	"OP_NOP",
	"OP_RETCALL",
};

bool g_debug_display = false;
//bool g_debug_display = true;
value_t *g_kernel_proc = NULL;


vm_t *vm_create(unsigned long p_stack_size, value_t *p_vm_parent)
{
	vm_t *vm = (vm_t *)malloc(sizeof(vm_t));
	vm->m_heap_g0 = NULL;
	vm->m_heap_g1 = NULL;
	vm->m_pool_g0 = pool_alloc(0);
	vm->m_free_heap = NULL;
	vm->m_sp = 0;
	vm->m_csp = 0;
	vm->m_bp = 0;
	vm->m_ip = 0;
	vm->m_ev = 0;
	vm->m_exp = 0;
p_stack_size = EXEC_STACK_SIZE;
	vm->m_stack = (value_t **)malloc(sizeof(value_t *) * p_stack_size);
	vm->m_exec_stack = (value_t **)malloc(sizeof(value_t *) * EXEC_STACK_SIZE);
	vm->m_c_stack = (value_t **)malloc(sizeof(value_t *) * C_STACK_SIZE);
	vm->m_count = 0;
	vm->m_running_state = VM_RUNNING;

	vm_init_message(vm);

	// CANNOT create any value_t's until the above is initialized

	vm->m_current_env = (value_t **)malloc(sizeof(value_t *) * p_stack_size);
	if (p_vm_parent == NULL) {
		vm->m_kernel_env = value_create_environment(vm, NULL);
		vm->m_user_env = value_create_environment(vm, vm->m_kernel_env);
		vm->m_current_env[0] = vm->m_user_env;
	} else {
		vm_t *pvm = *(vm_t **)p_vm_parent->m_data;

		vm->m_kernel_env = pvm->m_kernel_env;
		vm->m_user_env = pvm->m_user_env;
		vm->m_current_env[0] = pvm->m_current_env[pvm->m_ev - 1];
	}

	vm->m_ev = 1;

	return vm;
}

void vm_destroy(vm_t *p_vm)
{
	vm_shutdown_message(p_vm);
	gc_shutdown(p_vm);
	p_vm->m_heap_g0 = NULL;
	p_vm->m_heap_g1 = NULL;
	p_vm->m_current_env[0] = NULL;
	p_vm->m_ev = 0;
	pool_free(p_vm->m_pool_g0);
	free(p_vm->m_stack);
	free(p_vm->m_current_env);
	free(p_vm);
}


void bind_internal(vm_t *p_vm, value_t *p_symbol, value_t *p_value, bool p_func, bool p_dynamic)
{
//printf("binding (%d, %d)", p_func, p_dynamic); value_print(p_vm, p_symbol); printf(" to "); 
//value_print(p_vm, p_value); printf("\n");

	if (p_dynamic == true) {
		if (p_func == true) {
			p_symbol->m_cons[2] = value_create_cons(p_vm, p_value, voidobj);
		} else {
			p_symbol->m_cons[1] = value_create_cons(p_vm, p_value, voidobj);
		}
		return;
	}

	if (p_func == true) {
		if (p_symbol->m_cons[2] != voidobj) {
			p_symbol->m_cons[2] = value_create_cons(p_vm, p_value, p_symbol->m_cons[2]);
			return;
		}
	} else {
		if (p_symbol->m_cons[1] != voidobj) {
			p_symbol->m_cons[1] = value_create_cons(p_vm, p_value, p_symbol->m_cons[1]);
			return;
		}
	}

	value_t *binding = value_create_binding(p_vm, p_symbol, p_value);
	binding_t *bind = (binding_t *)binding->m_data;


	environment_t *env = NULL;
	env = (environment_t *)(p_vm->m_current_env[p_vm->m_ev - 1])->m_data;

	if (p_func == true) {
		bind->m_next = env->m_function_bindings;
		env->m_function_bindings = binding;
	} else {
		bind->m_next = env->m_bindings;
		env->m_bindings = binding;
	}
}


// TODO - Bind calls should replace existing bindings
void vm_bind(vm_t *p_vm, char const *p_symbol, value_t *p_value, bool p_dynamic)
{
	value_t *sym = value_create_symbol(p_vm, p_symbol);
	bind_internal(p_vm, sym, p_value, false, p_dynamic);
}

// TODO - Bind calls should replace existing bindings
void vm_bindf(vm_t *p_vm, char const *p_symbol, vm_func_t p_func, unsigned long p_param_count, bool p_is_macro, bool p_dynamic)
{
	value_t *int_func = value_create_internal_func(p_vm, p_func, p_param_count, p_is_macro);
	value_t *sym = value_create_symbol(p_vm, p_symbol);
	bind_internal(p_vm, sym, int_func, true, p_dynamic);

}

void vm_bindf(vm_t *p_vm, char const *p_symbol, value_t *p_code, bool p_dynamic)
{
	value_t *sym = value_create_symbol(p_vm, p_symbol);
	bind_internal(p_vm, sym, p_code, true, p_dynamic);
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
	vm_push(p_vm, nil);

	for(int i = p_args; i > 0; i--) {
		vm_cons(p_vm);
	}

}

/*
value_t **vm_c_push(vm_t *p_vm, value_t *p_value)
{
	assert(p_vm->m_csp < C_STACK_SIZE);
	p_vm->m_c_stack[p_vm->m_csp++] = p_value;
	return &p_vm->m_c_stack[p_vm->m_csp - 1];
}
*/

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

static void vm_print_env_bindings(vm_t *p_vm, value_t *p_bindings)
{
	if (p_bindings == NULL) {
		return;
	}

	printf("%p] ", p_bindings); 
	value_print(p_vm, p_bindings); 
	printf("\n");

	vm_print_env_bindings(p_vm, binding_get_next(p_bindings));

	
}

void vm_print_env(vm_t *p_vm)
{
	for (unsigned long i = 0; i < p_vm->m_ev; i++) {
		printf("Env[%lu -- %p]\n", i, p_vm->m_current_env[i]);
		printf("Bindings\n");
		vm_print_env_bindings(p_vm, environment_get_bindings(p_vm->m_current_env[i]));
		printf("FBindings\n");
		vm_print_env_bindings(p_vm, environment_get_fbindings(p_vm->m_current_env[i]));
		printf("Parent\n");
		vm_print_env_bindings(p_vm, environment_get_parent(p_vm->m_current_env[i]));
	}
}

void vm_print_stack(vm_t *p_vm)
{
	for (unsigned long i = 0; i < p_vm->m_sp; i++) {
		printf("S %lu] ", i); value_print(p_vm, p_vm->m_stack[i]); printf("\n");
	}
}

void vm_print_exec_stack(vm_t *p_vm)
{
	for (unsigned long i = 0; i < p_vm->m_exp; i++) {
		printf("S %lu] ", i); value_print(p_vm, p_vm->m_exec_stack[i]); printf("\n");
	}
}

void vm_print_symbols(vm_t *p_vm)
{
	value_t *sym = g_symbol_table;
	while(sym) {
		printf("Sym] "); value_print(p_vm, sym); printf(" %p\n", sym);
		sym = sym->m_next_symbol;
	}
}


void vm_state_push(vm_t *p_vm)
{
	value_t *state = value_create_vm_state(p_vm);
	vm_push(p_vm, state);
}

void vm_state_pop(vm_t *p_vm)
{
	value_t *val = p_vm->m_stack[p_vm->m_bp - 1];
	vm_state_t *vm_state = (vm_state_t *)val->m_data;
	p_vm->m_bp = vm_state->m_bp;
}

