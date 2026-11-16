/*
 * sys/wait.c - the C part of sys/wait.h, on the runtime routines of <_Thread.h>.
 */
#include <sys/wait.h>

void wait_pid(long pid);                /* [rt] */
void wait_all(void);                    /* [rt] */

pid_t waitpid(pid_t __wt_pid, int *__wt_st, int __wt_options) {
    (void)__wt_options;
    if (__wt_pid > 0) wait_pid(__wt_pid);
    else wait_all();
    if (__wt_st) *__wt_st = 0;
    return __wt_pid;
}

pid_t wait(int *__wt_st) {
    wait_all();
    if (__wt_st) *__wt_st = 0;
    return -1;
}
