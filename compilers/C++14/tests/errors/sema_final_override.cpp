struct A { virtual void f() final; };
struct B : A { void f(); };
int main() { return 0; }
