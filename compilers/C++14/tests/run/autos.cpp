#include <cstdio>

constexpr int N = 4;
const int M = N * 2 + 1;
static_assert(M == 9, "M should be 9");
static_assert(sizeof(char) == 1, "char");

int twice(int x) { return 2 * x; }

int main()
{
    auto a = 5;
    auto b = 2.5;
    auto c = 'x';
    auto d = true;
    auto e = a * 2L;
    auto s = "text";
    auto p = &a;
    auto &r = a;
    const auto k = 3;
    r++;
    printf("%d %.1f %c %d %ld %s %d %d\n", a, b, c, d, e, s, *p, k);
    int arr[M];
    for (int i = 0; i < M; i++) arr[i] = i;
    printf("%d\n", arr[M - 1]);
    decltype(a) x = 7;
    decltype(b) y = 1.5;
    printf("%d %.1f\n", x, y);
    auto f = twice;
    printf("%d\n", f(4));
    auto *q = &x;
    printf("%d\n", *q);
    enum { Z = N + 1 };
    int big[Z];
    big[Z - 1] = 3;
    printf("%d\n", big[Z - 1]);
    return 0;
}
