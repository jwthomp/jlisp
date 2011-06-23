#include "vm.h"
#include "minunit.h"
#include "reader.h"
#include "compile.h"
#include "value_helpers.h"
#include "lib.h"

#include <stdio.h>
#include <stdlib.h>

int tests_run = 0;


// Tests are of form static char *test_foo() ...
// Run them with mu_run_test(func_name)
// Use mu_assert(msg, bool) to do assert tests. Returns on error


///////////////////////// VM ////////////////////////////////////
static char const * test_vm_start()
{
	printf("///////////////////////// VM ////////////////////////////////////\n");
	vm_t *vm = vm_create(1024);

	mu_assert("vm is NULL", vm != NULL);
	vm_destroy(vm);
	return 0;
}



//////////////////////// LAMBDA /////////////////////////////////
static char const *test_lambda_basic()
{
	printf("//////////////////////// LAMBDA /////////////////////////////////\n");
	vm_t *vm = vm_create(1024);
	lib_init(vm);
	stream_t *strm = stream_create("(lambda () (+ 1 2))");
	reader(vm, strm, false);
	value_t *rd = vm->m_stack[vm->m_sp - 1];
	eval(vm, rd);
	mu_assert("(lambda () (+ 1 2)) -> Did not have a vm->m_sp of 1\n", vm->m_sp == 1);
	mu_assert("(lambda () (+ 1 2)) -> Did not return a closure\n", is_closure(vm, vm->m_stack[vm->m_sp - 1]));
	vm_destroy(vm);
	return 0;
}

//////////////////////// FUNCALL /////////////////////////////////
static char const *test_funcall_basic()
{
	printf("//////////////////////// FUNCALL /////////////////////////////////\n");
	vm_t *vm = vm_create(1024);
	lib_init(vm);
	stream_t *strm = stream_create("(funcall (lambda () (+ 1 3)))");
	reader(vm, strm, false);
	value_t *rd = vm->m_stack[vm->m_sp - 1];
	eval(vm, rd);
	mu_assert("(funcall (lambda () (+ 1 3))) -> Did not have a vm->m_sp of 1\n", vm->m_sp == 1);
	mu_assert("(funcall (lambda () (+ 1 3))) -> Did not return a fixnum\n", is_fixnum(vm->m_stack[vm->m_sp - 1]));
	mu_assert("(funcall (lambda () (+ 1 3))) -> Did not return 4\n", to_fixnum(vm->m_stack[vm->m_sp - 1]) == 4);
	vm->m_sp = 0;
	
	strm = stream_create("(funcall (lambda () (+ 1 3) (+ 1 1)))");
	reader(vm, strm, false);
	rd = vm->m_stack[vm->m_sp - 1];
	eval(vm, rd);
	mu_assert("(funcall (lambda () (+ 1 3) (+ 1 1))) -> Did not have a vm->m_sp of 1\n", vm->m_sp == 1);
	mu_assert("(funcall (lambda () (+ 1 3) (+ 1 1))) -> Did not return a fixnum\n", is_fixnum(vm->m_stack[vm->m_sp - 1]));
	mu_assert("(funcall (lambda () (+ 1 3) (+ 1 1))) -> Did not return 2\n", to_fixnum(vm->m_stack[vm->m_sp - 1]) == 2);
	vm->m_sp = 0;
	vm_destroy(vm);
	return 0;
}
	
//////////////////////// PROGN /////////////////////////////////
static char const *test_progn_basic()
{
	printf("//////////////////////// PROGN /////////////////////////////////\n");
	vm_t *vm = vm_create(1024);
	lib_init(vm);
	stream_t *strm = stream_create("(progn (+ 1 3))");
	reader(vm, strm, false);
	value_t *rd = vm->m_stack[vm->m_sp - 1];
	eval(vm, rd);
printf("sp: %lu\n", vm->m_sp);
	mu_assert("(progn (+ 1 3)) -> Did not have a vm->m_sp of 1\n", vm->m_sp == 1);
	mu_assert("(progn (+ 1 3)) -> Did not return a fixnum\n", is_fixnum(vm->m_stack[vm->m_sp - 1]));
	mu_assert("(progn (+ 1 3)) -> Did not return 4\n", to_fixnum(vm->m_stack[vm->m_sp - 1]) == 4);
	vm->m_sp = 0;
	
	strm = stream_create("(progn (+ 1 3) (+ 1 1))");
	reader(vm, strm, false);
	rd = vm->m_stack[vm->m_sp - 1];
	eval(vm, rd);
	mu_assert("(progn (+ 1 3) (+ 1 1)) -> Did not have a vm->m_sp of 1\n", vm->m_sp == 1);
	mu_assert("(progn (+ 1 3) (+ 1 1)) -> Did not return a fixnum\n", is_fixnum(vm->m_stack[vm->m_sp - 1]));
	mu_assert("(progn (+ 1 3) (+ 1 1)) -> Did not return 2\n", to_fixnum(vm->m_stack[vm->m_sp - 1]) == 2);
	vm->m_sp = 0;
	
	strm = stream_create("(progn (progn (progn (+ 1 3)) (+ 1 2)))");
	reader(vm, strm, false);
	rd = vm->m_stack[vm->m_sp - 1];
	eval(vm, rd);
	mu_assert("(progn (progn (progn (+ 1 3)) (+ 1 2))) -> Did not have a vm->m_sp of 1\n", vm->m_sp == 1);
	mu_assert("(progn (progn (progn (+ 1 3)) (+ 1 2))) -> Did not return a fixnum\n", is_fixnum(vm->m_stack[vm->m_sp - 1]));
	mu_assert("(progn (progn (progn (+ 1 3)) (+ 1 2))) -> Did not return 3\n", to_fixnum(vm->m_stack[vm->m_sp - 1]) == 3);
	vm->m_sp = 0;
	

	vm_destroy(vm);
	return 0;
}

//////////////////////// ENV /////////////////////////////////


static char const * all_tests()
{
	mu_run_test(test_vm_start);
	mu_run_test(test_lambda_basic);
	mu_run_test(test_funcall_basic);
	mu_run_test(test_progn_basic);
	return 0;
}

void unit_test()
{
	g_debug_display = true;

	char const *result = all_tests();
	if (result != 0) {
		printf("%s\n", result);
	} else {
		printf("ALL TESTS PASS\n");
	}
}
