/* a program without _Thread_local objects gets no thread-local code at all,
   not even in thread_create (see tls_none.noasm) */
#include <stdio.h>
#include <stdlib.h>
#include <_Thread.h>

static long total;

static void worker(void *arg)
{
    atomic_add(&total, *(long *)arg);
}

int main(void)
{
    long *v = malloc(2 * sizeof(long));
    long t1, t2;
    v[0] = 3;
    v[1] = 4;
    t1 = thread_create(worker, &v[0]);
    t2 = thread_create(worker, &v[1]);
    thread_join(t1);
    thread_join(t2);
    printf("total=%ld\n", total);
    free(v);
    return 0;
}
