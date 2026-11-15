/*
 * sema_sym.c - symbols, scopes, namespaces, the file options and the
 * builtin members of the .NET library that the compiler knows.
 */

SymVec prog_modules;
SymVec prog_fields;
Sym *prog_main = 0;

void sv_push(SymVec *v, Sym *s) {
    if (v->n == v->cap) {
        v->cap = v->cap ? v->cap * 2 : 8;
        v->v = xrealloc(v->v, sizeof(Sym *) * v->cap);
    }
    v->v[v->n++] = s;
}

/* ------------------------------------------------------------ errors */

static void serr(Node *at, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(at ? at->file : -1, at ? at->line : 0, fmt, ap);
    va_end(ap);
}

static void not_yet(Node *at, const char *what) {
    serr(at, "%s not supported yet.", what);
}

/* ------------------------------------------------------------ file options */

typedef struct {
    int strict;
    int explicit_;
    int infer;
    int compare_text;
    SymVec imports;         /* imported namespaces and types */
    Name **alias_names;     /* Imports A = X */
    Sym **alias_syms;
    int naliases;
} Opts;

static Opts *file_opts;
static Opts *opts;          /* the options of the file being analyzed */

/* ------------------------------------------------------------ symbols and scopes */

static Sym *new_sym(int kind, Name *name, const char *spelling) {
    Sym *s = xalloc(sizeof(Sym));
    s->kind = kind;
    s->name = name;
    s->spelling = spelling ? spelling : (name ? name->key : "");
    return s;
}

static Scope *new_scope(int kind, Scope *parent, Sym *owner) {
    Scope *sc = xalloc(sizeof(Scope));
    sc->kind = kind;
    sc->parent = parent;
    sc->owner = owner;
    return sc;
}

enum { SC_NS, SC_MODULE, SC_TYPE, SC_METHOD, SC_BLOCK, SC_LAMBDA };

static Sym *scope_find(Scope *sc, Name *n) {
    Sym *s;
    if (!sc) return 0;
    for (s = sc->syms; s; s = s->next)
        if (s->name == n) return s;
    return 0;
}

/* adds at the end, so that members are kept in declaration order */
static void scope_add(Scope *sc, Sym *s) {
    Sym **p = &sc->syms;
    while (*p) p = &(*p)->next;
    s->next = 0;
    *p = s;
}

const char *sym_kind_name(Sym *s) {
    switch (s->kind) {
    case S_NAMESPACE: return "namespace";
    case S_MODULE: return "module";
    case S_TYPE: return "type";
    case S_METHOD: return "method";
    case S_FIELD: return "field";
    case S_CONST: return "const";
    case S_LOCAL: return (s->mods & M_STATIC) ? "static" : "local";
    case S_PARAM: return "param";
    case S_RETVAL: return "return value";
    }
    return "?";
}

static Sym *ns_global;

static Sym *ns_child(Sym *ns, Name *n, const char *spelling) {
    Sym *s = scope_find(ns->members, n);
    if (s) return s;
    s = new_sym(S_NAMESPACE, n, spelling);
    s->owner = ns;
    s->members = new_scope(SC_NS, 0, s);
    scope_add(ns->members, s);
    return s;
}

static Sym *ns_path(const char *path) {
    Sym *ns = ns_global;
    char buf[100];
    while (*path) {
        int k = 0;
        while (*path && *path != '.') buf[k++] = *path++;
        buf[k] = 0;
        if (*path == '.') path++;
        ns = ns_child(ns, intern_word(buf), xstrdup(buf));
    }
    return ns;
}

/* the qualified name of a namespace, module or type (.NET style: a type
   nested in a type or module is Outer+Inner) */
static const char *qual_name(Sym *s) {
    StrBuf b;
    const char *outer;
    if (!s->owner || s->owner == ns_global || s->owner->kind == S_METHOD) return s->spelling;
    outer = qual_name(s->owner);
    sb_init(&b);
    sb_puts(&b, outer);
    sb_putc(&b, s->kind == S_TYPE && (s->owner->kind == S_TYPE || s->owner->kind == S_MODULE) ? '+' : '.');
    sb_puts(&b, s->spelling);
    return b.s;
}

/* ------------------------------------------------------------ builtin members */

/* kinds of builtin members */
enum { BK_SHARED_METHOD = 1, BK_INSTANCE_METHOD, BK_INSTANCE_PROP, BK_CONST };

typedef struct {
    const char *ns;
    const char *type;
    const char *member;
    int kind;
    int id;
    const char *sig;    /* methods: "ret|name:type|..."; constants: "type|value" */
    const char *rt;     /* the run-time function (see Sym.rt) */
} Builtin;

/* type codes: v void, b Boolean, Y SByte, y Byte, h Short, H UShort,
   i Integer, I UInteger, l Long, L ULong, f Single, d Double, c Char,
   s String, o Object; a following '[' makes a 1-D array; '*' before the
   name marks a ParamArray, '&' a ByRef parameter */
