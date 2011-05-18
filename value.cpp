#include "value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

value_t * value_create(value_type_t p_type, char const * const p_arg, unsigned long p_size)
{
	value_t *v = (value_t *)malloc(sizeof(value_t) + p_size);
	v->m_type = p_type;
	v->m_size = p_size;
	v->m_next = NULL;
	memcpy(v->m_data, p_arg, p_size);
	return v;
}

void value_destroy(value_t *p_value)
{
	free(p_value);
}

value_t * value_create_number(int p_number)
{
	return value_create(VT_NUMBER, (char *)&p_number, 4);
}

value_t * value_create_symbol(char const * const p_symbol)
{
	return value_create(VT_SYMBOL, p_symbol, strlen(p_symbol));
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
		case VT_SYMBOL:
		{
			printf("%s", p_value->m_data);
			break;
		}
		default:
			break;
	};
}
