class Base {
public:
    Base() {}
    Base(int x) : v(x) {}
    virtual ~Base() {}
    virtual int get() const { return v; }
    virtual void pure() = 0;
    static int count;
    int v;
protected:
    int prot;
private:
    int priv = 5;
    int arr[4] = {1, 2, 3, 4};
    friend class Other;
    friend int helper(Base &);
};
class Other;
struct Derived : public Base, private Other2 {
    Derived() : Base(1), w(2) {}
    Derived(const Derived &) = default;
    Derived &operator=(const Derived &) = delete;
    int get() const override { return w; }
    void pure() override final {}
    operator int() const { return w; }
    explicit operator bool() const { return w != 0; }
    Derived operator+(const Derived &o) const { return *this; }
    int &operator[](int i) { return w; }
    bool operator==(const Derived &) const;
    Derived &operator++();
    Derived operator++(int);
    void *operator new(unsigned long n);
    void operator delete(void *p);
    ~Derived();
    int w : 3;
    unsigned : 0;
    mutable int cache;
    static const int K = 3;
    enum Kind { A, B, C } kind;
    struct Inner { int z; } inner;
    typedef int value_type;
    using size_type = unsigned long;
};
class Other2 { };
class Other { };
bool Derived::operator==(const Derived &o) const { return w == o.w; }
Derived &Derived::operator++() { ++w; return *this; }
Derived::~Derived() {}
int Base::count = 0;
union U { int i; float f; };
struct S { int x; S(int x) : x(x) {} };
struct Empty {};
enum class Mode : unsigned char { Off, On = 1, Auto };
enum Plain { P1, P2 = 5, P3 };
enum Fwd : int;
