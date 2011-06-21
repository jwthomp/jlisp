#include "value.h"
#include "value_helpers.h"
#include "lambda.h"
#include "assert.h"
#include "gc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char const *g_valuetype_print[] = {
	"VT_NUMBER",
	"VT_POOL",
	"VT_SYMBOL",
	"VT_INTERNAL_FUNCTION",
	"VT_CONS",
	"VT_BYTECODE",
	"VT_LAMBDA",
	"VT_CLOSURE",
	"VT_ENVIRONMENT",
	"VT_STRING",
	"VT_BINDING",
	"VT_MACRO",
	"VT_VOID",
};



value_t *t;
value_t *nil;
value_t *voidobj;

char g_string[4096];


value_t * value_create(vm_t *p_vm, value_type_t p_type, unsigned long p_size, bool p_is_static)
{

	value_t *v = gc_alloc(p_vm, sizeof(value_t) + p_size, p_is_static);
	v->m_type = p_type;
	v->m_size = p_size;
	v->m_is_static = p_is_static;
	v->m_age = 0;
	memset(v->m_data, 0, p_size);

//printf("value_create: %p %s\n", v, valuetype_print(p_type));
	return v;
}

void value_destroy(value_t *p_value)
{
}

value_t * value_create_number(vm_t *p_vm, int p_number)
{
	assert(!"YOU SHOULDNT BE HERE");
	value_t *ret =  value_create(p_vm, VT_NUMBER, 4, false);
	*(int *)ret->m_data = p_number;
	return ret;
}

value_t * value_create_static_string(vm_t *p_vm, char const * const p_string)
{
	value_t *ret =  value_create(p_vm, VT_STRING, strlen(p_string) + 1, true);
	strcpy((char *)ret->m_data, p_string);
	return ret;
}

value_t * value_create_string(vm_t *p_vm, char const * const p_string)
{
	value_t *ret =  value_create(p_vm, VT_STRING, strlen(p_string) + 1, false);
	strcpy((char *)ret->m_data, p_string);
	return ret;
}

value_t * value_create_symbol(vm_t *p_vm, char const * const p_symbol)
{
	value_t *string = value_create_static_string(p_vm, p_symbol);
	value_t *sym =  value_create(p_vm, VT_SYMBOL, sizeof(value_t *) * 2, true);
	sym->m_cons[0] = string;
	sym->m_cons[1] = voidobj;

	return sym;
}

value_t * value_create_internal_func(vm_t *p_vm, vm_func_t p_func, int p_nargs, bool p_is_macro)
{
	value_t *ret =  value_create(p_vm, VT_INTERNAL_FUNCTION, sizeof(vm_func_t) + 5, true);
	memcpy(ret->m_data, (char *)&p_func, sizeof(vm_func_t));
	*((int *)&ret->m_data[sizeof(vm_func_t)]) = p_nargs;
	*((bool *)&ret->m_data[sizeof(vm_func_t) + 4]) = p_is_macro;
	return ret;
}

value_t * value_create_cons(vm_t *p_vm, value_t *p_car, value_t *p_cdr)
{
	value_t *ret =  value_create(p_vm, VT_CONS, sizeof(value_t *) * 2, false);
	ret->m_cons[0] = p_car;
	ret->m_cons[1] = p_cdr;
	return ret;
}

value_t * value_create_bytecode(vm_t *p_vm, bytecode_t *p_code, int p_code_count)
{
	value_t *ret = value_create(p_vm, VT_BYTECODE, sizeof(bytecode_t) * p_code_count, false);
	memcpy(ret->m_data, (char *)p_code, sizeof(bytecode_t) * p_code_count);
	return ret;
}

value_t * value_create_pool(vm_t *p_vm, value_t *p_literals[], int p_literal_count)
{
	value_t *ret = value_create(p_vm, VT_POOL, sizeof(value_t *) * p_literal_count, false);
	memcpy(ret->m_data, p_literals, p_literal_count * sizeof(value_t *));
	return ret;
}

value_t * value_create_lambda(vm_t *p_vm, value_t *p_parameters, value_t *p_bytecode, value_t *p_pool)
{
	assert((p_parameters == nil) || is_cons(p_parameters));
	assert(p_bytecode && is_bytecode(p_bytecode));
	assert((p_pool != NULL) && is_pool(p_pool));

	value_t *ret = value_create(p_vm, VT_LAMBDA, sizeof(lambda_t), false);
	lambda_t *l = (lambda_t *)ret->m_data;
	l->m_parameters = p_parameters;
	l->m_bytecode = p_bytecode;
	l->m_pool = p_pool;
	l->m_is_macro = false;
	return ret;
}

