#include <cstdio>

template<class T> constexpr T pi = T(3.5);
template<class T> const int width = sizeof(T) * 0 + 7;
template<int N> struct Arr { int a[N]; int size() const { return N; } };

template<class T> T twice(T x) { return x + x; }
int addone(int x) { return x + 1; }

template<class T> struct Cmp { static bool lt(const T &a, const T &b) { return a < b; } };

template<class T, bool (*F)(const T &, const T &)> int count_if3(const T *p) {
    return (int)F(p[0], p[1]) + (int)F(p[1], p[2]);
}

template<class T> struct Fn {
    typedef int (*type)(int);
};

template<class T> struct Mem {
    T v;
    int get() const { return (int)v; }
};

template<class T> T sq(T x) { return x * x; }

int apply(int (*f)(int), int x) { return f(x); }

int main()
{
    printf("%g %g\n", pi<double>, (double)pi<int>);
    printf("%d\n", width<char> + width<long>);
    Arr<6> a;
    printf("%d\n", a.size());
    int (*fp)(int) = twice<int>;
    printf("%d\n", fp(21));
    printf("%d\n", apply(sq<int>, 9));
    printf("%d\n", apply(twice, 4));
    int v[3] = {1, 5, 2};
    printf("%d\n", count_if3<int, Cmp<int>::lt>(v));
    Fn<int>::type g = addone;
    printf("%d\n", g(1));
    Mem<double> m = {4.5};
    printf("%d\n", m.get());
    return 0;
}
