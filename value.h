#ifndef __VALUE_H_
#define __VALUE_H_

typedef enum {
	VT_NUMBER,
	VT_POOL,
	VT_SYMBOL
} value_type_t;

typedef struct value_s {
	value_type_t  m_type;
	union {
		long number;
		long pool_index;
		unsigned char symbol[16];
	} m_data;
} value_t;

#endif /* __VALUE_H_ */
