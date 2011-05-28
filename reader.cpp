#include "vm.h"

#include "assert.h"
#include "reader.h"
#include "value_helpers.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//
// 1 the definition of lispobj * and utility functions like car, cdr, cons -- at the C level, not the VM level)
// 2 the VM - stack manipulation, function calls ,etc
// 3 a reader - something that takes a string/stream and returns a lispobj* from it
// 4 compiler - given a lispobj * generate bytecode for it
//

//
// eval(const char *s) {
//	lispobj *p = read(s);
//	lispobj *fn = compile(p);
//	lispobj *result = execute(fn);
//	return results;
// }


stream_t *stream_create(char const *code)
{
    stream_t * stream = (stream_t *)malloc(sizeof(stream_t));
    stream->code = code;
    stream->index = 0;
	stream->length = strlen(code);
    return stream;
}

void stream_destroy(stream_t *p_stream)
{
	free(p_stream);
}


value_t *read_atom(stream_t *p_stream)
{
	char atom[33];
	int index = 0;

	while(p_stream->index < p_stream->length) {
		char val = p_stream->pop();
		if ((val == ' ') || (val == '(') || (val == ')')) {
			atom[index] = 0;
			value_t *ret = value_create_symbol(atom);
			if ((val == '(') || (val == ')')) {
				p_stream->restore();
			}

			return ret;
		}
		atom[index++] = val;

		assert(index < 32);
	}

	atom[index] = 0;
	value_t *ret = value_create_symbol(atom);
	return ret;
}

int reader(vm_t *p_vm, stream_t *p_stream, bool p_in_list)
{
	int list_size = 0;
	while(p_stream->index < p_stream->length) {
		char val = p_stream->pop();
		if (val == ' ') {
			// Clear out white space
			continue;
		} else if (val == ')') {
			assert(p_in_list);
			return list_size;
		} else if (val == '(') {
			
			// Need to read in rest
			int args = reader(p_vm, p_stream, true);

			// Need to change this to vm_list since there could be more than two
			// args in the list, this will also let me handle the list of size
			// 0, otherwise known as nil
			vm_list(p_vm, args);
			list_size++;

		} else {
			// This is an atom (symbol or fixnum)
			p_stream->restore();
			value_t *ret = read_atom(p_stream);
			vm_push(p_vm, ret);
			list_size++;
		}
	}

	return 1;
}

