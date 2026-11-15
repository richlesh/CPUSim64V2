template<class T> struct Base { typedef T type; T value; void hello() {} template<class U> U conv() const { return U(); } };
template<class T> struct Derived : Base<T> {
    using Base<T>::value;
    using typename Base<T>::type;
    typename Base<T>::type get() const { return this->value; }
    template<class U> U as() const { return this->template conv<U>(); }
    void call() { Base<T>::hello(); this->hello(); }
};
template<class D> struct Crtp { D &self() { return static_cast<D &>(*this); } };
struct Impl : Crtp<Impl> { int x; };
template<int N> struct Fact { static const int value = N * Fact<N - 1>::value; };
template<> struct Fact<0> { static const int value = 1; };
template<class T, class = void> struct has_x { static const bool value = false; };
template<class T> struct has_x<T, typename T::x_type> { static const bool value = true; };
template<class... Ts> struct Tuple { };
template<class H, class... Ts> struct Tuple<H, Ts...> : Tuple<Ts...> { H head; };
template<class T> T sum(T t) { return t; }
template<class T, class... R> T sum(T t, R... r) { return t + sum(r...); }
template<class F, class... A> auto call_f(F f, A &&... a) -> decltype(f(static_cast<A &&>(a)...)) { return f(static_cast<A &&>(a)...); }
template<class T> struct Outer {
    template<class U> struct Inner { T t; U u; };
    template<class U> static Inner<U> make(T t, U u) { Inner<U> r; r.t = t; r.u = u; return r; }
    friend bool operator==(const Outer &, const Outer &) { return true; }
};
template<typename T, unsigned N> unsigned count_of(T (&)[N]) { return N; }
template<class T> void fwd(T &&t) { sum(static_cast<T &&>(t)); }
int driver() {
    Outer<int>::Inner<double> in;
    auto r = Outer<int>::make<char>(1, 'c');
    int arr[5];
    unsigned n = count_of(arr);
    int f = Fact<5>::value;
    Derived<int> d;
    int g = d.as<int>() + d.get();
    auto generic = [](auto x, auto y) { return x + y; };
    int s = sum(1, 2, 3) + generic(1, 2);
    return n + f + g + s + has_x<int>::value;
}
