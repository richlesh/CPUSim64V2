#include <stdio.h>
#include <string.h>

int main(void)
{
    printf("%d\n", __LINE__);
#line 100 "line_directive.c"
    printf("%d\n", __LINE__);
    printf("%d\n", __LINE__);
#line 500
    printf("%d\n", __LINE__);
    printf("%d\n", strlen(__FILE__) > 0);
    return 0;
}
