// std::thread: functions, lambdas (capturing), functors, arguments, std::ref, join, detach, ids
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <vector>
#include <cstdio>

static int g_sum = 0;
static std::atomic<int> g_flag(0);

void add_to_global(int a, int b) { g_sum = a + b; }
void set_ref(int &out, int v) { out = v * 2; }
void add_atomic(std::atomic<int> *p, int n) { p->fetch_add(n); }

struct Functor {
    int base;
    int *out;
    void operator()(int x) { *out = base + x; }
};

int main()
{
    // a function with arguments
    std::thread t1(add_to_global, 20, 22);
    std::printf("joinable before join: %d\n", (int)t1.joinable());
    t1.join();
    std::printf("joinable after join: %d, g_sum = %d\n", (int)t1.joinable(), g_sum);

    // a default constructed thread is not joinable
    std::thread t0;
    std::printf("default joinable: %d\n", (int)t0.joinable());
    std::printf("default id equals id(): %d\n", (int)(t0.get_id() == std::thread::id()));

    // a capturing lambda writing through a pointer (heap: threads share it)
    int *result = new int(0);
    std::thread t2([result]() { *result = 123; });
    t2.join();
    std::printf("lambda result = %d\n", *result);

    // a lambda with arguments, a capture by value
    int k = 5;
    std::thread t3([result, k](int a, int b) { *result = k * (a + b); }, 3, 4);
    t3.join();
    std::printf("lambda with args = %d\n", *result);

    // a functor
    Functor f;
    f.base = 1000;
    f.out = result;
    std::thread t4(f, 7);
    t4.join();
    std::printf("functor = %d\n", *result);

    // std::ref for a reference parameter (the object lives on the heap)
    int *slot = new int(0);
    std::thread t5(set_ref, std::ref(*slot), 21);
    t5.join();
    std::printf("ref = %d\n", *slot);

    // a move: the new owner joins
    std::thread a([result]() { *result = 77; });
    std::thread b(std::move(a));
    std::printf("moved-from joinable: %d, new joinable: %d\n", (int)a.joinable(), (int)b.joinable());
    b.join();
    std::printf("moved = %d\n", *result);

    // move assignment into a default thread, swap
    std::thread c;
    std::thread d([result]() { *result = 88; });
    c = std::move(d);
    c.join();
    std::printf("assigned = %d\n", *result);
    std::thread e, h;
    e.swap(h);
    std::swap(e, h);
    std::printf("swapped joinable: %d %d\n", (int)e.joinable(), (int)h.joinable());

    // several threads
    std::atomic<int> *counter = new std::atomic<int>(0);
    std::vector<std::thread> pool;
    for (int i = 1; i <= 4; i++) pool.push_back(std::thread(add_atomic, counter, i));
    for (size_t i = 0; i < pool.size(); i++) pool[i].join();
    std::printf("pool total = %d\n", counter->load());

    // ids: a thread's id differs from main's, equals what the thread sees; printable
    std::thread::id main_id = std::this_thread::get_id();
    std::thread::id *seen = new std::thread::id();
    std::thread t6([seen]() { *seen = std::this_thread::get_id(); });
    std::thread::id tid = t6.get_id();
    t6.join();
    std::printf("thread id differs from main: %d\n", (int)(tid != main_id));
    std::printf("thread id equals id seen inside: %d\n", (int)(tid == *seen));
    std::printf("main id is not the empty id: %d\n", (int)(main_id != std::thread::id()));
    std::cout << "ids printable: " << (main_id == main_id ? "yes" : "no") << std::endl;

    // detach: the thread signals through an atomic on the heap
    std::atomic<int> *done = new std::atomic<int>(0);
    std::thread t7([done]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        done->store(1);
    });
    t7.detach();
    std::printf("detached joinable: %d\n", (int)t7.joinable());
    while (done->load() == 0) std::this_thread::yield();
    std::printf("detached thread ran\n");

    std::printf("hardware_concurrency > 0: %d\n", (int)(std::thread::hardware_concurrency() > 0));

    delete result;
    delete slot;
    return 0;
}
