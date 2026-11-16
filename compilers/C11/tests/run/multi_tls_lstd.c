/* modules + -lstd: the main module defines no thread-local object but creates
   the threads; the objects are in a module that is compiled after it. */
#include <stdio.h>
#include <stdlib.h>
#include <_Thread.h>
#include "../multi/multi_tls.h"

_Thread_local int tls_own = 1;

static void worker(void *arg)
{
    int id = *(int *)arg;
    tls_bump(id);
    tls_table[0] = id;
    printf("worker %d: counter=%d table0=%d label=%s\n", id, tls_counter, tls_table[0], tls_label);
}

int main(void)
{
    int *ids = malloc(2 * sizeof(int));
    long t;
    ids[0] = 1;
    ids[1] = 2;
    t = thread_create(worker, &ids[0]);         /* one after the other: ordered output */
    thread_join(t);
    t = thread_create(worker, &ids[1]);
    thread_join(t);
    printf("main: counter=%d table0=%d\n", tls_counter, tls_table[0]);
    free(ids);
    return 0;
}
