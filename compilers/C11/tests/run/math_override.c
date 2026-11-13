#include <stdio.h>
#include <math>
/* User definitions with library names replace the library versions. */
double fmax(double a, double b) { return a + b; }
double hypot(double a, double b) { return a * b; }
int main(void) {
    printf("%.1f %.1f %.1f\n", fmax(2.0, 3.0), hypot(3.0, 4.0), fmin(2.0, 3.0));
    return 0;
}
