struct A {
    explicit A(int x) {}
};
void f(A a) {}
int main() {
    f(3);
    return 0;
}
