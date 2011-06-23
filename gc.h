#ifndef __GC_H_
#define __GC_H_

#include "value.h"
#include "vm.h"

#include <sys/types.h>


value_t *gc_alloc(vm_t *p_vm, size_t p_size, alloc_type p_alloc_type);

unsigned long gc(vm_t *p_vm, int p_age);

void gc_shutdown(vm_t *p_vm);

unsigned long mem_allocated(vm_t *p_vm, bool p_count_mark_sweep);
unsigned long mem_free(vm_t *p_vm);


#endif /* __GC_H_ */
