#include "vm.h"

#include <stdio.h>

#include "value.h"
#include "value_helpers.h"
#include "reader.h"
#include "compile.h"

value_t *cons(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	return value_create_cons(first, second);
}

value_t *plus(vm_t *p_vm)
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
	vm_bindf(vm, "cons", cons);

	printf("so: %d\n", sizeof(value_t));

	char blah[] = "(cons 2 3)";
	stream_t *strm = stream_create(blah);
	reader(vm, strm, false);

	printf("Reader result: ");
	bytecode_t bc[100];
	bc[0].m_opcode = OP_PRINT;
	vm_exec(vm, bc, 1, NULL);

	printf("sp: %lu bp: %lu\n", vm->m_sp, vm->m_bp);

	printf("Calling eval:\n");
	value_t *res = eval(vm, vm->m_stack[0]);
	printf("\n");
	printf("lambda: "); value_print(res); printf("\n");

	vm_destroy(vm);
}
