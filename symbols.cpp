#include "symbols.h"

#include "vm_exec.h"

value_t * value_create_symbol(vm_t *p_vm, char const * const p_symbol)
{

    // See if it already exists..
    value_t *sym = g_symbol_table;
    while(sym != voidobj) {
        if (is_symbol_name(p_symbol, sym) == true) {
            return sym;
        }
        sym = sym->m_next_symbol;
    }


    value_t *string = value_create_static_string(p_vm, p_symbol);
    sym =  value_create(p_vm, VT_SYMBOL, sizeof(value_t *) * 3, true);
    sym->m_cons[0] = string;
    sym->m_cons[1] = voidobj;
    sym->m_cons[2] = voidobj;
    sym->m_next_symbol = g_symbol_table;
    g_symbol_table = sym;

    return sym;
}

