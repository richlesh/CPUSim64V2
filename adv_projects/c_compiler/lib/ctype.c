/*
 * ctype.c - C implementations for lib/ctype.h ("C" locale).
 *
 * Appended to the program by the compiler; only used functions are
 * compiled.  Parameter names start with __ct_ because this file shares the
 * program's macro namespace.
 */
#include <ctype.h>

int isdigit(int __ct_c) { return __ct_c >= '0' && __ct_c <= '9'; }
int isupper(int __ct_c) { return __ct_c >= 'A' && __ct_c <= 'Z'; }
int islower(int __ct_c) { return __ct_c >= 'a' && __ct_c <= 'z'; }
int isalpha(int __ct_c) { return isupper(__ct_c) || islower(__ct_c); }
int isalnum(int __ct_c) { return isalpha(__ct_c) || isdigit(__ct_c); }
int isxdigit(int __ct_c) {
    return isdigit(__ct_c) || (__ct_c >= 'a' && __ct_c <= 'f') || (__ct_c >= 'A' && __ct_c <= 'F');
}
int isspace(int __ct_c) { return __ct_c == ' ' || (__ct_c >= 9 && __ct_c <= 13); }
int isblank(int __ct_c) { return __ct_c == ' ' || __ct_c == '\t'; }
int iscntrl(int __ct_c) { return (__ct_c >= 0 && __ct_c < 32) || __ct_c == 127; }
int isprint(int __ct_c) { return __ct_c >= 32 && __ct_c < 127; }
int isgraph(int __ct_c) { return __ct_c > 32 && __ct_c < 127; }
int ispunct(int __ct_c) { return isgraph(__ct_c) && !isalnum(__ct_c); }
int tolower(int __ct_c) { return isupper(__ct_c) ? __ct_c + 32 : __ct_c; }
int toupper(int __ct_c) { return islower(__ct_c) ? __ct_c - 32 : __ct_c; }
