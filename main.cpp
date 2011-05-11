#include "vm.h"

#include <stdio.h>

int main(int argc, char *arg[])
{
	vm_t *vm = vm_create(128);

	bytecode_t bc[3];
	bc[0].opcode = OP_PUSH;
	bc[0].argument = 1;
	bc[2].opcode = OP_BIND;
	bc[2].argument = 2;

	vm_exec(vm, bc, 3);
	
	printf("vm sp: %lu %ld \n", 
		vm->m_sp, 
		* ((unsigned long *)(vm->m_stack[0]->m_data)));


	vm_destroy(vm);
}
