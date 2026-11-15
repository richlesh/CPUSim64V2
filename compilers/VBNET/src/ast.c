/*
 * ast.c - AST nodes and the --ast dump.
 */

const char *node_names[] = {
#define NK(id, nm, la, lb, lc, ld, l1, l2, l3) nm,
#include "nodes.inc"
#undef NK
    ""
};

static const char *node_labels[][7] = {
#define NK(id, nm, la, lb, lc, ld, l1, l2, l3) { la, lb, lc, ld, l1, l2, l3 },
#include "nodes.inc"
#undef NK
    { "", "", "", "", "", "", "" }
};

static const char *mod_names[M_NBITS] = {
    "Public", "Private", "Protected", "Friend", "Shadows", "Shared", "Overridable",
    "NotOverridable", "MustOverride", "Overrides", "Overloads", "Partial", "MustInherit",
    "NotInheritable", "ReadOnly", "WriteOnly", "WithEvents", "Dim", "Const", "Static",
    "Default", "Widening", "Narrowing", "Async", "Iterator", "ByVal", "ByRef", "Optional",
    "ParamArray"
};

static const char *flag_names[F_NBITS] = {
    "Nullable", "HasArgs", "NullConditional", "CallKeyword", "SingleLine", "HasElse",
    "Preserve", "AsNew", "MultiLine", "Global", "Key", "Descending", "Ascending", "Else",
    "Auto", "Custom", "HasFinally", "IsNot", "In", "Out", "Top", "Bottom", "Until", "While",
    "Parens", "Escaped", "Explicit", "CopyBack", "Direct", "TryCast", "Enumerator"
};

static const char *lit_suffix[] = { "", "S", "US", "I", "UI", "L", "UL", "F", "R", "D" };

Node *new_node(int kind, Token *t) {
    Node *n = xalloc(sizeof(Node));
    n->kind = kind;
    n->file = t->file;
    n->line = t->line;
    n->col = t->col;
    return n;
}

NodeVec *nv_new(void) {
    return xalloc(sizeof(NodeVec));
}

void nv_push(NodeVec *v, Node *n) {
    if (v->n == v->cap) {
        v->cap = v->cap ? v->cap * 2 : 4;
        v->v = xrealloc(v->v, sizeof(Node *) * v->cap);
    }
    v->v[v->n++] = n;
}

/* ------------------------------------------------------------ dump */

static void indent(FILE *f, int n) {
    while (n-- > 0) fputc(' ', f);
}

static void dump_quoted(FILE *f, const char *s) {
    fputc('"', f);
    for (; *s; s++) {
        if (*s == '\n') fputs("\\n", f);
        else if (*s == '\r') fputs("\\r", f);
        else if (*s == '\t') fputs("\\t", f);
        else if (*s == '"') fputs("\"\"", f);
        else fputc(*s, f);
    }
    fputc('"', f);
}

static void dump_node(FILE *f, Node *n, int ind);

static void dump_child(FILE *f, Node *c, const char *label, int ind) {
    if (!c) return;
    if (label[0]) {
        indent(f, ind + 2);
        fprintf(f, "%s:\n", label);
        dump_node(f, c, ind + 4);
    } else dump_node(f, c, ind + 2);
}

static void dump_list(FILE *f, NodeVec *v, const char *label, int ind) {
    int i;
    if (!v || v->n == 0) return;
    if (label[0]) {
        indent(f, ind + 2);
        fprintf(f, "%s:\n", label);
        ind += 2;
    }
    for (i = 0; i < v->n; i++) {
        if (v->v[i]) dump_node(f, v->v[i], ind + 2);
        else {
            indent(f, ind + 2);
            fputs("(omitted)\n", f);
        }
    }
}

static void dump_node(FILE *f, Node *n, int ind) {
    const char **lab = node_labels[n->kind];
    int i;
    indent(f, ind);
    fputs(node_names[n->kind], f);
    if (n->kind == N_OPTION) fprintf(f, " %s", n->name->key);
    else if (n->name) fprintf(f, " %s", n->text ? n->text : n->name->key);
    if (n->tchar && n->kind != N_INT && n->kind != N_FLOAT) fprintf(f, " type%c", n->tchar);
    switch (n->kind) {
    case N_INT:
        if (n->type && ty_signed(n->type)) fprintf(f, " %lld%s", (long long)n->ival, lit_suffix[n->tchar]);
        else fprintf(f, " %llu%s", n->ival, lit_suffix[n->tchar]);
        break;
    case N_FLOAT:
        fprintf(f, " %.17g%s", n->dval, lit_suffix[n->tchar]);
        break;
    case N_STRING:
        fputc(' ', f);
        dump_quoted(f, n->text);
        break;
    case N_CHAR:
        fprintf(f, " U+%04llX", n->ival);
        break;
    case N_BOOL:
        fputs(n->ival ? " True" : " False", f);
        break;
    case N_TARRAY:
        fprintf(f, " rank %llu", n->ival);
        break;
    case N_LABEL: case N_GOTO:
        if (!n->name) fprintf(f, " %llu", n->ival);
        break;
    case N_OPTION: case N_QOP:
        fprintf(f, " %s", n->text);
        break;
    case N_INTERPHOLE:
        if (n->text) {
            fputs(" format ", f);
            dump_quoted(f, n->text);
        }
        break;
    default:
        if (n->text && !n->name) {
            fputc(' ', f);
            dump_quoted(f, n->text);
        }
        break;
    }
    if (n->op) fprintf(f, " '%s'", tok_spelling_of(n->op));
    for (i = 0; i < M_NBITS; i++)
        if (n->mods & (1 << i)) fprintf(f, " %s", mod_names[i]);
    for (i = 0; i < F_NBITS; i++)
        if (n->flags & (1 << i)) fprintf(f, " +%s", flag_names[i]);
    if (n->type) fprintf(f, " : %s", type_name(n->type));
    if (n->sym && (n->kind == N_NAME || n->kind == N_INVOKE))
        fprintf(f, " -> %s %s", sym_kind_name(n->sym), n->sym->spelling);
    fprintf(f, " @%d\n", n->line);
    dump_child(f, n->a, lab[0], ind);
    dump_child(f, n->b, lab[1], ind);
    dump_child(f, n->c, lab[2], ind);
    dump_child(f, n->d, lab[3], ind);
    dump_list(f, n->list, lab[4], ind);
    dump_list(f, n->list2, lab[5], ind);
    dump_list(f, n->list3, lab[6], ind);
}

void dump_ast(FILE *f, Node *n) {
    dump_node(f, n, 0);
}
