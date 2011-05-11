#include "value.h"

#include <stdlib.h>

value_t * value_create(value_type_t, unsigned long)
{
	malloc(sizeof(value_t) - sizeof(unsigned char[0]));
}

void value_destroy(value_t *)
{
}
