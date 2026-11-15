#include <cstdio>

template<class T, int N = 4>
class Stack {
    T items[N];
    int count;
public:
    typedef T value_type;
    Stack() : count(0) {}
    void push(const T &v);
    T pop();
    bool empty() const { return count == 0; }
    int size() const { return count; }
    static int capacity() { return N; }
    static int created;
};

template<class T, int N> void Stack<T, N>::push(const T &v) { items[count++] = v; }
template<class T, int N> T Stack<T, N>::pop() { return items[--count]; }
template<class T, int N> int Stack<T, N>::created = 7;

template<class T> struct Traits { static const char *name() { return "generic"; } enum { size = 1 }; };
template<> struct Traits<int> { static const char *name() { return "int"; } enum { size = 4 }; };
template<class T> struct Traits<T *> { static const char *name() { return "pointer"; } enum { size = 8 }; };
template<class T> struct Traits<const T> { static const char *name() { return "const"; } enum { size = 2 }; };

template<int N> struct Fact { enum { value = N * Fact<N - 1>::value }; };
template<> struct Fact<0> { enum { value = 1 }; };

template<class T> struct Pair {
    T first, second;
    Pair(const T &a, const T &b) : first(a), second(b) {}
    T sum() const { return first + second; }
};

template<class D> struct Base {
    int run() { return static_cast<D *>(this)->impl() + 1; }
};
struct Derived : Base<Derived> {
    int impl() { return 41; }
};

template<class T> struct Wrapper : Pair<T> {
    Wrapper(T a, T b) : Pair<T>(a, b) {}
    T twice() const { return this->sum() * 2; }
};

template<class T> struct Counter {
    static int n;
    Counter() { ++n; }
    ~Counter() { --n; }
};
template<class T> int Counter<T>::n = 0;

template<class T> struct Shape {
    virtual ~Shape() {}
    virtual T area() const = 0;
};
template<class T> struct Sq : Shape<T> {
    T s;
    Sq(T x) : s(x) {}
    T area() const { return s * s; }
};

template<class T> using IntPair = Pair<int>;

int main()
{
    Stack<int> s;
    s.push(1);
    s.push(2);
    s.push(3);
    printf("%d\n", s.size());
    int p3 = s.pop();
    int p2 = s.pop();
    printf("%d %d\n", p3, p2);
    printf("%d %d\n", Stack<int>::capacity(), Stack<char, 9>::capacity());
    printf("%d\n", Stack<int>::created);
    Stack<double, 2> sd;
    sd.push(1.5);
    printf("%d\n", sd.empty());
    printf("%g\n", sd.pop());
    printf("%d\n", sd.empty());
    Stack<int>::value_type v = 5;
    printf("%d\n", v);
    printf("%s %s %s %s\n", Traits<double>::name(), Traits<int>::name(), Traits<char *>::name(), Traits<const long>::name());
    printf("%d %d %d\n", (int)Traits<double>::size, (int)Traits<int>::size, (int)Traits<int *>::size);
    printf("%d %d\n", (int)Fact<5>::value, (int)Fact<0>::value);
    Pair<double> p(1.5, 2.0);
    printf("%g\n", p.sum());
    Derived d;
    printf("%d\n", d.run());
    Wrapper<int> w(3, 4);
    printf("%d\n", w.twice());
    {
        Counter<int> c1, c2;
        Counter<char> c3;
        printf("%d %d\n", Counter<int>::n, Counter<char>::n);
    }
    printf("%d\n", Counter<int>::n);
    Sq<int> sq(6);
    Shape<int> *sp = &sq;
    printf("%d\n", sp->area());
    IntPair<void> ip(2, 3);
    printf("%d\n", ip.sum());
    return 0;
}
