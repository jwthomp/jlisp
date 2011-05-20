#ifndef __VALUE_H_
#define __VALUE_H_

typedef enum {
	VT_NUMBER,
	VT_POOL,
	VT_SYMBOL,
	VT_INTERNAL_FUNCTION
} value_type_t;

typedef struct value_s {
	value_type_t  m_type;
	unsigned long m_size;

	// Used by symbol table
	struct value_s *m_next;
	char m_data[0];
} value_t;


value_t * value_create(value_type_t p_type, char const * const p_data, unsigned long p_size);
void value_destroy(value_t *);
void value_print(value_t *p_value);

#endif /* __VALUE_H_ */
