#include "value.h"
#include "value_helpers.h"

#include "binding.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


value_t * binding_find(vm_t *p_vm, value_t * p_bindings, value_t *p_key)
{
	if (p_bindings == NULL) {
		return NULL;
	}

	if (is_binding(p_vm, p_bindings) == false) {
		return NULL;
	}

	if (is_symbol(p_vm, p_key) == false) {
		return NULL;
	}

	binding_t *bind = (binding_t *)p_bindings->m_data;

	if (is_symbol(p_vm, bind->m_key)) {
		if (value_equal(bind->m_key, p_key)) {
			return p_bindings;
		}
	}

	return binding_find(p_vm, bind->m_next, p_key);
}
