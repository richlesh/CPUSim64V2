template<class T> struct A { void g() { T::x(); } };
int main() {
    A<int> a;
    a.g();
}
