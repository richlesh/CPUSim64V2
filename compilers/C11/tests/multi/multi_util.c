#include <stdio.h>
#include "multi.h"

int calls;
static int helper(void) { return 200; }

int add3(int x) { ++calls; counter += 3; return x + 3; }
int util_helper(void) { return helper(); }
void apply(int (*fn)(int), int x) { ++calls; printf("apply: %d\n", fn(x)); }
