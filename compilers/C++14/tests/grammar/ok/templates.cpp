template<typename T> T max_of(T a, T b) { return a > b ? a : b; }
template<class T, int N> struct Array {
    T data[N];
    T &operator[](int i) { return data[i]; }
    int size() const { return N; }
};
template<class T> class Stack {
public:
    Stack() : n(0) {}
    void push(const T &v) { items[n++] = v; }
    T pop() { return items[--n]; }
    typedef T value_type;
    typename Stack<T>::value_type top() const { return items[n - 1]; }
private:
    T items[16];
    int n;
};
template<> struct Array<char, 0> { };
template<class T> struct Array<T *, 1> { T *p; };
template<class T, class U = int> struct Pair { T first; U second; };
template<typename... Args> int count_args(Args... args) { return sizeof...(args); }
template<class T> using Vec = Stack<T>;
template<template<class> class C, class T> struct Wrap { C<T> c; };
template<class T> struct Traits { typedef typename T::type type; static const bool value = false; };
template int max_of<int>(int, int);
template class Stack<int>;
template<class T> void swap_values(T &a, T &b) { T t = a; a = b; b = t; }
int use() {
    Stack<int> s;
    s.push(1);
    Array<int, 4> arr;
    Pair<int, Stack<int>> pr;
    Vec<double> v;
    int m = max_of<int>(1, 2);
    int n = max_of(3, 4);
    bool lt = m < n;
    bool gt = m > n;
    Stack<Stack<int>> nested;
    return m + count_args(1, 2, 3) + Traits<Stack<int>>::value;
}
