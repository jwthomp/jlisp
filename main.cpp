#include "vm.h"

#include <stdio.h>

int main(int argc, char *arg[])
{
	vm_t *vm = vm_create(128);

	bytecode_t bc[2];
	bc[0].opcode = OP_PUSH;
	bc[0].argument = 1;
	bc[1].opcode = OP_PUSH;
	bc[1].argument = 2;


	vm_exec(vm, bc, 2);
	


	vm_destroy(vm);
}
