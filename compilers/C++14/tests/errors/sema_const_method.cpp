struct A {
    int n;
    void set(int v) { n = v; }
};
int main() {
    const A a = {1};
    a.set(2);
    return 0;
}
