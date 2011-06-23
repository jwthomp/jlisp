#include "value.h"
#include "value_helpers.h"
#include "reader.h"
#include "compile.h"
#include "env.h"
#include "gc.h"
#include "err.h"
#include "lib.h"
#include "unit-test.h"

//#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

void load_string(vm_t *p_vm, char const *p_code);


void load_string(vm_t *p_vm, char const *p_code)
{
	int vm_bp = p_vm->m_bp;
	int vm_sp = p_vm->m_sp;
	int vm_ev = p_vm->m_ev;
	int vm_ip = p_vm->m_ip;
	value_t *vm_current_env = p_vm->m_current_env[p_vm->m_ev - 1];

	stream_t *strm = stream_create(p_code);

	int i = setjmp(*push_handler_stack());
	if (i == 0) {
		int args = reader(p_vm, strm, false);

//printf("reader found %d forms\n", args);

		int count_down = args;
		while(count_down > 0) {
//printf("SAVE CSP: %lu\n", p_vm->m_csp);

			// get value off stack
			value_t *rd = p_vm->m_stack[p_vm->m_sp - count_down];
//printf("read form: "); value_print(p_vm, rd); printf("\n");

			// Evaluate it
			value_t *res = eval(p_vm, rd);

			// Print a result
printf("res: "); value_print(p_vm, p_vm->m_stack[p_vm->m_sp - count_down]); printf("\n");
			p_vm->m_sp--;

//printf("RESTORE CSP: %lu sp: %ld ev: %lu\n", p_vm->m_csp, p_vm->m_sp, p_vm->m_ev);

			count_down--;
		}
	} else {
		p_vm->m_bp = vm_bp;
		p_vm->m_sp = vm_sp;
		p_vm->m_ev = vm_ev;
		p_vm->m_ip = vm_ip;
		p_vm->m_current_env[p_vm->m_ev - 1] = vm_current_env;

		printf("\t%s\n", g_err);
	}

	verify(p_vm->m_sp == vm_sp &&  p_vm->m_bp == vm_bp, "internal error");

//	printf("env: %p\n", p_vm->m_current_env[p_vm->m_ev - 1]);

	pop_handler_stack();

	stream_destroy(strm);


	printf("B Free memory: %lu\n", p_vm->m_pool_g0->m_size - p_vm->m_pool_g0->m_pos);
	unsigned long mem = gc(p_vm, 1);
	printf("A Free memory: %lu\n", mem);
}


int main(int argc, char *arg[])
{
	vm_t *vm = vm_create(1024);
	lib_init(vm);

#if 0
	unit_test();


#elif 1
	char input[256];
	memset(input, 0, 256);

	printf("\nawesome-lang 0.0.1, copyright (c) 2011 by jeffrey thompson\n");
	printf("> ");
	while(gets(input) != NULL && strcmp(input, "quit")) {
		load_string(vm, input);

		input[0] = 0;

		printf("> ");
	}

#else
	load_string(vm, "(loop (print (eval (read))))");
#endif

//printf("---------- END OF DAYS---------\n");
	vm->m_sp = 0;
	vm->m_ev = 0;
	gc(vm, 1);
//printf("---------- END OF END OF DAYS---------\n");
	vm_destroy(vm);
}