int count_lambda_args(value_t *p_lambda)
{
	assert(p_lambda && is_lambda(p_lambda));

	lambda_t *l = (lambda_t *)p_lambda->m_data;
	value_t *p = l->m_parameters;
	int nargs = 0;
	bool has_rest = false;
	while (p != nil && p->m_cons[0] != nil) {
		nargs++;

// value_print(p->m_cons[0]); printf("\n");

		if (is_symbol(p->m_cons[0]) && is_symbol_name("&rest", p)) {
// printf("HAS REST\n"); 
			has_rest = true;
			nargs--;
		}

		p = p->m_cons[1];
	}

	if (has_rest == true) {
		nargs = -nargs;
	}

//printf("HAS %d ARGS\n", nargs);
//printf("- "); value_print(l->m_parameters); printf("\n");
	return nargs;
}


value_t * value_create_closure(vm_t *p_vm, value_t *p_env, value_t *p_lambda)
{
	value_t *ret = value_create(p_vm, VT_CLOSURE, sizeof(value_t *) * 3, false);
	ret->m_cons[0] = p_env;
	ret->m_cons[1] = p_lambda;
	ret->m_cons[2] = (value_t *)count_lambda_args(p_lambda);
	return ret;
}

value_t * value_create_binding(vm_t *p_vm, value_t *p_key, value_t *p_value)
{
	value_t *ret = value_create(p_vm, VT_BINDING, sizeof(binding_t), false);
	binding_t *bind = (binding_t *)ret->m_data;
	bind->m_key = p_key;
	bind->m_value = p_value;
	bind->m_next = NULL;

	return ret;

}

value_t * value_create_environment(vm_t *p_vm, value_t *p_env)
{
	value_t *ret = value_create(p_vm, VT_ENVIRONMENT, sizeof(environment_t), false);
	environment_t *env = (environment_t *)ret->m_data;
	env->m_bindings = NULL;
	env->m_function_bindings = NULL;
	env->m_parent = p_env;

	assert((p_env == NULL) || is_environment(p_env));

	return ret;
}



bool value_equal(value_t *p_value_1, value_t *p_value_2)
{
	bool first_num = is_fixnum(p_value_1);
	bool second_num = is_fixnum(p_value_2);

	if (first_num != second_num) {
		return false;
	}

	if (first_num == true && second_num == true) {
		return to_fixnum(p_value_1) == to_fixnum(p_value_2);
	}

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
		case VT_STRING:
		{
			return !strcmp(p_value_1->m_data, p_value_2->m_data);
		}
		case VT_SYMBOL:
		{
			value_t *str1 = p_value_1->m_cons[0];
			value_t *str2 = p_value_2->m_cons[0];
			return value_equal(str1, str2);
		}
		default:
			break;
	};

	return false;
}

void value_print(vm_t *p_vm, value_t *p_value) {
	printf("%s", value_sprint(p_vm, p_value)->m_data);
}

value_t * value_sprint(vm_t *p_vm, value_t *p_value)
{
	value_t *ret = value_create(p_vm, VT_STRING, 1024, false);

	assert(p_value != NULL);
	if(p_value == NULL) {
		snprintf(ret->m_data, 1024, "()");
		return ret;
	}

	if (is_fixnum(p_value)) {
		snprintf(ret->m_data, 1024, "%ld", to_fixnum(p_value));
		return ret;
	}

	switch(p_value->m_type) {
		case VT_CLOSURE:
			snprintf(ret->m_data, 1024, "closure");
			break;
		case VT_ENVIRONMENT:
			snprintf(ret->m_data, 1024, "environment");
			break;
		case VT_BYTECODE:
		{
			snprintf(ret->m_data, 1024, "bytecode");
			//printf("bytecode: \n");
			//bytecode_t *bc = (bytecode_t *)p_value->m_data;
			//for (unsigned long i = 0; i < (p_value->m_size / sizeof(bytecode_t)); i++) {
		//		printf("op: %s code: %lu\n", g_opcode_print[bc[i].m_opcode], bc[i].m_value);
		//	}
			break;
		}
		case VT_LAMBDA:
		{
			snprintf(ret->m_data, 1024, "lambda <0x%p>", p_value);
#if 0
			lambda_t *l = (lambda_t *)p_value->m_data;
			printf("lambda: args: ");
			value_print(l->m_parameters);
			printf("\n");

			printf("pool\n");
			value_print(l->m_pool);
			

			printf("bc:\n");
			value_print(l->m_bytecode);
#endif
			break;
		}
		case VT_POOL:
		{
			snprintf(ret->m_data, 1024, "pool <0x%p>", p_value);
#if 0
			int pool_size = p_value->m_size / sizeof(value_t *);
			printf("pool: ");
			for (int i = 0; i < pool_size; i++) {
				printf("%d] ", i); value_print(((value_t **)p_value->m_data)[i]); printf("\n");
			}
#endif
			break;
		}
		case VT_NUMBER:
		{
			int number = *((int *)p_value->m_data);
			snprintf(ret->m_data, 1024, "%d", number);
			break;
		}
		case VT_INTERNAL_FUNCTION:
		{
			snprintf(ret->m_data, 1024, "INTERNAL FUNCTION: <0x%p>", p_value);
			break;
		}
		case VT_STRING:
		{
			snprintf(ret->m_data, 1024, "\"%s\"", p_value->m_data);
			break;
		}
		case VT_MACRO:
		case VT_SYMBOL:
		{
			value_t *dt = p_value->m_cons[0];
			snprintf(ret->m_data, 1024, "%s", dt->m_data);
			break;
		}
		case VT_CONS:
		{
			snprintf(ret->m_data, 1024, "(%s %s)", 
				value_sprint(p_vm, p_value->m_cons[0])->m_data,
				value_sprint(p_vm, p_value->m_cons[1])->m_data);
			break;
		}
		case VT_BINDING:
		{
			snprintf(ret->m_data, 1024, "BINDING <%s>", 
				value_sprint(p_vm, ((binding_t *)p_value->m_data)->m_key)->m_data); 
			break;
		}
		default:
			printf("no printer for value type: %d\n", p_value->m_type);
			break;
	};

	return ret;
}

