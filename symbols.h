#ifndef __SYMBOLS_H_
#define __SYMBOLS_H_

#include "value.h"
#include "value_helpers.h"
#include "vm.h"

inline bool is_symbol_function_dynamic(vm_t *p_vm, value_t *p_val)
{
    if (p_val->m_cons[2] != p_vm->voidobj) {
        return true;
    }

    return false;
}

inline bool is_symbol_dynamic(vm_t *p_vm, value_t *p_val)
{
    if (p_val->m_cons[1] != p_vm->voidobj) {
        return true;
    }

    return false;
}


inline bool is_symbol(vm_t *p_vm, value_t *p_val)
{
    if(p_val == p_vm->nil || is_fixnum(p_val)) {
        return false;
    }

    return p_val->m_type == VT_SYMBOL;
}

value_t * value_create_symbol(vm_t *p_vm, char const * const p_symbol);

#endif /* __SYMBOLS_H_ */