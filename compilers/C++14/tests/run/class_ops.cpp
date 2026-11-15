#include <cstdio>

struct Vec {
    int x, y;
    Vec() : x(0), y(0) {}
    Vec(int a, int b) : x(a), y(b) {}
    Vec operator+(const Vec &o) const { return Vec(x + o.x, y + o.y); }
    Vec operator-(const Vec &o) const { return Vec(x - o.x, y - o.y); }
    Vec operator*(int k) const { return Vec(x * k, y * k); }
    Vec &operator+=(const Vec &o) { x += o.x; y += o.y; return *this; }
    Vec operator-() const { return Vec(-x, -y); }
    bool operator==(const Vec &o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vec &o) const { return !(*this == o); }
    int &operator[](int i) { return i == 0 ? x : y; }
    const int &operator[](int i) const { return i == 0 ? x : y; }
    Vec &operator++() { ++x; ++y; return *this; }
    Vec operator++(int) { Vec old = *this; ++x; ++y; return old; }
    int operator()(int k) const { return x * k + y; }
};

Vec operator*(int k, const Vec &v) { return v * k; }

struct Money {
    long cents;
    Money(long c) : cents(c) {}
    operator long() const { return cents; }
    explicit operator bool() const { return cents != 0; }
};

struct Celsius {
    double deg;
    Celsius(double d) : deg(d) {}
};
struct Fahrenheit {
    double deg;
    Fahrenheit(const Celsius &c) : deg(c.deg * 9 / 5 + 32) {}
};

int show(const Vec &v) { return v.x * 100 + v.y; }
double ftemp(Fahrenheit f) { return f.deg; }
long total(long a, long b) { return a + b; }

struct Flag {
    bool on;
    Flag(bool b = false) : on(b) {}
    bool operator!() const { return !on; }
};

int main()
{
    Vec a(1, 2), b(3, 4);
    Vec c = a + b;
    printf("%d %d\n", c.x, c.y);
    c += Vec(10, 10);
    printf("%d %d\n", c.x, c.y);
    printf("%d\n", show(a - b));
    printf("%d\n", show(a * 3));
    printf("%d\n", show(2 * b));
    printf("%d\n", show(-a));
    printf("%d %d\n", a == b, a != b);
    printf("%d %d\n", a == Vec(1, 2), a != Vec(1, 2));
    a[0] = 50;
    printf("%d %d\n", a[0], a[1]);
    const Vec k(7, 8);
    printf("%d %d\n", k[0], k[1]);
    Vec p(1, 1);
    Vec q = p++;
    printf("%d %d\n", p.x, q.x);
    printf("%d\n", (++p).x);
    printf("%d\n", p.y);
    printf("%d\n", a(2));
    Money m(250);
    long l = m;
    printf("%ld %ld\n", l, total(m, 50));
    if (m) printf("nonzero\n");
    Money z(0);
    if (!z) printf("zero\n");
    printf("%.1f\n", ftemp(Celsius(100)));
    Flag f(true);
    if (!f) printf("off\n"); else printf("on\n");
    Vec t;
    t = a;
    t = Vec(9, 9);
    printf("%d\n", show(t));
    return 0;
}
