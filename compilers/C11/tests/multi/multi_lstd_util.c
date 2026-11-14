#include <stdio.h>
#include <stdlib.h>

static void bye(void) { printf("atexit handler from multi_lstd_util.c\n"); }

void setup(void) { atexit(bye); srand(5); }
int next(void) { return rand(); }
