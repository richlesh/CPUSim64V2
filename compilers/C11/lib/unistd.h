/*
 * unistd.h - the POSIX process functions that CPUSim64 supports (a small
 * subset of POSIX <unistd.h>, not a C standard header).
 *
 * fork() makes a child process with a copy of the whole memory (the
 * simulator's fork); it returns 0 in the child and the child's id in the
 * parent, or -1.  Wait for a child with waitpid() or wait() from
 * <sys/wait.h>.  sysconf(_SC_NPROCESSORS_ONLN) is the number of processor
 * cores of the host, the usual way to size a pool of threads or processes.
 * Everything is implemented in C in lib/unistd.c on the runtime's routines.
 */
#ifndef _CPUSIM64_UNISTD_H
#define _CPUSIM64_UNISTD_H

#include <stddef.h>

#ifndef _CPUSIM64_PID_T
#define _CPUSIM64_PID_T
typedef long pid_t;
#endif

#define _SC_NPROCESSORS_CONF    1
#define _SC_NPROCESSORS_ONLN    2

pid_t fork(void);                       /* 0 in the child, the child's id in the parent, -1 on failure */
pid_t getpid(void);                     /* the id of the current process (or thread) */
long sysconf(int name);                 /* _SC_NPROCESSORS_ONLN / _CONF: the host's cores; -1 for others */
unsigned int sleep(unsigned int seconds);   /* returns 0 */
int usleep(long microseconds);              /* returns 0 */

#endif
