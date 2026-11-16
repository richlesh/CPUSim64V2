// std::condition_variable: producer/consumer, notify_all, wait_for, wait_until, predicates
#include <condition_variable>
#include <mutex>
#include <thread>
#include <chrono>
#include <queue>
#include <vector>
#include <cstdio>

static std::mutex g_m;
static std::condition_variable g_cv;
static std::queue<int> g_queue;
static bool g_done = false;
static long g_consumed_sum = 0;
static int g_consumed_count = 0;

static bool g_go = false;
static int g_released = 0;

static std::mutex g_pm;
static std::condition_variable g_pcv;
static int g_ping = 0;

void producer(int count)
{
    for (int i = 1; i <= count; i++) {
        {
            std::lock_guard<std::mutex> lock(g_m);
            g_queue.push(i);
        }
        g_cv.notify_one();
    }
    {
        std::lock_guard<std::mutex> lock(g_m);
        g_done = true;
    }
    g_cv.notify_all();
}

void consumer()
{
    for (;;) {
        std::unique_lock<std::mutex> lock(g_m);
        g_cv.wait(lock, []() { return !g_queue.empty() || g_done; });
        while (!g_queue.empty()) {
            g_consumed_sum += g_queue.front();
            g_consumed_count++;
            g_queue.pop();
        }
        if (g_done) return;
    }
}

void gate_waiter()
{
    std::unique_lock<std::mutex> lock(g_m);
    while (!g_go) g_cv.wait(lock);              // the classic loop form
    g_released++;
}

int main()
{
    // one producer, two consumers
    std::thread c1(consumer);
    std::thread c2(consumer);
    std::thread p(producer, 200);
    p.join();
    c1.join();
    c2.join();
    std::printf("consumed %d items, sum %ld\n", g_consumed_count, g_consumed_sum);

    // notify_all releases every waiter
    std::vector<std::thread> waiters;
    for (int i = 0; i < 4; i++) waiters.push_back(std::thread(gate_waiter));
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    {
        std::lock_guard<std::mutex> lock(g_m);
        g_go = true;
    }
    g_cv.notify_all();
    for (size_t i = 0; i < waiters.size(); i++) waiters[i].join();
    std::printf("released %d waiters\n", g_released);

    // ping-pong between two threads with one condition variable
    std::thread ponger([]() {
        for (int i = 0; i < 20; i++) {
            std::unique_lock<std::mutex> lock(g_pm);
            g_pcv.wait(lock, []() { return g_ping % 2 == 1; });
            g_ping++;
            lock.unlock();
            g_pcv.notify_all();
        }
    });
    for (int i = 0; i < 20; i++) {
        std::unique_lock<std::mutex> lock(g_pm);
        g_pcv.wait(lock, []() { return g_ping % 2 == 0; });
        g_ping++;
        lock.unlock();
        g_pcv.notify_all();
    }
    {
        std::unique_lock<std::mutex> lock(g_pm);
        g_pcv.wait(lock, []() { return g_ping == 40; });
    }
    ponger.join();
    std::printf("ping-pong reached %d\n", g_ping);

    // wait_for times out when nobody notifies
    {
        std::mutex m;
        std::condition_variable cv;
        std::unique_lock<std::mutex> lock(m);
        std::cv_status s = cv.wait_for(lock, std::chrono::milliseconds(30));
        std::printf("wait_for timed out: %d\n", (int)(s == std::cv_status::timeout));
        std::printf("lock held after wait_for: %d\n", (int)lock.owns_lock());
        bool r = cv.wait_for(lock, std::chrono::milliseconds(20), []() { return false; });
        std::printf("wait_for with false predicate: %d\n", (int)r);
        bool r2 = cv.wait_for(lock, std::chrono::milliseconds(20), []() { return true; });
        std::printf("wait_for with true predicate: %d\n", (int)r2);
        std::chrono::steady_clock::time_point until = std::chrono::steady_clock::now() + std::chrono::milliseconds(30);
        std::cv_status s2 = cv.wait_until(lock, until);
        std::printf("wait_until timed out: %d\n", (int)(s2 == std::cv_status::timeout));
    }

    // wait_for that is notified in time (heap: shared with the notifier)
    {
        std::mutex *m = new std::mutex;
        std::condition_variable *cv = new std::condition_variable;
        bool *ready = new bool(false);
        std::thread t([m, cv, ready]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            {
                std::lock_guard<std::mutex> lock(*m);
                *ready = true;
            }
            cv->notify_one();
        });
        std::unique_lock<std::mutex> lock(*m);
        bool ok = cv->wait_for(lock, std::chrono::seconds(10), [ready]() { return *ready; });
        std::printf("notified before the timeout: %d\n", (int)ok);
        lock.unlock();
        t.join();
        delete m;
        delete cv;
        delete ready;
    }
    return 0;
}
