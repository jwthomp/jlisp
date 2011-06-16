#include "gc.h"

#include "lambda.h"
#include "value.h"
#include "value_helpers.h"
#include "assert.h"

#include <stdio.h>
#include <stdlib.h>

#define GC_AGE 1

void mark(vm_t *p_vm);
void sweep(vm_t *p_vm, value_t **p_heap, value_t **p_tenured_heap);

value_t *gc_alloc(vm_t *p_vm, size_t p_size, bool p_is_static)
{
	value_t * val = (value_t *)malloc(p_size);

	if (p_is_static == false) {
		val->m_heapptr = p_vm->m_heap_g0;
		p_vm->m_heap_g0 = val;
	} else {
		val->m_heapptr = p_vm->m_static_heap;
		p_vm->m_static_heap = val;
	}
	val->m_is_static = p_is_static;
	return val;
}

void gc_shutdown(vm_t *p_vm)
{
	gc(p_vm, 0);

	value_t* p = p_vm->m_static_heap;
	value_t* safe = NULL;

	for(;p;p = safe) {
		safe = p->m_heapptr;
//printf("Freeing(%d): %p\n", p->m_type, p);
		free(p);
	}
}

void gc(vm_t *p_vm, int p_age)
{
	mark(p_vm);

	if (p_age == 0) {
		sweep(p_vm, &p_vm->m_heap_g0, NULL);
	} else {
		sweep(p_vm, &p_vm->m_heap_g0, &p_vm->m_heap_g1);
	}
}

void retain(value_t *p_value)
{

	if (p_value == NULL) {
		return;
	}

	if (is_fixnum(p_value) || p_value->m_in_use || p_value->m_is_static) {
		return;
	}

	p_value->m_in_use = true;

	switch(p_value->m_type) {
		case VT_VOID:
		case VT_NUMBER:
		case VT_INTERNAL_FUNCTION:
		case VT_BYTECODE:
		case VT_STRING:
			return;
		case VT_BINDING:
			retain(((binding_t *)p_value->m_data)->m_key);
			retain(((binding_t *)p_value->m_data)->m_value);
			retain(((binding_t *)p_value->m_data)->m_next);
			break;

		case VT_POOL:
			for (unsigned long i = 0; i < (p_value->m_size / sizeof(value_t *)); i++) {
				retain(((value_t **)p_value->m_data)[i]);
			}
			break;

		case VT_LAMBDA:
			retain(((lambda_t *)p_value->m_data)->m_parameters);
			retain(((lambda_t *)p_value->m_data)->m_bytecode);
			retain(((lambda_t *)p_value->m_data)->m_pool);
			break;

		case VT_ENVIRONMENT:
			retain(((environment_t *)p_value->m_data)->m_bindings);
			retain(((environment_t *)p_value->m_data)->m_function_bindings);
			retain(((environment_t *)p_value->m_data)->m_parent);
			break;

		case VT_SYMBOL:
		case VT_CLOSURE:
		case VT_CONS:
			retain(p_value->m_cons[0]);
			retain(p_value->m_cons[1]);
			break;
		default:
			break;
	}

}

void mark(vm_t *p_vm)
{
	for(unsigned long i = 0; i < p_vm->m_sp; i++) {
		retain(p_vm->m_stack[i]);
	}

	if (p_vm->m_ev > 0) {
		retain(p_vm->m_current_env[p_vm->m_ev - 1]);
	}
}


void sweep(vm_t *p_vm, value_t **p_heap, value_t **p_tenured_heap)
{
	value_t* p = *p_heap;
	value_t* safe = NULL;

	// reset heap
	*p_heap = NULL;

	// traverse old heap
	for(;p;p = safe) {
	safe = p->m_heapptr;

assert(p->m_is_static == false);

	// keep on heap or free
	if (p->m_in_use) {
		p->m_in_use = false;

		if (++p->m_age >= GC_AGE && p_tenured_heap != NULL) {
			p->m_heapptr = *p_tenured_heap;
			*p_tenured_heap = p;
		} else {
			p->m_heapptr = *p_heap;
			*p_heap = p;
		}

    } else {
#if 1
		p->m_heapptr = NULL;
		if (p->m_size > 0) {
			p->m_data[0] = 0;
		}
      free(p);
#else

		value_t *test = p_vm->m_free_heap;
		while(test) {
			assert(test != p);
			test = test->m_heapptr;
		}

		p->m_heapptr = p_vm->m_free_heap;
		p_vm->m_free_heap = p;
#endif
    }
  }
}


