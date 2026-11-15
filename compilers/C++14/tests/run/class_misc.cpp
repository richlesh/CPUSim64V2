#include <cstdio>

namespace geo {
    class Shape {
    public:
        enum Kind { Square, Circle };
        typedef int id_type;
        static const int MAX = 10;
        Shape(Kind k, id_type id) : kind_(k), id_(id) { ++count; }
        ~Shape() { --count; }
        Kind kind() const { return kind_; }
        id_type id() const { return id_; }
        static int count;
        int area(int s) const;
        struct Info { int w; int h; };
        Info info() const { Info i; i.w = 2; i.h = 3; return i; }
    private:
        Kind kind_;
        id_type id_;
        friend int peek(const Shape &s);
        friend class Spy;
    };
    int Shape::count = 0;
    int Shape::area(int s) const { return kind_ == Square ? s * s : 3 * s * s; }
    int peek(const Shape &s) { return s.id_ * 100; }
    class Spy { public: static int look(const Shape &s) { return s.id_ + 1; } };
}

struct Node {
    int val;
    Node *next;
    Node(int v, Node *n = nullptr) : val(v), next(n) {}
};

struct Stack {
    Node *top = nullptr;
    int size = 0;
    ~Stack() { while (top) { Node *n = top; top = top->next; delete n; } }
    void push(int v) { top = new Node(v, top); ++size; }
    int pop() { Node *n = top; int v = n->val; top = n->next; delete n; --size; return v; }
};

struct Rect {
    int w, h;
    int area() const { return w * h; }
    Rect scaled(int k) const { return Rect{w * k, h * k}; }
};

class Account {
    double balance;
    static int next_id;
    int id;
public:
    Account(double b = 0) : balance(b), id(next_id++) {}
    void deposit(double d) { balance += d; }
    double get() const { return balance; }
    int get_id() const { return id; }
};
int Account::next_id = 1;

int main()
{
    geo::Shape s(geo::Shape::Circle, 7);
    geo::Shape t(geo::Shape::Square, 8);
    printf("%d %d %d\n", s.kind() == geo::Shape::Circle, s.id(), geo::Shape::count);
    printf("%d %d\n", s.area(2), t.area(3));
    printf("%d %d\n", geo::peek(s), geo::Spy::look(t));
    geo::Shape::Info in = s.info();
    printf("%d %d %d\n", in.w, in.h, geo::Shape::MAX);
    {
        geo::Shape u(geo::Shape::Square, 9);
        printf("%d\n", geo::Shape::count);
    }
    printf("%d\n", geo::Shape::count);
    Stack st;
    for (int i = 1; i <= 4; i++) st.push(i * i);
    printf("%d\n", st.size);
    printf("%d\n", st.pop());
    Rect r{3, 4};
    printf("%d %d\n", r.area(), r.scaled(2).area());
    Account a1(10), a2;
    a1.deposit(5.5);
    printf("%.1f %.1f %d %d\n", a1.get(), a2.get(), a1.get_id(), a2.get_id());
    Rect *rp = new Rect{5, 6};
    printf("%d\n", rp->area());
    delete rp;
    return 0;
}
