#ifndef __VALUE_HELPERS_H_
#define __VALUE_HELPERS_H_

#include "vm.h"

value_t * value_create_number(int p_number);
value_t * value_create_symbol(char const * const p_symbol);
value_t * value_create_internal_func(vm_func_t p_func);


#endif /* __VALUE_HELPERS_H_ */
