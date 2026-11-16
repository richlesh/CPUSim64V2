// <sys/wait.h>: waitpid() and wait() of the C library's sys/wait.h for C++ programs.
// The simulator does not report a child's exit status: *stat_loc is set to 0.
#ifndef _CPPC_SYS_WAIT_H
#define _CPPC_SYS_WAIT_H
#pragma cppc c_include <sys/wait.h>
#define WIFEXITED(s)    (((s) & 0x7f) == 0)
#define WEXITSTATUS(s)  (((s) >> 8) & 0xff)
extern "C" {
typedef long pid_t;
pid_t waitpid(pid_t pid, int *stat_loc, int options);   // waits for child pid (pid > 0) or all (-1); returns pid
pid_t wait(int *stat_loc);                              // waits for all children; returns -1 when there were none
}
#endif
