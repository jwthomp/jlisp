#include "pool.h"

#include <stdio.h>
#include <stdlib.h>

#define POOL_SIZE 409600

pool_t *pool_alloc(unsigned long p_size)
{
	if (p_size == 0) {
		p_size = POOL_SIZE;
	}


	pool_t *ret = (pool_t *)malloc(sizeof(pool_t));
	ret->m_bytes = (char *)malloc(p_size);
	ret->m_size = p_size;
	ret->m_pos = 0;

	return ret;
}

void pool_free(pool_t *p_pool)
{
	free(p_pool->m_bytes);
	free(p_pool);
}

