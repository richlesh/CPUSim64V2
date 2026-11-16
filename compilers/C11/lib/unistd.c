/*
 * unistd.c - the C part of unistd.h, on the runtime routines of <_Thread.h>
 * (declared here, so <unistd.h> does not add their names to the program).
 */
#include <unistd.h>

long get_pid(void);                     /* [rt] */
long get_num_cores(void);               /* [rt] */
void sleep_ms(long ms);                 /* [rt] */

pid_t getpid(void) {
    return (pid_t)get_pid();
}

long sysconf(int __un_name) {
    if (__un_name == _SC_NPROCESSORS_ONLN || __un_name == _SC_NPROCESSORS_CONF)
        return get_num_cores();
    return -1;
}

unsigned int sleep(unsigned int __un_s) {
    sleep_ms((long)__un_s * 1000);
    return 0;
}

int usleep(long __un_us) {
    sleep_ms(__un_us / 1000);
    return 0;
}
