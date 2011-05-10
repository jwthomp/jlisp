#ifndef __BINDING_H_
#define __BINDING_H_

#include "value.h"

typedef char binding_key_t[16];

typedef struct binding_s {
    binding_key_t m_key;
    value_t m_value;
    struct binding_s *m_next;
} binding_t;


void bindings_free(binding_t *);


#endif /* __BINDING_H_ */
