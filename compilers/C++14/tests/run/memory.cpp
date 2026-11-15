#include <cstdio>
#include <cstdlib>

int main()
{
    int *p = new int;
    *p = 41;
    printf("%d\n", *p + 1);
    delete p;
    int *q = new int(7);
    printf("%d\n", *q);
    delete q;
    int *arr = new int[5];
    for (int i = 0; i < 5; i++) arr[i] = i * i;
    int s = 0;
    for (int i = 0; i < 5; i++) s += arr[i];
    printf("%d\n", s);
    delete[] arr;
    double *d = new double(2.5);
    printf("%.1f\n", *d * 2);
    delete d;
    char *str = (char *)malloc(8);
    str[0] = 'o'; str[1] = 'k'; str[2] = 0;
    printf("%s\n", str);
    free(str);
    int *z = new int{};
    printf("%d\n", *z);
    delete z;
    return 0;
}
