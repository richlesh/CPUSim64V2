class A { int x; public: A() : x(1) {} };
class B : public A { public: int get() { return x; } };
int main() { return 0; }
