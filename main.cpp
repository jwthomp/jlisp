#include "value.h"
#include "value_helpers.h"
#include "reader.h"
#include "compile.h"
#include "env.h"
#include "gc.h"
#include "err.h"
#include "lib.h"
#include "unit-test.h"
#include "vm.h"
#include "vm_exec.h"

//#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>


void load_string(vm_t *p_vm, char const *p_code)
{
	value_t *result = p_vm->nil;
	unsigned long vm_bp = p_vm->m_bp;
	unsigned long vm_sp = p_vm->m_sp;
	unsigned long vm_ev = p_vm->m_ev;
	int vm_ip = p_vm->m_ip;
	value_t *vm_current_env = p_vm->m_current_env[p_vm->m_ev - 1];

//printf("Load string: %s\n", p_code);

	stream_t *strm = stream_create(p_code);

	int i = setjmp(*push_handler_stack());
	if (i == 0) {
		int args = reader(p_vm, strm, false);

//printf("reader found %d forms\n", args);
		vm_push(p_vm, p_vm->nil);

		int count_down = args;
		while(count_down > 0) {
//printf("SAVE CSP: %lu\n", p_vm->m_csp);

			// get value off stack
			value_t *rd = p_vm->m_stack[p_vm->m_sp - count_down - 1];

//printf("read form: "); value_print(p_vm, rd); printf("\n");
//printf("cd: %d sp: %lu\n", count_down, p_vm->m_sp);
//vm_print_stack(p_vm);

			// Evaluate it
			eval(p_vm, rd);

			// Print a result
//printf("res: "); value_print(p_vm, p_vm->m_stack[p_vm->m_sp - count_down]); printf("\n");
result = p_vm->m_stack[p_vm->m_sp - count_down];

//printf("RESTORE CSP: %lu sp: %ld ev: %lu\n", p_vm->m_csp, p_vm->m_sp, p_vm->m_ev);

			count_down--;
		}

		p_vm->m_sp -= args + 1;
	} else {
vm_print_stack(p_vm);
		p_vm->m_bp = vm_bp;
		p_vm->m_sp = vm_sp;
		p_vm->m_ev = vm_ev;
		p_vm->m_ip = vm_ip;
		p_vm->m_current_env[p_vm->m_ev - 1] = vm_current_env;

		printf("\t%s\n", g_err);
	}

	verify(p_vm->m_sp == vm_sp &&  p_vm->m_bp == vm_bp, "internal error");

	p_vm->m_stack[p_vm->m_sp++] = result;

//	printf("env: %p\n", p_vm->m_current_env[p_vm->m_ev - 1]);

	pop_handler_stack();

	stream_destroy(strm);

//vm_print_env(p_vm);
//vm_print_stack(p_vm);
//printf("sp: %lu ev: %lu\n", p_vm->m_sp, p_vm->m_ev);


	gc(p_vm, 1);

//printf("---------------- gc ---------------\n");
//vm_print_env(p_vm);
//	printf("Memory: Free: %lu Allocated: %lu\n", mem_free(p_vm), mem_allocated(p_vm, true));
}


int main(int argc, char *arg[])
{
	vm_t *vm = vm_create(1000000, NULL);
	lib_init(vm);

	value_t *vm_val = value_create(vm, VT_PROCESS, sizeof(vm_t *), false);
	*(vm_t **)vm_val->m_data = vm;
	g_kernel_proc = vm_val;


	if (argc > 1) {
		unit_test();
	} else {

#if 0
	stream_t *strm = stream_create("(loop (print (eval (read))))");
	//stream_t *strm = stream_create("(+ 1 2)");
	//stream_t *strm = stream_create("(+ 1 (* 2 3))");
	//stream_t *strm = stream_create("(+ (* 2 3) 1) (+ 1 1)");
	//stream_t *strm = stream_create("(call (lambda () (+ 1 2))) (+ 4 5)");
	//stream_t *strm = stream_create("(call (lambda (a b) (+ a b)) 3 4)");
	//stream_t *strm = stream_create("(defun x (a b) (+ a b)) (x 5 6)");
	//stream_t *strm = stream_create("(load \"stf.awe\")");
	//stream_t *strm = stream_create("(load \"test\")");
	//stream_t *strm = stream_create("(load \"qsort\") (cadr '(1 2))");
	//stream_t *strm = stream_create("(load \"qsort\") (pivot (seq 10))");
	int args = reader(vm, strm, false);
printf("args: %d\n", args);
	int start_sp = vm->m_sp;
	int i = 1;
	while (i <= args) {
		value_t *form = vm->m_stack[start_sp - i];
		eval(vm, form);
		i++;

printf("===========================================\n");
printf("===========================================\n");
	}

	vm->m_sp -= args;

	vm_exec(vm);
printf("res: "); value_print(vm, vm->m_stack[vm->m_sp - 1]); printf("\n");

	vm_print_stack(vm);

#elif 0
	load_string(vm, "(load \"qsort\") (pivot (seq 1000))");
#elif 0
	value_t *vm1_val = value_create_process(vm, vm_val);
	vm_t *vm1 = *(vm_t **)vm1_val->m_data;
	int flip = 0;
	char input[256];
	memset(input, 0, 256);

	printf("\nawesome-lang 0.0.1, copyright (c) 2011 by jeffrey thompson\n");
	printf("%d> ", flip);
	while(gets(input) != NULL && strcmp(input, "quit")) {
		if (!flip) {
			load_string(vm, input);
			flip = 1;
		} else {
			load_string(vm1, input);
			flip = 0;
		}

		input[0] = 0;
		printf("%d> ", flip);
	}

#elif 1
	int flip = 0;
	char input[256];
	memset(input, 0, 256);

	printf("\nawesome-lang 0.0.1, copyright (c) 2011 by jeffrey thompson\n");
	printf("%d> ", flip);
	while(gets(input) != NULL && strcmp(input, "quit")) {
		stream_t *strm = stream_create(input);
		int args = reader(vm, strm, false);
		int start_sp = vm->m_sp;
		int i = 1;
		while (i <= args) {
			value_t *form = vm->m_stack[start_sp - i];
			eval(vm, form);
			i++;

		}

		vm->m_sp -= args;

		vm_exec(vm, 0);
		printf("res: "); value_print(vm, vm->m_stack[vm->m_sp - 1]); printf("\n");
		flip++;
		vm_print_stack(vm);
		vm->m_sp--;
		printf("%d> ", flip);
	}

#else
	load_string(vm, "(loop (print (eval (read))))");
#endif
}

//printf("---------- END OF DAYS---------\n");
	vm->m_sp = 0;
	vm->m_ev = 0;
	gc(vm, 1);
//printf("---------- END OF END OF DAYS---------\n");
	vm_destroy(vm);
}
