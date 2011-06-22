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
	vm_t *vm = vm_create(1024);

	mu_assert("vm is NULL", vm != NULL);
	return 0;
}



//////////////////////// GC /////////////////////////////////
static char const *test_gc_basic()
{
	vm_t *vm = vm_create(1024);
	lib_init(vm);
	stream_t *strm = stream_create("(lambda () (+ 1 2))");
	reader(vm, strm, false);
	value_t *rd = vm->m_stack[vm->m_sp - 1];
	eval(vm, rd);
	printf("res: "); value_print(vm, vm->m_stack[vm->m_sp - 1]); printf("\n");
	return 0;
}

//////////////////////// ENV /////////////////////////////////


static char const * all_tests()
{
	mu_run_test(test_vm_start);
	mu_run_test(test_gc_basic);
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
