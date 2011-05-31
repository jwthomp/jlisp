#ifndef __LABMDA_H_
#define __LAMBDA_H_

#include "value.h"

typedef struct lambda_s {
	value_t *m_parameters;
	value_t *m_bytecode;
	value_t *m_pool;
} lambda_t;


#endif /* __LAMBDA_H_ */
