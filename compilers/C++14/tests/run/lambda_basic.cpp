#include <cstdio>

template<class F> int apply(F f, int x) { return f(x); }
template<class F> void repeat(int n, F f) { for (int i = 0; i < n; i++) f(i); }

struct Counter {
    int total;
    Counter() : total(0) {}
    void add_all(int n) {
        repeat(n, [this](int i) { total += i; });
    }
    int scaled(int k) const {
        return apply([=](int x) { return x * k + total; }, 3);
    }
};

struct Tracker {
    int id;
    Tracker(int i) : id(i) { printf("Tracker(%d)\n", i); }
    Tracker(const Tracker &o) : id(o.id + 100) { printf("copy %d\n", id); }
    ~Tracker() { printf("~Tracker %d\n", id); }
};

auto make_adder(int k) { return [k](int x) { return x + k; }; }

int main()
{
    int base = 10;
    auto add = [base](int x) { return x + base; };
    printf("%d\n", add(5));
    int n = 0;
    auto inc = [&n]() { n++; };
    inc();
    inc();
    printf("%d\n", n);
    auto mul = [](int a, int b) { return a * b; };
    printf("%d\n", mul(6, 7));
    auto cnt = [c = 0]() mutable { return ++c; };
    cnt();
    printf("%d\n", cnt());
    printf("%d\n", apply([](int x) { return x * x; }, 9));
    int acc = 0;
    repeat(5, [&](int i) { acc += i * i; });
    printf("%d\n", acc);
    auto gen = [](auto a, auto b) { return a + b; };
    printf("%d %g\n", gen(1, 2), gen(1.5, 2.0));
    auto add5 = make_adder(5);
    printf("%d\n", add5(1));
    Counter c;
    c.add_all(5);
    printf("%d %d\n", c.total, c.scaled(2));
    auto dbl = [](int x) -> double { return x / 2.0; };
    printf("%g\n", dbl(5));
    Tracker t(1);
    {
        auto byval = [t]() { return t.id; };
        printf("%d\n", byval());
    }
    auto byref = [&t]() { return t.id; };
    printf("%d\n", byref());
    const int k = 4;
    auto usek = [](int x) { return x + k; };
    printf("%d\n", usek(1));
    auto nested = [&](int x) { return [&](int y) { return x + y + base; }(5); };
    printf("%d\n", nested(1));
    return 0;
}
