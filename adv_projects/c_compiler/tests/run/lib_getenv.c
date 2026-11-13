#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    /* CPUSIM64_TEST_VAR is exported by tests/run_tests.sh */
    char *a = getenv("CPUSIM64_TEST_VAR");
    char *b = getenv("CPUSIM64_TEST_VAR");
    printf("value=[%s] same=%d\n", a ? a : "(null)", a == b);

    char name[32];
    strcpy(name, "CPUSIM64_TEST_VAR");
    printf("copy of name: same=%d\n", getenv(name) == a);

    printf("unset=%d again=%d\n", getenv("CPUSIM64_NO_SUCH_VARIABLE") == NULL,
           getenv("CPUSIM64_NO_SUCH_VARIABLE") == NULL);
    char *path = getenv("PATH");
    printf("path set=%d\n", path != NULL && strlen(path) > 0);
    printf("a still valid: [%s]\n", a);
    return 0;
}
