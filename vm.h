#ifndef __VM_H_
#define __VM_H_

#include "env.h"
#include "value.h"
#include "pool.h"

typedef struct bytecode_s {
    unsigned long m_opcode :5;
    long m_value :27;
} bytecode_t;


typedef enum opcode_s {
	OP_PUSH,		// 0
	OP_BIND,		// 1
	OP_BINDF,		// 2
	OP_CATCH,		// 3
	OP_DUP,			// 4
	OP_LOAD,		// 5
	OP_LOADF,		// 6
	OP_CALL,		// 7
	OP_LAMBDA,		// 8
	OP_BINDGF,		// 9
	OP_BINDG,		// 10
	OP_IFNILJMP,	// 11
	OP_RET,			// 12
	OP_UPDATE,		// 13
	OP_BINDD,		// 14
	OP_BINDDF,		// 15
	OP_JMP,			// 16
	OP_POP,			// 17
	OP_SET_PROC_STATUS,	// 18
} opcode_e;

extern char const *g_opcode_print[];

extern bool g_debug_display;

extern value_t *g_kernel_proc;

typedef struct {
	unsigned long m_sp;
	unsigned long m_bp;
	unsigned long m_ev;
	unsigned long m_csp;
	int m_ip;
} vm_state_t;

typedef struct vm_s {
	value_t *m_kernel_env;
	value_t *m_user_env;

	value_t *t;
	value_t *nil;
	value_t *voidobj;

    value_t **m_current_env;
	value_t **m_stack;
	value_t **m_c_stack;

	value_t *m_static_heap;
	value_t *m_heap_g0;
	pool_t *m_pool_g0;
	value_t *m_heap_g1;
	value_t *m_free_heap;

	value_t *m_symbol_table;

	unsigned long m_sp;
	unsigned long m_bp;
	unsigned long m_ev;
	unsigned long m_csp;
	int m_ip;

	value_t *m_next;
} vm_t;



typedef value_t *(*vm_func_t)(vm_t *p_vm);

vm_t *vm_create(unsigned long p_stack_size, value_t *p_vm_parent);
void vm_destroy(vm_t *);
void vm_exec(vm_t *p_vm, value_t ** volatile p_closure, int p_arg_count );
void vm_bind(vm_t *p_vm, char const *p_symbol, value_t *p_value, bool p_dynamic);
void vm_bindf(vm_t *p_vm, char const *p_symbol, vm_func_t p_func, unsigned long p_param_count, bool p_is_macro, bool p_dynamic);
void vm_bindf(vm_t *p_vm, char const *p_symbol, value_t *p_func, bool p_dynamic);
void vm_cons(vm_t *p_vm);
void vm_list(vm_t *p_vm, int p_list_size);
void vm_push(vm_t *p_vm, value_t *p_value);
void vm_push_env(vm_t *p_vm, value_t *p_env);
void vm_pop_env(vm_t *p_vm);

void bind_internal(vm_t *p_vm, value_t *p_symbol, value_t *p_value, bool p_func, bool p_dynamic);

void vm_print_stack(vm_t *p_vm);
void vm_print_env(vm_t *p_vm);
void vm_print_symbols(vm_t *p_vm);

#endif /* __VM_H_ */
