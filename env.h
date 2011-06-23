#ifndef __ENV_H_
#define __ENV_H_

#include "binding.h"
#include "value.h"
#include "vm.h"


typedef struct environment_s {
    value_t *m_bindings;
    value_t *m_function_bindings;
    value_t *m_parent;
} environment_t;

value_t *environment_binding_find(vm_t *p_vm, value_t * p_symbol, bool p_func);
value_t *environment_binding_find(vm_t *p_vm, value_t * p_env, value_t * p_symbol, bool p_func);

#endif /* __ENV_H_ */
