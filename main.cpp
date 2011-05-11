#include "vm.h"

#include <stdio.h>

int main(int argc, char *arg[])
{
	vm_t *vm = vm_create(128);

	bytecode_t bc[3];
	bc[0].opcode = OP_PUSH;
	bc[0].argument = 1;
	bc[1].opcode = OP_PUSH_ENV;
	bc[1].argument = 2;
	bc[2].opcode = OP_POP_ENV;
	bc[2].argument = 2;

	vm_exec(vm, bc, 3);
	
	printf("vm sp: %lu %ld %ld\n", 
		vm->m_sp, 
		vm->m_stack[0].m_data.number,
		vm->m_stack[1].m_data.number);


	vm_destroy(vm);
}
