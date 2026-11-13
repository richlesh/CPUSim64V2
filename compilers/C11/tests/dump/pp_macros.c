#include <stdio.h>
#include "a.h"
#define SQ(x) ((x)*(x))
#define STR(x) #x
#define XSTR(x) STR(x)
#define CAT(a,b) a##b
#define LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)
int CAT(foo,bar) = SQ(1+2);
char *s = XSTR(VERSION) STR(a  "b\n" '"');
#if defined(VERSION) && VERSION >= 3
int yes = 1;
#elif 1
int no = 1;
#else
int never;
#endif
#ifdef UNDEFINED
bad
#endif
#ifndef UNDEFINED
int line = __LINE__;
#endif
LOG("x"); LOG("%d", 1, 2);
#define f(a) a + f(a)
f(f(z));
x = 0x1fUL + 012 + 1.5e3 + 'a' + '\n' + .5;
