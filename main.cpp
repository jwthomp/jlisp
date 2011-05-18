#include "vm.h"

#include <stdio.h>

#include "value.h"
#include "binding.h"

int main(int argc, char *arg[])
{
	vm_t *vm = vm_create(128);

	printf("so: %d\n", sizeof(value_t));
	printf("valtest: ");
	value_print(value_create_symbol("b"));
	printf("\n");


	bytecode_t bc[100];
	int c = 0;
	bc[c].opcode = OP_PUSH;
	bc[c++].argument = value_create_symbol("a");
	bc[c].opcode = OP_PUSH;
	bc[c++].argument = value_create_number(1);
	bc[c++].opcode = OP_BIND;
	bc[c].opcode = OP_DUP;
	bc[c++].argument = (void *)0;
	bc[c++].opcode = OP_PRINT;
	bc[c++].opcode = OP_LOAD;
	bc[c++].opcode = OP_PRINT;

	vm_exec(vm, bc, c);

	binding_t *b = vm->m_current_env->m_bindings;
printf("b: %p\n", (unsigned int *)b);
	while(b) {
		printf("key/val: ");
		binding_print(b);
		printf("\n");
		b = b->m_next;
	}

	value_t *key = value_create_symbol("a");
	b = binding_find(vm->m_current_env->m_bindings, key);

	printf("cmp: ");
	value_print(key);
	printf(" ");
	value_print(vm->m_current_env->m_bindings->m_key);
	printf(" bind: %p\n", b);


	//binding_print(b);
	printf("\n");

	value_print(vm->m_stack[0]);
	printf(" ");
	value_print(vm->m_stack[1]);
	printf("\n");
	

	value_destroy(key);
	vm_destroy(vm);
}
