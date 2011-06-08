#ifndef __VALUE_H_
#define __VALUE_H_


typedef enum {
	VT_NUMBER = 0,	
	VT_POOL = 1,
	VT_SYMBOL = 2,
	VT_INTERNAL_FUNCTION = 3,
	VT_CONS = 4,
	VT_BYTECODE = 5,
	VT_LAMBDA = 6,
	VT_CLOSURE = 7,
	VT_ENVIRONMENT = 8,
	VT_STRING = 9,
	VT_BINDING = 10,
	VT_MACRO = 11,
	VT_VOID = 11,
} value_type_t;

typedef struct value_s {
	value_type_t  m_type;
	unsigned long m_size;

	bool m_in_use;
	bool m_is_static;
	struct value_s *m_heapptr;

	union {
		char m_data[0];
		struct value_s *m_cons[0];
	};

} value_t;

extern char const *g_valuetype_print[];
extern value_t *nil;
extern value_t *t;
extern value_t *voidobj;

typedef struct vm_s vm_t;

value_t * value_create(vm_t *p_vm, value_type_t p_type, unsigned long p_size, bool p_is_static);
void value_destroy(value_t *);
void value_print(value_t *p_value);

value_t *car(value_t *p_value);
value_t *cdr(value_t *p_value);
value_t *cadr(value_t *p_value);

bool value_equal(value_t *p_value_1, value_t *p_value_2);
bool is_symbol_name(char const *p_name, value_t *p_symbol);

#endif /* __VALUE_H_ */
