#include <stdio.h>
#include <stdlib.h>

int main(void) {
    // getcwd() lives in POSIX <unistd.h>, which isn't part of standard C.
    // Shells usually set the PWD environment variable to the current
    // working directory, so we read that instead.
    const char *cwd = getenv("PWD");

    if (cwd != NULL && cwd[0] != '\0') {
        printf("Current working directory: %s\n", cwd);
        return EXIT_SUCCESS;
    }

    fprintf(stderr, "Unable to determine the current working directory "
                    "(PWD is not set).\n");
    return EXIT_FAILURE;
}