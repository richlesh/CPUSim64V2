/*
 * setjmp.h - non-local jumps for the CPUSim64 C compiler.
 *
 * Both functions are provided by the assembly runtime.  A jmp_buf holds the
 * caller's stack frame (SF), the return address and the stack pointer at
 * the point setjmp returns.  Generated code keeps every variable in memory,
 * so all variables have their latest values after longjmp (C only
 * guarantees this for volatile variables).
 *
 * As in C, longjmp must not be used after the function that called setjmp
 * has returned, and setjmp may only appear as the whole controlling
 * expression of if/switch/while/for, as an operand of ! or of a comparison
 * with an integer constant, or as an expression statement (assigning its
 * result to a variable also works here).  In other expressions partially
 * computed values can be lost when longjmp returns.
 */
#ifndef _CPUSIM64_SETJMP_H
#define _CPUSIM64_SETJMP_H

typedef long jmp_buf[3];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);    /* setjmp then returns val (1 if 0) */

#endif
