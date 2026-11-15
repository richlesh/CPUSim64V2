/*
 * ast.c - AST nodes and the --ast dump.
 */

static const char *node_names[] = {
    "",
#define N(x) #x,
#include "nodes.inc"
#undef N
    ""
};

const char *node_kind_name(int kind) {
    return node_names[kind];
}

Node *node_new(int kind, Token *at) {
    Node *n = anew(sizeof(Node));
    n->kind = kind;
    if (at) {
        n->file = at->file;
        n->line = at->line;
    }
    return n;
}

static void indent(FILE *f, int depth) {
    int i;
    for (i = 0; i < depth; i++) fputs("  ", f);
}

static void dump_list(FILE *f, Node *n, const char *label, int depth);

static void dump_node(FILE *f, Node *n, int depth) {
    indent(f, depth);
    fputs(node_names[n->kind], f);
    if (n->name) fprintf(f, " name=%s", n->name->s);
    if (n->text) fprintf(f, " text=%s", n->text);
    if (n->op) {
        if (n->op >= OPKW) fprintf(f, " op=%s", kw_spelling(n->op - OPKW));
        else fprintf(f, " op=%s", punct_spelling(n->op));
    }
    if (n->flags) fprintf(f, " flags=0x%x", n->flags);
    if (n->val) fprintf(f, " val=%lld", n->val);
    fputc('\n', f);
    dump_list(f, n->a, "a", depth + 1);
    dump_list(f, n->b, "b", depth + 1);
    dump_list(f, n->c, "c", depth + 1);
    dump_list(f, n->d, "d", depth + 1);
}

/* a child and its siblings: the label is printed once, before the first */
static void dump_list(FILE *f, Node *n, const char *label, int depth) {
    if (!n) return;
    indent(f, depth);
    fprintf(f, "%s:\n", label);
    for (; n; n = n->next) dump_node(f, n, depth + 1);
}

void dump_ast(FILE *f, Node *root) {
    for (; root; root = root->next) dump_node(f, root, 0);
}
