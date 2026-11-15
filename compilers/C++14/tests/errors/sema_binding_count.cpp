struct P { int a, b; };
int main() {
    P p{1, 2};
    auto [x] = p;
    return x;
}
