#include "assert.h"
#include "err.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static jmp_buf s_jump_buffers[32];
static int s_jsp = 0;

char g_err[4096];

// need stack of error messages (string)

//but what i did was i made error() and verify() actually take an enum value that was the error code and i returned that in the longjmp()
// then i had a matching const char* errmsg[] array that had formats for those

jmp_buf * push_handler_stack()
{
	return &s_jump_buffers[s_jsp++];
}

void pop_handler_stack()
{
	s_jsp--;
	assert(s_jsp >= 0);
}

void error(const char *format, ...)
{
	va_list argp;
	va_start(argp, format);
	vsprintf(g_err, format, argp);
	va_end(argp);

	assert(s_jsp > 0);
	longjmp(s_jump_buffers[s_jsp - 1], -1);
}

void verify(int cond, const char *format, ...)
{
	va_list argp;

	if (cond == false) {
		va_start(argp, format);
		error(format, argp);
		va_end(argp);
	}
}


