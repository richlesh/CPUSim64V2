#include <cstdio>

void swap_ref(int &a, int &b) { int t = a; a = b; b = t; }
int &larger(int &a, int &b) { return a > b ? a : b; }
int sum_ref(const int &a, const int &b) { return a + b; }
void inc(int &x) { ++x; }
void fill(int (&arr)[4], int v) { for (int i = 0; i < 4; i++) arr[i] = v + i; }
double half(const double &d) { return d / 2; }
int global_value = 10;
int &get_global() { return global_value; }

int main()
{
    int x = 3, y = 9;
    swap_ref(x, y);
    printf("%d %d\n", x, y);
    larger(x, y) = 100;
    printf("%d %d\n", x, y);
    printf("%d %d\n", sum_ref(x, y), sum_ref(4, 5));
    int &r = x;
    r += 5;
    inc(r);
    printf("%d %d\n", x, r);
    const int &cr = 42;
    printf("%d\n", cr);
    int arr[4];
    fill(arr, 10);
    printf("%d %d %d %d\n", arr[0], arr[1], arr[2], arr[3]);
    printf("%.2f %.2f\n", half(5.0), half(3));
    get_global() = 77;
    printf("%d\n", global_value);
    int &gr = get_global();
    gr++;
    printf("%d\n", get_global());
    int *p = &x;
    int *&pr = p;
    *pr = 1;
    printf("%d\n", x);
    return 0;
}
