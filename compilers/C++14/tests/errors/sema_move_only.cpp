struct M { M() {} M(M &&) {} };
int main() {
    M a;
    M b = a;
    return 0;
}
