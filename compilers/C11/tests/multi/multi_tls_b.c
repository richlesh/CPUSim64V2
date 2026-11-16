/* module without thread-local objects that creates the threads: they must
   still get the initial values of the objects of the other modules */
#include <_Thread.h>
#include <stdlib.h>
#include "multi_tls.h"

void tls_run(void (*fn)(void *), void *jobs, int n, size_t size)
{
    long *tids = malloc(n * sizeof(long));
    int i;
    for (i = 0; i < n; i++) tids[i] = thread_create(fn, (char *)jobs + i * size);
    for (i = 0; i < n; i++) thread_join(tids[i]);
    free(tids);
}
