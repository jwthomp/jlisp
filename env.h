#ifndef __ENV_H_
#define __ENV_H_

#include "binding.h"
#include "value.h"
#include "vm.h"

struct vm_s;

typedef struct environment_s {
    binding_t *m_bindings;
    binding_t *m_function_bindings;
    value_t *m_parent;
} environment_t;

environment_t * environment_create(value_t *);
void environment_destroy(environment_t *);
binding_t *environment_binding_find(struct vm_s *p_vm, value_t * p_symbol, bool p_func);
binding_t *environment_binding_find(value_t * p_env, value_t * p_symbol, bool p_func);

#endif /* __ENV_H_ */
