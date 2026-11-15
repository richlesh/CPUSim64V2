#include <cstdio>

struct B {
    int v;
    B(int x) : v(x) { printf("B(%d)\n", x); }
    B(int x, int y) : v(x + y) { printf("B(%d,%d)\n", x, y); }
};
struct D : B {
    using B::B;
    int extra = 5;
};

struct S { int a[3]; int n; };

enum class Color : unsigned char { red = 1, green, blue };
enum Plain : short { P1 = 10, P2 };

struct Flag {
    explicit operator bool() const { return on; }
    bool on;
};

int counter() { static int n = 0; return ++n; }

int main()
{
    D d1(3), d2(4, 5);
    printf("%d %d %d\n", d1.v, d2.v, d1.extra);
    printf("%d\n", (int)(sizeof(S::a) / sizeof(int)));
    Color c = Color::green;
    printf("%d %d %d\n", (int)c, (int)Color::blue, (int)P2);
    Flag f{true}, g{false};
    if (f) printf("f on\n");
    if (!g) printf("g off\n");
    counter();
    printf("%d\n", counter());
    decltype(nullptr) np = nullptr;
    int *p = np;
    printf("%d\n", p == nullptr);
    return 0;
}
