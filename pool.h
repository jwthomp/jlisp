#ifndef __POOL_H_
#define __POOL_H_

typedef struct pool_s{
	char *m_bytes;
	unsigned long m_size;
	unsigned long m_pos;
} pool_t;

// p_size of 0 allocates default amount
pool_t *pool_alloc(unsigned long p_size);
void pool_free(pool_t *p_pool);


#endif /* __POOL_H_ */
