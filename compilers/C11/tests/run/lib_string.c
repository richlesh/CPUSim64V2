#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int sgn(int v) { return v < 0 ? -1 : v > 0 ? 1 : 0; }

int main(void) {
    char buf[64], buf2[64];

    /* copying */
    strcpy(buf, "hello");
    printf("strcpy=%s len=%d\n", buf, (int)strlen(buf));
    strncpy(buf2, "abc", 6);
    printf("strncpy=%s pad=%d%d%d\n", buf2, buf2[3], buf2[4], buf2[5]);
    strncpy(buf2, "abcdef", 3);
    buf2[3] = 0;
    printf("strncpy3=%s\n", buf2);
    memcpy(buf2, "0123456789", 11);
    memmove(buf2 + 2, buf2, 5);
    printf("memmove fwd=%s\n", buf2);
    memcpy(buf2, "0123456789", 11);
    memmove(buf2, buf2 + 3, 5);
    printf("memmove back=%s\n", buf2);
    char *d = strdup("duplicate");
    printf("strdup=%s\n", d);
    free(d);
    d = strndup("truncated", 5);
    printf("strndup=%s\n", d);
    free(d);

    /* concatenation */
    strcpy(buf, "foo");
    strcat(buf, "bar");
    strncat(buf, "bazqux", 3);
    printf("cat=%s\n", buf);

    /* comparison */
    printf("strcmp %d %d %d %d\n", sgn(strcmp("abc", "abd")), sgn(strcmp("abc", "abc")),
           sgn(strcmp("b", "a")), sgn(strcmp("ab", "abc")));
    printf("strncmp %d %d\n", sgn(strncmp("abcX", "abcY", 3)), sgn(strncmp("abcX", "abcY", 4)));
    printf("memcmp %d %d\n", sgn(memcmp("aaa", "aab", 3)), sgn(memcmp("aaa", "aab", 2)));
    printf("strcoll %d\n", sgn(strcoll("x", "y")));
    int xn = (int)strxfrm(buf2, "xfrm", sizeof buf2);
    printf("strxfrm %d %s\n", xn, buf2);

    /* searching */
    const char *s = "the quick brown fox";
    printf("strchr=%s\n", strchr(s, 'q'));
    printf("strrchr=%s\n", strrchr(s, 'o'));
    printf("strchr nul=%d\n", strchr(s, 0) == s + strlen(s));
    printf("strchr none=%d\n", strchr(s, 'z') == NULL);
    printf("strstr=%s\n", strstr(s, "brown"));
    printf("strstr none=%d empty=%d\n", strstr(s, "cat") == NULL, strstr(s, "") == s);
    printf("memchr=%s\n", (char *)memchr(s, 'b', strlen(s)));
    printf("memchr none=%d\n", memchr(s, 'q', 3) == NULL);
    printf("strspn=%d strcspn=%d\n", (int)strspn("aabbcxyz", "abc"), (int)strcspn("hello, world", ",;"));
    printf("strpbrk=%s none=%d\n", strpbrk("key=value", "=:"), strpbrk("abc", "xyz") == NULL);

    /* tokenizing */
    char text[] = "  alpha, beta;;gamma ,delta  ";
    char *tok = strtok(text, " ,;");
    while (tok) {
        printf("[%s]", tok);
        tok = strtok(NULL, " ,;");
    }
    printf("\n");
    char text2[] = "a:b:c", *save, *t2;
    for (t2 = strtok_r(text2, ":", &save); t2; t2 = strtok_r(NULL, ":", &save))
        printf("<%s>", t2);
    printf("\n");

    /* miscellaneous */
    memset(buf, 'z', 4);
    buf[4] = 0;
    printf("memset=%s\n", buf);
    printf("strnlen=%d %d\n", (int)strnlen("hello", 3), (int)strnlen("hi", 10));
    printf("strerror ok=%d\n", strerror(0) != NULL);
    return 0;
}
