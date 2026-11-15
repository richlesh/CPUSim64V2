#ifndef _CPPC_CTYPE_H
#define _CPPC_CTYPE_H
#pragma cppc c_include <ctype.h>
extern "C" {
int isalpha(int c);
int isdigit(int c);
int isalnum(int c);
int isspace(int c);
int isupper(int c);
int islower(int c);
int ispunct(int c);
int isprint(int c);
int toupper(int c);
int tolower(int c);
}
#endif
