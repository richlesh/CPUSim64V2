/* One file with -lstd: the library functions come from the library modules
   instead of being appended to the program. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

int main(void) {
    char s[20];
    strcpy(s, "lstd");
    s[0] = (char)toupper(s[0]);
    printf("%s %d %.4f\n", s, (int)strlen(s), sqrt(2.0));
    return 3;
}
