#include <cstdio>

int g1 = 5;
int g2 = g1 + 1;
const int g3 = 7;
static int g4 = 3 * 4;
double g5 = 1.5;
const char *g6 = "global";
int garr[3] = {1, 2, 3};
int zero;
int twice(int x) { return 2 * x; }
int g7 = twice(21);
int *g8 = new int(99);

int counter()
{
    static int n = twice(1);
    n += 1;
    return n;
}

int main()
{
    printf("%d %d %d %d %.1f %s\n", g1, g2, g3, g4, g5, g6);
    printf("%d %d %d %d\n", garr[0], garr[1], garr[2], zero);
    printf("%d %d\n", g7, *g8);
    counter();
    printf("%d\n", counter());
    g1 = 100;
    printf("%d\n", g1 + g3);
    return 0;
}
