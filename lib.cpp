#include "value.h"
#include "value_helpers.h"
#include "vm.h"
#include "assert.h"
#include "err.h"

#include "lib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


typedef struct {
	char const*m_name;
	vm_func_t m_func;
	int m_arg_count;
} internal_func_def_t;

value_t *print(vm_t *p_vm);
value_t *cons(vm_t *p_vm);
value_t *car(vm_t *p_vm);
value_t *cdr(vm_t *p_vm);
value_t *call(vm_t *p_vm);
value_t *plus(vm_t *p_vm);
value_t *status(vm_t *p_vm);
value_t *eq(vm_t *p_vm);
value_t *load(vm_t *p_vm);

static internal_func_def_t g_ifuncs[] = {
	{"print", print, -1},
	{"cons", cons, 2},
	{"car", car, 1},
	{"cdr", cdr, 1},
	{"call", call, 2},
	{"+", plus, 2},
	{"status", status, 0},
	{"eq", eq, 2},
	{"load", load, 1},
};

#define NUM_IFUNCS 9


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


value_t *load(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	assert(first->m_type == VT_STRING);

	struct stat st;
	if (stat(first->m_data, &st) != 0) {
		return nil;
	}

	FILE *fp = fopen(first->m_data, "r");
	char *input = (char *)malloc(st.st_size + 1);
	memset(input, 0, st.st_size + 1);

	unsigned long file_size = (unsigned long)st.st_size;

	fread(input, file_size, 1, fp);
	
	load_string(p_vm, input);

	free(input);
	fclose(fp);

	return nil;
}


value_t *call(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	int nargs = p_vm->m_sp - p_vm->m_bp - 2;

//printf("exec: args: %d", nargs); value_print(first); printf("\n");

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


	if (first->m_type != second->m_type) {
		return nil;
	}

	verify(first->m_type == second->m_type, "eq: Types are not the same %d != %d\n",
		first->m_type, second->m_type);

	switch (first->m_type) {
		case VT_NUMBER:
		case VT_SYMBOL:
		case VT_STRING:
			if (value_equal(first, second) == true) {
				return value_create_symbol(p_vm, "t");
			} else {
				return nil;
			}

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
//	printf("%lu>> ", p_vm->m_sp);
	value_print(p_vm->m_stack[p_vm->m_bp + 1]);
	printf("\n");
	return p_vm->m_stack[p_vm->m_bp + 1];
}



void lib_init(vm_t *p_vm)
{
	int i;
	for (i = 0; i < NUM_IFUNCS; i++) {
		vm_bindf(p_vm, g_ifuncs[i].m_name, g_ifuncs[i].m_func, g_ifuncs[i].m_arg_count);
	}

	nil = value_create_symbol(p_vm, "nil");
	vm_bind(p_vm, "nil", nil);
	t = value_create_symbol(p_vm, "nil");
	vm_bind(p_vm, "t", t);

	voidobj = value_create(p_vm, VT_VOID, 0, true);
}
