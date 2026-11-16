/* every kind of access to a thread-local object, in main */
#include <stdio.h>
#include <string.h>

_Thread_local int i = 1;
_Thread_local long l = 2;
_Thread_local short s = 3;
_Thread_local unsigned char uc = 255;
_Thread_local double f = 1.5;
_Thread_local double d = 2.5;
_Thread_local char *p;
_Thread_local int arr[4] = { 10, 20, 30, 40 };
_Thread_local struct { int a; char b; double c; } st = { 5, 'x', 0.25 };
_Thread_local _Bool b;
_Thread_local int *ip;                  /* &arr is not a constant expression: set in main */

static int read_i(void) { return i; }
static void write_i(int v) { i = v; }

int main(void)
{
    int k, *q;
    int a1, a2, a3, a4;
    ip = arr + 1;
    i += 5; i *= 2; i -= 1; i <<= 1; i |= 1; i ^= 2; i %= 100;
    printf("i=%d\n", i);
    a1 = i++; a2 = ++i; a3 = i--; a4 = --i;
    printf("i++=%d ++i=%d i--=%d --i=%d i=%d\n", a1, a2, a3, a4, i);
    l = l * 1000000000L + i;
    s -= 10;
    uc--;
    f += 0.25;
    d = d * d - f;
    printf("l=%ld s=%d uc=%d f=%g d=%g\n", l, s, uc, f, d);
    p = "text";
    p++;
    printf("p=%s *p=%c\n", p, *p);
    for (k = 0; k < 4; k++) arr[k] += k;
    q = arr;
    printf("arr=%d,%d,%d,%d *ip=%d ip-arr=%d q[2]=%d sizeof=%d\n", arr[0], arr[1], arr[2], arr[3], *ip,
           (int)(ip - arr), q[2], (int)sizeof(arr));
    ip[1]++;
    *ip += 100;
    printf("arr=%d,%d,%d,%d\n", arr[0], arr[1], arr[2], arr[3]);
    st.a += 2; st.b++; st.c *= 2;
    printf("st=%d,%c,%g\n", st.a, st.b, st.c);
    b = 5;
    printf("b=%d\n", b);
    b += 0;
    b--;
    printf("b=%d\n", b);
    write_i(read_i() + 1000);
    printf("i=%d cond=%d logic=%d\n", i, i > 100 ? 1 : 2, i && d && !b);
    memset(arr, 0, sizeof(arr));
    printf("arr=%d,%d,%d,%d\n", arr[0], arr[1], arr[2], arr[3]);
    return i % 256;
}
