#include <cstdio>
#include <exception>
#include <stdexcept>

struct T {
    int id;
    T(int i) : id(i) { printf("T(%d)\n", id); }
    T(const T &o) : id(o.id + 10) { printf("T copy %d\n", id); }
    ~T() { printf("~T(%d)\n", id); }
};

struct AppError : std::exception {
    int code;
    AppError(int c) : code(c) {}
    const char *what() const noexcept { return "app error"; }
};

int risky(int n)
{
    T a(n);
    if (n > 2) throw AppError(n);
    T b(n + 100);
    return n;
}

int through_loop()
{
    for (int i = 0; i < 5; i++) {
        T t(i);
        try {
            if (i == 2) throw i;
        } catch (int) {
            printf("loop catch\n");
            return i * 10;
        }
    }
    return -1;
}

template<class F> int guarded(F f)
{
    try {
        return f();
    } catch (const std::exception &e) {
        printf("guarded: %s\n", e.what());
        return -1;
    }
}

struct Arr {
    T items[2];
    Arr() : items{T(1), T(2)} {}
};

void rethrow_copy()
{
    try {
        throw AppError(5);
    } catch (AppError e) {
        printf("by value %d\n", e.code);
        e.code = 6;
        throw e;
    }
}

int main()
{
    try { risky(1); risky(5); } catch (const AppError &e) { printf("code %d %s\n", e.code, e.what()); }
    printf("%d\n", through_loop());
    printf("%d\n", guarded([] { return risky(3); }));
    printf("%d\n", guarded([] { return risky(1); }));
    try { rethrow_copy(); } catch (const AppError &e) { printf("final %d\n", e.code); }
    try { throw std::bad_exception(); } catch (const std::exception &e) { printf("%s\n", e.what()); }
    {
        Arr a;
    }
    try {
        throw std::runtime_error("uncaught in main");
    } catch (...) {
        printf("catch all\n");
    }
    return 0;
}
