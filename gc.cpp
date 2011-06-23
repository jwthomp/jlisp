#include "gc.h"

#include "pool.h"
#include "lambda.h"
#include "value.h"
#include "value_helpers.h"
#include "assert.h"
#include "err.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define GC_AGE 3

static int g_count = 0;
static int g_alloc_count = 0;
static int g_retain_level = 0;

static pool_t *g_pool;

static value_t *retain(vm_t *p_vm, value_t *p_value, pool_t *p_pool_new);
static pool_t * do_gc(vm_t *p_vm, int p_age, unsigned long p_pool_size);

static value_t *alloc_from_pool(pool_t *p_pool, size_t p_size)
{
	
	value_t *val = (value_t *)&(p_pool->m_bytes[p_pool->m_pos]);
	if (p_size + p_pool->m_pos > p_pool->m_size)
	{
		return NULL;
	} 

	p_pool->m_pos += p_size;
	return val;
}

value_t *gc_alloc(vm_t *p_vm, size_t p_size, alloc_type p_alloc_type)
{
	value_t *val;
	if (p_alloc_type == STATIC) {
		val = (value_t *)malloc(p_size);
	} else if (p_alloc_type == MALLOC_MARK_SWEEP) {
		val = (value_t *)malloc(p_size);
	} else {
//printf("alloc %lu bytes\n", p_size);
		val = alloc_from_pool(p_vm->m_pool_g0, p_size);
		if (val == NULL) {
			// Allocate a new pool and continue on
			pool_t *p = pool_alloc(0);
			p->m_next = p_vm->m_pool_g0;
			p_vm->m_pool_g0 = p;
			val = alloc_from_pool(p_vm->m_pool_g0, p_size);
		}

		verify(val != NULL, "Couldn't allocate any more memory\n");
		//printf("alloc %d: %p\n", g_alloc_count++, val);
	}

	if (p_alloc_type == MALLOC_MARK_SWEEP) {
		val->m_heapptr = p_vm->m_heap_g0;
		p_vm->m_heap_g0 = val;
	} else if (p_alloc_type == STATIC) {
		val->m_heapptr = p_vm->m_static_heap;
		p_vm->m_static_heap = val;
	}

	return val;
}

void gc_shutdown(vm_t *p_vm)
{
	gc(p_vm, 0);

	value_t* p = p_vm->m_static_heap;
	value_t* safe = NULL;

	for(;p;p = safe) {
		safe = p->m_heapptr;
		free(p);
	}
}


static value_t *retain(vm_t *p_vm, value_t *p_value, pool_t *p_pool_new)
{
	g_retain_level++;


	// Don't do anything for fixnums or static objects
	if (is_fixnum(p_value) || p_value->m_alloc_type == STATIC) {
//is_fixnum(p_value) ? printf("%d retain %d: %p is_fixnum\n", g_retain_level, g_count++, p_value) : 
//					printf("%d retain %d: %p is static\n", g_retain_level, g_count++, p_value);
g_retain_level--;
		return p_value;
	}

	// Handle binary objects with mark/sweep algorithm
	if (p_value->m_alloc_type == MALLOC_MARK_SWEEP) {
		p_value->m_in_use = true;
		return p_value;
	}

	// Has this already been relocated? If so just return it

	// 0x1 is a bit in pointer that signifies whether this pointer
	// has been re-allocated, so return the new pointer which is stored
	// in the old memory location
	int *x =(int *)p_value;
	if ((*x) & 0x1) {
		value_t *ret = (value_t *)(*x & ~1);

//printf("%d p_value %d %p already redirected %p -> %s\n", g_retain_level, g_count++, p_value, ret, valuetype_print(ret->m_type));

g_retain_level--;
		return ret;
	}

g_count++;

	// This object hasn't been relocated so let's relocate it
	int size = sizeof(value_t) + p_value->m_size;
	value_t *new_value = alloc_from_pool(p_pool_new, size);
	memcpy(new_value, p_value, size);

	assert(new_value->m_type == p_value->m_type);


unsigned int old_value_val = *(unsigned int *)p_value;

//printf("%d retain %d: %p -> %p -- %s (%d)\n", g_retain_level, g_count, p_value, new_value, valuetype_print(p_value->m_type), p_value->m_type);


	// Flag the old data that this was redirected
	*((unsigned long *)p_value) = ((unsigned long)new_value) | 0x1;


//printf("retain: oldptr: %p old val: %x new val: %x\nnew_ptr: %p new val: %x\n",
//	p_value, old_value_val, *(unsigned int *)p_value,
//	new_value, *(unsigned int *)new_value);

	value_t *ret;
	// Now follow this object down the rabbit hole
	switch(new_value->m_type) {
		case VT_NUMBER:
		case VT_VOID:
		case VT_INTERNAL_FUNCTION:
		case VT_BYTECODE:
		case VT_STRING:
			break;
		case VT_BINDING:
//printf("key: %s ", ((binding_t *)p_value->m_data)->m_key->m_cons[0]->m_data);
//printf("val: %s\n", valuetype_print(new_value->m_type));

			ret = retain(p_vm, ((binding_t *)p_value->m_data)->m_key, p_pool_new);
			((binding_t *)new_value->m_data)->m_key = ret;
			ret = retain(p_vm, ((binding_t *)p_value->m_data)->m_value, p_pool_new);
			((binding_t *)new_value->m_data)->m_value = ret;
			if (((binding_t *)p_value->m_data)->m_next != NULL) {
				ret = retain(p_vm, ((binding_t *)p_value->m_data)->m_next, p_pool_new);
				((binding_t *)new_value->m_data)->m_next = ret;
			}
			break;

		case VT_POOL:
			for (unsigned long i = 0; i < (p_value->m_size / sizeof(value_t *)); i++) {
				ret = retain(p_vm, ((value_t **)p_value->m_data)[i], p_pool_new);
				((value_t **)new_value->m_data)[i] = ret;
			}
			break;

		case VT_LAMBDA:
			ret = retain(p_vm, ((lambda_t *)p_value->m_data)->m_parameters, p_pool_new);
			((lambda_t *)new_value->m_data)->m_parameters = ret;
			ret = retain(p_vm, ((lambda_t *)p_value->m_data)->m_bytecode, p_pool_new);
			((lambda_t *)new_value->m_data)->m_bytecode = ret;
			ret = retain(p_vm, ((lambda_t *)p_value->m_data)->m_pool, p_pool_new);
			((lambda_t *)new_value->m_data)->m_pool = ret;
			break;

		case VT_ENVIRONMENT:
			if (((environment_t *)p_value->m_data)->m_bindings != NULL) {
				ret = retain(p_vm, ((environment_t *)p_value->m_data)->m_bindings, p_pool_new);
				((environment_t *)new_value->m_data)->m_bindings = ret;
			}

			if (((environment_t *)p_value->m_data)->m_function_bindings != NULL) {
				ret = retain(p_vm, ((environment_t *)p_value->m_data)->m_function_bindings, p_pool_new);
				((environment_t *)new_value->m_data)->m_function_bindings = ret;
			}

			if (((environment_t *)p_value->m_data)->m_parent != NULL) {
				ret = retain(p_vm, ((environment_t *)p_value->m_data)->m_parent, p_pool_new);
				((environment_t *)new_value->m_data)->m_parent = ret;
			}
			break;

		case VT_CLOSURE:
		case VT_SYMBOL:
		case VT_CONS:
			ret = retain(p_vm, p_value->m_cons[0], p_pool_new);
			new_value->m_cons[0] = ret;
			ret = retain(p_vm, p_value->m_cons[1], p_pool_new);
			new_value->m_cons[1] = ret;
			break;
		default:
			assert(!"SHOULD NEVER GET HERE");
			break;
	}
	
g_retain_level--;
	return new_value;
	
}


