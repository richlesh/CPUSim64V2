/* module with _Thread_local objects of external and internal linkage */
#include "multi_tls.h"

_Thread_local int tls_counter = 100;
_Thread_local int tls_table[4] = { 1, 2, 3, 4 };
_Thread_local const char *tls_label = "module a";
static _Thread_local int hidden = 50;           /* not visible to other modules */
_Thread_local int tls_own_a = 5;                /* an object only this module uses */

int tls_bump(int n)
{
    tls_counter += n;
    tls_own_a += n;
    return tls_counter;
}

int tls_hidden_bump(void)
{
    static _Thread_local int calls = 1000;
    hidden++;
    return hidden + ++calls + tls_own_a;
}

int tls_other(void)
{
    return tls_own;                             /* object of the main module */
}
