#ifndef __VM_EXEC_H_
#define __VM_EXEC_H_

#include "vm.h"

void *vm_exec_proc_create();
void vm_exec_proc_destroy(void *);

void vm_exec_proc_add_vm(void *, vm_t *p_vm);
void vm_exec_proc_remove_vm(void *, vm_t *p_vm);

void vm_push_exec_state(vm_t *p_vm, value_t *p_closure);


void vm_exec(vm_t *p_vm, unsigned long p_return_on_exp);

#endif /* __VM_EXEC_H_ */
