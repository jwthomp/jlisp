#include "binding.h"
#include "assert.h"
#include "value_helpers.h"

#include "env.h"

#include <stdio.h>
#include <stdlib.h>

value_t *environment_binding_find(vm_t *p_vm, value_t * p_symbol, bool p_func)
{
	return environment_binding_find(p_vm, p_vm->m_current_env[p_vm->m_ev - 1], p_symbol, p_func);
}

value_t *environment_binding_find(vm_t *p_vm, value_t * p_env, value_t * p_symbol, bool p_func)
{
	if (p_env == nil || is_fixnum(p_env) || p_env == NULL || p_env->m_type != VT_ENVIRONMENT) {
        return NULL;
    }
	if (p_symbol == nil || is_fixnum(p_symbol) || p_symbol == NULL || p_symbol->m_type != VT_SYMBOL) {
        return NULL;
    }


	while(p_env) {
		environment_t *env = (environment_t *)p_env->m_data;
		assert((env->m_parent == NULL) || is_environment(env->m_parent));
		assert((env->m_bindings == NULL) || is_binding(env->m_bindings));
		assert((env->m_function_bindings == NULL) || is_binding(env->m_function_bindings));

		value_t *top_binding = p_func == true ? 
				((environment_t *)p_env->m_data)->m_function_bindings :
				((environment_t *)p_env->m_data)->m_bindings;
				
		value_t *b = binding_find(p_vm, top_binding, p_symbol);

		if (b != NULL) {
			return b;
		}

		p_env = env->m_parent;
	}

	return NULL;
}

value_t *environment_get_bindings(value_t *p_env)
{
	environment_t *env = (environment_t *)p_env->m_data;
	return env->m_bindings;
}

value_t *environment_get_fbindings(value_t *p_env)
{
	environment_t *env = (environment_t *)p_env->m_data;
	return env->m_function_bindings;
}

value_t *environment_get_parent(value_t *p_env)
{
	environment_t *env = (environment_t *)p_env->m_data;
	return env->m_parent;
}

