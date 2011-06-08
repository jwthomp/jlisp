#include "value.h"
#include "value_helpers.h"
#include "reader.h"
#include "compile.h"
#include "env.h"
#include "gc.h"
#include "err.h"
#include "lib.h"

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
			// get value off stack
			value_t *rd = p_vm->m_stack[p_vm->m_sp - count_down];
//printf("read form: "); value_print(rd); printf("\n");

			// Evaluate it
			value_t *res = eval(p_vm, rd);

			// Print a result
//printf("res: "); value_print(p_vm->m_stack[p_vm->m_sp - count_down]); printf("\n");
//value_print(p_vm->m_stack[p_vm->m_sp - count_down]); printf("\n");
value_print(res); printf("\n");
			p_vm->m_sp--;

			count_down--;
		}

		p_vm->m_sp -= args;

	} else {
		p_vm->m_bp = vm_bp;
		p_vm->m_sp = vm_sp;
		p_vm->m_ev = vm_ev;
		p_vm->m_ip = vm_ip;
		p_vm->m_current_env[p_vm->m_ev - 1] = vm_current_env;

		printf("Error: %s\n", g_err);
	}
	pop_handler_stack();

	stream_destroy(strm);

	verify(p_vm->m_sp == vm_sp &&  p_vm->m_bp == vm_bp, "internal error");

	gc(p_vm);
}


int main(int argc, char *arg[])
{
	vm_t *vm = vm_create(1024);

	lib_init(vm);

	char input[256];
	memset(input, 0, 256);

	printf("> ");
	while(gets(input) != NULL && strcmp(input, "quit")) {
		load_string(vm, input);

		input[0] = 0;

		printf("> ");
	}

	vm->m_sp = 0;
	vm->m_ev = 0;
	gc(vm);
	vm_destroy(vm);
}
