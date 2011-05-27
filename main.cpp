#include "vm.h"

#include <stdio.h>

#include "value.h"
#include "value_helpers.h"
#include "reader.h"
#include "compile.h"
#include "env.h"

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

value_t *print(vm_t *p_vm)
{
	printf("EXEC'd PRINT\n");
	return NULL;
}


int main(int argc, char *arg[])
{
	vm_t *vm = vm_create(128);
	vm_bindf(vm, "print", print);
	vm_bindf(vm, "cons", cons);
	vm_bind(vm, "a", value_create_number(1));
	vm_bind(vm, "b", value_create_number(2));

	binding_t *b = environment_binding_find(vm, value_create_symbol("a"), false);

	printf("b: %lu so: %d\n", b, sizeof(value_t));

	char blah[] = "(cons a b)";
	stream_t *strm = stream_create(blah);
	reader(vm, strm, false);

	printf("Reader result: ");
	printf("\nVAL: ");
    value_print(vm->m_stack[vm->m_sp - 1]);
    printf("\n");

	printf("sp: %lu bp: %lu\n", vm->m_sp, vm->m_bp);

	printf("Calling eval:\n");
	eval(vm, vm->m_stack[0]);
	printf("\n");
	printf("lambda: "); value_print(vm->m_stack[0]); printf("\n");

	vm_destroy(vm);
}
