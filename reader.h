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

    char pop(bool p_is_string) {
		assert(index < length);
		if (p_is_string == false) {
	        return toupper(code[index++]);
		} else {
	        return code[index++];
		}
    }

    char pop() {
		return pop(false);
    }

    void restore() {
        index--;
    }
} stream_t;

stream_t *stream_create(char const *code);
void stream_destroy(stream_t *);
int reader(vm_t *p_vm, stream_t *p_stream, bool p_in_list);
value_t *read(vm_t *p_vm);

#endif /* __READER_H_ */

