#include "value.h"

#include "binding.h"

#include <stdlib.h>
#include <string.h>

binding_t * binding_create(binding_key_t p_key, value_t p_value)
{
	binding_t *bind = (binding_t *)malloc(sizeof(binding_t));
	strncpy(bind->m_key, p_key, 16);
	bind->m_value = p_value;
	return bind;
	
}

void bindings_free(binding_t *p_bindings)
{
	binding_t *next = p_bindings->m_next;
	
}

binding_t * binding_find(binding_t * p_bindings, binding_key_t p_key)
{
	if (p_bindings == NULL) {
		return NULL;
	}

	if(!strcmp(p_key, p_bindings->m_key)) {
		return p_bindings;
	}

	return binding_find(p_bindings->m_next, p_key);
}
