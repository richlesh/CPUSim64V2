// <unistd.h>: the POSIX process functions of the C library's unistd.h (fork, getpid,
// sysconf, sleep, usleep) as the C++ translator sees them.  Wait for a child with
// waitpid() from <sys/wait.h>.
#ifndef _CPPC_UNISTD_H
#define _CPPC_UNISTD_H
#pragma cppc c_include <unistd.h>
#include <stddef.h>
#define _SC_NPROCESSORS_CONF    1
#define _SC_NPROCESSORS_ONLN    2
extern "C" {
typedef long pid_t;
pid_t fork(void);                       // 0 in the child, the child's id in the parent, -1 on failure
pid_t getpid(void);                     // the id of the current process (or thread)
long sysconf(int name);                 // _SC_NPROCESSORS_ONLN / _CONF: the host's cores; -1 for others
unsigned int sleep(unsigned int seconds);
int usleep(long microseconds);
}
#endif
