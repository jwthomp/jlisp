#ifndef __VM_EXEC_H_
#define __VM_EXEC_H_

#include "vm.h"

void vm_exec_init();
void vm_exec_wait();

void vm_exec_add_vm(value_t *p_vm);
void vm_exec_remove_vm(value_t *p_vm);

void vm_push_exec_state(vm_t *p_vm, value_t *p_closure);

void vm_exec(vm_t *p_vm, unsigned long p_return_on_exp, bool p_allow_preemption);

extern value_t *g_static_heap;

extern value_t *g_symbol_table;
#endif /* __VM_EXEC_H_ */
