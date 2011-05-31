#include "value.h"

#include "binding.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


value_t * binding_find(value_t * p_bindings, value_t *p_key)
{
	if (p_bindings == NULL) {
		return NULL;
	}

	if (p_bindings->m_type != VT_BINDING) {
		return NULL;
	}

	if (p_key->m_type != VT_SYMBOL) {
		return NULL;
	}

	binding_t *bind = (binding_t *)p_bindings->m_data;

	if (bind->m_key->m_type == VT_SYMBOL) {
		if (!memcmp(bind->m_key->m_data, p_key->m_data, p_key->m_size)) {
			return p_bindings;
		}
	}

	return binding_find(bind->m_next, p_key);
}
