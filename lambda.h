#ifndef __LABMDA_H_
#define __LAMBDA_H_

#include "value.h"

typedef struct lambda_s {
	value_t *m_parameters;
	value_t *m_bytecode;
	value_t *m_pool;
	unsigned long m_bytecode_len;
	unsigned long m_pool_len;
} lambda_t;

lambda_t *lambda_create(value_t *p_parameters, value_t *p_bytecode, value_t *p_pool);
void lambda_destroy(lambda_t *p_lambda);

#endif /* __LAMBDA_H_ */
