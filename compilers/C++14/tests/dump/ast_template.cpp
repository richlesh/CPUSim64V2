template<class T, int N = 3> struct A { T v[N]; typedef T type; A<T, N> *self() const { return this; } };
A<int> x;
bool lt = sizeof(x) < 4, gt = sizeof(x) > 4;
