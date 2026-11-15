struct A { int x; };
struct B : A {};
int main() {
    A a;
    B *b = dynamic_cast<B *>(&a);
    return 0;
}
