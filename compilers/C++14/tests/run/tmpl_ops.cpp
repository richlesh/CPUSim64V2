#include <iostream>
#include <string>

template<class T> struct Vec2 {
    T x, y;
    Vec2(T a = T(), T b = T()) : x(a), y(b) {}
    Vec2 operator+(const Vec2 &o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 &operator+=(const Vec2 &o) { x += o.x; y += o.y; return *this; }
    bool operator==(const Vec2 &o) const { return x == o.x && y == o.y; }
    T dot(const Vec2 &o) const { return x * o.x + y * o.y; }
    void never_used() { T::does_not_exist(); }
};

template<class T> std::ostream &operator<<(std::ostream &os, const Vec2<T> &v)
{
    return os << "(" << v.x << ", " << v.y << ")";
}

template<class T> Vec2<T> operator*(T k, const Vec2<T> &v) { return Vec2<T>(k * v.x, k * v.y); }

template<class T> int overload(T) { return 1; }
template<class T> int overload(T *) { return 2; }
int overload(char) { return 4; }

template<class T> struct Node {
    T value;
    Node *next;
    Node(const T &v, Node *n = 0) : value(v), next(n) {}
};

template<class T> class List {
    Node<T> *head;
    int n;
public:
    List() : head(0), n(0) {}
    ~List() { while (head) { Node<T> *t = head; head = head->next; delete t; } }
    void push(const T &v) { head = new Node<T>(v, head); ++n; }
    int size() const { return n; }
    template<class F> void each(F f) const { for (Node<T> *p = head; p; p = p->next) f(p->value); }
    T sum() const { T s = T(); for (Node<T> *p = head; p; p = p->next) s = s + p->value; return s; }
};

void printer(const int &v) { std::cout << v << ' '; }

int main()
{
    Vec2<int> a(1, 2), b(3, 4);
    std::cout << a + b << "\n";
    a += b;
    std::cout << a << " " << (a == Vec2<int>(4, 6)) << " " << a.dot(b) << "\n";
    std::cout << 2 * b << "\n";
    Vec2<double> d(0.5, 1.5);
    std::cout << d + d << "\n";
    int i = 0;
    std::cout << overload(i) << overload(&i) << overload('c') << "\n";
    List<int> l;
    for (int k = 1; k <= 5; k++) l.push(k * k);
    std::cout << l.size() << " " << l.sum() << "\n";
    l.each(printer);
    std::cout << "\n";
    List<std::string> ls;
    ls.push("a");
    ls.push("bc");
    std::cout << ls.sum() << "\n";
    return 0;
}
