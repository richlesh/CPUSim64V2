#include <cstdio>
#include <utility>
#include <type_traits>
#include <string>

struct Base { virtual ~Base() {} };
struct Derived : Base {};
struct Plain { int x; };
enum E { e1 };

struct MoveOnly {
    int v;
    MoveOnly(int x) : v(x) {}
    MoveOnly(MoveOnly &&o) : v(o.v) { o.v = 0; }
    MoveOnly &operator=(MoveOnly &&o) { v = o.v; o.v = 0; return *this; }
    MoveOnly(const MoveOnly &) = delete;
};

template<class T> void check(const char *name)
{
    printf("%s: int=%d ptr=%d class=%d enum=%d signed=%d arith=%d\n", name, (int)std::is_integral<T>::value, (int)std::is_pointer<T>::value,
           (int)std::is_class<T>::value, (int)std::is_enum<T>::value, (int)std::is_signed<T>::value, (int)std::is_arithmetic<T>::value);
}

int main()
{
    check<int>("int");
    check<unsigned>("unsigned");
    check<double>("double");
    check<int *>("int*");
    check<Plain>("Plain");
    check<E>("E");
    printf("%d %d\n", (int)std::is_same<int, int>::value, (int)std::is_same<int, const int>::value);
    printf("%d\n", (int)std::is_same<std::decay<const int &>::type, int>::value);
    printf("%d\n", (int)std::is_same<std::remove_reference<int &&>::type, int>::value);
    printf("%d %d\n", (int)std::is_base_of<Base, Derived>::value, (int)std::is_base_of<Derived, Base>::value);
    printf("%d %d\n", (int)std::is_polymorphic<Base>::value, (int)std::is_polymorphic<Plain>::value);
    printf("%d\n", (int)std::is_same<std::conditional<true, int, char>::type, int>::value);
    int a = 1, b = 2;
    std::swap(a, b);
    printf("%d %d\n", a, b);
    std::pair<int, std::string> p(3, "three");
    std::pair<int, std::string> q = std::make_pair(4, std::string("four"));
    printf("%d %s %d %s\n", p.first, p.second.c_str(), q.first, q.second.c_str());
    std::swap(p, q);
    printf("%d %s\n", p.first, p.second.c_str());
    printf("%d %d %d\n", (int)(p < q), (int)(p == p), (int)(q != p));
    MoveOnly m1(5);
    MoveOnly m2 = std::move(m1);
    printf("%d %d\n", m1.v, m2.v);
    std::string s = "moved";
    std::string t = std::move(s);
    printf("[%s] [%s]\n", s.c_str(), t.c_str());
    int arr1[2] = {1, 2}, arr2[2] = {3, 4};
    std::swap(arr1, arr2);
    printf("%d %d\n", arr1[0], arr2[1]);
    std::pair<std::string, std::pair<int, int>> nested("k", std::make_pair(1, 2));
    printf("%s %d\n", nested.first.c_str(), nested.second.second);
    return 0;
}
