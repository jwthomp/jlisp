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


int main(int argc, char *arg[])
{

	vm_exec_init();
	value_t *vm_val = value_create_process(NULL, NULL);
	vm_t *vm = *(vm_t **)vm_val->m_data;
	lib_init(vm);

	unsigned long vm_bp = vm->m_bp;
	unsigned long vm_sp = vm->m_sp;
	unsigned long vm_ev = vm->m_ev;
	int vm_ip = vm->m_ip;
	value_t *vm_current_env = vm->m_current_env[vm->m_ev - 1];


	int i = setjmp(*push_handler_stack());
	if (i == 0) {
		char input[256];
		memset(input, 0, 256);
//		printf("> ");
		//stream_t *strm = stream_create("(eval '(+ 1 2))");
		stream_t *strm = stream_create("(loop (print (eval (read))))");
		int args = reader(vm, strm, false);
		int start_sp = vm->m_sp;
		int i = 1;
		while (i <= args) {
			value_t *form = vm->m_stack[start_sp - i];
			eval(vm, form);
			i++;
		}

		vm_exec_add_vm(vm_val);

//		printf("\nres: "); value_print(vm, vm->m_stack[vm->m_sp - 1]); printf("\n");
//		vm->m_sp--;

		// Put the process back in the list 
	//	vm_exec_add_vm(vm_val);

		vm_exec_wait();
	} else {
vm_print_stack(vm);
		vm->m_bp = vm_bp;
		vm->m_sp = vm_sp;
		vm->m_ev = vm_ev;
		vm->m_ip = vm_ip;
		vm->m_current_env[vm->m_ev - 1] = vm_current_env;

		printf("\t%s\n", g_err);
	}

	vm->m_sp = 0;
	vm->m_ev = 0;
	gc(vm, 1);
	vm_destroy(vm);

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
#endif

}
