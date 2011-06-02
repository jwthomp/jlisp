#ifndef __ERR_H_
#define __ERR_H_

#include <setjmp.h>

extern char g_err[4096];

jmp_buf *push_handler_stack();
void pop_handler_stack();

void verify(int cond, const char *format, ...);

#endif /* __ERR_H_ */
