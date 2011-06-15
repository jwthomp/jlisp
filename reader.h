#ifndef __READER_H_
#define __READER_H_

#include "value.h"
#include "vm.h"
#include "assert.h"

#include <ctype.h>

typedef struct stream_s {
    char const * code;
    unsigned long index;
	unsigned long length;

    char pop() {
		assert(index < length);
        return toupper(code[index++]);
    }

    void restore() {
        index--;
    }
} stream_t;

stream_t *stream_create(char const *code);
void stream_destroy(stream_t *);
int reader(vm_t *p_vm, stream_t *p_stream, bool p_in_list);

#endif /* __READER_H_ */