static const Builtin builtins[] = {
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v" },
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v|value:b" },
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v|value:c" },
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v|buffer:c[" },
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v|value:d" },
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v|value:i" },
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v|value:l" },
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v|value:o" },
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v|value:f" },
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v|value:s" },
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v|value:I" },
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v|value:L" },
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v|format:s|arg0:o" },
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v|format:s|arg0:o|arg1:o" },
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v|format:s|arg0:o|arg1:o|arg2:o" },
    { "System", "Console", "WriteLine", BK_SHARED_METHOD, B_CONSOLE_WRITELINE, "v|format:s|*arg:o[" },
    { "System", "Console", "Write", BK_SHARED_METHOD, B_CONSOLE_WRITE, "v|value:b" },
    { "System", "Console", "Write", BK_SHARED_METHOD, B_CONSOLE_WRITE, "v|value:c" },
    { "System", "Console", "Write", BK_SHARED_METHOD, B_CONSOLE_WRITE, "v|buffer:c[" },
    { "System", "Console", "Write", BK_SHARED_METHOD, B_CONSOLE_WRITE, "v|value:d" },
    { "System", "Console", "Write", BK_SHARED_METHOD, B_CONSOLE_WRITE, "v|value:i" },
    { "System", "Console", "Write", BK_SHARED_METHOD, B_CONSOLE_WRITE, "v|value:l" },
    { "System", "Console", "Write", BK_SHARED_METHOD, B_CONSOLE_WRITE, "v|value:o" },
    { "System", "Console", "Write", BK_SHARED_METHOD, B_CONSOLE_WRITE, "v|value:f" },
    { "System", "Console", "Write", BK_SHARED_METHOD, B_CONSOLE_WRITE, "v|value:s" },
    { "System", "Console", "Write", BK_SHARED_METHOD, B_CONSOLE_WRITE, "v|value:I" },
    { "System", "Console", "Write", BK_SHARED_METHOD, B_CONSOLE_WRITE, "v|value:L" },
    { "System", "Console", "Write", BK_SHARED_METHOD, B_CONSOLE_WRITE, "v|format:s|arg0:o" },
    { "System", "Console", "Write", BK_SHARED_METHOD, B_CONSOLE_WRITE, "v|format:s|arg0:o|arg1:o" },
    { "System", "Console", "Write", BK_SHARED_METHOD, B_CONSOLE_WRITE, "v|format:s|arg0:o|arg1:o|arg2:o" },
    { "System", "Console", "Write", BK_SHARED_METHOD, B_CONSOLE_WRITE, "v|format:s|*arg:o[" },
    { "System", "Console", "ReadLine", BK_SHARED_METHOD, B_CONSOLE_READLINE, "s" },
    { "System", "String", "Length", BK_INSTANCE_PROP, B_STRING_LENGTH, "i" },
    { "System", "SByte", "MaxValue", BK_CONST, 0, "Y|127" },
    { "System", "SByte", "MinValue", BK_CONST, 0, "Y|-128" },
    { "System", "Byte", "MaxValue", BK_CONST, 0, "y|255" },
    { "System", "Byte", "MinValue", BK_CONST, 0, "y|0" },
    { "System", "Int16", "MaxValue", BK_CONST, 0, "h|32767" },
    { "System", "Int16", "MinValue", BK_CONST, 0, "h|-32768" },
    { "System", "UInt16", "MaxValue", BK_CONST, 0, "H|65535" },
    { "System", "UInt16", "MinValue", BK_CONST, 0, "H|0" },
    { "System", "Int32", "MaxValue", BK_CONST, 0, "i|2147483647" },
    { "System", "Int32", "MinValue", BK_CONST, 0, "i|-2147483648" },
    { "System", "UInt32", "MaxValue", BK_CONST, 0, "I|4294967295" },
    { "System", "UInt32", "MinValue", BK_CONST, 0, "I|0" },
    { "System", "Int64", "MaxValue", BK_CONST, 0, "l|9223372036854775807" },
    { "System", "Int64", "MinValue", BK_CONST, 0, "l|-9223372036854775808" },
    { "System", "UInt64", "MaxValue", BK_CONST, 0, "L|18446744073709551615" },
    { "System", "UInt64", "MinValue", BK_CONST, 0, "L|0" },
    { "System", "Double", "MaxValue", BK_CONST, 0, "d|1.7976931348623157E+308" },
    { "System", "Double", "MinValue", BK_CONST, 0, "d|-1.7976931348623157E+308" },
    { "System", "Double", "Epsilon", BK_CONST, 0, "d|4.94065645841247E-324" },
    { "System", "Double", "PositiveInfinity", BK_CONST, 0, "d|inf" },
    { "System", "Double", "NegativeInfinity", BK_CONST, 0, "d|-inf" },
    { "System", "Double", "NaN", BK_CONST, 0, "d|nan" },
    { "System", "Single", "MaxValue", BK_CONST, 0, "f|3.40282347E+38" },
    { "System", "Single", "MinValue", BK_CONST, 0, "f|-3.40282347E+38" },
    { "System", "Char", "MaxValue", BK_CONST, 0, "c|65535" },
    { "System", "Char", "MinValue", BK_CONST, 0, "c|0" },
    { "System", "String", "Empty", BK_CONST, 0, "s|" },
    { "System", "Math", "PI", BK_CONST, 0, "d|3.1415926535897931" },
    { "System", "Math", "E", BK_CONST, 0, "d|2.7182818284590451" },
    { "System", "String", "ToUpper", BK_INSTANCE_METHOD, 0, "s", "s_toupper" },
    { "System", "String", "ToLower", BK_INSTANCE_METHOD, 0, "s", "s_tolower" },
    { "System", "String", "Trim", BK_INSTANCE_METHOD, 0, "s", "s_trim" },
    { "System", "String", "Trim", BK_INSTANCE_METHOD, 0, "s|*trimChars:c[", "s_trimc" },
    { "System", "String", "TrimStart", BK_INSTANCE_METHOD, 0, "s", "s_trimstart" },
    { "System", "String", "TrimStart", BK_INSTANCE_METHOD, 0, "s|*trimChars:c[", "s_trimstartc" },
    { "System", "String", "TrimEnd", BK_INSTANCE_METHOD, 0, "s", "s_trimend" },
    { "System", "String", "TrimEnd", BK_INSTANCE_METHOD, 0, "s|*trimChars:c[", "s_trimendc" },
    { "System", "String", "Substring", BK_INSTANCE_METHOD, 0, "s|startIndex:i", "s_substring" },
    { "System", "String", "Substring", BK_INSTANCE_METHOD, 0, "s|startIndex:i|length:i", "s_substring2" },
    { "System", "String", "IndexOf", BK_INSTANCE_METHOD, 0, "i|value:s", "s_indexof" },
    { "System", "String", "IndexOf", BK_INSTANCE_METHOD, 0, "i|value:c", "s_indexofc" },
    { "System", "String", "IndexOf", BK_INSTANCE_METHOD, 0, "i|value:s|startIndex:i", "s_indexof2" },
    { "System", "String", "IndexOf", BK_INSTANCE_METHOD, 0, "i|value:c|startIndex:i", "s_indexofc2" },
    { "System", "String", "LastIndexOf", BK_INSTANCE_METHOD, 0, "i|value:s", "s_lastindexof" },
    { "System", "String", "LastIndexOf", BK_INSTANCE_METHOD, 0, "i|value:c", "s_lastindexofc" },
    { "System", "String", "Contains", BK_INSTANCE_METHOD, 0, "b|value:s", "s_contains" },
    { "System", "String", "Contains", BK_INSTANCE_METHOD, 0, "b|value:c", "s_containsc" },
    { "System", "String", "StartsWith", BK_INSTANCE_METHOD, 0, "b|value:s", "s_startswith" },
    { "System", "String", "EndsWith", BK_INSTANCE_METHOD, 0, "b|value:s", "s_endswith" },
    { "System", "String", "Replace", BK_INSTANCE_METHOD, 0, "s|oldValue:s|newValue:s", "s_replace" },
    { "System", "String", "Replace", BK_INSTANCE_METHOD, 0, "s|oldChar:c|newChar:c", "s_replacec" },
    { "System", "String", "Insert", BK_INSTANCE_METHOD, 0, "s|startIndex:i|value:s", "s_insert" },
    { "System", "String", "Remove", BK_INSTANCE_METHOD, 0, "s|startIndex:i", "s_remove" },
    { "System", "String", "Remove", BK_INSTANCE_METHOD, 0, "s|startIndex:i|count:i", "s_remove2" },
    { "System", "String", "PadLeft", BK_INSTANCE_METHOD, 0, "s|totalWidth:i", "s_padleft" },
    { "System", "String", "PadLeft", BK_INSTANCE_METHOD, 0, "s|totalWidth:i|paddingChar:c", "s_padleftc" },
    { "System", "String", "PadRight", BK_INSTANCE_METHOD, 0, "s|totalWidth:i", "s_padright" },
    { "System", "String", "PadRight", BK_INSTANCE_METHOD, 0, "s|totalWidth:i|paddingChar:c", "s_padrightc" },
    { "System", "String", "ToCharArray", BK_INSTANCE_METHOD, 0, "c[", "s_tochararray" },
    { "System", "String", "Split", BK_INSTANCE_METHOD, 0, "s[|*separator:c[", "s_split" },
    { "System", "String", "Split", BK_INSTANCE_METHOD, 0, "s[|separator:c", "s_splitc" },
    { "System", "String", "Split", BK_INSTANCE_METHOD, 0, "s[|separator:s", "s_splits" },
    { "System", "String", "Equals", BK_INSTANCE_METHOD, 0, "b|value:s", "s_equals" },
    { "System", "String", "CompareTo", BK_INSTANCE_METHOD, 0, "i|strB:s", "s_compareto" },
    { "System", "String", "IsNullOrEmpty", BK_SHARED_METHOD, 0, "b|value:s", "str_isnullorempty" },
    { "System", "String", "IsNullOrWhiteSpace", BK_SHARED_METHOD, 0, "b|value:s", "str_isnullorwhitespace" },
    { "System", "String", "Join", BK_SHARED_METHOD, 0, "s|separator:s|*value:s[", "str_join" },
    { "System", "String", "Join", BK_SHARED_METHOD, 0, "s|separator:s|*values:o[", "str_join" },
    { "System", "String", "Concat", BK_SHARED_METHOD, 0, "s|str0:s|str1:s", "concat" },
    { "System", "String", "Concat", BK_SHARED_METHOD, 0, "s|*values:s[", "str_concat_arr" },
    { "System", "String", "Format", BK_SHARED_METHOD, 0, "s|format:s|*args:o[", "format" },
    { "System", "String", "Compare", BK_SHARED_METHOD, 0, "i|strA:s|strB:s", "str_compare:0" },
    { "System", "String", "Compare", BK_SHARED_METHOD, 0, "i|strA:s|strB:s|ignoreCase:b", "str_compare" },
    { "System", "Char", "IsDigit", BK_SHARED_METHOD, 0, "b|c:c", "char_isdigit" },
    { "System", "Char", "IsLetter", BK_SHARED_METHOD, 0, "b|c:c", "char_isletter" },
    { "System", "Char", "IsLetterOrDigit", BK_SHARED_METHOD, 0, "b|c:c", "char_isletterordigit" },
    { "System", "Char", "IsWhiteSpace", BK_SHARED_METHOD, 0, "b|c:c", "char_iswhitespace" },
    { "System", "Char", "IsUpper", BK_SHARED_METHOD, 0, "b|c:c", "char_isupper" },
    { "System", "Char", "IsLower", BK_SHARED_METHOD, 0, "b|c:c", "char_islower" },
    { "System", "Char", "IsPunctuation", BK_SHARED_METHOD, 0, "b|c:c", "char_ispunctuation" },
    { "System", "Char", "ToUpper", BK_SHARED_METHOD, 0, "c|c:c", "char_toupper" },
    { "System", "Char", "ToLower", BK_SHARED_METHOD, 0, "c|c:c", "char_tolower" },
    { "System", "SByte", "Parse", BK_SHARED_METHOD, 0, "Y|s:s", "parse_int:2" },
    { "System", "SByte", "TryParse", BK_SHARED_METHOD, 0, "b|s:s|&result:Y", "tryparse_int:2" },
    { "System", "Byte", "Parse", BK_SHARED_METHOD, 0, "y|s:s", "parse_int:3" },
    { "System", "Byte", "TryParse", BK_SHARED_METHOD, 0, "b|s:s|&result:y", "tryparse_int:3" },
    { "System", "Int16", "Parse", BK_SHARED_METHOD, 0, "h|s:s", "parse_int:4" },
    { "System", "Int16", "TryParse", BK_SHARED_METHOD, 0, "b|s:s|&result:h", "tryparse_int:4" },
    { "System", "UInt16", "Parse", BK_SHARED_METHOD, 0, "H|s:s", "parse_int:5" },
    { "System", "UInt16", "TryParse", BK_SHARED_METHOD, 0, "b|s:s|&result:H", "tryparse_int:5" },
    { "System", "Int32", "Parse", BK_SHARED_METHOD, 0, "i|s:s", "parse_int:6" },
    { "System", "Int32", "TryParse", BK_SHARED_METHOD, 0, "b|s:s|&result:i", "tryparse_int:6" },
    { "System", "UInt32", "Parse", BK_SHARED_METHOD, 0, "I|s:s", "parse_int:7" },
    { "System", "UInt32", "TryParse", BK_SHARED_METHOD, 0, "b|s:s|&result:I", "tryparse_int:7" },
    { "System", "Int64", "Parse", BK_SHARED_METHOD, 0, "l|s:s", "parse_int:8" },
    { "System", "Int64", "TryParse", BK_SHARED_METHOD, 0, "b|s:s|&result:l", "tryparse_int:8" },
    { "System", "UInt64", "Parse", BK_SHARED_METHOD, 0, "L|s:s", "parse_int:9" },
    { "System", "UInt64", "TryParse", BK_SHARED_METHOD, 0, "b|s:s|&result:L", "tryparse_int:9" },
    { "System", "Double", "Parse", BK_SHARED_METHOD, 0, "d|s:s", "parse_dbl:0" },
    { "System", "Double", "TryParse", BK_SHARED_METHOD, 0, "b|s:s|&result:d", "tryparse_dbl:0" },
    { "System", "Single", "Parse", BK_SHARED_METHOD, 0, "f|s:s", "parse_dbl:1" },
    { "System", "Single", "TryParse", BK_SHARED_METHOD, 0, "b|s:s|&result:f", "tryparse_dbl:1" },
    { "System", "Boolean", "Parse", BK_SHARED_METHOD, 0, "b|value:s", "parse_bool" },
    { "System", "Boolean", "TryParse", BK_SHARED_METHOD, 0, "b|value:s|&result:b", "tryparse_bool" },
    { "System", "Convert", "ToInt32", BK_SHARED_METHOD, 0, "i|value:s", "convert_str_int:6" },
    { "System", "Convert", "ToInt32", BK_SHARED_METHOD, 0, "i|value:d", "d2i:6" },
    { "System", "Convert", "ToInt32", BK_SHARED_METHOD, 0, "i|value:o", "unbox:6" },
    { "System", "Convert", "ToInt64", BK_SHARED_METHOD, 0, "l|value:s", "convert_str_int:8" },
    { "System", "Convert", "ToInt64", BK_SHARED_METHOD, 0, "l|value:d", "d2i:8" },
    { "System", "Convert", "ToInt64", BK_SHARED_METHOD, 0, "l|value:o", "unbox:8" },
    { "System", "Convert", "ToInt16", BK_SHARED_METHOD, 0, "h|value:s", "convert_str_int:4" },
    { "System", "Convert", "ToInt16", BK_SHARED_METHOD, 0, "h|value:d", "d2i:4" },
    { "System", "Convert", "ToInt16", BK_SHARED_METHOD, 0, "h|value:o", "unbox:4" },
    { "System", "Convert", "ToByte", BK_SHARED_METHOD, 0, "y|value:s", "convert_str_int:3" },
    { "System", "Convert", "ToByte", BK_SHARED_METHOD, 0, "y|value:d", "d2i:3" },
    { "System", "Convert", "ToByte", BK_SHARED_METHOD, 0, "y|value:o", "unbox:3" },
    { "System", "Convert", "ToUInt32", BK_SHARED_METHOD, 0, "I|value:s", "convert_str_int:7" },
    { "System", "Convert", "ToUInt32", BK_SHARED_METHOD, 0, "I|value:d", "d2i:7" },
    { "System", "Convert", "ToUInt32", BK_SHARED_METHOD, 0, "I|value:o", "unbox:7" },
    { "System", "Convert", "ToDouble", BK_SHARED_METHOD, 0, "d|value:s", "convert_str_dbl" },
    { "System", "Convert", "ToDouble", BK_SHARED_METHOD, 0, "d|value:d", "@id" },
    { "System", "Convert", "ToDouble", BK_SHARED_METHOD, 0, "d|value:o", "unbox_d" },
    { "System", "Convert", "ToString", BK_SHARED_METHOD, 0, "s|value:o", "obj_str" },
    { "System", "Convert", "ToBoolean", BK_SHARED_METHOD, 0, "b|value:s", "parse_bool" },
    { "System", "Convert", "ToChar", BK_SHARED_METHOD, 0, "c|value:s", "unbox:14" },
    { "System", "Math", "Abs", BK_SHARED_METHOD, 0, "i|value:i", "math_absi:6" },
    { "System", "Math", "Abs", BK_SHARED_METHOD, 0, "l|value:l", "math_absi:8" },
    { "System", "Math", "Abs", BK_SHARED_METHOD, 0, "h|value:h", "math_absi:4" },
    { "System", "Math", "Abs", BK_SHARED_METHOD, 0, "Y|value:Y", "math_absi:2" },
    { "System", "Math", "Abs", BK_SHARED_METHOD, 0, "d|value:d", "math_absd" },
    { "System", "Math", "Abs", BK_SHARED_METHOD, 0, "f|value:f", "math_absd" },
    { "System", "Math", "Max", BK_SHARED_METHOD, 0, "i|val1:i|val2:i", "math_maxi" },
    { "System", "Math", "Max", BK_SHARED_METHOD, 0, "l|val1:l|val2:l", "math_maxi" },
    { "System", "Math", "Max", BK_SHARED_METHOD, 0, "h|val1:h|val2:h", "math_maxi" },
    { "System", "Math", "Max", BK_SHARED_METHOD, 0, "y|val1:y|val2:y", "math_maxi" },
    { "System", "Math", "Max", BK_SHARED_METHOD, 0, "I|val1:I|val2:I", "math_maxi" },
    { "System", "Math", "Max", BK_SHARED_METHOD, 0, "L|val1:L|val2:L", "math_maxu" },
    { "System", "Math", "Max", BK_SHARED_METHOD, 0, "d|val1:d|val2:d", "math_maxd" },
    { "System", "Math", "Max", BK_SHARED_METHOD, 0, "f|val1:f|val2:f", "math_maxd" },
    { "System", "Math", "Min", BK_SHARED_METHOD, 0, "i|val1:i|val2:i", "math_mini" },
    { "System", "Math", "Min", BK_SHARED_METHOD, 0, "l|val1:l|val2:l", "math_mini" },
    { "System", "Math", "Min", BK_SHARED_METHOD, 0, "h|val1:h|val2:h", "math_mini" },
    { "System", "Math", "Min", BK_SHARED_METHOD, 0, "y|val1:y|val2:y", "math_mini" },
    { "System", "Math", "Min", BK_SHARED_METHOD, 0, "I|val1:I|val2:I", "math_mini" },
    { "System", "Math", "Min", BK_SHARED_METHOD, 0, "L|val1:L|val2:L", "math_minu" },
    { "System", "Math", "Min", BK_SHARED_METHOD, 0, "d|val1:d|val2:d", "math_mind" },
    { "System", "Math", "Min", BK_SHARED_METHOD, 0, "f|val1:f|val2:f", "math_mind" },
    { "System", "Math", "Sqrt", BK_SHARED_METHOD, 0, "d|d:d", "#107" },
    { "System", "Math", "Floor", BK_SHARED_METHOD, 0, "d|d:d", "#105" },
    { "System", "Math", "Ceiling", BK_SHARED_METHOD, 0, "d|d:d", "#104" },
    { "System", "Math", "Sin", BK_SHARED_METHOD, 0, "d|d:d", "#120" },
    { "System", "Math", "Cos", BK_SHARED_METHOD, 0, "d|d:d", "#121" },
    { "System", "Math", "Tan", BK_SHARED_METHOD, 0, "d|d:d", "#122" },
    { "System", "Math", "Asin", BK_SHARED_METHOD, 0, "d|d:d", "#123" },
    { "System", "Math", "Acos", BK_SHARED_METHOD, 0, "d|d:d", "#124" },
    { "System", "Math", "Atan", BK_SHARED_METHOD, 0, "d|d:d", "#126" },
    { "System", "Math", "Exp", BK_SHARED_METHOD, 0, "d|d:d", "#108" },
    { "System", "Math", "Sinh", BK_SHARED_METHOD, 0, "d|d:d", "#127" },
    { "System", "Math", "Cosh", BK_SHARED_METHOD, 0, "d|d:d", "#128" },
    { "System", "Math", "Tanh", BK_SHARED_METHOD, 0, "d|d:d", "#129" },
    { "System", "Math", "Log", BK_SHARED_METHOD, 0, "d|d:d", "#109" },
    { "System", "Math", "Pow", BK_SHARED_METHOD, 0, "d|x:d|y:d", "#110" },
    { "System", "Math", "Atan2", BK_SHARED_METHOD, 0, "d|y:d|x:d", "#125" },
    { "System", "Math", "Log", BK_SHARED_METHOD, 0, "d|a:d|newBase:d", "math_logb" },
    { "System", "Math", "Log10", BK_SHARED_METHOD, 0, "d|d:d", "math_log10" },
    { "System", "Math", "Round", BK_SHARED_METHOD, 0, "d|a:d", "math_round" },
    { "System", "Math", "Round", BK_SHARED_METHOD, 0, "d|value:d|digits:i", "math_round2" },
    { "System", "Math", "Truncate", BK_SHARED_METHOD, 0, "d|d:d", "math_truncate" },
    { "System", "Math", "Sign", BK_SHARED_METHOD, 0, "i|value:d", "math_signd" },
    { "System", "Math", "Sign", BK_SHARED_METHOD, 0, "i|value:i", "math_signi" },
    { "System", "Math", "Sign", BK_SHARED_METHOD, 0, "i|value:l", "math_signi" },
    { "System", "Environment", "Exit", BK_SHARED_METHOD, 0, "v|exitCode:i", "env_exit" },
    { "System", "Environment", "GetCommandLineArgs", BK_SHARED_METHOD, 0, "s[", "env_args" },
    { "System", "Environment", "NewLine", BK_CONST, 0, "s|\n", 0 },
    { "Microsoft.VisualBasic", "Strings", "Len", BK_SHARED_METHOD, 0, "i|Expression:s", "vb_len" },
    { "Microsoft.VisualBasic", "Strings", "Left", BK_SHARED_METHOD, 0, "s|str:s|Length:i", "vb_left" },
    { "Microsoft.VisualBasic", "Strings", "Right", BK_SHARED_METHOD, 0, "s|str:s|Length:i", "vb_right" },
    { "Microsoft.VisualBasic", "Strings", "Mid", BK_SHARED_METHOD, 0, "s|str:s|Start:i", "vb_mid" },
    { "Microsoft.VisualBasic", "Strings", "Mid", BK_SHARED_METHOD, 0, "s|str:s|Start:i|Length:i", "vb_mid2" },
    { "Microsoft.VisualBasic", "Strings", "InStr", BK_SHARED_METHOD, 0, "i|String1:s|String2:s", "vb_instr" },
    { "Microsoft.VisualBasic", "Strings", "InStr", BK_SHARED_METHOD, 0, "i|Start:i|String1:s|String2:s", "vb_instr2" },
    { "Microsoft.VisualBasic", "Strings", "InStrRev", BK_SHARED_METHOD, 0, "i|StringCheck:s|StringMatch:s", "vb_instrrev" },
    { "Microsoft.VisualBasic", "Strings", "UCase", BK_SHARED_METHOD, 0, "s|Value:s", "vb_ucase" },
    { "Microsoft.VisualBasic", "Strings", "LCase", BK_SHARED_METHOD, 0, "s|Value:s", "vb_lcase" },
    { "Microsoft.VisualBasic", "Strings", "Trim", BK_SHARED_METHOD, 0, "s|str:s", "vb_trim" },
    { "Microsoft.VisualBasic", "Strings", "LTrim", BK_SHARED_METHOD, 0, "s|str:s", "vb_ltrim" },
    { "Microsoft.VisualBasic", "Strings", "RTrim", BK_SHARED_METHOD, 0, "s|str:s", "vb_rtrim" },
    { "Microsoft.VisualBasic", "Strings", "Chr", BK_SHARED_METHOD, 0, "c|CharCode:i", "vb_chr" },
    { "Microsoft.VisualBasic", "Strings", "ChrW", BK_SHARED_METHOD, 0, "c|CharCode:i", "vb_chrw" },
    { "Microsoft.VisualBasic", "Strings", "Asc", BK_SHARED_METHOD, 0, "i|String:s", "vb_asc" },
    { "Microsoft.VisualBasic", "Strings", "Asc", BK_SHARED_METHOD, 0, "i|String:c", "@id" },
    { "Microsoft.VisualBasic", "Strings", "AscW", BK_SHARED_METHOD, 0, "i|String:s", "vb_asc" },
    { "Microsoft.VisualBasic", "Strings", "AscW", BK_SHARED_METHOD, 0, "i|String:c", "@id" },
    { "Microsoft.VisualBasic", "Strings", "Space", BK_SHARED_METHOD, 0, "s|Number:i", "vb_space" },
    { "Microsoft.VisualBasic", "Strings", "StrDup", BK_SHARED_METHOD, 0, "s|Number:i|Character:c", "vb_strdup" },
    { "Microsoft.VisualBasic", "Strings", "StrReverse", BK_SHARED_METHOD, 0, "s|Expression:s", "vb_strreverse" },
    { "Microsoft.VisualBasic", "Strings", "Replace", BK_SHARED_METHOD, 0, "s|Expression:s|Find:s|Replacement:s", "vb_replace" },
    { "Microsoft.VisualBasic", "Strings", "Split", BK_SHARED_METHOD, 0, "s[|Expression:s", "vb_split1" },
    { "Microsoft.VisualBasic", "Strings", "Split", BK_SHARED_METHOD, 0, "s[|Expression:s|Delimiter:s", "vb_split" },
    { "Microsoft.VisualBasic", "Strings", "Join", BK_SHARED_METHOD, 0, "s|SourceArray:s[", "vb_join1" },
    { "Microsoft.VisualBasic", "Strings", "Join", BK_SHARED_METHOD, 0, "s|SourceArray:s[|Delimiter:s", "vb_join" },
    { "Microsoft.VisualBasic", "Strings", "StrComp", BK_SHARED_METHOD, 0, "i|String1:s|String2:s", "vb_strcomp" },
    { "Microsoft.VisualBasic", "Strings", "Format", BK_SHARED_METHOD, 0, "s|Expression:o|Style:s", "vb_format" },
    { "Microsoft.VisualBasic", "Strings", "Format", BK_SHARED_METHOD, 0, "s|Expression:o", "obj_str" },
    { "Microsoft.VisualBasic", "Conversion", "Val", BK_SHARED_METHOD, 0, "d|InputStr:s", "vb_val" },
    { "Microsoft.VisualBasic", "Conversion", "Hex", BK_SHARED_METHOD, 0, "s|Number:i", "vb_hex32" },
    { "Microsoft.VisualBasic", "Conversion", "Hex", BK_SHARED_METHOD, 0, "s|Number:l", "vb_hex" },
    { "Microsoft.VisualBasic", "Conversion", "Oct", BK_SHARED_METHOD, 0, "s|Number:i", "vb_oct32" },
    { "Microsoft.VisualBasic", "Conversion", "Oct", BK_SHARED_METHOD, 0, "s|Number:l", "vb_oct" },
    { "Microsoft.VisualBasic", "Conversion", "Int", BK_SHARED_METHOD, 0, "d|Number:d", "vb_int" },
    { "Microsoft.VisualBasic", "Conversion", "Int", BK_SHARED_METHOD, 0, "i|Number:i", "@id" },
    { "Microsoft.VisualBasic", "Conversion", "Int", BK_SHARED_METHOD, 0, "l|Number:l", "@id" },
    { "Microsoft.VisualBasic", "Conversion", "Fix", BK_SHARED_METHOD, 0, "d|Number:d", "vb_fix" },
    { "Microsoft.VisualBasic", "Conversion", "Fix", BK_SHARED_METHOD, 0, "i|Number:i", "@id" },
    { "Microsoft.VisualBasic", "Conversion", "Fix", BK_SHARED_METHOD, 0, "l|Number:l", "@id" },
    { "Microsoft.VisualBasic", "Information", "IsNumeric", BK_SHARED_METHOD, 0, "b|Expression:o", "vb_isnumeric" },
    { "Microsoft.VisualBasic", "Information", "IsNothing", BK_SHARED_METHOD, 0, "b|Expression:o", "vb_isnothing" },
    { "Microsoft.VisualBasic", "VBMath", "Rnd", BK_SHARED_METHOD, 0, "f", "vb_rnd" },
    { "Microsoft.VisualBasic", "VBMath", "Rnd", BK_SHARED_METHOD, 0, "f|Number:f", "vb_rnd1" },
    { "Microsoft.VisualBasic", "VBMath", "Randomize", BK_SHARED_METHOD, 0, "v", "vb_randomize" },
    { "Microsoft.VisualBasic", "VBMath", "Randomize", BK_SHARED_METHOD, 0, "v|Number:d", "vb_randomize1" },
    { "Microsoft.VisualBasic", "Constants", "vbCrLf", BK_CONST, 0, "s|\r\n", 0 },
    { "Microsoft.VisualBasic", "Constants", "vbNewLine", BK_CONST, 0, "s|\r\n", 0 },
    { "Microsoft.VisualBasic", "Constants", "vbCr", BK_CONST, 0, "s|\r", 0 },
    { "Microsoft.VisualBasic", "Constants", "vbLf", BK_CONST, 0, "s|\n", 0 },
    { "Microsoft.VisualBasic", "Constants", "vbTab", BK_CONST, 0, "s|\t", 0 },
    { "Microsoft.VisualBasic", "Constants", "vbBack", BK_CONST, 0, "s|\b", 0 },
    { "Microsoft.VisualBasic", "Constants", "vbFormFeed", BK_CONST, 0, "s|\f", 0 },
    { "Microsoft.VisualBasic", "Constants", "vbVerticalTab", BK_CONST, 0, "s|\v", 0 },
    { "Microsoft.VisualBasic", "ControlChars", "CrLf", BK_CONST, 0, "s|\r\n", 0 },
    { "Microsoft.VisualBasic", "ControlChars", "NewLine", BK_CONST, 0, "s|\r\n", 0 },
    { "Microsoft.VisualBasic", "ControlChars", "Cr", BK_CONST, 0, "c|13", 0 },
    { "Microsoft.VisualBasic", "ControlChars", "Lf", BK_CONST, 0, "c|10", 0 },
    { "Microsoft.VisualBasic", "ControlChars", "Tab", BK_CONST, 0, "c|9", 0 },
    { "Microsoft.VisualBasic", "ControlChars", "Quote", BK_CONST, 0, "c|34", 0 },
    { "Microsoft.VisualBasic", "ControlChars", "NullChar", BK_CONST, 0, "c|0", 0 },
    { "Microsoft.VisualBasic", "ControlChars", "Back", BK_CONST, 0, "c|8", 0 },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "FullTypeName", BK_SHARED_METHOD, 0, "s|o:o", "type_fullname" },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "CharsToString", BK_SHARED_METHOD, 0, "s|chars:c[|start:i|length:i", "chars_sub" },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "FileReadAll", BK_SHARED_METHOD, 0, "s|path:s", "file_readall" },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "FileWriteAll", BK_SHARED_METHOD, 0, "v|path:s|text:s|append:b", "file_writeall" },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "FileExists", BK_SHARED_METHOD, 0, "b|path:s", "file_exists" },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "FileDelete", BK_SHARED_METHOD, 0, "v|path:s", "file_delete" },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "FileOpen", BK_SHARED_METHOD, 0, "l|path:s|mode:i", "file_open" },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "FileReadLine", BK_SHARED_METHOD, 0, "s|f:l", "file_readline" },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "FileReadToEnd", BK_SHARED_METHOD, 0, "s|f:l", "file_readtoend" },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "FileRead", BK_SHARED_METHOD, 0, "i|f:l", "file_read" },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "FilePeek", BK_SHARED_METHOD, 0, "i|f:l", "file_peek" },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "FileWrite", BK_SHARED_METHOD, 0, "v|f:l|s:s", "file_write" },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "FileClose", BK_SHARED_METHOD, 0, "v|f:l", "file_close" },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "CloneObject", BK_SHARED_METHOD, 0, "o|obj:o", "clone" },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "TickSeed", BK_SHARED_METHOD, 0, "i", "tick_seed" },
    { "Microsoft.VisualBasic.CompilerServices", "Utils", "CompareObjects", BK_SHARED_METHOD, 0, "i|a:o|b:o", "obj_compare" },
    { "System", "GC", "Collect", BK_SHARED_METHOD, 0, "v", "gc_collect" },
    { "System", "GC", "GetTotalMemory", BK_SHARED_METHOD, 0, "l|forceFullCollection:b", "gc_total" },
    { "System", "GC", "CollectionCount", BK_SHARED_METHOD, 0, "i|generation:i", "gc_count_gen" },
    { "System", "Object", "Equals", BK_INSTANCE_METHOD, 0, "b|obj:o", "obj_equals" },
    { "System", "Object", "GetHashCode", BK_INSTANCE_METHOD, 0, "i", "obj_hash" },
    { "System", "Object", "ReferenceEquals", BK_SHARED_METHOD, 0, "b|objA:o|objB:o", "ref_equals" },
    { 0, 0, 0, 0, 0, 0, 0 }
};

