#include "binding.h"

#include "env.h"

#include <stdio.h>
#include <stdlib.h>

value_t *environment_binding_find(vm_t *p_vm, value_t * p_symbol, bool p_func)
{
	return environment_binding_find(p_vm->m_current_env[p_vm->m_ev - 1], p_symbol, p_func);
}

value_t *environment_binding_find(value_t * p_env, value_t * p_symbol, bool p_func)
{
	while(p_env) {
		value_t *top_binding = p_func == true ? 
				((environment_t *)p_env->m_data)->m_function_bindings :
				((environment_t *)p_env->m_data)->m_bindings;
				
		value_t *b = binding_find(top_binding, p_symbol);

		if (b != NULL) {
			return b;
		}

		p_env = ((environment_t *)p_env->m_data)->m_parent;
	}
}

