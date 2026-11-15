#include <cstdio>

template<class T> T add(T a, T b) { return a + b; }
template<class T> T maxof(const T &a, const T &b) { return a < b ? b : a; }
template<class T> void swap_values(T &a, T &b) { T t = a; a = b; b = t; }
template<class T, class U> double mix(T a, U b) { return a * 1.5 + b; }
template<class T> T *first(T *p) { return p; }
template<class T, int N> int array_size(T (&)[N]) { return N; }
template<int N> int times(int x) { return x * N; }
template<class T> const char *kind(T) { return "other"; }
int kind(int) { return "int"[0] == 'i' ? 1 : 0; }
template<class R, class A> R apply(R (*f)(A), A a) { return f(a); }
int twice(int x) { return 2 * x; }

struct P {
    int v;
    P(int x) : v(x) {}
    P operator+(const P &o) const { return P(v + o.v); }
    bool operator<(const P &o) const { return v < o.v; }
};

template<class T> struct Box {
    T value;
    Box(T v) : value(v) {}
    T get() const { return value; }
    void set(T v) { value = v; }
    template<class U> U as() const { return (U)value; }
};

int main()
{
    printf("%d\n", add(1, 2));
    printf("%g\n", add(1.5, 2.25));
    printf("%d\n", add<int>(3, 4));
    printf("%d\n", maxof(3, 9));
    printf("%g\n", maxof(2.5, 1.5));
    int a = 1, b = 2;
    swap_values(a, b);
    printf("%d %d\n", a, b);
    printf("%g\n", mix(2, 3));
    int arr[5] = {1, 2, 3, 4, 5};
    printf("%d\n", *first(arr + 2));
    printf("%d\n", array_size(arr));
    printf("%d\n", times<7>(6));
    printf("%s %d\n", kind(1.5), kind(3));
    printf("%d\n", apply(twice, 21));
    P p = add(P(3), P(4));
    printf("%d\n", p.v);
    printf("%d\n", maxof(P(5), P(8)).v);
    Box<int> bi(41);
    bi.set(bi.get() + 1);
    printf("%d\n", bi.get());
    Box<double> bd(2.5);
    printf("%g %d\n", bd.get(), bd.as<int>());
    return 0;
}
