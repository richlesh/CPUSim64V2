#include <cstdio>
#include <cstring>

int main()
{
    printf("%s|%s|%s\n", "tab\there", "quote\"s", "back\\slash");
    printf("%s\n", "con" "cat" "enated");
    printf("%s\n", R"(raw \n "string")");
    printf("%d %d %d %d\n", 'a', '\n', '\0', '\x41');
    printf("%d %d %d %d\n", 0x1F, 017, 0b101, 1'000);
    printf("%ld %lu %lld\n", 100000L, 4000000000UL, 123456789012LL);
    printf("%.3f %.3f %.1f\n", 1.5, 2e-3, 10.0f);
    printf("%d\n", (int)sizeof("abc") == (int)strlen("abc") + 1);
    char s[] = "abc";
    printf("%d\n", (int)(sizeof(s) / sizeof(s[0])));
    const char *p = "multi"
                    "line";
    printf("%s %d\n", p, (int)strlen(p));
    printf("%c%c%c\n", 'a', 98, 'c' + 0);
    printf("%d %d\n", true, false);
    return 0;
}
