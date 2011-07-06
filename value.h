#ifndef __VALUE_H_
#define __VALUE_H_



typedef enum {
	VT_NUMBER = 0,						// 0
	VT_POOL = 1 << 1, 					// 2
	VT_SYMBOL = 1 << 2,					// 4
	VT_INTERNAL_FUNCTION = 1 << 3,		// 8
	VT_CONS = 1 << 4,					// 16
	VT_BYTECODE = 1 << 5,				// 32
	VT_LAMBDA = 1 << 6,
	VT_CLOSURE = 1 << 7,
	VT_ENVIRONMENT = 1 << 8,
	VT_STRING = 1 << 9,
	VT_BINDING = 1 << 10,
	VT_MACRO =  1 << 11,
	VT_VOID = 1 << 12,
	VT_PID = 1 << 13,
	VT_PROCESS = 1 << 14,
	VT_VM_STATE = 1 << 15,
} value_type_t;

typedef enum {
    COPY_COMPACT,
    STATIC,
    MALLOC_MARK_SWEEP
} alloc_type;


typedef struct value_s {
	value_type_t  m_type;
	unsigned long m_size;

	alloc_type m_alloc_type;

	int m_age;

	struct value_s *m_heapptr;
	bool m_in_use;

	struct value_s *m_next_symbol;

	union {
		char m_data[0];
		struct value_s *m_cons[0];
	};

} value_t;

extern char const *g_valuetype_print[];

typedef struct vm_s vm_t;

value_t * value_create(vm_t *p_vm, value_type_t p_type, unsigned long p_size, bool p_is_static);
void value_destroy(value_t *);
void value_print(vm_t *p_vm, value_t *p_value);
value_t *value_sprint(vm_t *p_vm, value_t *p_value);

value_t *car(vm_t *p_vm, value_t *p_value);
value_t *cdr(vm_t *p_vm, value_t *p_value);
value_t *cadr(vm_t *p_vm, value_t *p_value);

bool value_equal(value_t *p_value_1, value_t *p_value_2);
bool is_symbol_name(char const *p_name, value_t *p_symbol);

#endif /* __VALUE_H_ */
