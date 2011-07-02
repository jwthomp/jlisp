#include "value.h"
#include "value_helpers.h"

#include "binding.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


value_t * binding_find(vm_t *p_vm, value_t * p_bindings, value_t *p_key)
{
//	if (p_key == p_vm->nil || is_fixnum(p_key) || p_key->m_type != VT_SYMBOL) {
//		return NULL;
//	}

	value_t *cur_bindings = p_bindings;
	while(cur_bindings) {
		if (cur_bindings == NULL || cur_bindings == p_vm->nil || is_fixnum(cur_bindings)) {
			return NULL;
		}

		if (cur_bindings->m_type != VT_BINDING) {
			return NULL;
		}

		binding_t *bind = (binding_t *)cur_bindings->m_data;

		if (bind->m_key == p_key) {
			return cur_bindings;
		}

		cur_bindings = bind->m_next;
	}

	return NULL;
}

value_t *binding_get_value(value_t *p_binding)
{
	binding_t *bind = (binding_t *)p_binding->m_data;
	return bind->m_value;
}

value_t *binding_get_key(value_t *p_binding)
{
	binding_t *bind = (binding_t *)p_binding->m_data;
	return bind->m_key;
}

value_t *binding_get_next(value_t *p_binding)
{
	binding_t *bind = (binding_t *)p_binding->m_data;
	return bind->m_next;
}

