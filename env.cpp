#include "binding.h"

#include "env.h"

#include <stdlib.h>

environment_t * environment_create(environment_t *p_parent)
{
	environment_t *env = (environment_t *)malloc(sizeof(environment_t));
	env->m_parent = p_parent;
	env->m_bindings = NULL;
	return env;
}

void environment_destroy(environment_t *p_env)
{
	if (p_env->m_bindings != NULL) {
		bindings_free(p_env->m_bindings);
	}

	free(p_env);
}
