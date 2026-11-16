// <chrono> basics, this_thread::sleep_for / sleep_until / yield, and POSIX fork / waitpid
#include <chrono>
#include <thread>
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>

using namespace std::chrono;

int main()
{
    // durations
    milliseconds ms(1500);
    microseconds us = ms;
    seconds s = duration_cast<seconds>(ms);
    nanoseconds ns = duration_cast<nanoseconds>(ms);
    std::printf("%lld ms = %lld us = %lld s (truncated) = %lld ns\n", (long long)ms.count(), (long long)us.count(),
                (long long)s.count(), (long long)ns.count());
    std::printf("sum %lld ms\n", (long long)(milliseconds(250) + seconds(2)).count());
    std::printf("difference %lld us\n", (long long)(milliseconds(3) - microseconds(500)).count());
    std::printf("equal: %d, less: %d\n", (int)(seconds(1) == milliseconds(1000)), (int)(milliseconds(999) < seconds(1)));
    duration<double> half(0.5);
    std::printf("double seconds %g -> %lld ms\n", half.count(), (long long)duration_cast<milliseconds>(half).count());
    minutes m(2);
    std::printf("2 minutes = %lld s, %d hours\n", (long long)duration_cast<seconds>(m).count(), (int)hours(3).count());

    // clocks
    steady_clock::time_point t0 = steady_clock::now();
    std::this_thread::sleep_for(milliseconds(60));
    steady_clock::time_point t1 = steady_clock::now();
    long long waited = duration_cast<milliseconds>(t1 - t0).count();
    std::printf("slept at least 60 ms: %d, less than 5 s: %d\n", (int)(waited >= 60), (int)(waited < 5000));
    std::printf("time_point ordering: %d\n", (int)(t1 > t0));

    steady_clock::time_point deadline = steady_clock::now() + milliseconds(40);
    std::this_thread::sleep_until(deadline);
    std::printf("sleep_until reached the deadline: %d\n", (int)(steady_clock::now() >= deadline));

    std::this_thread::sleep_for(microseconds(1));
    std::this_thread::sleep_for(seconds(0));
    std::this_thread::yield();

    system_clock::time_point now = system_clock::now();
    std::printf("system_clock after 2020: %d\n", (int)(system_clock::to_time_t(now) > 1577836800));

    // POSIX: sysconf, getpid, fork and waitpid
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    std::printf("cores > 0: %d\n", (int)(cores > 0));
    std::printf("getpid > 0: %d\n", (int)(getpid() > 0));
    usleep(1000);
    std::fflush(stdout);                    // the child must not repeat buffered output
    pid_t child = fork();
    if (child == 0) return 0;               // the child ends at once
    int status = -1;
    pid_t w = waitpid(child, &status, 0);
    std::printf("waitpid returned the child: %d, exited: %d\n", (int)(w == child), (int)WIFEXITED(status));
    return 0;
}