static Type *code_type(const char **p) {
    Type *t;
    switch (*(*p)++) {
    case 'v': return 0;
    case 'b': t = ty[TY_BOOLEAN]; break;
    case 'Y': t = ty[TY_SBYTE]; break;
    case 'y': t = ty[TY_BYTE]; break;
    case 'h': t = ty[TY_SHORT]; break;
    case 'H': t = ty[TY_USHORT]; break;
    case 'i': t = ty[TY_INTEGER]; break;
    case 'I': t = ty[TY_UINTEGER]; break;
    case 'l': t = ty[TY_LONG]; break;
    case 'L': t = ty[TY_ULONG]; break;
    case 'f': t = ty[TY_SINGLE]; break;
    case 'd': t = ty[TY_DOUBLE]; break;
    case 'c': t = ty[TY_CHAR]; break;
    case 's': t = ty[TY_STRING]; break;
    default: t = ty[TY_OBJECT]; break;
    }
    while (**p == '[') {
        (*p)++;
        t = ty_array(t, 1);
    }
    return t;
}

/* the System type symbol of a .NET type name */
static Sym *builtin_type(const char *ns, const char *name) {
    Sym *nsym = ns_path(ns), *s;
    Name *n = intern_word(name);
    int k;
    s = scope_find(nsym->members, n);
    if (s) return s;
    /* the Microsoft.VisualBasic modules: their members are promoted */
    s = new_sym((strcmp(ns, "Microsoft.VisualBasic") == 0 && strcmp(name, "ControlChars") != 0) ? S_MODULE : S_TYPE, n, name);
    s->owner = nsym;
    s->members = new_scope(SC_TYPE, 0, s);
    for (k = 1; k < TY_NOTHING; k++)
        if (strcmp(ty[k]->sysname, name) == 0 && k != TY_VOID) {
            s->type = ty[k];
            ty[k]->sym = s;
        }
    scope_add(nsym->members, s);
    return s;
}

