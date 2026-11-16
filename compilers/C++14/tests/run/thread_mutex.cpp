// std::mutex, lock_guard, unique_lock, recursive_mutex, timed_mutex, std::lock, call_once
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdio>

static std::mutex g_mutex;
static long g_counter = 0;
static std::recursive_mutex g_rmutex;
static int g_depth_total = 0;
static std::once_flag g_once;
static int g_once_runs = 0;

void bump(int n)
{
    for (int i = 0; i < n; i++) {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_counter++;
    }
}

void bump_unique(int n)
{
    for (int i = 0; i < n; i++) {
        std::unique_lock<std::mutex> lock(g_mutex);
        g_counter++;
        lock.unlock();
        lock.lock();
        g_counter++;
    }
}

void recurse(int depth)
{
    std::lock_guard<std::recursive_mutex> lock(g_rmutex);
    g_depth_total++;
    if (depth > 0) recurse(depth - 1);
}

void init_once() { g_once_runs++; }
void init_once_arg(int *p, int v) { *p += v; }

int main()
{
    // lock_guard: exact totals from several threads
    std::vector<std::thread> pool;
    for (int i = 0; i < 4; i++) pool.push_back(std::thread(bump, 500));
    for (size_t i = 0; i < pool.size(); i++) pool[i].join();
    std::printf("lock_guard counter = %ld\n", g_counter);

    // unique_lock with unlock / lock
    g_counter = 0;
    std::vector<std::thread> pool2;
    for (int i = 0; i < 3; i++) pool2.push_back(std::thread(bump_unique, 300));
    for (size_t i = 0; i < pool2.size(); i++) pool2[i].join();
    std::printf("unique_lock counter = %ld\n", g_counter);

    // unique_lock modes
    {
        std::unique_lock<std::mutex> d(g_mutex, std::defer_lock);
        std::printf("defer_lock owns: %d\n", (int)d.owns_lock());
        d.lock();
        std::printf("after lock owns: %d\n", (int)d.owns_lock());
        d.unlock();
        std::printf("after unlock owns: %d\n", (int)d.owns_lock());
        bool ok = d.try_lock();
        std::printf("try_lock: %d\n", (int)ok);
        std::mutex *released = d.release();
        std::printf("released owns: %d, mutex kept: %d\n", (int)d.owns_lock(), (int)(released == &g_mutex));
        released->unlock();
        std::unique_lock<std::mutex> a(g_mutex, std::defer_lock);
        a.lock();
        std::unique_lock<std::mutex> moved(std::move(a));
        std::printf("moved owns: %d, source owns: %d\n", (int)moved.owns_lock(), (int)a.owns_lock());
    }

    // a mutex held by another thread: try_lock fails there
    {
        g_mutex.lock();
        bool *got = new bool(true);
        std::thread t([got]() {
            *got = g_mutex.try_lock();
            if (*got) g_mutex.unlock();
        });
        t.join();
        g_mutex.unlock();
        std::printf("try_lock from another thread while held: %d\n", (int)*got);
        std::thread t2([got]() {
            *got = g_mutex.try_lock();
            if (*got) g_mutex.unlock();
        });
        t2.join();
        std::printf("try_lock from another thread when free: %d\n", (int)*got);
        delete got;
    }

    // adopt_lock
    g_mutex.lock();
    {
        std::lock_guard<std::mutex> adopt(g_mutex, std::adopt_lock);
    }
    std::printf("adopt_lock released: %d\n", (int)g_mutex.try_lock());
    g_mutex.unlock();

    // recursive_mutex
    std::thread r1(recurse, 5);
    std::thread r2(recurse, 3);
    r1.join();
    r2.join();
    std::printf("recursive depth total = %d\n", g_depth_total);

    // timed_mutex
    {
        std::timed_mutex *p = new std::timed_mutex;     // on the heap: the threads share it
        std::timed_mutex &tm = *p;
        bool first = tm.try_lock_for(std::chrono::milliseconds(10));
        std::printf("timed_mutex try_lock_for free: %d\n", (int)first);
        tm.unlock();
        tm.lock();
        bool *got = new bool(true);
        std::thread t([p, got]() {
            *got = p->try_lock_for(std::chrono::milliseconds(30));
            if (*got) p->unlock();
        });
        t.join();
        tm.unlock();
        std::printf("timed_mutex try_lock_for while held: %d\n", (int)*got);
        delete got;
        delete p;
    }

    // std::lock of two mutexes, taken in opposite orders by two threads
    {
        std::mutex *ma = new std::mutex;
        std::mutex *mb = new std::mutex;
        int *shared = new int(0);
        std::thread t1([ma, mb, shared]() {
            for (int i = 0; i < 200; i++) {
                std::unique_lock<std::mutex> l1(*ma, std::defer_lock);
                std::unique_lock<std::mutex> l2(*mb, std::defer_lock);
                std::lock(l1, l2);
                (*shared)++;
            }
        });
        std::thread t2([ma, mb, shared]() {
            for (int i = 0; i < 200; i++) {
                std::unique_lock<std::mutex> l1(*mb, std::defer_lock);
                std::unique_lock<std::mutex> l2(*ma, std::defer_lock);
                std::lock(l1, l2);
                (*shared)++;
            }
        });
        t1.join();
        t2.join();
        std::printf("std::lock total = %d\n", *shared);
        delete ma;
        delete mb;
        delete shared;
    }

    // call_once from several threads
    {
        std::vector<std::thread> ts;
        for (int i = 0; i < 4; i++) ts.push_back(std::thread([]() { std::call_once(g_once, init_once); }));
        for (size_t i = 0; i < ts.size(); i++) ts[i].join();
        std::printf("call_once ran %d time(s)\n", g_once_runs);
        std::once_flag f2;
        int v = 0;
        std::call_once(f2, init_once_arg, &v, 5);
        std::call_once(f2, init_once_arg, &v, 5);
        std::printf("call_once with arguments: %d\n", v);
    }
    return 0;
}
