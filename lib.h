#ifndef __LIB_H
#define __LIB_H_

void lib_init(vm_t *p_vm);

extern void load_string(vm_t *p_vm, char const *p_code);

#endif /* __LIB_H_ */
