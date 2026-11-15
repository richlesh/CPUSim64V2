template<class T> struct S { static_assert(sizeof(T) > 100, "too small"); };
int main() {
    S<int> s;
}
