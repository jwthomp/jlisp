#include "value.h"
#include "value_helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

value_t * value_create(value_type_t p_type, unsigned long p_size)
{
	value_t *v = (value_t *)malloc(sizeof(value_t) + p_size);
	v->m_type = p_type;
	v->m_size = p_size;
	v->m_next = NULL;
	memset(v->m_data, 0, p_size);
	return v;
}

void value_destroy(value_t *p_value)
{
	free(p_value);
}

value_t * value_create_number(int p_number)
{
	value_t *ret =  value_create(VT_NUMBER, 4);
	memcpy(ret->m_data, (char *)&p_number, 4);
	return ret;
}

value_t * value_create_symbol(char const * const p_symbol)
{
	value_t *ret =  value_create(VT_SYMBOL, strlen(p_symbol));
	memcpy(ret->m_data, p_symbol, strlen(p_symbol));
	return ret;
}

value_t * value_create_internal_func(vm_func_t p_func)
{
	value_t *ret =  value_create(VT_INTERNAL_FUNCTION, sizeof(vm_func_t));
	memcpy(ret->m_data, (char *)&p_func, sizeof(vm_func_t));
	return ret;
}

value_t * value_create_cons(value_t *p_car, value_t *p_cdr)
{
	value_t *ret =  value_create(VT_CONS, sizeof(value_t *) * 2);
	ret->m_cons[0] = p_car;
	ret->m_cons[1] = p_cdr;
	return ret;
}

value_t * value_create_bytecode(bytecode_t *p_code, int p_code_count)
{
	value_t *ret = value_create(VT_BYTECODE, sizeof(bytecode_t) * p_code_count);
	memcpy(ret->m_data, (char *)p_code, sizeof(bytecode_t) * p_code_count);
	return ret;
}


bool value_equal(value_t *p_value_1, value_t *p_value_2)
{
	if (p_value_1->m_size != p_value_2->m_size) {
		return false;
	}

	if (p_value_1->m_type != p_value_2->m_type) {
		return false;
	}

	switch(p_value_1->m_type) {
		case VT_NUMBER:
		{
			int number_1 = *((int *)p_value_1->m_data);
			int number_2 = *((int *)p_value_2->m_data);
			return number_1 == number_2 ? true : false;
		}
		case VT_SYMBOL:
		{
			if (!memcmp(p_value_1->m_data, p_value_2->m_data, p_value_1->m_size)) {
				return true;
			} else {
				return false;
			}
		}
		default:
			break;
	};

	return false;
}

void value_print(value_t *p_value)
{
	switch(p_value->m_type) {
		case VT_NUMBER:
		{
			int number = *((int *)p_value->m_data);
			printf("%d", number);
			break;
		}
		case VT_INTERNAL_FUNCTION:
		{
			int number = *((int *)p_value->m_data);
			printf("%d", number);
			break;
		}
		case VT_SYMBOL:
		{
			printf("%s", p_value->m_data);
			break;
		}
		case VT_CONS:
		{
			printf("( ");
			value_print(p_value->m_cons[0]);
			printf(" ");
			value_print(p_value->m_cons[1]);
			printf(" )");
			break;
		}
		default:
			break;
	};
}
