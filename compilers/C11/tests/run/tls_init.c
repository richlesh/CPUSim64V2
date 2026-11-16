/* _Thread_local initializers: every new thread starts with the initial values,
   whatever other threads (and main) have done to their copies. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <_Thread.h>

int shared_value = 42;
int shared_array[3] = { 7, 8, 9 };

struct point { int x, y; double w; };
struct holder { const char *name; int *ptr; int n; };

_Thread_local int scalar = 11;
_Thread_local int array[5] = { 1, 2, 3 };               /* the rest is zero */
_Thread_local struct point pt = { 3, 4, 2.5 };
_Thread_local int *pointer = &shared_value;             /* address of a global */
_Thread_local struct holder holder = { "holder", shared_array + 1, 99 };
_Thread_local const char *text = "hello";
_Thread_local char chars[8] = "abc";
_Thread_local long zero_array[200];                     /* a big one without an initializer */
_Thread_local unsigned char small = 250;
_Thread_local _Bool flag = 1;

static void show(const char *who)
{
    long z = 0;
    int i;
    for (i = 0; i < 200; i++) z += zero_array[i];
    printf("%s: scalar=%d array=%d,%d,%d,%d,%d pt=%d,%d,%g *pointer=%d holder=%s,%d,%d text=%s chars=%s small=%u flag=%d zeros=%ld\n",
           who, scalar, array[0], array[1], array[2], array[3], array[4], pt.x, pt.y, pt.w,
           *pointer, holder.name, *holder.ptr, holder.n, text, chars, small, flag, z);
}

static void scribble(void)
{
    int i;
    scalar = -1;
    for (i = 0; i < 5; i++) array[i] = -i;
    pt.x = pt.y = 0;
    pt.w = 0;
    pointer = shared_array;
    holder.n = 0;
    holder.ptr = &shared_value;
    text = "changed";
    strcpy(chars, "xyz");
    small = 1;
    flag = 0;
    for (i = 0; i < 200; i++) zero_array[i] = i;
}

static void worker(void *arg)
{
    show((const char *)arg);
    scribble();
    show((const char *)arg);
}

int main(void)
{
    long t1, t2, t3;
    show("main start");
    scribble();                     /* main changes its copies before creating the threads */
    show("main changed");
    t1 = thread_create(worker, "thread A");
    thread_join(t1);                /* one at a time: the output order is fixed */
    t2 = thread_create(worker, "thread B");
    thread_join(t2);
    t3 = thread_create(worker, "thread C");
    thread_join(t3);
    show("main end");
    return 0;
}
