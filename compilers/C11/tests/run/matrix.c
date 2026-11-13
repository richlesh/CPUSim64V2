#include <stdio.h>
#define N 3
typedef double Mat[N][N];
void mul(Mat a, Mat b, Mat c) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            double s = 0;
            for (int k = 0; k < N; k++) s += a[i][k] * b[k][j];
            c[i][j] = s;
        }
}
int main(void) {
    Mat a = { { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } };
    Mat id = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
    Mat c, d;
    mul(a, id, c);
    mul(c, a, d);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) printf("%6.1f", d[i][j]);
        printf("\n");
    }
    int im[2][3] = { { 1, 2, 3 }, { 4, 5, 6 } }, t[3][2];
    for (int i = 0; i < 2; i++) for (int j = 0; j < 3; j++) t[j][i] = im[i][j];
    for (int j = 0; j < 3; j++) printf("%d %d\n", t[j][0], t[j][1]);
    printf("%d %d\n", (int)sizeof(Mat), (int)sizeof(t[0]));
    return 0;
}
