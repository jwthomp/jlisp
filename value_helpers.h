#ifndef __VALUE_HELPERS_H_
#define __VALUE_HELPERS_H_

#include "vm.h"
#include "env.h"

#include <stdlib.h>

#define make_fixnum(n) ((value_t *)(((n) << 1)|1))
#define is_fixnum(p) (((size_t)(p)) & 1)
#define to_fixnum(p) (((long)(p)) >> 1)

bool is_environment(vm_t *p_vm, value_t *);
bool is_binding(vm_t *p_vm, value_t *);
bool is_closure(vm_t *p_vm, value_t *);
bool is_ifunc(vm_t *p_vm, value_t *);
bool is_string(vm_t *p_vm, value_t *);
bool is_cons(vm_t *p_vm, value_t *);
bool is_macro(vm_t *p_vm, value_t *);
bool is_number(vm_t *p_vm, value_t *);
bool is_bytecode(vm_t *p_vm, value_t *);
bool is_pool(vm_t *p_vm, value_t *);
bool is_lambda(vm_t *p_vm, value_t *);
bool is_null(vm_t *p_vm, value_t *);
bool is_pid(vm_t *p_vm, value_t *);
bool is_process(vm_t *p_vm, value_t *);


value_t * value_create_number(vm_t *p_vm, int p_number);
value_t * value_create_static_string(vm_t *p_vm, char const * const p_symbol);
value_t * value_create_string(vm_t *p_vm, char const * const p_symbol);
value_t * value_create_internal_func(vm_t *p_vm, vm_func_t p_func, int p_nargs, bool p_is_macro);
value_t * value_create_cons(vm_t *p_vm, value_t *p_car, value_t *p_cdr);
value_t * value_create_bytecode(vm_t *p_vm, bytecode_t *p_code, int p_code_count);
value_t * value_create_pool(vm_t *p_vm, value_t *p_literals[], int p_literal_count);
value_t * value_create_lambda(vm_t *p_vm, value_t *p_parameters, value_t *p_bytecode, value_t *p_pool);
value_t * value_create_closure(vm_t *p_vm, value_t *p_env, value_t *p_lambda);
value_t * value_create_environment(vm_t *p_vm, value_t *p_env);
value_t * value_create_binding(vm_t *p_vm, value_t *p_key, value_t *p_binding);
value_t * value_create_process(vm_t *p_vm, value_t *p_parent_vm);
value_t * value_create_vm_state(vm_t *p_vm);

value_t * list(vm_t *p_vm, value_t *p_value);

char const *valuetype_print(int p_val);

#endif /* __VALUE_HELPERS_H_ */
