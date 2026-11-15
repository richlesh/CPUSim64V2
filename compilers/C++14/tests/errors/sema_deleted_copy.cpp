struct M { M() {} M(const M &) = delete; };
int main() {
    M a;
    M b = a;
    return 0;
}
