#ifndef __BINDING_H_
#define __BINDING_H_

#include "value.h"

typedef struct binding_s {
	value_t m_key;
    value_t m_value;
    struct binding_s *m_next;
} binding_t;


void bindings_free(binding_t *);
binding_t * binding_create(value_t, value_t);


#endif /* __BINDING_H_ */
