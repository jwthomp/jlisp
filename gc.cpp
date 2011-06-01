#include "gc.h"

#include "lambda.h"
#include "value.h"
#include "value_helpers.h"
#include "assert.h"

#include <stdio.h>
#include <stdlib.h>

value_t *gc_alloc(vm_t *p_vm, size_t p_size, bool p_is_static)
{
	value_t * val = (value_t *)malloc(p_size);

	if (p_is_static == false) {
		val->m_heapptr = p_vm->m_heap;
		p_vm->m_heap = val;
	} else {
		val->m_heapptr = p_vm->m_static_heap;
		p_vm->m_static_heap = val;
	}
	val->m_is_static = p_is_static;
	return val;
}

void gc(vm_t *p_vm)
{
	mark(p_vm);
	sweep(p_vm);
}

void retain(value_t *p_value)
{

	if (p_value == NULL) {
		return;
	}

	assert(p_value->m_is_static == false);

	if (p_value->m_in_use || p_value->m_is_static) {
		return;
	}

	p_value->m_in_use = true;

	switch(p_value->m_type) {
		case VT_NUMBER:
		case VT_SYMBOL:
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

	retain(p_vm->m_current_env[p_vm->m_ev - 1]);
}

void sweep(vm_t *p_vm)
{
  value_t* p = p_vm->m_heap;
  value_t* safe = NULL;

  // reset heap
  p_vm->m_heap = NULL;

  // traverse old heap
  for(;p;p = safe) {
    safe = p->m_heapptr;

assert(p->m_is_static == false);

    // keep on heap or free
    if (p->m_in_use || p->m_is_static) {
      p->m_in_use = false;
      p->m_heapptr = p_vm->m_heap;
      p_vm->m_heap = p;
    } else {
printf("Freeing(%d): %p\n", p->m_type, p);
      free(p);
//		p->m_heapptr = p_vm->m_free_heap;
//		p_vm->m_free_heap = p;
    }
  }
}


