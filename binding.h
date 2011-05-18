#ifndef __BINDING_H_
#define __BINDING_H_

#include "value.h"

typedef struct binding_s {
	value_t *m_key;
    value_t *m_value;
    struct binding_s *m_next;
} binding_t;


void bindings_free(binding_t *);
binding_t * binding_create(value_t *m_key, value_t *m_value);
binding_t * binding_find(binding_t * p_bindings, value_t *p_key);

void binding_print(binding_t *p_binding);


#endif /* __BINDING_H_ */
