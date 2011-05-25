#include "assert.h"

void assert(bool x) {
    if (x == false) {
        int a = *(int *)0;
    }
}
