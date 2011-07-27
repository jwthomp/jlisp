#ifndef __COMPILE_H_
#define __COMPILE_H_

#include "vm.h"
#include "value.h"

value_t * eval(vm_t *p_vm, value_t *p_form, bool p_pop);
value_t *compile(vm_t *p_vm, value_t *p_parameters, value_t *p_body);
value_t * make_closure(vm_t *p_vm, value_t *p_lambda);

#endif /* __COMPILE_H_ */
