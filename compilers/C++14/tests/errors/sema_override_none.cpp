struct A { virtual void f(); };
struct B : A { void g() override; };
int main() { return 0; }