static Node *lit_node(int kind, Type *t, Node *at);   /* sema_const.c */

static void add_builtin(const Builtin *b) {
    Sym *tsym = builtin_type(b->ns, b->type), *m, *prev;
    Name *n = intern_word(b->member);
    const char *p = b->sig;
    prev = scope_find(tsym->members, n);
    m = new_sym(b->kind == BK_CONST ? S_CONST : S_METHOD, n, b->member);
    m->owner = tsym;
    m->builtin = b->id;
    m->rt = b->rt;
    m->mods = M_PUBLIC | (b->kind == BK_SHARED_METHOD || b->kind == BK_CONST ? M_SHARED : 0);
    if (b->kind == BK_INSTANCE_PROP) m->mods |= M_READONLY;
    m->type = code_type(&p);
    if (b->kind == BK_INSTANCE_METHOD || b->kind == BK_INSTANCE_PROP) m->sflags |= SF_INSTANCE;
    if (b->kind == BK_CONST) {
        Node at;
        Node *v;
        memset(&at, 0, sizeof at);
        p++;                        /* '|' */
        if (m->type->kind == TY_STRING) {
            v = lit_node(N_STRING, m->type, &at);
            v->text = p;
        } else if (ty_float(m->type)) {
            v = lit_node(N_FLOAT, m->type, &at);
            v->dval = strtod(p, 0);
        } else if (m->type->kind == TY_CHAR) {
            v = lit_node(N_CHAR, m->type, &at);
            v->ival = (u64)strtoll(p, 0, 10);
        } else {
            v = lit_node(N_INT, m->type, &at);
            v->ival = *p == '-' ? (u64)strtoll(p, 0, 10) : (u64)strtoull(p, 0, 10);
        }
        m->cval = v;
        m->state = 2;
    } else {
        while (*p == '|') {
            Sym *par;
            char buf[40];
            int k = 0, mods = M_BYVAL;
            p++;
            if (*p == '*') {
                mods |= M_PARAMARRAY;
                p++;
            }
            if (*p == '&') {
                mods = M_BYREF;
                p++;
            }
            while (*p != ':') buf[k++] = *p++;
            buf[k] = 0;
            p++;
            par = new_sym(S_PARAM, intern_word(buf), xstrdup(buf));
            par->mods = mods;
            par->type = code_type(&p);
            par->index = m->params.n;
            par->owner = m;
            sv_push(&m->params, par);
        }
    }
    if (prev) {
        while (prev->overload) prev = prev->overload;
        prev->overload = m;
    } else scope_add(tsym->members, m);
}

