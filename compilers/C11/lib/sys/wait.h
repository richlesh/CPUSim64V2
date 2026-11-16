/*
 * sys/wait.h - waiting for child processes made with fork() (a small subset
 * of POSIX <sys/wait.h>, not a C standard header).
 *
 * The simulator does not report a child's exit status, so *stat_loc is set
 * to 0 (WIFEXITED true, WEXITSTATUS 0).  options must be 0.
 */
#ifndef _CPUSIM64_SYS_WAIT_H
#define _CPUSIM64_SYS_WAIT_H

#ifndef _CPUSIM64_PID_T
#define _CPUSIM64_PID_T
typedef long pid_t;
#endif

#define WIFEXITED(s)    (((s) & 0x7f) == 0)
#define WEXITSTATUS(s)  (((s) >> 8) & 0xff)

pid_t waitpid(pid_t pid, int *stat_loc, int options);  /* waits for child pid (pid > 0) or all (-1); returns pid */
pid_t wait(int *stat_loc);                              /* waits for all children; returns -1 when there were none */

#endif
