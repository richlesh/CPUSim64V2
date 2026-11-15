struct A { virtual void f() = 0; };
struct B : A { };
int main() {
    B *b = new B;
    return 0;
}
