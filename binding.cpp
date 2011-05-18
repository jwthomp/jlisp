#include "value.h"

#include "binding.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

binding_t * binding_create(value_t *p_key, value_t *p_value)
{
	binding_t *bind = (binding_t *)malloc(sizeof(binding_t));
	bind->m_key = p_key;
	bind->m_value = p_value;
	return bind;
	
}

void bindings_free(binding_t *p_bindings)
{
	binding_t *next = p_bindings->m_next;
	
}

binding_t * binding_find(binding_t * p_bindings, value_t *p_key)
{
	if (p_bindings == NULL) {
		return NULL;
	}

	if (p_key->m_type != VT_SYMBOL) {
		return NULL;
	}

	if (p_bindings->m_key->m_type == VT_SYMBOL) {
		if (!memcmp(p_bindings->m_key->m_data, p_key->m_data, p_key->m_size)) {
			return p_bindings;
		}
	}

	return binding_find(p_bindings->m_next, p_key);
}

void binding_print(binding_t *p_binding)
{
	// Print key
	value_print(p_binding->m_key);

	printf(" ");

	// Print value
	value_print(p_binding->m_value);
}