static pool_t * do_gc(vm_t *p_vm, int p_age, unsigned long p_pool_size)
{
	pool_t *pool_cur = p_vm->m_pool_g0;
	pool_t *pool_new = pool_alloc(p_pool_size);

g_count = 0;

printf("GC FIRED: sp: %lu csp: %lu ev: %ld gc: %d\n", p_vm->m_sp, p_vm->m_csp, p_vm->m_ev, g_count);

//printf("IN ev: %p\n",  p_vm->m_current_env[p_vm->m_ev - 1]);
	
	value_t *ret;
	for(unsigned long i = 0; i < p_vm->m_ev; i++) {
		ret = retain(p_vm, p_vm->m_current_env[i], pool_new);
		p_vm->m_current_env[i] = ret;
	}

//printf("GC STACK: sp: %lu csp: %lu gc: %d\n", p_vm->m_sp, p_vm->m_csp, g_count);

	// Keep the values on the stack
	for(unsigned long i = 0; i < p_vm->m_sp; i++) {

//printf("stack: %s\n", valuetype_print(p_vm->m_stack[i]->m_type));
		ret = retain(p_vm, p_vm->m_stack[i], pool_new);
		p_vm->m_stack[i] = ret;
	}

//printf("GC C STACK: sp: %lu csp: %lu gc: %d\n", p_vm->m_sp, p_vm->m_csp, g_count);

	// Keep the values on the c_stack
	for(unsigned long i = 0; i < p_vm->m_csp; i++) {
		ret = retain(p_vm, p_vm->m_c_stack[i], pool_new);
assert(ret != NULL);
		p_vm->m_c_stack[i] = ret;
	}

//printf("GC DONE: sp: %lu csp: %lu ev: %lu gc: %d\n", p_vm->m_sp, p_vm->m_csp, p_vm->m_ev, g_count);

	pool_t *p = p_vm->m_pool_g0;
	while(p) {
		pool_t *np = p->m_next;
		memset((void *)p->m_bytes, 0xFF, p->m_size);
		pool_free(p);
		p = np;
	}

	p_vm->m_pool_g0 = pool_new;

	return pool_new;
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

		assert(p->m_alloc_type != STATIC);

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

printf("free!\n");
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


unsigned long mem_allocated(vm_t *p_vm)
{
	unsigned long size = 0;

	// Count up compacting memory
	pool_t *p = p_vm->m_pool_g0;
	while(p) {
		size += p->m_size;
		p = p->m_next;
	}

	// Count up mark/sweep memory
	value_t *h = p_vm->m_heap_g0;
	while(h) {
		size += sizeof(value_t) + h->m_size;
		h = h->m_heapptr;
	}

	h = p_vm->m_heap_g1;
	while(h) {
		size += sizeof(value_t) + h->m_size;
		h = h->m_heapptr;
	}

	return size;
}

unsigned long gc(vm_t *p_vm, int p_age)
{


	pool_t *pool_new = do_gc(p_vm, p_age, mem_allocated(p_vm));

	if (p_age == 0) {
		sweep(p_vm, &p_vm->m_heap_g0, NULL);
	} else {
		sweep(p_vm, &p_vm->m_heap_g0, &p_vm->m_heap_g1);
	}


	return pool_new->m_size - pool_new->m_pos;

//printf("OUT ev: %p\n",  p_vm->m_current_env[p_vm->m_ev - 1]);
}


