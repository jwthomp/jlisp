#include "value.h"
#include "value_helpers.h"
#include "lambda.h"
#include "assert.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char const *g_opcode_print[] =  {
	"OP_PUSH",
	"OP_PUSH_ENV",
	"OP_POP_ENV",
	"OP_BIND",
	"OP_BINDF",
	"OP_PRINT",
	"OP_DUP",
	"OP_LOAD",
	"OP_LOADF",
	"OP_CALL"
};


value_t * value_create(value_type_t p_type, unsigned long p_size)
{
	value_t *v = (value_t *)malloc(sizeof(value_t) + p_size);
	v->m_type = p_type;
	v->m_size = p_size;
	v->m_next = NULL;
	memset(v->m_data, 0, p_size);
	return v;
}

void value_destroy(value_t *p_value)
{
	free(p_value);
}

value_t * value_create_number(int p_number)
{
	value_t *ret =  value_create(VT_NUMBER, 4);
	memcpy(ret->m_data, (char *)&p_number, 4);
	return ret;
}

value_t * value_create_symbol(char const * const p_symbol)
{
	value_t *ret =  value_create(VT_SYMBOL, strlen(p_symbol));
	memcpy(ret->m_data, p_symbol, strlen(p_symbol));
	return ret;
}

value_t * value_create_internal_func(vm_func_t p_func)
{
	value_t *ret =  value_create(VT_INTERNAL_FUNCTION, sizeof(vm_func_t));
	memcpy(ret->m_data, (char *)&p_func, sizeof(vm_func_t));
	return ret;
}

value_t * value_create_cons(value_t *p_car, value_t *p_cdr)
{
	value_t *ret =  value_create(VT_CONS, sizeof(value_t *) * 2);
	ret->m_cons[0] = p_car;
	ret->m_cons[1] = p_cdr;
	return ret;
}

value_t * value_create_bytecode(bytecode_t *p_code, int p_code_count)
{
	value_t *ret = value_create(VT_BYTECODE, sizeof(bytecode_t) * p_code_count);
	memcpy(ret->m_data, (char *)p_code, sizeof(bytecode_t) * p_code_count);
	return ret;
}

value_t * value_create_pool(value_t *p_literals[], int p_literal_count)
{
	value_t *ret = value_create(VT_POOL, sizeof(value_t *) * p_literal_count);
	memcpy(ret->m_data, p_literals, p_literal_count * sizeof(value_t *));
	return ret;
}

value_t * value_create_lambda(value_t *p_parameters, value_t *p_bytecode, value_t *p_pool)
{
	assert((p_parameters == NULL) || (p_parameters->m_type == VT_CONS));
	assert(p_bytecode && (p_bytecode->m_type == VT_BYTECODE));
	assert((p_pool == NULL) || (p_pool->m_type == VT_POOL));

	value_t *ret = value_create(VT_LAMBDA, sizeof(lambda_t));
	lambda_t l;
	l.m_parameters = p_parameters;
	l.m_bytecode = p_bytecode;
	l.m_pool = p_pool;
	memcpy(ret->m_data, (char *)&l, sizeof(lambda_t));
	return ret;
}


value_t * value_create_closure(value_t *p_env, value_t *p_lambda)
{
	value_t *ret = value_create(VT_CLOSURE, sizeof(value_t *) * 2);
	ret->m_cons[0] = p_env;
	ret->m_cons[1] = p_lambda;
	return ret;
}

value_t * value_create_environment(environment_t *p_env)
{
	value_t *ret = value_create(VT_ENVIRONMENT, sizeof(environment_t));
	memcpy(ret->m_data, (char *)&p_env, sizeof(environment_t));
	return ret;
}



bool value_equal(value_t *p_value_1, value_t *p_value_2)
{
	if (p_value_1->m_size != p_value_2->m_size) {
		return false;
	}

	if (p_value_1->m_type != p_value_2->m_type) {
		return false;
	}

	switch(p_value_1->m_type) {
		case VT_NUMBER:
		{
			int number_1 = *((int *)p_value_1->m_data);
			int number_2 = *((int *)p_value_2->m_data);
			return number_1 == number_2 ? true : false;
		}
		case VT_SYMBOL:
		{
			if (!memcmp(p_value_1->m_data, p_value_2->m_data, p_value_1->m_size)) {
				return true;
			} else {
				return false;
			}
		}
		default:
			break;
	};

	return false;
}

void value_print(value_t *p_value)
{
	if(p_value == NULL) {
		printf("()");
		return;
	}

	switch(p_value->m_type) {
		case VT_BYTECODE:
		{
			bytecode_t *bc = (bytecode_t *)p_value->m_data;
			for (unsigned long i = 0; i < (p_value->m_size / sizeof(bytecode_t)); i++) {
				printf("op: %s code: %lu\n", g_opcode_print[bc[i].m_opcode], bc[i].m_value);
			}
			break;
		}
		case VT_LAMBDA:
		{
			lambda_t *l = (lambda_t *)p_value->m_data;
			printf(" args: ");
			value_print(l->m_parameters);
			printf("\n");

			printf("pool\n");
			value_print(l->m_pool);
			

			printf("bc:\n");
			value_print(l->m_bytecode);
			break;
		}
		case VT_POOL:
		{
			int pool_size = p_value->m_size / sizeof(value_t *);
			for (unsigned long i = 0; i < pool_size; i++) {
				printf("%lu] ", i); value_print(((value_t **)p_value->m_data)[i]); printf("\n");
			}
			break;
		}
		case VT_NUMBER:
		{
			int number = *((int *)p_value->m_data);
			printf("%d", number);
			break;
		}
		case VT_INTERNAL_FUNCTION:
		{
			int number = *((int *)p_value->m_data);
			printf("%d", number);
			break;
		}
		case VT_SYMBOL:
		{
			printf("%s", p_value->m_data);
			break;
		}
		case VT_CONS:
		{
			printf("(");
			if (p_value->m_cons[0] != NULL) {
				value_print(p_value->m_cons[0]);

				if (p_value->m_cons[1] != NULL) {
					printf(" ");
					value_print(p_value->m_cons[1]);
				}
			}
			printf(")");
			break;
		}
		default:
			break;
	};
}

bool is_null(value_t *p_val)
{
	return p_val == NULL ? true : false;
}


bool is_symbol(value_t *p_val)
{
	if(is_null(p_val)) {
		return false;
	}

	return p_val->m_type == VT_SYMBOL;
}

bool is_number(value_t *p_val)
{
	if(is_null(p_val)) {
		return false;
	}

	return p_val->m_type == VT_NUMBER;
}

bool is_cons(value_t *p_val)
{
	if(is_null(p_val)) {
		return false;
	}

	return (p_val->m_type == VT_CONS);
}

bool is_bytecode(value_t *p_val)
{
	if(is_null(p_val)) {
		return false;
	}

	return p_val->m_type == VT_BYTECODE;
}

bool is_pool(value_t *p_val)
{
	if(is_null(p_val)) {
		return false;
	}

	return p_val->m_type == VT_POOL;
}

bool is_lambda(value_t *p_val)
{
	if(is_null(p_val)) {
		return false;
	}

	return p_val->m_type == VT_LAMBDA;
}

value_t * list(value_t *p_value)
{
	value_create_cons(p_value, NULL);
}

