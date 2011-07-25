#include "message.h"

#include "value.h"
#include "value_helpers.h"

#include <stdlib.h>

#define MESSAGE_QUEUE_SIZE 256

void vm_init_message(vm_t *p_vm)
{
	p_vm->m_msg_queue = (value_t **)malloc(sizeof(value_t *) * MESSAGE_QUEUE_SIZE);
	p_vm->m_mp = 0;
}

void vm_shutdown_message(vm_t *p_vm)
{
	p_vm->m_mp = 0;
	free(p_vm->m_msg_queue);
	p_vm->m_msg_queue = NULL;
}


void message_send(vm_t *p_to_vm, value_t *p_message)
{
	p_to_vm->m_msg_queue[p_to_vm->m_mp++] = p_message;
}

value_t *message_recv(vm_t *p_vm)
{
	if (p_vm->m_mp == 0) {
		return voidobj;
	}

	return p_vm->m_msg_queue[p_vm->m_mp-- - 1];
}
