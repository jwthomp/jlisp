#ifndef __BINDING_H_
#define __BINDING_H_

#include "value.h"

typedef struct binding_s {
	value_t *m_key;
    value_t *m_value;
    value_t *m_next;
} binding_t;


value_t * binding_find(vm_t *p_vm, value_t * p_bindings, value_t *p_key);

value_t *binding_get_value(value_t *p_binding);
value_t *binding_get_key(value_t *p_binding);
value_t *binding_get_next(value_t *p_binding);


#endif /* __BINDING_H_ */
