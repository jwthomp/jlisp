#ifndef __VALUE_H_
#define __VALUE_H_

typedef enum {
	VT_NUMBER,
	VT_POOL,
	VT_SYMBOL
} value_type_t;

typedef struct value_s {
	value_type_t  m_type;
	unsigned long m_size;
	char m_data[0];
} value_t;

value_t * value_create(value_type_t, unsigned long);
void value_destroy(value_t *);

#endif /* __VALUE_H_ */
