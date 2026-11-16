/* <unistd.h> and <sys/wait.h>: fork, waitpid, getpid, sysconf */
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

long shared_counter;            /* not shared: the child has a copy */

int main(void)
{
    pid_t pid;
    int st = -1;
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    printf("cores > 0: %d, same as _CONF: %d, unknown: %ld\n",
           cores > 0, cores == sysconf(_SC_NPROCESSORS_CONF), sysconf(999));
    printf("getpid > 0: %d\n", getpid() > 0);
    pid = fork();
    if (pid == 0) {
        shared_counter = 42;
        printf("child\n");
        return 0;
    }
    {
        pid_t w = waitpid(pid, &st, 0);     /* before printing: no interleaving */
        printf("fork > 0: %d\n", pid > 0);
        printf("waitpid: %d, status exited: %d code %d\n", w == pid, WIFEXITED(st), WEXITSTATUS(st));
    }
    printf("parent copy unchanged: %ld\n", shared_counter);
    printf("sleep: %u usleep: %d\n", sleep(0), usleep(1000));
    return 0;
}
