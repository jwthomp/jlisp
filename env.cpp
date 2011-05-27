#include "binding.h"

#include "env.h"

#include <stdio.h>
#include <stdlib.h>

environment_t * environment_create(value_t *p_parent)
{
	environment_t *env = (environment_t *)malloc(sizeof(environment_t));
	env->m_parent = p_parent;
	env->m_bindings = NULL;
	env->m_function_bindings = NULL;
	return env;
}

void environment_destroy(environment_t *p_env)
{
	if (p_env->m_bindings != NULL) {
		bindings_free(p_env->m_bindings);
	}

	free(p_env);
}

binding_t *environment_binding_find(vm_t *p_vm, value_t * p_symbol, bool p_func)
{
	return environment_binding_find(p_vm->m_current_env[p_vm->m_ev - 1], p_symbol, p_func);
}

binding_t *environment_binding_find(value_t * p_env, value_t * p_symbol, bool p_func)
{
	while(p_env) {
		binding_t *top_binding = p_func == true ? 
				((environment_t *)p_env->m_data)->m_function_bindings :
				((environment_t *)p_env->m_data)->m_bindings;
				
		binding_t *b = binding_find(top_binding, p_symbol);

		if (b != NULL) {
			return b;
		}

		p_env = ((environment_t *)p_env->m_data)->m_parent;
	}
}

