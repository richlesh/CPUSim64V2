#ifndef MULTI_H
#define MULTI_H
extern int counter;                 /* defined in multi_data.c */
extern int calls;                   /* defined in multi_util.c */
extern int table[3];
extern const char *name;
int add3(int x);
int util_helper(void);
void apply(int (*fn)(int), int x);
int twice(int x);                   /* defined in the main module */
#endif
