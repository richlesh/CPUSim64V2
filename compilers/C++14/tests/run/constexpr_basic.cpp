#include <cstdio>

constexpr int sq(int x) { return x * x; }
constexpr int fact(int n) { return n <= 1 ? 1 : n * fact(n - 1); }
constexpr int fib(int n) { int a = 0, b = 1; for (int i = 0; i < n; i++) { int t = a + b; a = b; b = t; } return a; }
constexpr bool is_prime(int n)
{
    if (n < 2) return false;
    for (int d = 2; d * d <= n; d++)
        if (n % d == 0) return false;
    return true;
}
constexpr int gcd(int a, int b) { return b == 0 ? a : gcd(b, a % b); }
constexpr int sum_to(int n) { int s = 0; int i = 0; while (i <= n) { s += i; i++; } return s; }

struct Limits {
    static constexpr int size() { return 4; }
    static const int max = sq(5);
};

template<int N> struct Buf { char data[N]; int n() const { return N; } };

enum { A = sq(3), B = fact(4) };

int main()
{
    constexpr int n = sq(7);
    int arr[fact(4)];
    static_assert(fib(10) == 55, "fib");
    static_assert(is_prime(97) && !is_prime(91), "prime");
    printf("%d %d %d\n", n, (int)(sizeof(arr) / sizeof(arr[0])), gcd(84, 36));
    Buf<sq(4) + 1> b;
    printf("%d %d %d\n", b.n(), A, B);
    printf("%d %d\n", Limits::size() * 2, Limits::max);
    switch (6) {
    case sq(2) + 2: printf("six\n"); break;
    default: printf("other\n");
    }
    int rt = 5;
    printf("%d %d\n", sq(rt), sum_to(100));
    return 0;
}
