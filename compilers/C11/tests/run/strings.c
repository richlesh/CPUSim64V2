#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
int main(void) {
    char name[] = "world";
    printf("Hello, %s!\n", "world");
    printf("Hello, %s! (%d chars)\n", name, (int)strlen(name));
    puts("puts literal");
    puts(name);
    char *lit = "literal as char*";
    printf("%s %c %d\n", lit, lit[3], (int)strlen(lit));
    char buf[64];
    int n = sprintf(buf, "%d-%s-%c", 42, name, 'z');
    printf("[%s] %d\n", buf, n);
    char *d = strdup("dup");
    d[0] = toupper(d[0]);
    printf("%s %s\n", d, strchr("abcdef", 'd'));
    free(d);
    printf("%d %d %d\n", atoi("123"), atoi("-45x"), (int)strtol("ff", 0, 16));
    printf("%s|%5d|%-5d|%05d|%x|%c\n", "fmt", 7, 7, 7, 255, 65);
    printf("%ld %lu %i %%\n", 10L, 11UL, 12);
    printf("\u00e9\t\"q\"\\\n");
    char line[3][8] = { "ab", "cd", "ef" };
    for (int i = 0; i < 3; i++) printf("%s", line[i]);
    printf("\n%s\n", strstr("haystack", "st"));
    printf("%d %d\n", memcmp("abc", "abd", 3) < 0, isdigit('7') + isalpha('!'));
    putchar('O'); putchar('K'); putchar('\n');
    return 0;
}
