#include "binding.h"
#include "assert.h"

#include "env.h"

#include <stdio.h>
#include <stdlib.h>

value_t *environment_binding_find(vm_t *p_vm, value_t * p_symbol, bool p_func)
{
	return environment_binding_find(p_vm->m_current_env[p_vm->m_ev - 1], p_symbol, p_func);
}

value_t *environment_binding_find(value_t * p_env, value_t * p_symbol, bool p_func)
{
	assert(p_env && p_env->m_type == VT_ENVIRONMENT);
	assert(p_symbol && p_symbol->m_type == VT_SYMBOL);


	while(p_env) {
		environment_t *env = (environment_t *)p_env->m_data;
		assert((env->m_parent == NULL) || (env->m_parent->m_type == VT_ENVIRONMENT));

		assert((env->m_bindings == NULL) || (env->m_bindings->m_type == VT_BINDING));
		assert((env->m_function_bindings == NULL) || (env->m_function_bindings->m_type == VT_BINDING));

		value_t *top_binding = p_func == true ? 
				((environment_t *)p_env->m_data)->m_function_bindings :
				((environment_t *)p_env->m_data)->m_bindings;
				
		value_t *b = binding_find(top_binding, p_symbol);

		if (b != NULL) {
			return b;
		}

		p_env = env->m_parent;
	}

	return NULL;
}

