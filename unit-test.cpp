#include "vm.h"
#include "minunit.h"

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
	char const *result = all_tests();
	if (result != 0) {
		printf("%s\n", result);
	} else {
		printf("ALL TESTS PASS\n");
	}
}
