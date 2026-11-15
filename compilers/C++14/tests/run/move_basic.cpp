#include <cstdio>

template<class T> struct rm_ref { typedef T type; };
template<class T> struct rm_ref<T &> { typedef T type; };
template<class T> struct rm_ref<T &&> { typedef T type; };
template<class T> typename rm_ref<T>::type &&mv(T &&t) { return static_cast<typename rm_ref<T>::type &&>(t); }
template<class T> T &&fwd(typename rm_ref<T>::type &t) { return static_cast<T &&>(t); }

struct Buf {
    int *p;
    int n;
    explicit Buf(int k) : p(new int[k]), n(k) { for (int i = 0; i < k; i++) p[i] = i; printf("Buf(%d)\n", k); }
    Buf(const Buf &o) : p(new int[o.n]), n(o.n) { for (int i = 0; i < n; i++) p[i] = o.p[i]; printf("copy %d\n", n); }
    Buf(Buf &&o) : p(o.p), n(o.n) { o.p = 0; o.n = 0; printf("move %d\n", n); }
    Buf &operator=(const Buf &o) { if (this != &o) { delete[] p; n = o.n; p = new int[n]; for (int i = 0; i < n; i++) p[i] = o.p[i]; } printf("copy= %d\n", n); return *this; }
    Buf &operator=(Buf &&o) { if (this != &o) { delete[] p; p = o.p; n = o.n; o.p = 0; o.n = 0; } printf("move= %d\n", n); return *this; }
    ~Buf() { printf("~Buf %d\n", n); delete[] p; }
};

Buf make(int k) { Buf b(k); return b; }
Buf pass(Buf b) { return b; }

struct Holder {
    Buf b;
    int tag;
    Holder(int k) : b(k), tag(k) {}
};

struct OnlyMove {
    int v;
    OnlyMove(int x) : v(x) {}
    OnlyMove(OnlyMove &&o) : v(o.v) { o.v = -1; }
    OnlyMove &operator=(OnlyMove &&o) { v = o.v; o.v = -1; return *this; }
    OnlyMove(const OnlyMove &) = delete;
};

void sink(Buf &&b) { printf("sink %d\n", b.n); }
void sink(const Buf &b) { printf("lsink %d\n", b.n); }

int main()
{
    Buf a(3);
    Buf b = mv(a);
    printf("a.n=%d b.n=%d\n", a.n, b.n);
    Buf c(2);
    c = mv(b);
    printf("b.n=%d c.n=%d\n", b.n, c.n);
    Buf d = make(4);
    printf("d.n=%d\n", d.n);
    Buf e(1);
    e = make(5);
    printf("e.n=%d\n", e.n);
    Buf f = pass(mv(d));
    printf("f.n=%d\n", f.n);
    Buf g(7);
    sink(g);
    sink(mv(g));
    sink(Buf(8));
    Holder h(6);
    Holder h2 = mv(h);
    printf("%d %d %d\n", h2.b.n, h2.tag, h.b.n);
    OnlyMove m1(5);
    OnlyMove m2 = mv(m1);
    OnlyMove m3(0);
    m3 = mv(m2);
    printf("%d %d %d\n", m1.v, m2.v, m3.v);
    Buf x(9);
    Buf z = pass(mv(x));
    printf("z.n=%d\n", z.n);
    return 0;
}
