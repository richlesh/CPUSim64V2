// std::atomic: counters under several threads, compare_exchange loops, pointers, flags, bool, operators
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdio>

static std::atomic<int> g_count(0);
static std::atomic<long> g_sub(1000000);
static std::atomic<long> g_max(0);
static std::atomic<unsigned> g_bits(0);
static std::atomic<long> g_mult(1);
static std::atomic<bool> g_ready(false);
static std::atomic_flag g_lock;
static long g_guarded = 0;

void count_up(int n)
{
    for (int i = 0; i < n; i++) {
        g_count++;
        g_sub -= 2;
    }
}

void raise_max(long v)
{
    long cur = g_max.load();
    while (cur < v && !g_max.compare_exchange_weak(cur, v)) {
    }
}

// a multiply done with a compare_exchange loop
void double_it(int times)
{
    for (int i = 0; i < times; i++) {
        long old = g_mult.load();
        while (!g_mult.compare_exchange_strong(old, old * 2)) {
        }
    }
}

void spin_guarded(int n)
{
    for (int i = 0; i < n; i++) {
        while (g_lock.test_and_set()) std::this_thread::yield();
        g_guarded++;
        g_lock.clear();
    }
}

int main()
{
    // counters: exact totals
    std::vector<std::thread> pool;
    for (int i = 0; i < 4; i++) pool.push_back(std::thread(count_up, 1000));
    for (size_t i = 0; i < pool.size(); i++) pool[i].join();
    std::printf("g_count = %d, g_sub = %ld\n", g_count.load(), g_sub.load());

    // the full operator set, single thread
    std::atomic<int> a(10);
    int r1, r2;
    r1 = a++;
    r2 = a;
    std::printf("post++ %d, now %d\n", r1, r2);
    r1 = ++a;
    std::printf("pre++ %d\n", r1);
    r1 = a--;
    r2 = --a;
    std::printf("post-- %d, pre-- %d\n", r1, r2);
    r1 = (a += 5);
    r2 = (a -= 3);
    std::printf("+= %d, -= %d\n", r1, r2);
    a = 0xF0;
    r1 = (a &= 0x3C);
    r2 = (a |= 0x03);
    int r3 = (a ^= 0xFF);
    std::printf("&= %d, |= %d, ^= %d\n", r1, r2, r3);
    int plain = a;
    std::printf("implicit conversion %d\n", plain);
    r1 = a.exchange(7);
    std::printf("exchange %d, now %d\n", r1, a.load());
    r1 = a.fetch_add(3);
    r2 = a.fetch_sub(1);
    std::printf("fetch_add %d, fetch_sub %d, now %d\n", r1, r2, a.load());
    r1 = a.fetch_and(6);
    r2 = a.fetch_or(8);
    r3 = a.fetch_xor(1);
    std::printf("fetch_and %d, fetch_or %d, fetch_xor %d, now %d\n", r1, r2, r3, a.load());
    std::printf("is_lock_free %d\n", (int)a.is_lock_free());

    // compare_exchange: expected is updated on failure
    std::atomic<int> c(5);
    int expected = 4;
    bool ok = c.compare_exchange_strong(expected, 9);
    std::printf("cas fail: %d, expected now %d, value %d\n", (int)ok, expected, c.load());
    ok = c.compare_exchange_strong(expected, 9);
    std::printf("cas ok: %d, value %d\n", (int)ok, c.load());

    // a compare_exchange loop for a maximum
    std::vector<std::thread> maxers;
    for (int i = 1; i <= 4; i++) maxers.push_back(std::thread([i]() {
        for (int k = 0; k < 100; k++) raise_max(i * 1000 + k);
    }));
    for (size_t i = 0; i < maxers.size(); i++) maxers[i].join();
    std::printf("max = %ld\n", g_max.load());

    // a multiply by compare_exchange
    std::vector<std::thread> doublers;
    for (int i = 0; i < 4; i++) doublers.push_back(std::thread(double_it, 5));
    for (size_t i = 0; i < doublers.size(); i++) doublers[i].join();
    std::printf("doubled 20 times: %ld\n", g_mult.load());

    // bit flags set by several threads
    std::vector<std::thread> setters;
    for (int i = 0; i < 8; i++) setters.push_back(std::thread([i]() { g_bits.fetch_or(1u << i); }));
    for (size_t i = 0; i < setters.size(); i++) setters[i].join();
    std::printf("bits = %u\n", g_bits.load());

    // atomic_flag as a spin lock
    std::vector<std::thread> spinners;
    for (int i = 0; i < 4; i++) spinners.push_back(std::thread(spin_guarded, 300));
    for (size_t i = 0; i < spinners.size(); i++) spinners[i].join();
    std::printf("spin-guarded total = %ld\n", g_guarded);
    {
        bool t1 = g_lock.test_and_set();
        bool t2 = g_lock.test_and_set();
        std::printf("flag test_and_set %d then %d\n", (int)t1, (int)t2);
    }
    g_lock.clear();

    // atomic bool as a start signal
    std::atomic<int> *started = new std::atomic<int>(0);
    std::vector<std::thread> runners;
    for (int i = 0; i < 3; i++) runners.push_back(std::thread([started]() {
        while (!g_ready.load()) std::this_thread::yield();
        started->fetch_add(1);
    }));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    std::printf("started before the signal: %d\n", started->load());
    g_ready = true;
    for (size_t i = 0; i < runners.size(); i++) runners[i].join();
    std::printf("started after the signal: %d\n", started->load());
    std::atomic<bool> b(false);
    {
        bool old_b = b.exchange(true);
        std::printf("bool exchange %d, now %d\n", (int)old_b, (int)b.load());
    }

    // atomic pointers: arithmetic scales by the element size
    int *arr = new int[10];
    for (int i = 0; i < 10; i++) arr[i] = i * 11;
    std::atomic<int *> p(arr);
    std::printf("load %d\n", *p.load());
    int *old = p.fetch_add(3);
    std::printf("fetch_add old %d, now %d\n", *old, *p.load());
    p += 2;
    std::printf("+= 2 -> %d\n", *p.load());
    ++p;
    std::printf("++ -> %d\n", *p.load());
    p--;
    std::printf("-- -> %d\n", *p.load());
    int *e = arr + 5;
    bool pok = p.compare_exchange_strong(e, arr + 8);
    std::printf("pointer cas: %d -> %d\n", (int)pok, *p.load());
    e = arr;
    pok = p.compare_exchange_strong(e, arr + 1);
    std::printf("pointer cas fail: %d, expected now %d\n", (int)pok, *e);
    std::printf("exchange old %d\n", *p.exchange(arr + 9));
    std::printf("difference %ld\n", (long)(p.load() - arr));

    // pointers claimed by several threads: every slot is taken exactly once
    std::atomic<int *> *next = new std::atomic<int *>(arr);
    int *taken = new int[10];
    for (int i = 0; i < 10; i++) taken[i] = 0;
    std::vector<std::thread> claimers;
    for (int i = 0; i < 5; i++) claimers.push_back(std::thread([next, arr, taken]() {
        for (int k = 0; k < 2; k++) {
            int *mine = next->fetch_add(1);
            taken[mine - arr]++;
        }
    }));
    for (size_t i = 0; i < claimers.size(); i++) claimers[i].join();
    int bad = 0;
    for (int i = 0; i < 10; i++) if (taken[i] != 1) bad++;
    std::printf("slots taken once: %s\n", bad == 0 ? "all" : "NOT all");

    // other integer types and typedefs
    std::atomic_long al(5);
    std::atomic<unsigned long> aul(3);
    std::atomic_bool ab(true);
    al += 10;
    aul.fetch_add(4);
    std::printf("atomic_long %ld, unsigned long %lu, atomic_bool %d\n", al.load(), aul.load(), (int)ab.load());
    std::atomic<int> st;
    st.store(42);
    std::printf("store/load %d\n", st.load(std::memory_order_acquire));
    st.store(43, std::memory_order_release);
    std::printf("exchange relaxed %d\n", st.exchange(1, std::memory_order_relaxed));
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return 0;
}
