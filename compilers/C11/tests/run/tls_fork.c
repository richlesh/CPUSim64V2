/* fork copies the memory, including the thread-local area: the child starts
   with the values of the parent, and then the two are independent. */
#include <stdio.h>
#include <stdlib.h>
#include <_Thread.h>

_Thread_local int value = 10;
_Thread_local int array[4] = { 1, 2, 3, 4 };

int main(void)
{
    long pid;
    long *shared = alloc_shared(3);
    shared[0] = shared[1] = shared[2] = 0;
    value = 20;
    array[2] = 30;
    pid = fork();
    if (pid == 0) {
        shared[0] = value;                  /* 20: inherited */
        shared[1] = array[2];               /* 30 */
        value = 99;
        array[0] = 99;
        shared[2] = value + array[0];       /* 198 */
        exit(0);
    }
    wait_pid(pid);
    printf("child saw %ld %ld, changed to %ld\n", shared[0], shared[1], shared[2]);
    printf("parent: value=%d array=%d,%d,%d,%d\n", value, array[0], array[1], array[2], array[3]);
    return 0;
}
