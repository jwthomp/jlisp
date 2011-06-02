#ifndef __GC_H_
#define __GC_H_

#include "value.h"
#include "vm.h"

#include <sys/types.h>

value_t *gc_alloc(vm_t *p_vm, size_t p_size, bool p_is_static);

void gc(vm_t *p_vm);
void mark(vm_t *p_vm);
void sweep(vm_t *p_vm);

void gc_shutdown(vm_t *p_vm);

#endif /* __GC_H_ */
