#ifndef __VM_H_
#define __VM_H_

#include "env.h"
#include "value.h"

typedef struct bytecode_s {
    unsigned long opcode;
    void *argument;
} bytecode_t;

typedef enum opcode_s {
	OP_PUSH,
	OP_PUSH_ENV,
	OP_POP_ENV,
	OP_BIND,
	OP_BINDF,
	OP_PRINT,
	OP_DUP,
	OP_LOAD,
	OP_LOADF,
// LOAD
// LOADF
// CALL
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

vm_t *vm_create(unsigned long p_stack_size);
void vm_destroy(vm_t *);
void vm_exec(vm_t *, bytecode_t *, unsigned long);

#endif /* __VM_H_ */
