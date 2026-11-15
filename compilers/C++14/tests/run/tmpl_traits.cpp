#include <cstdio>

template<class T, T v> struct integral_constant { static const T value = v; typedef T value_type; };
typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

template<class A, class B> struct is_same : false_type {};
template<class A> struct is_same<A, A> : true_type {};

template<class T> struct remove_reference { typedef T type; };
template<class T> struct remove_reference<T &> { typedef T type; };
template<class T> struct remove_reference<T &&> { typedef T type; };

template<class T> struct remove_const { typedef T type; };
template<class T> struct remove_const<const T> { typedef T type; };

template<bool C, class T = void> struct enable_if {};
template<class T> struct enable_if<true, T> { typedef T type; };

template<bool C, class A, class B> struct conditional { typedef A type; };
template<class A, class B> struct conditional<false, A, B> { typedef B type; };

template<class T> struct is_pointer : false_type {};
template<class T> struct is_pointer<T *> : true_type {};

template<class T> typename remove_reference<T>::type &&move(T &&t) { return static_cast<typename remove_reference<T>::type &&>(t); }

template<class T> typename enable_if<is_pointer<T>::value, int>::type describe(T) { return 1; }
template<class T> typename enable_if<!is_pointer<T>::value, int>::type describe(T) { return 2; }

template<class C> typename C::value_type first_of(const C &c) { return c.get(0); }
struct IntList {
    typedef int value_type;
    int d[3];
    int get(int i) const { return d[i]; }
};

template<class T> struct Outer {
    T v;
    struct Inner { T w; T get() const { return w; } };
    Inner make() const { Inner i; i.w = v + 1; return i; }
};

template<class T> struct Num {
    T v;
    Num(T x) : v(x) {}
    template<class U> Num(const Num<U> &o) : v((T)o.v) {}
    template<class U> friend struct Num;
};

template<class T, class U = T> struct Defaulted { typedef U type; };

template<class T> int ident(T x) { static_assert(sizeof(T) > 0, "size"); return (int)x; }

template<class T> struct Holder {
    T t;
    Holder(const T &x) : t(x) {}
    const T &get() const { return t; }
};
template<class T> Holder<T> make_holder(const T &x) { return Holder<T>(x); }

struct Tag { int id; };
template<int Tag::*P> struct Unused {};

template<typename T> struct Self {
    typedef Self<T> type;
    static int depth() { return 1; }
};

int main()
{
    printf("%d %d\n", (int)is_same<int, int>::value, (int)is_same<int, long>::value);
    printf("%d\n", (int)is_same<remove_reference<int &>::type, int>::value);
    printf("%d\n", (int)is_same<remove_const<const double>::type, double>::value);
    printf("%d\n", (int)is_same<conditional<true, int, char>::type, int>::value);
    printf("%d\n", (int)is_same<conditional<false, int, char>::type, char>::value);
    printf("%d %d\n", describe(&printf), describe(5));
    IntList il = {{7, 8, 9}};
    printf("%d\n", first_of(il));
    Outer<int> o = {41};
    Outer<int>::Inner in = o.make();
    printf("%d\n", in.get());
    Num<int> n1(3);
    Num<double> n2(n1);
    printf("%g\n", n2.v);
    printf("%d\n", (int)is_same<Defaulted<int>::type, int>::value);
    printf("%d\n", ident(3.9));
    int x = 4;
    printf("%d\n", move(x));
    Holder<int> h = make_holder(12);
    printf("%d\n", h.get());
    printf("%d\n", Self<int>::type::depth());
    return 0;
}
