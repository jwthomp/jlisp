#include "assert.h"
#include "err.h"

void assert(bool x) {
	if (x == false) {
		int a = *(int *)0;
		a++;
	}
}
