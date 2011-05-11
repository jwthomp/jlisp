#include "value.h"

#include <stdlib.h>

value_t * value_create(value_type_t p_type, unsigned long p_size)
{
	value_t *v = (value_t *)malloc(sizeof(value_t) - sizeof(unsigned char[0]));
	v->m_type = p_type;
	v->m_size = p_size;
	v->m_next = NULL;
	v->m_data[0] = 0;
}

void value_destroy(value_t *p_value)
{
	free(p_value);
}
