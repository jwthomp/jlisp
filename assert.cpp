#include "assert.h"
#include "err.h"

void assert(bool x) {
	verify(x, "An error!");
}
