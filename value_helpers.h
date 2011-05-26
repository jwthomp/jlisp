#ifndef __VALUE_HELPERS_H_
#define __VALUE_HELPERS_H_

#include "vm.h"
#include "env.h"

bool is_cons(value_t *);
bool is_symbol(value_t *);
bool is_number(value_t *);
bool is_bytecode(value_t *);
bool is_pool(value_t *);
bool is_lambda(value_t *);
bool is_null(value_t *);

value_t * value_create_number(int p_number);
value_t * value_create_symbol(char const * const p_symbol);
value_t * value_create_internal_func(vm_func_t p_func);
value_t * value_create_cons(value_t *p_car, value_t *p_cdr);
value_t * value_create_bytecode(bytecode_t *p_code, int p_code_count);
value_t * value_create_pool(value_t *p_literals[], int p_literal_count);
value_t * value_create_lambda(value_t *p_parameters, value_t *p_bytecode, value_t *p_pool);
value_t * value_create_closure(value_t *p_env, value_t *p_lambda);
value_t * value_create_environment(environment_t *p_env);

value_t * list(value_t *p_value);


#endif /* __VALUE_HELPERS_H_ */
