/*
 * ctype.h - character classification for the CPUSim64 C compiler.
 *
 * Implemented in C in lib/ctype.c; only used functions are compiled.  A char
 * holds a Unicode codepoint; the classes are those of the "C" locale (ASCII),
 * so every codepoint above 127 is in none of the classes and is returned
 * unchanged by toupper and tolower.
 */
#ifndef _CPUSIM64_CTYPE_H
#define _CPUSIM64_CTYPE_H

int isalnum(int c);
int isalpha(int c);
int isblank(int c);
int iscntrl(int c);
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
int tolower(int c);
int toupper(int c);

#endif
