#ifndef __VM_H_
#define __VM_H_

#include "env.h"
#include "value.h"

typedef struct bytecode_s {
    unsigned long m_opcode :4;
    unsigned long m_value :28;
} bytecode_t;


typedef enum opcode_s {
	OP_PUSH,		// 0
	OP_PUSH_ENV,	// 1
	OP_POP_ENV,		// 2
	OP_BIND,		// 3
	OP_BINDF,		// 4
	OP_PRINT,		// 5
	OP_DUP,			// 6
	OP_LOAD,		// 7
	OP_LOADF,		// 8
	OP_CALL,		// 9
} opcode_e;


typedef struct vm_s {
	environment_t *m_kernel_env;
	environment_t *m_user_env;

    environment_t *m_current_env;
	value_t **m_stack;
	unsigned long m_sp;
	unsigned long m_bp;

	value_t *m_symbols;
	
} vm_t;

typedef value_t *(*vm_func_t)(vm_t *p_vm);

vm_t *vm_create(unsigned long p_stack_size);
void vm_destroy(vm_t *);
void vm_exec(vm_t *p_vm, bytecode_t *p_bc, unsigned long p_size, value_t *p_pool);
void vm_bind(vm_t *p_vm, char *p_symbol, value_t *p_value);
void vm_bindf(vm_t *p_vm, char *p_symbol, vm_func_t p_func);
void vm_bindf(vm_t *p_vm, char *p_symbol, value_t *p_func);
void vm_cons(vm_t *p_vm);
void vm_list(vm_t *p_vm, int p_list_size);
void vm_push(vm_t *p_vm, value_t *p_value);


#endif /* __VM_H_ */
