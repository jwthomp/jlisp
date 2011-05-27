#include "lambda.h"

#include <stdio.h>
#include <stdlib.h>

lambda_t *lambda_create(value_t *p_parameters, value_t *p_bytecode, value_t *p_pool)
{
	lambda_t *lambda = (lambda_t *)malloc(sizeof(lambda_t));
	lambda->m_parameters = p_parameters;
	lambda->m_bytecode = p_bytecode;
	lambda->m_pool = p_pool;

printf("lambda: p: %u b: %u p: %u\n", sizeof(p_parameters), sizeof(p_bytecode), sizeof(p_pool));

}

void lambda_destroy(lambda_t *p_lambda)
{
	value_destroy(p_lambda->m_parameters);
	value_destroy(p_lambda->m_bytecode);
	value_destroy(p_lambda->m_pool);
	free(p_lambda);
}

