#include "value.h"
#include "value_helpers.h"
#include "reader.h"
#include "compile.h"
#include "env.h"
#include "gc.h"
#include "err.h"

//#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

void load_string(vm_t *p_vm, char const *p_code);

value_t *atom(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	if (first && first->m_type == VT_CONS) {
		return nil;
	}

	return value_create_symbol(p_vm, "t");
}

value_t *status(vm_t *p_vm)
{
	// Display stack
	printf("sp: %lu\n", p_vm->m_sp);

	// Display memory
	value_t *heap = p_vm->m_heap;
	int count = 0;
	unsigned long mem = 0;
	while(heap) {
		count++;
		mem += sizeof(value_t) + heap->m_size;
		printf("%d] %lu ", count, sizeof(value_t) + heap->m_size);
		value_print(heap);
		printf("\n");
		heap = heap->m_heapptr;
	}

#if 0
	printf("\n\n-----FREE HEAP----\n\n");

	heap = p_vm->m_free_heap;
	while(heap) {
		printf("%d] %lu ", count, sizeof(value_t) + heap->m_size);
		value_print(heap);
		printf("\n");
		heap = heap->m_heapptr;
	}
#endif


	printf("Live Objects: %d\n", count);
	printf("Memory used: %lu\n", mem);
	
	return nil;
}

value_t *gc_go(vm_t *p_vm)
{
	gc(p_vm);
	return nil;
}
	

value_t *load(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	assert(first->m_type == VT_STRING);

	struct stat st;
	if (stat(first->m_data, &st) != 0) {
		return nil;
	}

	FILE *fp = fopen(first->m_data, "r");
	char *input = (char *)malloc(st.st_size);

	unsigned long file_size = (unsigned long)st.st_size;

	fread(input, file_size, 1, fp);
	
	load_string(p_vm, input);

printf("free input: %p\n", input);
	free(input);
	fclose(fp);

	return nil;
}


value_t *call(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	int nargs = p_vm->m_sp - p_vm->m_bp - 2;

printf("exec: args: %d", nargs); value_print(first); printf("\n");

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

	return value_create_cons(p_vm, first, second);
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

value_t *eq(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	if (first->m_type == VT_SYMBOL && second->m_type == VT_CONS) {
		if (second->m_cons[0] == NULL && !strcmp("nil", first->m_data)) {
			return value_create_symbol(p_vm, "t");
		}
		return nil;
		
	} else if (first->m_type == VT_CONS && second->m_type == VT_SYMBOL) {
		if (first->m_cons[0] == NULL && !strcmp("nil", second->m_data)) {
			return value_create_symbol(p_vm, "t");
		}
		return nil;
	}

	if (first->m_type != second->m_type) {
		return nil;
	}

	verify(first->m_type == second->m_type, "eq: Types are not the same %d != %d\n",
		first->m_type, second->m_type);

	switch (first->m_type) {
		case VT_NUMBER:
			if (*(int *)(first->m_data) == *(int *)(second->m_data)) {
				return value_create_symbol(p_vm, "t");
			} else {
				return nil;
			}
			break;

		case VT_SYMBOL:
			if(!strcmp(first->m_data, second->m_data)) {
				return value_create_symbol(p_vm, "t");
			} else {
				return nil;
			}
			break;

		case VT_STRING:
			if(!strcmp(first->m_data, second->m_data)) {
				return value_create_symbol(p_vm, "t");
			} else {
				return nil;
			}
			break;

		default:
			assert(!"Trying to test unsupported type with eq");
			break;
	}

	return nil;
}

value_t *plus(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	assert(first->m_type == VT_NUMBER);
	assert(second->m_type == VT_NUMBER);

	int first_num = *(int *)first->m_data;
	int second_num = *(int *)second->m_data;
	return value_create_number(p_vm, first_num + second_num);
}

value_t *print(vm_t *p_vm)
{
	printf("%lu>> ", p_vm->m_sp);
	value_print(p_vm->m_stack[p_vm->m_sp - 1]);
	printf("\n");
	return nil;
}

void load_string(vm_t *p_vm, char const *p_code)
{
	int vm_bp = p_vm->m_bp;
	int vm_sp = p_vm->m_sp;
	int vm_ev = p_vm->m_ev;
	int vm_ip = p_vm->m_ip;
	value_t *vm_current_env = p_vm->m_current_env[p_vm->m_ev - 1];

	int i = setjmp(*push_handler_stack());
	if (i == 0) {
		stream_t *strm = stream_create(p_code);
		int args = reader(p_vm, strm, false);


		int count_down = args;
		while(count_down > 0) {
			// get value off stack
			value_t *rd = p_vm->m_stack[p_vm->m_sp - count_down];
printf("read: "); value_print(rd); printf("\n");

			// Evaluate it
			eval(p_vm, rd);

			// Print a result
printf("res: "); value_print(p_vm->m_stack[p_vm->m_sp - count_down]); printf("\n");
			count_down--;
		}

//			p_vm->m_sp--;
		p_vm->m_sp -= args * 2;



		stream_destroy(strm);
	} else {
		p_vm->m_bp = vm_bp;
		p_vm->m_sp = vm_sp;
		p_vm->m_ev = vm_ev;
		p_vm->m_ip = vm_ip;
		p_vm->m_current_env[p_vm->m_ev - 1] = vm_current_env;

		printf("Error: %s\n", g_err);
	}
	pop_handler_stack();

	verify(p_vm->m_sp == vm_sp &&  p_vm->m_bp == vm_bp, "internal error");

	gc(p_vm);
}


int main(int argc, char *arg[])
{
	vm_t *vm = vm_create(128);
	char p1[] = "print";
	char p2[] = "cons";
	char p3[] = "a";
	char p4[] = "b";
	char p5[] = "status";
	char p6[] = "car";
	char p7[] = "cdr";
	char p8[] = "nil";
	char p9[] = "call";
	char p10[] = "+";
	char p11[] = "eq";
	char p12[] = "load";
	char p13[] = "gc";
	char p14[] = "atom";

printf("vm ev: %lu\n", vm->m_ev);

	vm_bindf(vm, p1, print, 1);
	vm_bindf(vm, p2, cons, 2);
	vm_bindf(vm, p5, status, 0);
	vm_bindf(vm, p6, car, 1);
	vm_bindf(vm, p7, cdr, 1);
	vm_bindf(vm, p9, call, 2);
	vm_bindf(vm, p10, plus, 2);
	vm_bindf(vm, p11, eq, 2);
	vm_bindf(vm, p12, load, 1);
	vm_bindf(vm, p13, gc_go, 0);
	vm_bindf(vm, p14, atom, 1);
	nil = value_create_symbol(vm, "nil");
	vm_bind(vm, p8, nil);
	vm_bind(vm, p3, value_create_number(vm, 1));
	vm_bind(vm, p4, value_create_number(vm, 2));

	char input[256];
	input[0] = 0;
	printf("> ");
	while(gets(input) != NULL && strcmp(input, "quit")) {
		load_string(vm, input);

		input[0] = 0;

		printf("> ");
	}

	vm->m_sp = 0;
	vm->m_ev = 0;
	gc(vm);
	vm_destroy(vm);
}
