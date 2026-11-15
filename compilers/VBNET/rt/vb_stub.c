/*
 * vb_stub.c - placeholders for the functions that the compiler generates
 * for each program (vb_program, and the exception helpers of lib/System.vb).
 * build.sh compiles this file only so that the C11 compiler accepts vb_rt.c
 * as a module; the result is discarded.
 */
long vb_program(void) { return 0; }
long *vb_make_exception(long code, long *message) { return message + code; }
void vb_throw_obj(long *ex) { ex[0] = 0; }
