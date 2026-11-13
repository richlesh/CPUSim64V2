///////////////////////////////////////////////////////////////////////////////
// prelude.asm
//
// Built-in C source preprocessed before every translation unit.  It defines
// the standard macros and types and declares the C library.  Functions that
// are only declared here are implemented in assembly by runtime_emit.asm;
// functions defined here in C are compiled only when a program uses them.
// Tokens from the prelude carry the file name "<built-in>".
//
//   prelude_source() -> wstr of the prelude text
///////////////////////////////////////////////////////////////////////////////

PRELUDE_ASM_START:
    jump    PRELUDE_ASM_END

// dcs_lines_to_w(addr) -> wstr joining .dcs lines (terminated by .dci -1)
#def_func dcs_lines_to_w(ap)
    #var    a, sb
    load    a, ap
    #call   vec_new()
    move    sb, r0
    #while  TRUE
        load    r0, a[0]
        #if_cond r0 < 0
            #break
        #end_cond
        #call   sb_add_p(sb, a)
        #call   vec_push(sb, '\n')
        #call   dcs_next(a)
        move    a, r0
    #end_while
    #call   sb_to_w(sb)
#end_func

#def_func prelude_source()
    #call   dcs_lines_to_w(PRELUDE_SRC)
#end_func

PRELUDE_SRC:
    .dcs    "#define __CPUSIM64__ 1"
    .dcs    "#define __STDC__ 1"
    .dcs    "#define __STDC_VERSION__ 201112L"
    .dcs    "#define __STDC_HOSTED__ 1"
    .dcs    "#define NULL ((void *)0)"
    .dcs    "#define EOF (-1)"
    .dcs    "#define CHAR_BIT 64"
    .dcs    "#define CHAR_MIN 0"
    .dcs    "#define CHAR_MAX 1114111"
    .dcs    "#define SCHAR_MIN (-128)"
    .dcs    "#define SCHAR_MAX 127"
    .dcs    "#define UCHAR_MAX 255"
    .dcs    "#define SHRT_MIN (-32767-1)"
    .dcs    "#define SHRT_MAX 32767"
    .dcs    "#define USHRT_MAX 65535"
    .dcs    "#define INT_MAX 0x7fffffffffffffff"
    .dcs    "#define INT_MIN (-INT_MAX-1)"
    .dcs    "#define UINT_MAX 0xffffffffffffffffU"
    .dcs    "#define LONG_MAX INT_MAX"
    .dcs    "#define LONG_MIN INT_MIN"
    .dcs    "#define ULONG_MAX UINT_MAX"
    .dcs    "#define LLONG_MAX INT_MAX"
    .dcs    "#define LLONG_MIN INT_MIN"
    .dcs    "#define ULLONG_MAX UINT_MAX"
    .dcs    "#define SIZE_MAX UINT_MAX"
    .dcs    "#define DBL_MAX 1.7976931348623157e308"
    .dcs    "#define DBL_MIN 2.2250738585072014e-308"
    .dcs    "#define DBL_EPSILON 2.220446049250313e-16"
    .dcs    "#define va_start(ap, last) ((ap) = (char *)(&(last) + 1))"
    .dcs    "#define va_arg(ap, T) (*(T *)(((ap) = (ap) + sizeof(T)) - sizeof(T)))"
    .dcs    "#define va_end(ap) ((void)0)"
    .dcs    "#define va_copy(d, s) ((d) = (s))"
    .dcs    "#define offsetof(T, m) ((size_t)&((T *)0)->m)"
    .dcs    "#define assert(e) ((e) ? (void)0 : __assert_fail(#e, __FILE__, __LINE__))"
    .dcs    "typedef unsigned long size_t;"
    .dcs    "typedef long ssize_t;"
    .dcs    "typedef long ptrdiff_t;"
    .dcs    "typedef long intptr_t;"
    .dcs    "typedef unsigned long uintptr_t;"
    .dcs    "typedef long intmax_t;"
    .dcs    "typedef unsigned long uintmax_t;"
    .dcs    "typedef long int8_t;"
    .dcs    "typedef long int16_t;"
    .dcs    "typedef long int32_t;"
    .dcs    "typedef long int64_t;"
    .dcs    "typedef unsigned long uint8_t;"
    .dcs    "typedef unsigned long uint16_t;"
    .dcs    "typedef unsigned long uint32_t;"
    .dcs    "typedef unsigned long uint64_t;"
    .dcs    "typedef int wchar_t;"
    .dcs    "typedef char *va_list;"
    .dcs    "double __atof_exact(const char *s);"
    .dcs    "void __assert_fail(__pstr e, __pstr file, int line);"
    .dcs    "int isdigit(int c) { return c >= '0' && c <= '9'; }"
    .dcs    "int isalpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }"
    .dcs    "int isalnum(int c) { return isalpha(c) || isdigit(c); }"
    .dcs    "int isspace(int c) { return c == ' ' || (c >= 9 && c <= 13); }"
    .dcs    "int isupper(int c) { return c >= 'A' && c <= 'Z'; }"
    .dcs    "int islower(int c) { return c >= 'a' && c <= 'z'; }"
    .dcs    "int isxdigit(int c) { return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }"
    .dcs    "int isprint(int c) { return c >= 32 && c < 127; }"
    .dcs    "int isgraph(int c) { return c > 32 && c < 127; }"
    .dcs    "int iscntrl(int c) { return (c >= 0 && c < 32) || c == 127; }"
    .dcs    "int ispunct(int c) { return isgraph(c) && !isalnum(c); }"
    .dcs    "int toupper(int c) { return islower(c) ? c - 32 : c; }"
    .dcs    "int tolower(int c) { return isupper(c) ? c + 32 : c; }"
    .dci    -1

PRELUDE_ASM_END:
    nop
