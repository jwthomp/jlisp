#ifndef __ENV_H_
#define __ENV_H_

#include "binding.h"

typedef struct environment_s {
    binding_t *m_bindings;
    binding_t *m_function_bindings;
    struct environment_s *m_parent;
} environment_t;

environment_t * environment_create(environment_t *);
void environment_destroy(environment_t *);

#endif /* __ENV_H_ */