static void builtins_init(void) {
    int i;
    static const char *prims[] = {
        "Boolean", "SByte", "Byte", "Int16", "UInt16", "Int32", "UInt32", "Int64", "UInt64",
        "Single", "Double", "Char", "String", "Object", 0
    };
    for (i = 0; prims[i]; i++) builtin_type("System", prims[i]);
    for (i = 0; builtins[i].ns; i++) add_builtin(&builtins[i]);
    /* ToString() and ToString(format) of the primitive types */
    for (i = 0; prims[i]; i++) {
        Builtin b;
        memset(&b, 0, sizeof b);
        b.ns = "System";
        b.type = prims[i];
        b.member = "ToString";
        b.kind = BK_INSTANCE_METHOD;
        b.sig = "s";
        b.rt = "@tostr";
        add_builtin(&b);
        if (strcmp(prims[i], "String") && strcmp(prims[i], "Object") && strcmp(prims[i], "Boolean") && strcmp(prims[i], "Char")) {
            b.sig = "s|format:s";
            b.rt = "@tostrf";
            add_builtin(&b);
        }
        if (strcmp(prims[i], "String") && strcmp(prims[i], "Object")) {
            /* CompareTo(value) and Equals(value) of the same type */
            static const char *codes = "bYyhHiIlLfdc";
            static const char *names2[] = { "Boolean", "SByte", "Byte", "Int16", "UInt16", "Int32", "UInt32",
                                            "Int64", "UInt64", "Single", "Double", "Char", 0 };
            char sig[16];
            int k;
            for (k = 0; names2[k] && strcmp(names2[k], prims[i]); k++) ;
            if (!names2[k]) continue;
            b.member = "CompareTo";
            sprintf(sig, "i|value:%c", codes[k]);
            b.sig = xstrdup(sig);
            b.rt = (codes[k] == 'f' || codes[k] == 'd') ? "cmp_d" : (codes[k] == 'L') ? "ul_cmp" : "cmp_i";
            add_builtin(&b);
            b.member = "Equals";
            sprintf(sig, "b|obj:%c", codes[k]);
            b.sig = xstrdup(sig);
            b.rt = (codes[k] == 'f' || codes[k] == 'd') ? "eq_d" : "eq_i";
            add_builtin(&b);
        }
    }
    ns_path("System.Collections.Generic");
    ns_path("System.Collections");
    ns_path("System.Diagnostics");
    ns_path("System.Linq");
    ns_path("System.Text");
    ns_path("System.IO");
    ns_path("Microsoft.VisualBasic");
}

void sema_init(void) {
    types_init();
    ns_global = new_sym(S_NAMESPACE, 0, "Global");
    ns_global->members = new_scope(SC_NS, 0, ns_global);
    builtins_init();
}
