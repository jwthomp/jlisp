#include "vm.h"

#include <stdio.h>

#include "value.h"
#include "value_helpers.h"
#include "binding.h"

value_t *func(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	int first_num = *(int *)first->m_data;
	int second_num = *(int *)second->m_data;
	return value_create_number(first_num + second_num);
}

int main(int argc, char *arg[])
{
	vm_t *vm = vm_create(128);

	printf("so: %d\n", sizeof(value_t));

	char sym[2] = "+";
	vm_bindf(vm, sym, func);

	bytecode_t bc[100];
	int c = 0;
	bc[c].opcode = OP_LOADF;
	bc[c++].argument = value_create_symbol("+");
	bc[c].opcode = OP_PUSH;
	bc[c++].argument = value_create_number(1);
	bc[c].opcode = OP_LOADF;
	bc[c++].argument = value_create_symbol("+");
	bc[c].opcode = OP_PUSH;
	bc[c++].argument = value_create_number(1);
	bc[c].opcode = OP_PUSH;
	bc[c++].argument = value_create_number(2);
	bc[c].opcode = OP_CALL;
	bc[c++].argument = (void *)2;
	bc[c].opcode = OP_CALL;
	bc[c++].argument = (void *)2;
	bc[c++].opcode = OP_PRINT;

	vm_exec(vm, bc, c);

	vm_destroy(vm);
}
