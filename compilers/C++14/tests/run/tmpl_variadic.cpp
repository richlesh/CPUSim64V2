#include <cstdio>

int sum() { return 0; }
template<class T, class... R> int sum(T first, R... rest) { return first + sum(rest...); }

template<class... A> int count(A... a) { return sizeof...(A); }

void show(int x) { printf("int %d\n", x); }
void show(double x) { printf("double %g\n", x); }
void show(const char *x) { printf("str %s\n", x); }
void show_all() {}
template<class T, class... R> void show_all(const T &t, const R &... r) { show(t); show_all(r...); }

template<class T> T &&fwd(T &t) { return static_cast<T &&>(t); }
template<class T> struct Plain { static const char *n() { return "value"; } };
template<class T> struct Plain<T &> { static const char *n() { return "lvalue-ref"; } };

template<class F, class... A> int call(F f, A... a) { return f(a...); }
int add3(int a, int b, int c) { return a + b + c; }

template<class... T> struct Tuple;
template<> struct Tuple<> { static int size() { return 0; } };
template<class H, class... T> struct Tuple<H, T...> : Tuple<T...> {
    H head;
    Tuple(H h, T... t) : Tuple<T...>(t...), head(h) {}
    static int size() { return 1 + Tuple<T...>::size(); }
    H first() const { return head; }
};

template<int... Ns> struct Sum;
template<> struct Sum<> { enum { value = 0 }; };
template<int N, int... Ns> struct Sum<N, Ns...> { enum { value = N + Sum<Ns...>::value }; };

struct Obj {
    int a, b;
    Obj(int x, int y) : a(x), b(y) {}
};
template<class T, class... A> T make(A... a) { return T(a...); }

int main()
{
    printf("%d\n", sum(1, 2, 3, 4));
    printf("%d %d\n", count(), count(1, 2.5, 'c'));
    show_all(1, 2.5, "three");
    int x = 5;
    printf("%d\n", fwd(x));
    printf("%s %s\n", Plain<int>::n(), Plain<int &>::n());
    printf("%d\n", call(add3, 1, 2, 3));
    Tuple<int, double, char> t(1, 2.5, 'z');
    printf("%d %d\n", Tuple<int, double, char>::size(), t.first());
    printf("%d\n", (int)Sum<1, 2, 3, 4>::value);
    Obj o = make<Obj>(3, 4);
    printf("%d %d\n", o.a, o.b);
    return 0;
}
