
#include <stdio.h>

#include "value.h"
#include "value_helpers.h"
#include "reader.h"
#include "compile.h"
#include "env.h"

value_t *status(vm_t *p_vm)
{
	printf("sp: %lu\n", p_vm->m_sp);
	return NULL;
}

value_t *call(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	int nargs = p_vm->m_sp - p_vm->m_bp - 2;

printf("exec: "); value_print(first); printf("\n");

	vm_exec(p_vm, first, nargs);

	// Need to move the returned value to overwrite the clojure value on the stack
	p_vm->m_stack[p_vm->m_sp - 2] = p_vm->m_stack[p_vm->m_sp - 1];
	p_vm->m_sp--;
	return p_vm->m_stack[p_vm->m_sp - 1];
}

value_t *cons(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	return value_create_cons(first, second);
}

value_t *car(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	assert(is_cons(first));
	return first->m_cons[0];
}

value_t *cdr(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	assert(is_cons(first));
	return first->m_cons[1];
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
	printf("%lu>> ", p_vm->m_sp);
	value_print(p_vm->m_stack[p_vm->m_sp - 1]);
	printf("\n");
	return NULL;
}


int main(int argc, char *arg[])
{
	vm_t *vm = vm_create(128);
	char p1[] = "printf";
	char p2[] = "cons";
	char p3[] = "a";
	char p4[] = "b";
	char p5[] = "status";
	char p6[] = "car";
	char p7[] = "cdr";
	char p8[] = "1";
	char p9[] = "call";
	vm_bindf(vm, p1, print);
	vm_bindf(vm, p2, cons);
	vm_bindf(vm, p5, status);
	vm_bindf(vm, p6, car);
	vm_bindf(vm, p7, cdr);
	vm_bindf(vm, p9, call);
	vm_bind(vm, p3, value_create_number(1));
	vm_bind(vm, p4, value_create_number(2));
	vm_bind(vm, p8, value_create_number(1));

	char input[256];
	input[0] = 0;
	printf("> ");
	while(gets(input) != NULL) {
		stream_t *strm = stream_create(input);
		reader(vm, strm, false);

		value_t *rd = vm->m_stack[vm->m_sp - 1];
		vm->m_sp--;

printf("read: "); value_print(rd); printf("\n");

		eval(vm, rd);
printf("res: "); value_print(vm->m_stack[vm->m_sp - 1]); printf("\n");
		vm->m_sp--;


		stream_destroy(strm);
		input[0] = 0;

		printf("> ");
	}

	vm_destroy(vm);
}