bool is_null(value_t *p_val)
{
	return p_val ==  nil ? true : false;
}


bool is_symbol(value_t *p_val)
{
	if(is_fixnum(p_val) || is_null(p_val)) {
		return false;
	}

	return p_val->m_type == VT_SYMBOL;
}

bool is_number(value_t *p_val)
{
	if(is_null(p_val)) {
		return false;
	}

	return is_fixnum(p_val);
}

bool is_closure(value_t *p_val) {
	if (is_fixnum(p_val)) {
		return false;
	}

	return (p_val->m_type == VT_CLOSURE);
}
	
bool is_binding(value_t *p_val) {
	if (is_fixnum(p_val)) {
		return false;
	}

	return (p_val->m_type == VT_BINDING);
}
	
bool is_environment(value_t *p_val) {
	if (is_fixnum(p_val)) {
		return false;
	}

	return (p_val->m_type == VT_ENVIRONMENT);
}
	
bool is_string(value_t *p_val) {
	if (is_fixnum(p_val)) {
		return false;
	}

	return (p_val->m_type == VT_STRING);
}
	
bool is_ifunc(value_t *p_val) {
	if (is_fixnum(p_val)) {
		return false;
	}

	return (p_val->m_type == VT_INTERNAL_FUNCTION);
}
	

bool is_cons(value_t *p_val)
{
	if(is_fixnum(p_val) || is_null(p_val)) {
		return false;
	}

	return (p_val->m_type == VT_CONS);
}

bool is_bytecode(value_t *p_val)
{
	if(is_fixnum(p_val) || is_null(p_val)) {
		return false;
	}

	return p_val->m_type == VT_BYTECODE;
}

bool is_pool(value_t *p_val)
{
	if(is_fixnum(p_val) || is_null(p_val)) {
		return false;
	}

	return p_val->m_type == VT_POOL;
}

bool is_lambda(value_t *p_val)
{
	if(is_fixnum(p_val) || is_null(p_val)) {
		return false;
	}

	return p_val->m_type == VT_LAMBDA;
}

bool is_macro(value_t *p_val)
{
	if(is_fixnum(p_val)) {
		return false;
	}

	return p_val->m_type == VT_MACRO;
}

value_t * list(vm_t *p_vm, value_t *p_value)
{
	return value_create_cons(p_vm, p_value, nil);
}

value_t *car(value_t *p_value)
{
	if (p_value == nil) {
		return nil;
	}
	assert(p_value && is_cons(p_value));
	return p_value->m_cons[0];
}

value_t *cdr(value_t *p_value)
{
	if (p_value == nil) {
		return nil;
	}

	assert(p_value && is_cons(p_value));
	return p_value->m_cons[1];
}

value_t *cadr(value_t *p_value)
{
	if (p_value == nil) {
		return nil;
	}

	assert(p_value && is_cons(p_value));
	assert(p_value->m_cons[1] && is_cons(p_value->m_cons[1]));
	return p_value->m_cons[1]->m_cons[0];
}

bool is_symbol_name(char const *p_name, value_t *p_symbol)
{
	return !strcmp(p_name, p_symbol->m_cons[0]->m_data);
}


char const *valuetype_print(int p_val)
{
	switch (p_val) {
		case VT_NUMBER:
			return g_valuetype_print[0];
		case VT_POOL:
			return g_valuetype_print[1];
		case VT_SYMBOL:
			return g_valuetype_print[2];
		case VT_INTERNAL_FUNCTION:
			return g_valuetype_print[3];
		case VT_CONS:
			return g_valuetype_print[4];
		case VT_BYTECODE:
			return g_valuetype_print[5];
		case VT_LAMBDA:
			return g_valuetype_print[6];
		case VT_CLOSURE:
			return g_valuetype_print[7];
		case VT_ENVIRONMENT:
			return g_valuetype_print[8];
		case VT_STRING:
			return g_valuetype_print[9];
		case VT_BINDING:
			return g_valuetype_print[10];
		case VT_MACRO:
			return g_valuetype_print[11];
		case VT_VOID:
			return g_valuetype_print[12];
		default:
			return "Unknown value type";
	}
}
