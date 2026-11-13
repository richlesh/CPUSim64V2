#include <stdio.h>
#include <string.h>

/* printf family conformance.  Only types that are 64-bit both natively and
   on CPUSim64 (long, unsigned long, double, pointers) are used with
   length-sensitive conversions. */

int main(void) {
    int n;
    char buf[200];

    /* integers: flags, width, precision */
    printf("[%d] [%i] [%5d] [%-5d] [%05d] [%+d] [% d] [%+5d] [%-+5d]\n", 42, -42, 42, 42, 42, 42, 42, -7, 7);
    printf("[%.3d] [%8.3d] [%-8.3d] [%08.3d] [%.0d] [%.0d|] [%5.0d]\n", 7, -7, 7, 7, 0, 1, 0);
    printf("[%ld] [%ld] [%lu] [%lx] [%lX] [%lo]\n", 9223372036854775807L, -9223372036854775807L - 1,
           18446744073709551615UL, 18446744073709551615UL, 3054198966UL, 511UL);
    printf("[%x] [%X] [%#x] [%#X] [%#o] [%#o] [%o] [%#x]\n", 255, 255, 255, 255, 8, 0, 8, 0);
    printf("[%#10x] [%#-10x] [%#010x] [%10.5x]\n", 255, 255, 255, 255);
    printf("[%hd] [%hhd] [%hu] [%hhu] [%hx]\n", 70000, 300, 70000, 300, 0x12345);
    printf("[%*d] [%-*d] [%*d] [%.*d] [%*.*d]\n", 6, 1, 6, 2, -6, 3, 4, 5, 8, 3, -9);

    /* characters and strings */
    printf("[%c] [%3c] [%-3c] [%s] [%8s] [%-8s] [%.2s] [%8.3s] [%.0s]\n", 'x', 'y', 'z',
           "str", "right", "left", "truncate", "abcdef", "gone");
    printf("[%*s] [%-*s] [%.*s]\n", 7, "w", 7, "w", 3, "precision");
    char word[] = "array";
    printf("[%s] [%10s] [%c%c]\n", word, word, word[0], word[4]);
    printf("100%% [%%] [%5%]\n");

    /* floating point: %f */
    printf("[%f] [%.0f] [%.1f] [%.2f] [%.10f]\n", 3.14159265358979, 2.5, 0.05, 1.005, 1.0 / 3);
    printf("[%f] [%.0f] [%.0f] [%.0f] [%.0f] [%.1f] [%.1f]\n", -0.0, 0.5, 1.5, 2.5, -3.5, 0.25, 0.35);
    printf("[%10.3f] [%-10.3f] [%010.3f] [%+.2f] [% .2f] [%#.0f] [%+010.2f]\n",
           3.14159, 3.14159, -3.14159, 2.0, 2.0, 5.0, -1.5);
    printf("[%f] [%.3f] [%.20f]\n", 1e20, 123456789.0005, 0.1);
    printf("[%.0f] [%f]\n", 1.7976931348623157e308 / 1e290, 2.2250738585072014e-308);
    printf("[%.15f]\n", 5e-324 * 1e320);

    /* floating point: %e */
    printf("[%e] [%.0e] [%.3e] [%E] [%e] [%e]\n", 12345.6789, 15000.0, 0.000123456, 1e-10, 0.0, -1e100);
    printf("[%12.4e] [%-12.2e] [%+e] [%#.0e] [%e] [%e]\n", 3.14159, 3.14159, 1.0, 3.0, 1e300, 5e-324);
    printf("[%.2e] [%.0e] [%.1e]\n", 9.995, 9.5, 0.95);

    /* floating point: %g */
    printf("[%g] [%g] [%g] [%g] [%g] [%g] [%g]\n", 0.0, 1.0, 0.5, 100000.0, 1000000.0, 1e-4, 1e-5);
    printf("[%g] [%g] [%G] [%g] [%.3g] [%.10g] [%#g] [%#.3g]\n", 123456.0, 1234567.0, 1e-20,
           3.14159265358979, 3.14159, 1.0 / 3, 1.0, 100.0);
    printf("[%.0g] [%.1g] [%10.4g] [%-10.4g] [%g] [%g]\n", 0.0001234, 25.0, 3.14159, 3.14159,
           2.5e-310, 1.7976931348623157e308);

    /* special values */
    double inf = 1.0 / 0.0, nan = 0.0 / 0.0;
    printf("[%f] [%f] [%e] [%g] [%F] [%E] [%5.1f] [%-6f] [%+f]\n", inf, -inf, inf, -inf, inf, inf, inf, inf, inf);
    printf("[%f] [%E] [%g]\n", nan, nan, nan);

    /* hexadecimal floating point */
    printf("[%a] [%a] [%a] [%A] [%a] [%.2a] [%.0a] [%a]\n", 1.0, 0.5, -10.0, 255.5, 0.1, 1.0 / 3, 1.0, 0.0);

    /* pointers and %n */
    int x;
    char *p = 0;
    printf("[%p]%n\n", (void *)0, &n);
    printf("n=%d\n", n);
    printf("abc%nxyz\n", &n);
    printf("n=%d\n", n);
    sprintf(buf, "%p", (void *)&x);
    printf("ptr prefix=%c%c ok=%d\n", buf[0], buf[1], buf[0] == '0' && buf[1] == 'x');

    /* return values, sprintf, snprintf */
    int r = printf("%s=%d\n", "len", 12345);
    printf("printf returned %d\n", r);
    r = sprintf(buf, "%d-%s-%.2f", 42, "xy", 1.5);
    printf("sprintf [%s] %d %d\n", buf, r, (int)strlen(buf));
    r = snprintf(buf, 6, "%s", "truncated");
    printf("snprintf [%s] %d\n", buf, r);
    r = snprintf(buf, 1, "%d", 123);
    printf("snprintf1 [%s] %d\n", buf, r);
    r = snprintf(NULL, 0, "%d %s", 1234, "abc");
    printf("snprintf0 %d\n", r);
    char big[600];
    r = sprintf(big, "%300d|%-250s|", 7, "x");
    printf("long %d %d %c\n", r, (int)strlen(big), big[299]);
    fprintf(stdout, "fprintf %s %d\n", "stdout", 1);
    fprintf(stderr, "to stderr\n");
    /* field widths count characters (one per char word), not UTF-8 bytes */
    printf("unicode: %s %5s|\n", "h\u00e9llo \u4e16\u754c", "\u00fc");
    return 0;
}
