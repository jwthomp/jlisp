#ifndef __MESSAGE_H_
#define __MESSAGE_H_

#include "vm.h"

// VM Subsystem support
void vm_init_message(vm_t *p_vm);
void vm_shutdown_message(vm_t *p_vm);

void message_send(vm_t *p_to_vm, value_t *p_message);

value_t *message_recv(vm_t *p_vm);

#endif /* __MESSAGE_H_ */
