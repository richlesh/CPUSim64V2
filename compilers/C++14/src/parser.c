/*
 * parser.c - the C++14 parser: a recursive-descent parser that follows
 * grammars-v4's CPP14Parser.g4 and builds an AST (nodes.inc).
 *
 * C++ cannot be parsed without knowing what names mean: "a < b" is a template
 * name followed by arguments or a comparison, "T * x;" a declaration or a
 * multiplication.  So the parser keeps a symbol table of the names that are
 * types, templates and namespaces (scopes of namespaces, classes, templates
 * and blocks; Sym), filled as the declarations are parsed.  What is not a
 * known type is an expression, except where only a declaration is possible
 * (an unknown name followed by another name at the start of a statement).
 * Qualified names through a dependent qualifier (T::x) are types only after
 * "typename".
 *
 * Some choices are made by trial: speculate() parses a construct and, if
 * that fails, resets the position (longjmp from perr()).  The bodies of
 * member functions and their constructor initializers are parsed after the
 * class, because they can use members that are declared later.
 *
 * The tokens come from the preprocessor (pp_next) and are all read first.
 */
#include <setjmp.h>

/* ------------------------------------------------------------ symbols and scopes */

enum { SY_VAR = 1, SY_TYPE, SY_TEMPLATE, SY_NAMESPACE };
enum { SC_GLOBAL = 1, SC_NAMESPACE, SC_CLASS, SC_BLOCK, SC_TEMPLATE };

struct Sym {
    Name *name;
    int kind;
    int type_template;   /* SY_TEMPLATE: a class or alias template (names a type with arguments) */
    int dependent;       /* a template parameter, or declared in terms of one */
    Scope *members;      /* class, enum or namespace: the scope of the members */
    Sym *next;
};

struct Scope {
    Scope *parent;
    int kind;
    Name *name;          /* class or namespace name */
    Sym **tab;
    int tabmask;
    Scope **bases;       /* class: the scopes of the base classes */
    int nbases;
    Scope **usings;      /* namespace: using-directives */
    int nusings;
    int dependent_base;  /* a base class depends on a template parameter */
    int is_template;     /* a class template (members may be dependent) */
};

static Scope *global_scope, *cur_scope;

static Scope *scope_new(int kind, Scope *parent, Name *name) {
    Scope *s = anew(sizeof(Scope));
    int size = kind == SC_BLOCK || kind == SC_TEMPLATE ? 8 : 128;
    s->kind = kind;
    s->parent = parent;
    s->name = name;
    s->tab = anew(sizeof(Sym *) * size);
    s->tabmask = size - 1;
    return s;
}

static Sym *scope_find(Scope *sc, Name *n) {
    Sym *y;
    for (y = sc->tab[n->hash & sc->tabmask]; y; y = y->next)
        if (y->name == n) return y;
    return 0;
}

static Sym *declare(Scope *sc, Name *n, int kind) {
    Sym *y = scope_find(sc, n);
    if (y) {
        /* a redeclaration: the new kind wins unless it only repeats a type or template */
        if (y->kind == kind) return y;
        y->kind = kind;
        y->type_template = 0;
        y->dependent = 0;
        if (kind == SY_TEMPLATE && sc->kind == SC_CLASS) n->flags |= NF_MEMBER_TEMPLATE;
        return y;
    }
    y = anew(sizeof(Sym));
    y->name = n;
    y->kind = kind;
    if (kind == SY_TEMPLATE && sc->kind == SC_CLASS) n->flags |= NF_MEMBER_TEMPLATE;
    y->next = sc->tab[n->hash & sc->tabmask];
    sc->tab[n->hash & sc->tabmask] = y;
    return y;
}

/* a name in a scope, its base classes and used namespaces (not the enclosing scopes) */
static Sym *lookup_member(Scope *sc, Name *n, int depth) {
    Sym *y = scope_find(sc, n);
    int i;
    if (y) return y;
    if (depth > 20) return 0;
    for (i = 0; i < sc->nbases; i++) {
        y = lookup_member(sc->bases[i], n, depth + 1);
        if (y) return y;
    }
    for (i = 0; i < sc->nusings; i++) {
        y = lookup_member(sc->usings[i], n, depth + 1);
        if (y) return y;
    }
    return 0;
}

/* an unqualified name: the scope chain outwards from sc */
static Sym *lookup_chain(Scope *sc, Name *n) {
    for (; sc; sc = sc->parent) {
        Sym *y = lookup_member(sc, n, 0);
        if (y) return y;
    }
    return 0;
}

static void add_base(Scope *cls, Scope *base) {
    cls->bases = xrealloc(cls->bases, sizeof(Scope *) * (cls->nbases + 1));
    cls->bases[cls->nbases++] = base;
}

static void add_using(Scope *ns, Scope *used) {
    ns->usings = xrealloc(ns->usings, sizeof(Scope *) * (ns->nusings + 1));
    ns->usings[ns->nusings++] = used;
}

/* ------------------------------------------------------------ parser state */

static Token **tk;
static int ntk, pos;
static int speculating = 0;
static jmp_buf spec_jmp;
static int no_gt = 0;            /* inside template arguments: '>' ends the expression */
static Name *class_name = 0;     /* the class being defined (for constructors), or 0 */
static Scope *class_scope = 0;
static int in_template_params = 0;
static int decl_ctx = 0;         /* the context of the declaration being parsed (CX_xxx) */

#define T0 (tk[pos])
#define T1 (tk[pos + 1])
#define T2 (tk[pos + 2])
#define TK(n) (tk[pos + (n)])

static Name *nm_override, *nm_final, *nm_noexcept_unused;

static void perr(const char *fmt, ...) {
    va_list ap;
    if (speculating) longjmp(spec_jmp, 1);
    va_start(ap, fmt);
    verror_at(T0->file, T0->line, fmt, ap);
    va_end(ap);
}

static int is_p(Token *t, int id) {
    return t->kind == K_PUNCT && t->id == id;
}

static int is_kw(Token *t, int kw) {
    return t->kind == K_IDENT && t->id == kw;
}

/* an identifier that is not a keyword */
static int is_ident(Token *t) {
    return t->kind == K_IDENT && t->id == 0;
}

static int at_p(int id) {
    return T0->kind == K_PUNCT && T0->id == id;
}

static int at_kw(int kw) {
    return T0->kind == K_IDENT && T0->id == kw;
}

static int accept_p(int id) {
    if (at_p(id)) {
        pos++;
        return 1;
    }
    return 0;
}

static int accept_kw(int kw) {
    if (at_kw(kw)) {
        pos++;
        return 1;
    }
    return 0;
}

static void expect_p(int id) {
    if (!accept_p(id)) perr("expected '%s' before '%s'", punct_spelling(id), T0->text);
}

static void expect_kw(int kw) {
    if (!accept_kw(kw)) perr("expected '%s' before '%s'", kw_spelling(kw), T0->text);
}

static Token *expect_ident(void) {
    Token *t = T0;
    if (!is_ident(t)) perr("expected an identifier before '%s'", t->text);
    pos++;
    return t;
}

/* the tokens that were replaced when '>>' was split, so that a trial parse can be undone */
static int *mut_pos = 0;
static Token **mut_tok = 0;
static int nmut = 0, mut_cap = 0;

/* the '>' that closes template arguments: '>>' and '>=' are split */
static void expect_gt(void) {
    if (accept_p(P_GT)) return;
    if (at_p(P_SHR) || at_p(P_GE) || at_p(P_SHREQ)) {
        Token *c = tok_copy(T0);
        if (nmut == mut_cap) {
            mut_cap = mut_cap ? mut_cap * 2 : 64;
            mut_pos = xrealloc(mut_pos, sizeof(int) * mut_cap);
            mut_tok = xrealloc(mut_tok, sizeof(Token *) * mut_cap);
        }
        mut_pos[nmut] = pos;
        mut_tok[nmut] = T0;
        nmut++;
        c->id = T0->id == P_SHR ? P_GT : T0->id == P_GE ? P_ASSIGN : P_GE;
        c->text = T0->text + 1;
        tk[pos] = c;
        return;
    }
    perr("expected '>' before '%s'", T0->text);
}

static Node *new_at(int kind) {
    return node_new(kind, T0);
}

/* appends n to the list whose last node is *tail */
static void list_add(Node **head, Node **tail, Node *n) {
    if (!n) return;
    if (*tail) (*tail)->next = n;
    else *head = n;
    *tail = n;
    while ((*tail)->next) *tail = (*tail)->next;
}

/* appends the list n to the list *head */
static void append_list(Node **head, Node *n) {
    if (!n) return;
    if (!*head) {
        *head = n;
        return;
    }
    {
        Node *t = *head;
        while (t->next) t = t->next;
        t->next = n;
    }
}

/* ------------------------------------------------------------ trial parsing */

typedef struct {
    int pos;
    Scope *scope;
    int no_gt;
    Name *class_name;
    Scope *class_scope;
    int nmut;
} Mark;

static void mark_save(Mark *m) {
    m->pos = pos;
    m->scope = cur_scope;
    m->no_gt = no_gt;
    m->class_name = class_name;
    m->class_scope = class_scope;
    m->nmut = nmut;
}

static void mark_restore(Mark *m) {
    pos = m->pos;
    cur_scope = m->scope;
    no_gt = m->no_gt;
    class_name = m->class_name;
    class_scope = m->class_scope;
    while (nmut > m->nmut) {
        nmut--;
        tk[mut_pos[nmut]] = mut_tok[nmut];
    }
}

#define SPEC_BEGIN(ok) \
    do { \
        Mark spec_mark; \
        jmp_buf spec_saved; \
        mark_save(&spec_mark); \
        memcpy(&spec_saved, &spec_jmp, sizeof(jmp_buf)); \
        speculating++; \
        if (setjmp(spec_jmp) == 0) {

/* SPEC_END keeps the position when the trial succeeds; SPEC_PROBE_END always resets it */
#define SPEC_END(ok) \
            ok = 1; \
        } else { \
            ok = 0; \
            mark_restore(&spec_mark); \
        } \
        speculating--; \
        memcpy(&spec_jmp, &spec_saved, sizeof(jmp_buf)); \
    } while (0)

#define SPEC_PROBE_END(ok) \
            ok = 1; \
        } else { \
            ok = 0; \
        } \
        mark_restore(&spec_mark); \
        speculating--; \
        memcpy(&spec_jmp, &spec_saved, sizeof(jmp_buf)); \
    } while (0)

/* ------------------------------------------------------------ forward declarations */

static Node *parse_expr(void);
static Node *parse_assign(void);
static Node *parse_cond(void);
static Node *parse_cast_expr(void);
static Node *parse_unary(void);
static Node *parse_braced_init(void);
static Node *parse_initializer_clause(void);
static Node *parse_type_id(void);
static Node *parse_decl_specs(int ctx);
static Node *parse_declarator(int mode);
static Node *parse_declaration(int ctx);
static Node *parse_simple_declaration(int ctx);
static Node *parse_namespace(void);
static Node *parse_statement(void);
static Node *parse_block(void);
static Node *parse_id(int typename_seen);
static Node *parse_template_args(void);
static Node *parse_class_specifier(int key);
static void parse_member_list(Node *cls);
static int starts_type(void);
static Node *parse_lambda(void);
static Node *parse_binary_expr(int minprec);
static Node *parse_postfix(void);
static int is_template_name(Name *n);

/* contexts of declarations */
enum { CX_NAMESPACE = 1, CX_CLASS, CX_BLOCK, CX_PARAM, CX_CONDITION, CX_TYPEID };
/* declarator modes */
enum { DM_NAMED = 1, DM_ABSTRACT, DM_EITHER };

static int is_template_name(Name *n) {
    Sym *y = lookup_chain(cur_scope, n);
    return y && y->kind == SY_TEMPLATE;
}

/* ------------------------------------------------------------ names */

/* the name of a class template inside that class (the injected class name) is a type */
static int is_injected_name(Sym *y) {
    Scope *s;
    for (s = cur_scope; s; s = s->parent)
        if (s->kind == SC_CLASS && s->name == y->name) return 1;
    return 0;
}

static int is_type_sym(Sym *y, Node *part_args) {
    if (!y) return 0;
    if (y->kind == SY_TYPE) return 1;
    if (y->kind == SY_TEMPLATE && y->type_template && (part_args || is_injected_name(y))) return 1;
    return 0;
}

/* the scope that a qualifier names (0 if it is unknown or dependent) */
static Scope *qualifier_scope(Sym *y, int *dependent) {
    *dependent = 0;
    if (!y) {
        *dependent = 1;
        return 0;
    }
    if (y->dependent) {
        *dependent = 1;
        return 0;
    }
    if (y->members) return y->members;
    *dependent = 1;
    return 0;
}

/* the last name of a qualified or unqualified type-id: "A::A", "A::~A" etc. */
static int part_count(Node *id) {
    int n = 0;
    Node *p;
    for (p = id->a; p; p = p->next) n++;
    return n;
}

/* operator name after the keyword 'operator' (the keyword is consumed here) */
static Node *parse_operator_name(void) {
    Node *part = new_at(N_PART);
    Token *t;
    pos++;                                   /* operator */
    t = T0;
    if (t->kind == K_STR && strcmp(t->text, "\"\"") == 0) {
        Token *u;
        pos++;
        u = expect_ident();
        part->op = 0;
        part->flags = PART_LITERAL;
        part->name = u->name;
        return part;
    }
    t = T0;
    if (t->kind == K_PUNCT) {
        int id = t->id;
        part->flags = PART_OPERATOR;
        if (id == P_LPAREN && is_p(T1, P_RPAREN)) {
            pos += 2;
            part->op = P_LPAREN;
            part->text = "()";
            return part;
        }
        if (id == P_LBRACK && is_p(T1, P_RBRACK)) {
            pos += 2;
            part->op = P_LBRACK;
            part->text = "[]";
            return part;
        }
        switch (id) {
        case P_PLUS: case P_MINUS: case P_STAR: case P_SLASH: case P_PERCENT: case P_CARET:
        case P_AMP: case P_PIPE: case P_TILDE: case P_NOT: case P_ASSIGN: case P_LT: case P_GT:
        case P_ADDEQ: case P_SUBEQ: case P_MULEQ: case P_DIVEQ: case P_MODEQ: case P_XOREQ:
        case P_ANDEQ: case P_OREQ: case P_SHL: case P_SHR: case P_SHLEQ: case P_SHREQ:
        case P_EQ: case P_NE: case P_LE: case P_GE: case P_ANDAND: case P_OROR: case P_INC:
        case P_DEC: case P_COMMA: case P_ARROWSTAR: case P_ARROW:
            pos++;
            part->op = id;
            return part;
        }
        perr("expected an operator after 'operator' before '%s'", t->text);
    }
    if (is_kw(t, KW_new) || is_kw(t, KW_delete)) {
        pos++;
        part->flags = PART_OPERATOR;
        part->op = OPKW + t->id;
        if (at_p(P_LBRACK) && is_p(T1, P_RBRACK)) {
            pos += 2;
            part->flags |= PART_ARRAY;
        }
        return part;
    }
    /* a conversion function: operator type-id (without a declarator that has a name) */
    part->flags = PART_CONV;
    {
        Node *ty = new_at(N_TYPEID);
        ty->a = parse_decl_specs(CX_TYPEID);
        /* a conversion-declarator: pointer operators only */
        {
            Node *ops = 0, *tail = 0;
            while (at_p(P_STAR) || at_p(P_AMP) || at_p(P_ANDAND)) {
                Node *o = new_at(at_p(P_STAR) ? N_D_PTR : at_p(P_AMP) ? N_D_REF : N_D_RREF);
                pos++;
                while (at_kw(KW_const) || at_kw(KW_volatile)) {
                    o->flags |= at_kw(KW_const) ? DS_CONST : DS_VOLATILE;
                    pos++;
                }
                if (tail) tail->a = o;
                else ops = o;
                tail = o;
            }
            if (tail) {
                Node *leaf = new_at(N_D_NAME);
                tail->a = leaf;
            }
            ty->b = ops;
        }
        part->a = ty;
    }
    return part;
}

/* the template arguments after '<' ... '>' */
static Node *parse_template_args(void) {
    Node *head = 0, *tail = 0;
    expect_p(P_LT);
    no_gt++;
    if (!at_p(P_GT) && !at_p(P_SHR)) {
        for (;;) {
            Node *arg = new_at(N_TARG);
            if (starts_type()) {
                arg->a = parse_type_id();
            } else {
                arg->a = parse_assign();
            }
            if (accept_p(P_ELLIPSIS)) {
                Node *pk = new_at(N_PACKEXP);
                pk->a = arg->a;
                arg->a = pk;
            }
            list_add(&head, &tail, arg);
            if (!accept_p(P_COMMA)) break;
        }
    }
    no_gt--;
    expect_gt();
    return head;
}

/*
 * An id-expression or type name: [::] [nested-name-specifier] unqualified-id.
 * typename_seen: the 'typename' keyword came before.  The result's sym is the
 * symbol of the last component, if it was found; node->flags tells whether the
 * qualifier was dependent through val (1).
 */
#define IDV_DEPENDENT 1
#define IDV_UNKNOWN_QUAL 2

static Node *parse_id(int typename_seen) {
    Node *id = new_at(N_ID);
    Node *tail = 0;
    Scope *qual = 0;             /* the scope to look the next name up in, if qualified */
    int qualified = 0, dependent = typename_seen ? 0 : 0;
    Sym *y = 0;
    if (typename_seen) id->flags |= ID_TYPENAME;
    if (at_p(P_SCOPE)) {
        pos++;
        id->flags |= ID_GLOBAL;
        qualified = 1;
        qual = global_scope;
    }
    for (;;) {
        Node *part;
        int tmpl_kw = 0;
        Token *t = T0;
        if (at_kw(KW_template) && qualified) {
            pos++;
            tmpl_kw = 1;
            id->flags |= ID_TEMPLATE;
            t = T0;
        }
        if (is_kw(t, KW_operator)) {
            part = parse_operator_name();
            list_add(&id->a, &tail, part);
            y = 0;
            break;
        }
        if (is_p(t, P_TILDE)) {
            pos++;
            part = new_at(N_PART);
            part->flags = PART_DTOR;
            if (at_kw(KW_decltype)) {
                perr("decltype in a destructor name is not supported");
            }
            part->name = expect_ident()->name;
            list_add(&id->a, &tail, part);
            y = 0;
            break;
        }
        if (!is_ident(t)) perr("expected a name before '%s'", t->text);
        pos++;
        part = node_new(N_PART, t);
        part->name = t->name;
        /* look the name up */
        if (qualified) {
            if (qual) y = lookup_member(qual, t->name, 0);
            else y = 0;
        } else {
            y = lookup_chain(cur_scope, t->name);
        }
        /* template arguments? */
        if (at_p(P_LT) && (tmpl_kw || (y && y->kind == SY_TEMPLATE))) {
            part->flags |= PART_ARGS;
            part->b = parse_template_args();
        }
        list_add(&id->a, &tail, part);
        if (at_p(P_SCOPE)) {
            int dep;
            Scope *next = 0;
            /* a qualifier: namespace, class or (dependent) type */
            if (y && y->kind == SY_TEMPLATE && (part->flags & PART_ARGS) == 0 && !tmpl_kw) {
                /* A::B where A is a template without arguments: injected class name */
            }
            next = qualifier_scope(y, &dep);
            if (!y && !qualified) dep = 1;
            if (qualified && !qual) dep = 1;
            if (y && y->dependent) dep = 1;
            if (y && y->kind == SY_TEMPLATE && (part->flags & PART_ARGS) && y->dependent) dep = 1;
            pos++;
            qualified = 1;
            qual = dep ? 0 : next;
            if (dep) dependent = 1;
            continue;
        }
        break;
    }
    id->sym = y;
    id->val = 0;
    if (dependent) id->val |= IDV_DEPENDENT;
    return id;
}

/* ------------------------------------------------------------ types */

static int id_names_type(Node *id) {
    Node *last = id->a;
    if (id->flags & ID_TYPENAME) return 1;
    if (!last) return 0;
    while (last->next) last = last->next;
    if (last->flags & (PART_OPERATOR | PART_CONV | PART_DTOR | PART_LITERAL)) return 0;
    return is_type_sym(id->sym, (last->flags & PART_ARGS) ? last : 0);
}

/* the type keywords that can start a decl-specifier-seq (not names) */
static int is_type_keyword(Token *t) {
    if (t->kind != K_IDENT) return 0;
    switch (t->id) {
    case KW_void: case KW_bool: case KW_char: case KW_char16_t: case KW_char32_t: case KW_wchar_t:
    case KW_short: case KW_int: case KW_long: case KW_signed: case KW_unsigned: case KW_float:
    case KW_double: case KW_auto: case KW_decltype: case KW_class: case KW_struct: case KW_union:
    case KW_enum: case KW_typename: case KW_const: case KW_volatile:
        return 1;
    }
    return 0;
}

/* does a type-id start at the current token?  A name only counts if it is a known type. */
static int starts_type(void) {
    Token *t = T0;
    int ok, result = 0;
    if (is_type_keyword(t)) return 1;
    if (!(is_ident(t) || is_p(t, P_SCOPE))) return 0;
    SPEC_BEGIN(ok)
        Node *id = parse_id(0);
        result = id_names_type(id);
    SPEC_PROBE_END(ok);
    return ok && result;
}

/* the name that a declarator would define is a constructor-like name: A::A, A::~A, A::operator T */
static int is_ctor_like(Node *id, int next_is_paren) {
    Node *last = id->a, *prev = 0;
    if (!last) return 0;
    while (last->next) {
        prev = last;
        last = last->next;
    }
    if (last->flags & (PART_DTOR | PART_CONV | PART_OPERATOR | PART_LITERAL)) return prev != 0;
    if (!next_is_paren) return 0;
    if (prev && prev->name == last->name) return 1;
    return 0;
}

/* attributes [[...]] and alignas(...): skipped, kept as one node each */
static Node *parse_attributes(void) {
    Node *head = 0, *tail = 0;
    for (;;) {
        if (at_p(P_LBRACK) && is_p(T1, P_LBRACK)) {
            Node *a = new_at(N_ATTR);
            int depth = 0;
            StrBuf sb;
            sb_init(&sb);
            pos += 2;
            for (;;) {
                if (T0->kind == K_EOF) perr("unterminated attribute");
                if (at_p(P_RBRACK) && is_p(T1, P_RBRACK) && depth == 0) {
                    pos += 2;
                    break;
                }
                if (at_p(P_LBRACK) || at_p(P_LPAREN) || at_p(P_LBRACE)) depth++;
                if (at_p(P_RBRACK) || at_p(P_RPAREN) || at_p(P_RBRACE)) depth--;
                sb_puts(&sb, T0->text);
                pos++;
            }
            a->text = sb.s;
            list_add(&head, &tail, a);
        } else if (at_kw(KW_alignas)) {
            Node *a = new_at(N_ATTR);
            int depth = 0;
            StrBuf sb;
            sb_init(&sb);
            sb_puts(&sb, "alignas");
            pos++;
            expect_p(P_LPAREN);
            for (;;) {
                if (T0->kind == K_EOF) perr("unterminated alignas");
                if (at_p(P_RPAREN) && depth == 0) {
                    pos++;
                    break;
                }
                if (at_p(P_LPAREN)) depth++;
                if (at_p(P_RPAREN)) depth--;
                sb_puts(&sb, T0->text);
                pos++;
            }
            a->text = sb.s;
            list_add(&head, &tail, a);
        } else break;
    }
    return head;
}

static void add_builtin(Node *ts, int bit) {
    i64 v = ts->val;
    if (bit == BT_LONG && (v & BT_LONG)) {
        ts->val = (v & ~BT_LONG) | BT_LONG2;
        return;
    }
    if ((v & bit) && bit != BT_LONG) perr("duplicate type specifier '%s'", T0->text);
    ts->val = v | bit;
}

/* skips a balanced ( ... ) group: the current token is '(' */
static void skip_parens(void) {
    int depth = 0;
    for (;;) {
        if (T0->kind == K_EOF) perr("unbalanced parentheses");
        if (at_p(P_LPAREN)) depth++;
        if (at_p(P_RPAREN)) {
            depth--;
            if (depth == 0) {
                pos++;
                return;
            }
        }
        pos++;
    }
}

/*
 * A decl-specifier-seq.  ctx says what follows (CX_PARAM, CX_TYPEID ...: no
 * storage classes).  Returns an N_TYPE node; a = the named type, class or enum.
 * *The type may be absent* (constructors, destructors, conversion functions):
 * then val == 0 and a == 0.
 */
static Node *parse_decl_specs(int ctx) {
    Node *ts = new_at(N_TYPE);
    int seen_type = 0;
    for (;;) {
        Token *t = T0;
        if (t->kind == K_IDENT && t->id) {
            switch (t->id) {
            case KW_typedef: ts->flags |= DS_TYPEDEF; pos++; continue;
            case KW_static: ts->flags |= DS_STATIC; pos++; continue;
            case KW_extern:
                if (is_p(T1, P_LBRACE) || T1->kind == K_STR) break;
                ts->flags |= DS_EXTERN; pos++; continue;
            case KW_inline: ts->flags |= DS_INLINE; pos++; continue;
            case KW_virtual: ts->flags |= DS_VIRTUAL; pos++; continue;
            case KW_explicit:
                /* explicit(bool) is C++20 */
                ts->flags |= DS_EXPLICIT; pos++; continue;
            case KW_friend: ts->flags |= DS_FRIEND; pos++; continue;
            case KW_constexpr: ts->flags |= DS_CONSTEXPR; pos++; continue;
            case KW_mutable: ts->flags |= DS_MUTABLE; pos++; continue;
            case KW_register: ts->flags |= DS_REGISTER; pos++; continue;
            case KW_thread_local: ts->flags |= DS_THREAD; pos++; continue;
            case KW_const: ts->flags |= DS_CONST; pos++; continue;
            case KW_volatile: ts->flags |= DS_VOLATILE; pos++; continue;
            case KW_alignas:
                append_list(&ts->d, parse_attributes());
                continue;
            case KW_void: if (seen_type == 1) break; add_builtin(ts, BT_VOID); seen_type = 2; pos++; continue;
            case KW_bool: if (seen_type == 1) break; add_builtin(ts, BT_BOOL); seen_type = 2; pos++; continue;
            case KW_char: if (seen_type == 1) break; add_builtin(ts, BT_CHAR); seen_type = 2; pos++; continue;
            case KW_wchar_t: if (seen_type == 1) break; add_builtin(ts, BT_WCHAR); seen_type = 2; pos++; continue;
            case KW_char16_t: if (seen_type == 1) break; add_builtin(ts, BT_CHAR16); seen_type = 2; pos++; continue;
            case KW_char32_t: if (seen_type == 1) break; add_builtin(ts, BT_CHAR32); seen_type = 2; pos++; continue;
            case KW_short: if (seen_type == 1) break; add_builtin(ts, BT_SHORT); seen_type = 2; pos++; continue;
            case KW_int: if (seen_type == 1) break; add_builtin(ts, BT_INT); seen_type = 2; pos++; continue;
            case KW_long: if (seen_type == 1) break; add_builtin(ts, BT_LONG); seen_type = 2; pos++; continue;
            case KW_signed: if (seen_type == 1) break; add_builtin(ts, BT_SIGNED); seen_type = 2; pos++; continue;
            case KW_unsigned: if (seen_type == 1) break; add_builtin(ts, BT_UNSIGNED); seen_type = 2; pos++; continue;
            case KW_float: if (seen_type == 1) break; add_builtin(ts, BT_FLOAT); seen_type = 2; pos++; continue;
            case KW_double: if (seen_type == 1) break; add_builtin(ts, BT_DOUBLE); seen_type = 2; pos++; continue;
            case KW_auto: if (seen_type == 1) break; add_builtin(ts, BT_AUTO); seen_type = 2; pos++; continue;
            case KW_decltype:
                if (seen_type) break;
                pos++;
                {
                    Node *dt = new_at(N_TYPE_EXPR);
                    expect_p(P_LPAREN);
                    dt->op = OPKW + KW_decltype;
                    dt->a = parse_expr();
                    expect_p(P_RPAREN);
                    ts->a = dt;
                }
                seen_type = 1;
                continue;
            case KW_class: case KW_struct: case KW_union:
                if (seen_type) break;
                ts->a = parse_class_specifier(t->id);
                seen_type = 1;
                continue;
            case KW_enum:
                if (seen_type) break;
                {
                    Node *e = new_at(N_ENUM);
                    int named = 0;
                    pos++;
                    if (at_kw(KW_class) || at_kw(KW_struct)) {
                        e->flags |= EN_CLASS;
                        pos++;
                    }
                    append_list(&e->d, parse_attributes());
                    if (is_ident(T0) || at_p(P_SCOPE)) {
                        e->a = parse_id(0);
                        named = 1;
                    }
                    if (accept_p(P_COLON)) e->b = parse_type_id();
                    if (at_p(P_LBRACE)) {
                        Node *ehead = 0, *etail = 0;
                        Scope *escope = 0;
                        Sym *esym = 0;
                        if (named) {
                            Node *lp = e->a->a;
                            while (lp->next) lp = lp->next;
                            esym = declare(cur_scope, lp->name, SY_TYPE);
                            if (!esym->members) esym->members = scope_new(SC_CLASS, cur_scope, lp->name);
                            escope = esym->members;
                        }
                        pos++;
                        while (!at_p(P_RBRACE)) {
                            Node *en = new_at(N_ENUMERATOR);
                            Token *nt = expect_ident();
                            en->name = nt->name;
                            if (accept_p(P_ASSIGN)) en->a = parse_cond();
                            list_add(&ehead, &etail, en);
                            if (e->flags & EN_CLASS) {
                                if (escope) declare(escope, nt->name, SY_VAR);
                            } else {
                                declare(cur_scope, nt->name, SY_VAR);
                                if (escope) declare(escope, nt->name, SY_VAR);
                            }
                            if (!accept_p(P_COMMA)) break;
                        }
                        expect_p(P_RBRACE);
                        e->c = ehead;
                    } else {
                        e->flags |= EN_OPAQUE;
                        if (named && !(e->a->flags & ID_GLOBAL) && part_count(e->a) == 1) {
                            Sym *esym = declare(cur_scope, e->a->a->name, SY_TYPE);
                            if (!esym->members) esym->members = scope_new(SC_CLASS, cur_scope, e->a->a->name);
                        }
                    }
                    ts->a = e;
                }
                seen_type = 1;
                continue;
            case KW_typename:
                if (seen_type) break;
                pos++;
                ts->a = parse_id(1);
                seen_type = 1;
                continue;
            }
            break;
        }
        if (t->kind == K_IDENT && t->id == 0 && !seen_type) {
            /* attributes after the specifiers are handled by the callers */
        }
        if (t->kind == K_PUNCT && t->id == P_LBRACK && is_p(T1, P_LBRACK)) {
            append_list(&ts->d, parse_attributes());
            continue;
        }
        if (seen_type == 0 && (is_ident(t) || is_p(t, P_SCOPE))) {
            /* a type name, unless this is a constructor/destructor/conversion name */
            Mark m;
            Node *id;
            int ok, take = 0;
            mark_save(&m);
            SPEC_BEGIN(ok)
                id = parse_id(0);
                if (ctx != CX_TYPEID && is_ctor_like(id, at_p(P_LPAREN))) take = 0;
                else if (id_names_type(id)) take = 1;
                else if (ctx != CX_PARAM && ctx != CX_TYPEID && T0->kind == K_IDENT && T0->id == 0
                         && !is_kw(T1, KW_operator)) take = 2;   /* "unknown_type name" */
                else if ((ctx == CX_CLASS || ctx == CX_NAMESPACE) && (at_p(P_STAR) || at_p(P_AMP) || at_p(P_ANDAND))
                         && (is_ident(T1) || is_p(T1, P_STAR) || is_p(T1, P_AMP) || is_kw(T1, KW_const))) take = 2;
                else if (ctx == CX_PARAM && T0->kind == K_IDENT && T0->id == 0) take = 2;
                else if (ctx == CX_PARAM && (at_p(P_COMMA) || at_p(P_RPAREN) || at_p(P_ELLIPSIS) || at_p(P_ASSIGN)
                         || at_p(P_STAR) || at_p(P_AMP) || at_p(P_ANDAND))) take = 2;
                else if (ctx == CX_TYPEID) take = 2;
                else if (class_name && part_count(id) == 1 && id->a->name == class_name && at_p(P_LPAREN)) take = 0;
            SPEC_END(ok);
            if (ok && take) {
                if (ctx != CX_TYPEID && class_name && part_count(id) == 1 && id->a->name == class_name && at_p(P_LPAREN)) {
                    mark_restore(&m);
                    break;
                }
                ts->a = id;
                seen_type = 1;
                continue;
            }
            mark_restore(&m);
            break;
        }
        break;
    }
    return ts;
}

/* ------------------------------------------------------------ declarators */

static Node *parse_declarator(int mode);

/* cv-qualifier-seq as DS_CONST / DS_VOLATILE */
static int parse_cv(void) {
    int f = 0;
    for (;;) {
        if (at_kw(KW_const)) f |= DS_CONST;
        else if (at_kw(KW_volatile)) f |= DS_VOLATILE;
        else return f;
        pos++;
    }
}

/* a parameter-declaration-clause; the '(' has been consumed.  Sets *variadic. */
static Node *parse_params(int *variadic) {
    Node *head = 0, *tail = 0;
    *variadic = 0;
    if (at_p(P_RPAREN)) return 0;
    for (;;) {
        Node *p;
        if (at_p(P_ELLIPSIS)) {
            pos++;
            *variadic = 1;
            break;
        }
        p = new_at(N_PARAM);
        append_list(&p->d, parse_attributes());
        p->a = parse_decl_specs(CX_PARAM);
        if (!at_p(P_COMMA) && !at_p(P_RPAREN) && !at_p(P_ASSIGN) && !at_p(P_ELLIPSIS))
            p->b = parse_declarator(DM_EITHER);
        else if (at_p(P_ELLIPSIS) && !is_p(T1, P_RPAREN) && !is_p(T1, P_COMMA)) p->b = parse_declarator(DM_EITHER);
        if (at_p(P_ELLIPSIS)) {
            /* a parameter pack: T... or T... name (the pack belongs to the declarator) */
            Node *pk = new_at(N_D_PACK);
            pos++;
            pk->a = p->b;
            p->b = pk;
        }
        if (accept_p(P_ASSIGN)) {
            no_gt++;
            p->c = parse_initializer_clause();
            no_gt--;
        }
        list_add(&head, &tail, p);
        if (accept_p(P_COMMA)) {
            if (at_p(P_ELLIPSIS)) {
                pos++;
                *variadic = 1;
                break;
            }
            continue;
        }
        if (at_p(P_ELLIPSIS)) {
            pos++;
            *variadic = 1;
        }
        break;
    }
    return head;
}

/* dynamic exception specification throw(...) and noexcept(...) */
static void parse_exception_spec(Node *fn) {
    if (at_kw(KW_noexcept)) {
        pos++;
        fn->flags |= FQ_NOEXCEPT;
        if (at_p(P_LPAREN)) {
            pos++;
            fn->d = parse_cond();
            expect_p(P_RPAREN);
        }
    } else if (at_kw(KW_throw) && is_p(T1, P_LPAREN)) {
        pos++;
        fn->flags |= FQ_THROW;
        skip_parens();
    }
}

/* the suffixes after a function declarator's parameter list */
static void parse_function_tail(Node *fn) {
    for (;;) {
        if (at_kw(KW_const)) { fn->flags |= FQ_CONST; pos++; continue; }
        if (at_kw(KW_volatile)) { fn->flags |= FQ_VOLATILE; pos++; continue; }
        if (at_p(P_AMP)) { fn->flags |= FQ_REF; pos++; continue; }
        if (at_p(P_ANDAND)) { fn->flags |= FQ_RREF; pos++; continue; }
        break;
    }
    parse_exception_spec(fn);
    append_list(&fn->d, parse_attributes());
    if (accept_p(P_ARROW)) fn->c = parse_type_id();
    for (;;) {
        if (is_ident(T0) && T0->name == nm_override) { fn->flags |= FQ_OVERRIDE; pos++; continue; }
        if (is_ident(T0) && T0->name == nm_final) { fn->flags |= FQ_FINAL; pos++; continue; }
        break;
    }
}

/* a nested-name-specifier followed by '*' (a pointer to member: A::B::*): returns the class N_ID and leaves
   the position at the '*', or returns 0 and leaves the position unchanged */
static Node *match_memptr_prefix(void) {
    int ok, isptr = 0;
    Node *id = 0;
    Mark m;
    mark_save(&m);
    SPEC_BEGIN(ok)
        Node *tail = 0;
        id = new_at(N_ID);
        if (accept_p(P_SCOPE)) id->flags |= ID_GLOBAL;
        for (;;) {
            Token *t = expect_ident();
            Node *part = node_new(N_PART, t);
            part->name = t->name;
            if (at_p(P_LT)) {
                part->flags |= PART_ARGS;
                part->b = parse_template_args();
            }
            list_add(&id->a, &tail, part);
            expect_p(P_SCOPE);
            if (at_p(P_STAR)) {
                isptr = 1;
                break;
            }
        }
    SPEC_END(ok);
    if (!ok || !isptr) {
        mark_restore(&m);
        return 0;
    }
    return id;
}

/* does '(' at the current token start a nested declarator rather than a parameter list? */
static int paren_is_nested_declarator(int mode) {
    Token *t = T1;
    if (is_p(t, P_STAR) || is_p(t, P_AMP) || is_p(t, P_ANDAND)) return 1;
    if (is_p(t, P_LPAREN)) return mode != DM_NAMED || 1;
    if (is_p(t, P_SCOPE) || is_ident(t)) {
        /* A::* ... is a pointer to member; a plain unknown name in a named declarator is a nested declarator */
        int ok, result = 0, start = pos;
        Node *cls;
        pos++;
        cls = match_memptr_prefix();
        if (cls) {
            pos = start;
            return 1;
        }
        SPEC_BEGIN(ok)
            Node *id = parse_id(0);
            if (at_p(P_STAR)) result = 1;
            else if (mode != DM_ABSTRACT && !id_names_type(id)) result = 1;
        SPEC_PROBE_END(ok);
        pos--;
        return ok && result;
    }
    if (is_p(t, P_ELLIPSIS)) return 0;
    return 0;
}

/* a '(' after a declarator name: a parameter list, or the parenthesized initializer of a variable? */
static int paren_starts_params(int mode) {
    Token *t = T1;
    if (mode == DM_ABSTRACT) return 1;
    if (is_p(t, P_RPAREN) || is_p(t, P_ELLIPSIS)) return 1;
    if (t->kind == K_IDENT && t->id) {
        if (is_type_keyword(t)) return 1;
        if (t->id == KW_register || t->id == KW_alignas) return 1;
        return 0;
    }
    if (is_p(t, P_LBRACK) && is_p(T2, P_LBRACK)) return 1;
    if (is_ident(t) || is_p(t, P_SCOPE)) {
        int ok, result = 0;
        pos++;
        if (starts_type()) result = 1;
        else if (decl_ctx == CX_CLASS || decl_ctx == CX_NAMESPACE || decl_ctx == CX_PARAM) {
            /* an unknown name in a declaration is a type: (Unknown x), (Unknown *), (Unknown) ... */
            SPEC_BEGIN(ok)
                Node *pid = parse_id(0);
                if (!pid->sym && (is_ident(T0) || at_p(P_RPAREN) || at_p(P_COMMA) || at_p(P_STAR) || at_p(P_AMP) || at_p(P_ANDAND)
                    || at_p(P_LBRACK) || at_p(P_ELLIPSIS)))
                    result = 1;
            SPEC_PROBE_END(ok);
            if (decl_ctx == CX_NAMESPACE && result && !ok) result = 0;
        }
        pos--;
        return result;
    }
    return 0;
}

/* a declarator-id in a named declarator */
static Node *parse_declarator_id(void) {
    Node *d = new_at(N_D_NAME);
    d->a = parse_id(0);
    return d;
}

static Node *parse_direct_declarator(int mode) {
    Node *cur = 0;
    int is_pack = 0;
    if (at_p(P_ELLIPSIS)) {
        pos++;
        is_pack = 1;
    }
    if (at_p(P_LPAREN) && paren_is_nested_declarator(mode)) {
        pos++;
        cur = parse_declarator(mode);
        expect_p(P_RPAREN);
    } else if (is_ident(T0) || at_p(P_SCOPE) || at_kw(KW_operator) || at_p(P_TILDE)
               || (at_kw(KW_template) && 0)) {
        if (mode == DM_ABSTRACT) {
            cur = new_at(N_D_NAME);
        } else {
            cur = parse_declarator_id();
        }
    } else {
        cur = new_at(N_D_NAME);          /* no name: an abstract declarator */
        if (mode == DM_NAMED && !(at_p(P_COLON) && decl_ctx == CX_CLASS))
            perr("expected a declarator name before '%s'", T0->text);
    }
    if (is_pack) {
        Node *pk = new_at(N_D_PACK);
        pk->a = cur;
        cur = pk;
    }
    for (;;) {
        if (at_p(P_LBRACK) && !is_p(T1, P_LBRACK)) {
            Node *ar = new_at(N_D_ARRAY);
            pos++;
            ar->a = cur;
            if (!at_p(P_RBRACK)) ar->b = parse_cond();
            expect_p(P_RBRACK);
            append_list(&ar->d, parse_attributes());
            cur = ar;
            continue;
        }
        if (at_p(P_LPAREN) && paren_starts_params(mode)) {
            Node *fn = new_at(N_D_FUNC);
            int variadic;
            pos++;
            fn->a = cur;
            fn->b = parse_params(&variadic);
            if (variadic) fn->flags |= FQ_VARIADIC;
            expect_p(P_RPAREN);
            parse_function_tail(fn);
            cur = fn;
            continue;
        }
        break;
    }
    return cur;
}

/* ptr-operator* direct-declarator */
static Node *parse_declarator(int mode) {
    Node *first = 0, *last = 0;
    for (;;) {
        Node *o = 0;
        if (at_p(P_STAR)) {
            o = new_at(N_D_PTR);
            pos++;
            append_list(&o->d, parse_attributes());
            o->flags = parse_cv();
        } else if (at_p(P_AMP)) {
            o = new_at(N_D_REF);
            pos++;
            append_list(&o->d, parse_attributes());
        } else if (at_p(P_ANDAND)) {
            o = new_at(N_D_RREF);
            pos++;
            append_list(&o->d, parse_attributes());
        } else if (at_p(P_SCOPE) || (is_ident(T0) && (is_p(T1, P_SCOPE) || is_p(T1, P_LT)))) {
            /* a pointer to member: A::B::* */
            Node *cls = match_memptr_prefix();
            if (!cls) break;
            pos++;                                   /* * */
            o = new_at(N_D_MEMPTR);
            o->b = cls;
            o->flags = parse_cv();
        } else break;
        if (last) last->a = o;
        else first = o;
        last = o;
    }
    {
        Node *direct = parse_direct_declarator(mode);
        if (last) {
            last->a = direct;
            return first;
        }
        return direct;
    }
}

/* the innermost D_NAME's id of a declarator, or 0; *fn = the D_FUNC directly around it */
static Node *declarator_name_id(Node *d) {
    while (d) {
        if (d->kind == N_D_NAME) return d->a;
        d = d->a;
    }
    return 0;
}

static Node *declarator_function(Node *d) {
    /* the function declarator that declares the name: the first D_FUNC whose inner is the name (or a paren/pointer chain) */
    Node *r = 0;
    while (d) {
        if (d->kind == N_D_FUNC) r = d;
        d = d->a;
    }
    return r;
}

/* a type-id: type specifiers and an abstract declarator */
static Node *parse_type_id(void) {
    Node *t = new_at(N_TYPEID);
    t->a = parse_decl_specs(CX_TYPEID);
    if (!t->a->a && !t->a->val && !(t->a->flags & (DS_CONST | DS_VOLATILE))) perr("expected a type before '%s'", T0->text);
    if (!at_p(P_RPAREN) && !at_p(P_GT) && !at_p(P_COMMA) && !at_p(P_SEMI) && !at_p(P_RBRACK)
        && !at_p(P_ASSIGN) && !at_p(P_LBRACE) && !at_p(P_ELLIPSIS) && !at_p(P_SHR) && !at_p(P_COLON)
        && !at_p(P_QUESTION))
        t->b = parse_declarator(DM_ABSTRACT);
    else if (at_p(P_ELLIPSIS)) {
        /* T... : the pack expansion is handled by the caller */
    }
    return t;
}

/* ------------------------------------------------------------ expressions */

static Node *parse_expr(void) {
    Node *e = parse_assign();
    while (at_p(P_COMMA)) {
        Node *c = new_at(N_COMMA);
        pos++;
        c->a = e;
        c->b = parse_assign();
        e = c;
    }
    return e;
}

/* an initializer-clause: an assignment-expression or a braced-init-list */
static Node *parse_initializer_clause(void) {
    if (at_p(P_LBRACE)) return parse_braced_init();
    return parse_assign();
}

static Node *parse_braced_init(void) {
    Node *l = new_at(N_INITLIST);
    Node *tail = 0;
    int saved = no_gt;
    no_gt = 0;
    expect_p(P_LBRACE);
    while (!at_p(P_RBRACE)) {
        Node *e = parse_initializer_clause();
        if (at_p(P_ELLIPSIS)) {
            Node *pk = new_at(N_PACKEXP);
            pos++;
            pk->a = e;
            e = pk;
        }
        list_add(&l->a, &tail, e);
        if (!accept_p(P_COMMA)) break;
    }
    expect_p(P_RBRACE);
    no_gt = saved;
    return l;
}

/* expression-list: the arguments of a call or a parenthesized initializer; ')' is not consumed */
static Node *parse_arg_list(void) {
    Node *head = 0, *tail = 0;
    int saved = no_gt;
    no_gt = 0;
    if (!at_p(P_RPAREN)) {
        for (;;) {
            Node *e = parse_initializer_clause();
            if (at_p(P_ELLIPSIS)) {
                Node *pk = new_at(N_PACKEXP);
                pos++;
                pk->a = e;
                e = pk;
            }
            list_add(&head, &tail, e);
            if (!accept_p(P_COMMA)) break;
        }
    }
    no_gt = saved;
    return head;
}

static Node *parse_assign(void) {
    Node *l;
    if (at_kw(KW_throw)) {
        Node *t = new_at(N_THROW);
        pos++;
        if (!at_p(P_RPAREN) && !at_p(P_SEMI) && !at_p(P_COMMA) && !at_p(P_COLON) && !at_p(P_RBRACE)
            && !at_p(P_RBRACK) && !(at_p(P_GT) && no_gt))
            t->a = parse_assign();
        return t;
    }
    l = parse_cond();
    switch (T0->kind == K_PUNCT ? T0->id : 0) {
    case P_ASSIGN: case P_ADDEQ: case P_SUBEQ: case P_MULEQ: case P_DIVEQ: case P_MODEQ:
    case P_XOREQ: case P_ANDEQ: case P_OREQ: case P_SHLEQ: case P_SHREQ:
        {
            Node *a = new_at(N_ASSIGN);
            a->op = T0->id;
            pos++;
            a->a = l;
            a->b = parse_initializer_clause();
            return a;
        }
    }
    return l;
}

static Node *parse_cond(void) {
    Node *c = parse_binary_expr(1);
    if (at_p(P_QUESTION)) {
        Node *n = new_at(N_COND);
        int saved = no_gt;
        pos++;
        n->a = c;
        no_gt = 0;
        n->b = parse_expr();
        no_gt = saved;
        expect_p(P_COLON);
        n->c = parse_assign();
        return n;
    }
    return c;
}

static int binary_prec(Token *t) {
    if (t->kind != K_PUNCT) return 0;
    switch (t->id) {
    case P_OROR: return 1;
    case P_ANDAND: return 2;
    case P_PIPE: return 3;
    case P_CARET: return 4;
    case P_AMP: return 5;
    case P_EQ: case P_NE: return 6;
    case P_LT: case P_LE: case P_GE: return 7;
    case P_GT: return no_gt ? 0 : 7;
    case P_SHL: return 8;
    case P_SHR: return no_gt ? 0 : 8;
    case P_PLUS: case P_MINUS: return 9;
    case P_STAR: case P_SLASH: case P_PERCENT: return 10;
    case P_DOTSTAR: case P_ARROWSTAR: return 11;
    }
    return 0;
}

static Node *parse_binary_expr(int minprec) {
    Node *l = parse_cast_expr();
    for (;;) {
        Token *t = T0;
        int p = binary_prec(t);
        Node *n;
        if (p == 0 || p < minprec) return l;
        n = new_at(p == 11 ? N_PTRMEM : N_BINARY);
        n->op = t->id;
        pos++;
        n->a = l;
        n->b = parse_binary_expr(p + 1);
        l = n;
    }
}

/* the tokens after ')' that make a parenthesized type a cast */
static int starts_unary_after_cast(void) {
    Token *t = T0;
    switch (t->kind) {
    case K_NUM: case K_CHAR: case K_STR: return 1;
    case K_IDENT: return 1;
    case K_PUNCT:
        switch (t->id) {
        case P_LPAREN: case P_PLUS: case P_MINUS: case P_STAR: case P_AMP: case P_NOT: case P_TILDE:
        case P_INC: case P_DEC: case P_SCOPE: case P_LBRACK: case P_LBRACE:
            return 1;
        }
    }
    return 0;
}

static Node *parse_cast_expr(void) {
    if (at_p(P_LPAREN) && !is_p(T1, P_RPAREN)) {
        /* (type) cast-expression */
        int ok, is_cast = 0;
        Mark m;
        Node *ty = 0;
        pos++;
        if (starts_type()) {
            mark_save(&m);
            m.pos--;
            SPEC_BEGIN(ok)
                ty = parse_type_id();
                if (at_p(P_RPAREN)) {
                    pos++;
                    is_cast = starts_unary_after_cast();
                }
            SPEC_END(ok);
            if (ok && is_cast) {
                Node *c = node_new(N_CAST, tk[m.pos]);
                c->b = ty;
                if (at_p(P_LBRACE)) {
                    /* (T){...}: a compound literal */
                    c->a = parse_braced_init();
                } else c->a = parse_cast_expr();
                return c;
            }
            mark_restore(&m);
        } else pos--;
    }
    return parse_unary();
}

/* new-type-id: type specifiers and pointer operators; array bounds go to n->d (the first) and the type */
static Node *parse_new_type_id(Node *n) {
    Node *t = new_at(N_TYPEID);
    Node *first = 0, *last = 0;
    t->a = parse_decl_specs(CX_TYPEID);
    for (;;) {
        Node *o;
        if (!at_p(P_STAR)) break;
        o = new_at(N_D_PTR);
        pos++;
        o->flags = parse_cv();
        if (last) last->a = o;
        else first = o;
        last = o;
    }
    if (last) last->a = new_at(N_D_NAME);
    t->b = first;
    if (at_p(P_LBRACK) && !is_p(T1, P_LBRACK)) {
        Node *tail = 0;
        n->flags |= 4;                       /* an array */
        while (at_p(P_LBRACK)) {
            Node *dim = new_at(N_D_ARRAY);
            pos++;
            dim->b = parse_cond();
            expect_p(P_RBRACK);
            list_add(&n->d, &tail, dim);
        }
    }
    return t;
}

static Node *parse_unary(void) {
    Token *t = T0;
    Node *n;
    if (t->kind == K_PUNCT) {
        switch (t->id) {
        case P_INC: case P_DEC: case P_STAR: case P_AMP: case P_PLUS: case P_MINUS: case P_NOT: case P_TILDE:
            n = new_at(N_UNARY);
            n->op = t->id;
            pos++;
            n->a = parse_cast_expr();
            return n;
        case P_SCOPE:
            if (is_kw(T1, KW_new) || is_kw(T1, KW_delete)) {
                /* ::new, ::delete: the global flag is set below */
                break;
            }
            break;
        }
    }
    if (t->kind == K_IDENT && t->id) {
        switch (t->id) {
        case KW_sizeof:
            n = new_at(N_SIZEOF);
            pos++;
            if (at_p(P_ELLIPSIS)) {
                pos++;
                n->flags = 1;
                expect_p(P_LPAREN);
                n->a = new_at(N_ID);
                {
                    Token *nt = expect_ident();
                    Node *part = node_new(N_PART, nt);
                    part->name = nt->name;
                    n->a->a = part;
                }
                expect_p(P_RPAREN);
                return n;
            }
            if (at_p(P_LPAREN)) {
                pos++;
                if (starts_type()) {
                    n->b = parse_type_id();
                    expect_p(P_RPAREN);
                    return n;
                }
                pos--;
            }
            n->a = parse_unary();
            return n;
        case KW_alignof:
            n = new_at(N_ALIGNOF);
            pos++;
            expect_p(P_LPAREN);
            n->b = parse_type_id();
            expect_p(P_RPAREN);
            return n;
        case KW_noexcept:
            n = new_at(N_NOEXCEPT_EXPR);
            pos++;
            expect_p(P_LPAREN);
            n->a = parse_expr();
            expect_p(P_RPAREN);
            return n;
        case KW_new:
        case KW_delete:
            break;
        }
    }
    if (t->kind == K_IDENT && t->name && t->name->s[0] == '_' && t->name->s[1] == '_'
        && (strncmp(t->name->s, "__is_", 5) == 0 || strncmp(t->name->s, "__has_", 6) == 0) && is_p(T1, P_LPAREN)) {
        /* a type trait of the compiler: __is_class(T), __is_base_of(B, D) */
        n = new_at(N_TYPETRAIT);
        n->name = t->name;
        pos += 2;
        n->a = parse_type_id();
        if (accept_p(P_COMMA)) n->b = parse_type_id();
        expect_p(P_RPAREN);
        return n;
    }
    if ((t->kind == K_IDENT && (t->id == KW_new || t->id == KW_delete))
        || (is_p(t, P_SCOPE) && (is_kw(T1, KW_new) || is_kw(T1, KW_delete)))) {
        int global = 0;
        if (at_p(P_SCOPE)) {
            global = 1;
            pos++;
        }
        if (at_kw(KW_new)) {
            n = new_at(N_NEW);
            n->flags = global;
            pos++;
            if (at_p(P_LPAREN)) {
                /* placement: new (p) T, or a parenthesized type: new (T) */
                int ok;
                Node *pl = 0;
                pos++;
                if (starts_type()) {
                    /* new (T)... */
                    Mark m;
                    mark_save(&m);
                    m.pos--;
                    SPEC_BEGIN(ok)
                        n->b = parse_type_id();
                        expect_p(P_RPAREN);
                    SPEC_END(ok);
                    if (!ok) {
                        mark_restore(&m);
                        pos++;
                        pl = parse_arg_list();
                        expect_p(P_RPAREN);
                        n->a = pl;
                        n->b = parse_new_type_id(n);
                    }
                } else {
                    pl = parse_arg_list();
                    expect_p(P_RPAREN);
                    n->a = pl;
                    if (at_p(P_LPAREN)) {
                        pos++;
                        n->b = parse_type_id();
                        expect_p(P_RPAREN);
                    } else n->b = parse_new_type_id(n);
                }
            } else {
                n->b = parse_new_type_id(n);
            }
            if (at_p(P_LPAREN)) {
                pos++;
                n->c = parse_arg_list();
                expect_p(P_RPAREN);
                if (!n->c) n->c = new_at(N_INITLIST);
            } else if (at_p(P_LBRACE)) {
                n->c = parse_braced_init();
                n->flags |= 2;
            }
            return n;
        }
        n = new_at(N_DELETE);
        n->flags = global;
        pos++;
        if (at_p(P_LBRACK) && is_p(T1, P_RBRACK)) {
            pos += 2;
            n->flags |= 2;
        }
        n->a = parse_cast_expr();
        return n;
    }
    {
        /* postfix expression */
        return parse_postfix();
    }
}

static Node *parse_lambda(void) {
    Node *l = new_at(N_LAMBDA);
    Node *tail = 0;
    int saved = no_gt;
    no_gt = 0;
    expect_p(P_LBRACK);
    while (!at_p(P_RBRACK)) {
        Node *c = new_at(N_CAPTURE);
        if (at_p(P_ASSIGN)) {
            c->op = P_ASSIGN;
            pos++;
        } else if (at_p(P_AMP)) {
            c->op = P_AMP;
            pos++;
            if (is_ident(T0)) {
                c->name = T0->name;
                pos++;
                if (accept_p(P_ASSIGN)) c->a = parse_assign();
            }
        } else if (at_kw(KW_this)) {
            c->name = intern_str("this");
            pos++;
        } else {
            Token *nt = expect_ident();
            c->name = nt->name;
            if (accept_p(P_ASSIGN)) c->a = parse_assign();
            else if (at_p(P_LBRACE)) c->a = parse_braced_init();
        }
        if (accept_p(P_ELLIPSIS)) c->flags |= 1;
        list_add(&l->a, &tail, c);
        if (!accept_p(P_COMMA)) break;
    }
    expect_p(P_RBRACK);
    {
        Scope *saved_scope = cur_scope;
        cur_scope = scope_new(SC_BLOCK, cur_scope, 0);
        if (at_p(P_LPAREN)) {
            int variadic;
            pos++;
            l->flags |= LF_PARAMS;
            l->b = parse_params(&variadic);
            if (variadic) l->flags |= 8;
            expect_p(P_RPAREN);
        }
        if (accept_kw(KW_mutable)) l->flags |= LF_MUTABLE;
        if (at_kw(KW_noexcept) || at_kw(KW_throw)) {
            Node tmp;
            memset(&tmp, 0, sizeof tmp);
            parse_exception_spec(&tmp);
            l->flags |= LF_NOEXCEPT;
        }
        append_list(&l->d, parse_attributes());
        if (accept_p(P_ARROW)) l->c = parse_type_id();
        {
            Node *body = parse_block();
            body->next = 0;
            l->d = body;
        }
        cur_scope = saved_scope;
    }
    no_gt = saved;
    return l;
}

/* a primary expression */
static Node *parse_primary(void) {
    Token *t = T0;
    Node *n;
    switch (t->kind) {
    case K_NUM:
        n = new_at(N_NUM);
        n->text = t->text;
        pos++;
        return n;
    case K_CHAR:
        n = new_at(N_CHARLIT);
        n->text = t->text;
        pos++;
        return n;
    case K_STR:
        {
            Node *tail;
            n = new_at(N_STRLIT);
            n->text = t->text;
            pos++;
            tail = n;
            while (T0->kind == K_STR) {                  /* adjacent literals */
                Node *m = new_at(N_STRLIT);
                m->text = T0->text;
                pos++;
                tail->a = m;
                tail = m;
            }
            return n;
        }
    case K_PUNCT:
        if (t->id == P_LPAREN) {
            int saved = no_gt;
            n = new_at(N_PAREN);
            pos++;
            no_gt = 0;
            n->a = parse_expr();
            no_gt = saved;
            expect_p(P_RPAREN);
            return n;
        }
        if (t->id == P_LBRACK) return parse_lambda();
        if (t->id == P_LBRACE) return parse_braced_init();
        if (t->id == P_SCOPE) break;
        perr("expected an expression before '%s'", t->text);
        return 0;
    case K_IDENT:
        break;
    default:
        perr("expected an expression before '%s'", t->text);
        return 0;
    }
    if (t->kind == K_IDENT && t->id) {
        switch (t->id) {
        case KW_true: case KW_false:
            n = new_at(N_BOOLLIT);
            n->val = t->id == KW_true;
            pos++;
            return n;
        case KW_nullptr:
            n = new_at(N_NULLPTR);
            pos++;
            return n;
        case KW_this:
            n = new_at(N_THIS);
            pos++;
            return n;
        case KW_static_cast: case KW_dynamic_cast: case KW_const_cast: case KW_reinterpret_cast:
            n = new_at(N_NAMEDCAST);
            n->op = OPKW + t->id;
            pos++;
            expect_p(P_LT);
            no_gt++;
            n->b = parse_type_id();
            no_gt--;
            expect_gt();
            expect_p(P_LPAREN);
            n->a = parse_expr();
            expect_p(P_RPAREN);
            return n;
        case KW_typeid:
            n = new_at(N_TYPEID_EXPR);
            pos++;
            expect_p(P_LPAREN);
            if (starts_type()) n->b = parse_type_id();
            else n->a = parse_expr();
            expect_p(P_RPAREN);
            return n;
        case KW_operator:
            break;
        case KW_template: case KW_typename:
            break;
        default:
            if (is_type_keyword(t) && t->id != KW_typename) {
                /* a functional cast with a builtin type: int(x), unsigned long{1} */
                Node *f = new_at(N_FUNCCAST);
                f->b = new_at(N_TYPEID);
                f->b->a = parse_decl_specs(CX_TYPEID);
                if (at_p(P_LBRACE)) {
                    f->a = parse_braced_init();
                    f->flags = 1;
                } else {
                    expect_p(P_LPAREN);
                    f->a = parse_arg_list();
                    expect_p(P_RPAREN);
                }
                return f;
            }
            perr("expected an expression before '%s'", t->text);
        }
    }
    /* an id-expression, or a functional cast with a type name */
    {
        Node *id;
        int tn = 0;
        if (at_kw(KW_typename)) {
            pos++;
            tn = 1;
        }
        id = parse_id(tn);
        if (tn || (id_names_type(id) && (at_p(P_LPAREN) || at_p(P_LBRACE)))) {
            if (tn || at_p(P_LPAREN) || at_p(P_LBRACE)) {
                Node *f = new_at(N_FUNCCAST);
                Node *ty = new_at(N_TYPEID);
                Node *ts = new_at(N_TYPE);
                ts->a = id;
                ty->a = ts;
                f->b = ty;
                if (at_p(P_LBRACE)) {
                    f->a = parse_braced_init();
                    f->flags = 1;
                } else {
                    expect_p(P_LPAREN);
                    f->a = parse_arg_list();
                    expect_p(P_RPAREN);
                }
                return f;
            }
        }
        return id;
    }
}

static Node *parse_postfix(void) {
    Node *e;
    if (at_kw(KW_decltype) ) {
        /* decltype(e)(args) */
        Node *f = new_at(N_FUNCCAST);
        f->b = new_at(N_TYPEID);
        f->b->a = parse_decl_specs(CX_TYPEID);
        expect_p(P_LPAREN);
        f->a = parse_arg_list();
        expect_p(P_RPAREN);
        e = f;
    } else {
        e = parse_primary();
    }
    for (;;) {
        Token *t = T0;
        Node *n;
        if (t->kind != K_PUNCT) return e;
        switch (t->id) {
        case P_LBRACK:
            n = new_at(N_INDEX);
            pos++;
            n->a = e;
            {
                int saved = no_gt;
                no_gt = 0;
                n->b = at_p(P_LBRACE) ? parse_braced_init() : parse_expr();
                no_gt = saved;
            }
            expect_p(P_RBRACK);
            e = n;
            continue;
        case P_LPAREN:
            n = new_at(N_CALL);
            pos++;
            n->a = e;
            n->b = parse_arg_list();
            expect_p(P_RPAREN);
            e = n;
            continue;
        case P_DOT: case P_ARROW:
            n = new_at(N_MEMBER);
            n->op = t->id;
            pos++;
            n->a = e;
            {
                int tn = 0;
                if (at_kw(KW_template)) {
                    pos++;
                    tn = 1;
                }
                /* the member name: a name relative to the object (not looked up here) */
                {
                    Node *id = new_at(N_ID);
                    Node *tail = 0;
                    if (at_p(P_SCOPE)) {
                        pos++;
                        id->flags |= ID_GLOBAL;
                    }
                    for (;;) {
                        Node *part;
                        if (at_kw(KW_operator)) {
                            part = parse_operator_name();
                            list_add(&id->a, &tail, part);
                            break;
                        }
                        if (at_p(P_TILDE)) {
                            pos++;
                            part = new_at(N_PART);
                            part->flags = PART_DTOR;
                            if (at_kw(KW_decltype)) perr("decltype in a destructor name is not supported");
                            part->name = expect_ident()->name;
                            if (at_p(P_LT)) {
                                part->flags |= PART_ARGS;
                                part->b = parse_template_args();
                            }
                            list_add(&id->a, &tail, part);
                            break;
                        }
                        {
                            Token *nt = expect_ident();
                            part = node_new(N_PART, nt);
                            part->name = nt->name;
                        }
                        /* '<' after a member name: template arguments if 'template' was written, or if the
                           name is a known function template */
                        if (at_p(P_LT) && (tn || is_template_name(part->name))) {
                            part->flags |= PART_ARGS;
                            part->b = parse_template_args();
                        } else if (at_p(P_LT) && (part->name->flags & NF_MEMBER_TEMPLATE)) {
                            /* maybe the name of a member template: try the arguments, they must be followed by '(' or '::' */
                            int ok;
                            Node *args = 0;
                            SPEC_BEGIN(ok)
                                args = parse_template_args();
                                if (!at_p(P_LPAREN) && !at_p(P_SCOPE)) perr("not template arguments");
                            SPEC_END(ok);
                            if (ok) {
                                part->flags |= PART_ARGS;
                                part->b = args;
                            }
                        }
                        list_add(&id->a, &tail, part);
                        tn = 0;
                        if (at_p(P_SCOPE)) {
                            pos++;
                            if (at_kw(KW_template)) {
                                pos++;
                                tn = 1;
                            }
                            continue;
                        }
                        break;
                    }
                    n->b = id;
                }
            }
            e = n;
            continue;
        case P_INC: case P_DEC:
            n = new_at(N_POSTFIX);
            n->op = t->id;
            pos++;
            n->a = e;
            e = n;
            continue;
        }
        return e;
    }
}

/* ------------------------------------------------------------ statements */

/* does a declaration (rather than an expression) start at the current token? */
static int is_decl_start(void) {
    Token *t = T0;
    if (t->kind == K_IDENT && t->id) {
        switch (t->id) {
        case KW_typedef: case KW_static: case KW_extern: case KW_inline: case KW_virtual:
        case KW_explicit: case KW_friend: case KW_constexpr: case KW_mutable: case KW_register:
        case KW_thread_local: case KW_alignas: case KW_using: case KW_static_assert: case KW_asm:
        case KW_typename: case KW_namespace:
            return 1;
        case KW_decltype:
            return 1;
        }
        return is_type_keyword(t);
    }
    if (is_p(t, P_LBRACK) && is_p(T1, P_LBRACK)) return 1;
    if (is_ident(t) || is_p(t, P_SCOPE)) {
        int ok, result = 0;
        SPEC_BEGIN(ok)
            Node *id = parse_id(0);
            if (id_names_type(id)) {
                /* a type: T x; T *x; T &x; but T(x) and T{x} are expressions here */
                result = !(at_p(P_LPAREN) || at_p(P_LBRACE) || at_p(P_DOT) || at_p(P_ARROW)
                           || at_p(P_SEMI) || at_p(P_COMMA) || at_p(P_RPAREN));
                if (at_p(P_LPAREN) && is_p(T1, P_STAR)) result = 1;
            } else if (is_ident(T0) && !(id->flags & ID_GLOBAL && 0)) {
                result = 1;                             /* "Unknown name": only a declaration */
            }
        SPEC_PROBE_END(ok);
        return ok && result;
    }
    return 0;
}

static Node *parse_block(void) {
    Node *b = new_at(N_BLOCK);
    Node *tail = 0;
    Scope *saved = cur_scope;
    expect_p(P_LBRACE);
    cur_scope = scope_new(SC_BLOCK, saved, 0);
    b->scope = cur_scope;
    while (!at_p(P_RBRACE)) {
        if (T0->kind == K_EOF) perr("expected '}' at the end of the input");
        list_add(&b->a, &tail, parse_statement());
    }
    pos++;
    cur_scope = saved;
    return b;
}

/* a condition: an expression or a declaration with an initializer */
static Node *parse_condition(void) {
    if (is_decl_start()) {
        Node *d = new_at(N_DECL);
        Node *id = new_at(N_INITDECL);
        d->a = parse_decl_specs(CX_CONDITION);
        id->a = parse_declarator(DM_NAMED);
        if (accept_p(P_ASSIGN)) id->b = parse_initializer_clause();
        else if (at_p(P_LBRACE)) id->b = parse_braced_init();
        {
            Node *nid = declarator_name_id(id->a);
            if (nid && nid->a) declare(cur_scope, nid->a->name, SY_VAR);
        }
        d->b = id;
        return d;
    }
    return parse_expr();
}

/* the init-statement of if and switch (C++17): a declaration or an expression statement before ';', or 0 */
static Node *parse_init_statement(void) {
    int ok, depth = 0, i;
    Node *r = 0;
    /* is there a ';' before the closing parenthesis? */
    for (i = pos; tk[i]->kind != K_EOF; i++) {
        if (is_p(tk[i], P_LPAREN) || is_p(tk[i], P_LBRACK) || is_p(tk[i], P_LBRACE)) depth++;
        else if (is_p(tk[i], P_RPAREN) || is_p(tk[i], P_RBRACK) || is_p(tk[i], P_RBRACE)) {
            if (depth == 0) return 0;
            depth--;
        } else if (is_p(tk[i], P_SEMI) && depth == 0) break;
    }
    if (tk[i]->kind == K_EOF) return 0;
    if (is_decl_start()) {
        Node *s = new_at(N_DECL_STMT);
        s->a = parse_simple_declaration(CX_BLOCK);
        r = s;
    } else {
        Node *s = new_at(N_EXPR_STMT);
        s->a = parse_expr();
        expect_p(P_SEMI);
        r = s;
    }
    (void)ok;
    return r;
}

static Node *parse_statement(void) {
    Token *t = T0;
    Node *n;
    Scope *saved;
    if (is_p(t, P_LBRACK) && is_p(T1, P_LBRACK)) {
        Node *attrs = parse_attributes();
        (void)attrs;
        t = T0;
    }
    if (t->kind == K_PUNCT) {
        if (t->id == P_LBRACE) return parse_block();
        if (t->id == P_SEMI) {
            n = new_at(N_NULL_STMT);
            pos++;
            return n;
        }
    }
    if (t->kind == K_IDENT && t->id) {
        switch (t->id) {
        case KW_if:
            n = new_at(N_IF);
            pos++;
            if (at_kw(KW_constexpr)) {
                pos++;
                n->flags |= IF_CONSTEXPR;
            }
            expect_p(P_LPAREN);
            saved = cur_scope;
            cur_scope = scope_new(SC_BLOCK, saved, 0);
            n->d = parse_init_statement();
            n->a = parse_condition();
            expect_p(P_RPAREN);
            n->b = parse_statement();
            if (accept_kw(KW_else)) n->c = parse_statement();
            cur_scope = saved;
            return n;
        case KW_switch:
            n = new_at(N_SWITCH);
            pos++;
            expect_p(P_LPAREN);
            saved = cur_scope;
            cur_scope = scope_new(SC_BLOCK, saved, 0);
            n->d = parse_init_statement();
            n->a = parse_condition();
            expect_p(P_RPAREN);
            n->b = parse_statement();
            cur_scope = saved;
            return n;
        case KW_while:
            n = new_at(N_WHILE);
            pos++;
            expect_p(P_LPAREN);
            saved = cur_scope;
            cur_scope = scope_new(SC_BLOCK, saved, 0);
            n->a = parse_condition();
            expect_p(P_RPAREN);
            n->b = parse_statement();
            cur_scope = saved;
            return n;
        case KW_do:
            n = new_at(N_DO);
            pos++;
            n->a = parse_statement();
            expect_kw(KW_while);
            expect_p(P_LPAREN);
            n->b = parse_expr();
            expect_p(P_RPAREN);
            expect_p(P_SEMI);
            return n;
        case KW_for:
            n = new_at(N_FOR);
            pos++;
            expect_p(P_LPAREN);
            saved = cur_scope;
            cur_scope = scope_new(SC_BLOCK, saved, 0);
            if (is_decl_start()) {
                /* a declaration: simple-declaration or the declaration of a range-for */
                Node *d = new_at(N_DECL);
                Node *id = new_at(N_INITDECL);
                d->a = parse_decl_specs(CX_BLOCK);
                if ((d->a->val & BT_AUTO) && (at_p(P_LBRACK) || ((at_p(P_AMP) || at_p(P_ANDAND)) && is_p(T1, P_LBRACK)))) {
                    /* for (auto &[k, v] : range) */
                    Node *b = new_at(N_BINDING), *ntail = 0;
                    b->a = d->a;
                    if (!at_p(P_LBRACK)) {
                        b->op = T0->id;
                        pos++;
                    }
                    expect_p(P_LBRACK);
                    for (;;) {
                        Token *nt = expect_ident();
                        Node *idn = node_new(N_ID, nt), *part = node_new(N_PART, nt);
                        part->name = nt->name;
                        idn->a = part;
                        list_add(&b->c, &ntail, idn);
                        declare(cur_scope, nt->name, SY_VAR);
                        if (!accept_p(P_COMMA)) break;
                    }
                    expect_p(P_RBRACK);
                    expect_p(P_COLON);
                    n->kind = N_RANGEFOR;
                    n->a = b;
                    n->b = at_p(P_LBRACE) ? parse_braced_init() : parse_expr();
                    expect_p(P_RPAREN);
                    n->d = parse_statement();
                    cur_scope = saved;
                    return n;
                }
                id->a = parse_declarator(DM_NAMED);
                {
                    Node *nid = declarator_name_id(id->a);
                    if (nid && nid->a) declare(cur_scope, nid->a->name, SY_VAR);
                }
                if (accept_p(P_COLON)) {
                    n->kind = N_RANGEFOR;
                    d->b = id;
                    n->a = d;
                    n->b = at_p(P_LBRACE) ? parse_braced_init() : parse_expr();
                    expect_p(P_RPAREN);
                    n->d = parse_statement();
                    cur_scope = saved;
                    return n;
                }
                if (accept_p(P_ASSIGN)) id->b = parse_initializer_clause();
                else if (at_p(P_LPAREN)) {
                    pos++;
                    id->b = parse_arg_list();
                    expect_p(P_RPAREN);
                    if (!id->b) id->b = new_at(N_INITLIST);
                } else if (at_p(P_LBRACE)) id->b = parse_braced_init();
                d->b = id;
                while (accept_p(P_COMMA)) {
                    Node *id2 = new_at(N_INITDECL);
                    id2->a = parse_declarator(DM_NAMED);
                    {
                        Node *nid = declarator_name_id(id2->a);
                        if (nid && nid->a) declare(cur_scope, nid->a->name, SY_VAR);
                    }
                    if (accept_p(P_ASSIGN)) id2->b = parse_initializer_clause();
                    else if (at_p(P_LBRACE)) id2->b = parse_braced_init();
                    append_list(&d->b, id2);
                }
                n->a = d;
                expect_p(P_SEMI);
            } else if (!at_p(P_SEMI)) {
                Node *es = new_at(N_EXPR_STMT);
                es->a = parse_expr();
                n->a = es;
                expect_p(P_SEMI);
            } else pos++;
            if (!at_p(P_SEMI)) n->b = parse_condition();
            expect_p(P_SEMI);
            if (!at_p(P_RPAREN)) n->c = parse_expr();
            expect_p(P_RPAREN);
            n->d = parse_statement();
            cur_scope = saved;
            return n;
        case KW_break:
            n = new_at(N_BREAK);
            pos++;
            expect_p(P_SEMI);
            return n;
        case KW_continue:
            n = new_at(N_CONTINUE);
            pos++;
            expect_p(P_SEMI);
            return n;
        case KW_return:
            n = new_at(N_RETURN);
            pos++;
            if (!at_p(P_SEMI)) n->a = at_p(P_LBRACE) ? parse_braced_init() : parse_expr();
            expect_p(P_SEMI);
            return n;
        case KW_goto:
            n = new_at(N_GOTO);
            pos++;
            n->name = expect_ident()->name;
            expect_p(P_SEMI);
            return n;
        case KW_case:
            n = new_at(N_CASE);
            pos++;
            n->a = parse_cond();
            expect_p(P_COLON);
            n->b = parse_statement();
            return n;
        case KW_default:
            n = new_at(N_DEFAULT);
            pos++;
            expect_p(P_COLON);
            n->a = parse_statement();
            return n;
        case KW_try:
            {
                Node *tail = 0;
                n = new_at(N_TRY);
                pos++;
                n->a = parse_block();
                while (at_kw(KW_catch)) {
                    Node *c = new_at(N_CATCH);
                    pos++;
                    expect_p(P_LPAREN);
                    saved = cur_scope;
                    cur_scope = scope_new(SC_BLOCK, saved, 0);
                    if (at_p(P_ELLIPSIS)) pos++;
                    else {
                        Node *p = new_at(N_PARAM);
                        p->a = parse_decl_specs(CX_PARAM);
                        if (!at_p(P_RPAREN)) p->b = parse_declarator(DM_EITHER);
                        c->a = p;
                        {
                            Node *nid = declarator_name_id(p->b);
                            if (nid && nid->a) declare(cur_scope, nid->a->name, SY_VAR);
                        }
                    }
                    expect_p(P_RPAREN);
                    c->b = parse_block();
                    cur_scope = saved;
                    list_add(&n->b, &tail, c);
                }
                if (!n->b) perr("expected 'catch' after the try block");
                return n;
            }
        }
    }
    /* a label */
    if (is_ident(t) && is_p(T1, P_COLON)) {
        n = new_at(N_LABEL);
        n->name = t->name;
        pos += 2;
        n->a = at_p(P_RBRACE) ? 0 : parse_statement();
        return n;
    }
    if (is_decl_start()) {
        n = new_at(N_DECL_STMT);
        n->a = parse_declaration(CX_BLOCK);
        return n;
    }
    n = new_at(N_EXPR_STMT);
    n->a = parse_expr();
    expect_p(P_SEMI);
    return n;
}

/* ------------------------------------------------------------ declarations */

static int in_template_decl = 0;     /* the next declared name is a template */
static int template_nesting = 0;     /* inside a template declaration or class template */

/* the scope that a new name is declared in (not a template parameter scope) */
static Scope *decl_target(void) {
    Scope *s = cur_scope;
    while (s->kind == SC_TEMPLATE && s->parent) s = s->parent;
    return s;
}

static int in_dependent_context(void) {
    Scope *s;
    for (s = cur_scope; s; s = s->parent)
        if (s->kind == SC_TEMPLATE || s->is_template) return 1;
    return 0;
}

/* the last component of a declared id */
static Node *last_part(Node *id) {
    Node *p = id->a;
    while (p && p->next) p = p->next;
    return p;
}

/* declares the name of a declarator (typedef: a type; otherwise a variable or function) */
static void declare_declarator(Node *decl, Node *specs, int is_typedef) {
    Node *id = declarator_name_id(decl);
    Node *lp;
    Sym *y;
    Scope *target;
    if (!id) return;
    lp = last_part(id);
    if (!lp || lp->flags != PART_NAME || !lp->name) return;
    if (part_count(id) > 1 || (id->flags & ID_GLOBAL)) return;      /* a member defined outside its class */
    if (class_name && lp->name == class_name && (cur_scope == class_scope || (cur_scope->kind == SC_TEMPLATE && cur_scope->parent == class_scope))) return;   /* a constructor (or a constructor template) */
    target = decl_target();
    if (is_typedef) {
        y = declare(target, lp->name, SY_TYPE);
        if (specs->a && specs->a->kind == N_ID && specs->a->sym) y->members = specs->a->sym->members;
        if (specs->a && specs->a->kind == N_CLASS && specs->a->scope) y->members = specs->a->scope;
        if (in_dependent_context()) y->dependent = 1;
    } else {
        y = declare(target, lp->name, in_template_decl ? SY_TEMPLATE : SY_VAR);
        if (in_template_decl) y->type_template = 0;
    }
    in_template_decl = 0;
}

/* a function body that is parsed after the class */
typedef struct Deferred Deferred;
struct Deferred {
    Node *fn;
    int pos;
    Scope *scope;
    Name *class_name;
    Scope *class_scope;
    Deferred *next;
};
static Deferred *deferred_head = 0, *deferred_tail = 0;
static int class_depth = 0;

static void skip_braces(void) {
    int depth = 0;
    if (!at_p(P_LBRACE)) perr("expected '{' before '%s'", T0->text);
    for (;;) {
        if (T0->kind == K_EOF) perr("expected '}' at the end of the input");
        if (at_p(P_LBRACE)) depth++;
        if (at_p(P_RBRACE)) {
            depth--;
            if (depth == 0) {
                pos++;
                return;
            }
        }
        pos++;
    }
}

/* skips a function body: [try] [: ctor-initializers] { ... } [catch (...) { ... }]* */
static void skip_function_body(void) {
    int is_try = 0;
    if (at_kw(KW_try)) {
        pos++;
        is_try = 1;
    }
    if (at_p(P_COLON)) {
        pos++;
        for (;;) {
            while (!at_p(P_LPAREN) && !at_p(P_LBRACE)) {
                if (T0->kind == K_EOF || at_p(P_SEMI)) perr("expected a member initializer before '%s'", T0->text);
                pos++;
            }
            if (at_p(P_LPAREN)) skip_parens();
            else skip_braces();
            accept_p(P_ELLIPSIS);
            if (!accept_p(P_COMMA)) break;
        }
    }
    skip_braces();
    if (is_try) {
        while (at_kw(KW_catch)) {
            pos++;
            if (at_p(P_LPAREN)) skip_parens();
            skip_braces();
        }
    }
}

/* parses [: ctor-initializers] block [catch handlers] at the current position */
static void parse_function_body(Node *fn) {
    int is_try = accept_kw(KW_try);
    Scope *saved = cur_scope;
    cur_scope = scope_new(SC_BLOCK, saved, 0);
    /* the parameters are visible in the body (only as non-types) */
    {
        Node *fd = declarator_function(fn->b);
        Node *p;
        if (fd)
            for (p = fd->b; p; p = p->next) {
                Node *nid = declarator_name_id(p->b);
                if (nid && nid->a && last_part(nid->a ? nid : nid)) {
                    Node *lp = last_part(nid);
                    if (lp && lp->name && lp->flags == PART_NAME) declare(cur_scope, lp->name, SY_VAR);
                }
            }
    }
    if (accept_p(P_COLON)) {
        Node *tail = 0;
        for (;;) {
            Node *ci = new_at(N_CTORINIT);
            ci->a = parse_id(0);
            if (at_p(P_LBRACE)) {
                ci->b = parse_braced_init();
                ci->flags = 1;
            } else {
                expect_p(P_LPAREN);
                ci->b = parse_arg_list();
                expect_p(P_RPAREN);
            }
            if (accept_p(P_ELLIPSIS)) ci->flags |= 2;
            list_add(&fn->c, &tail, ci);
            if (!accept_p(P_COMMA)) break;
        }
    }
    fn->d = parse_block();
    if (is_try) {
        Node *tr = new_at(N_TRY);
        Node *tail = 0;
        tr->a = fn->d;
        while (at_kw(KW_catch)) {
            Node *c = new_at(N_CATCH);
            Scope *s2 = cur_scope;
            pos++;
            expect_p(P_LPAREN);
            cur_scope = scope_new(SC_BLOCK, s2, 0);
            if (at_p(P_ELLIPSIS)) pos++;
            else {
                Node *p = new_at(N_PARAM);
                p->a = parse_decl_specs(CX_PARAM);
                if (!at_p(P_RPAREN)) p->b = parse_declarator(DM_EITHER);
                c->a = p;
            }
            expect_p(P_RPAREN);
            c->b = parse_block();
            cur_scope = s2;
            list_add(&tr->b, &tail, c);
        }
        fn->d = tr;
    }
    cur_scope = saved;
}

static void run_deferred(void) {
    Deferred *d = deferred_head;
    int save_pos = pos;
    Scope *save_scope = cur_scope;
    Name *save_class = class_name;
    Scope *save_cscope = class_scope;
    deferred_head = deferred_tail = 0;
    for (; d; d = d->next) {
        pos = d->pos;
        cur_scope = d->scope;
        class_name = d->class_name;
        class_scope = d->class_scope;
        parse_function_body(d->fn);
    }
    pos = save_pos;
    cur_scope = save_scope;
    class_name = save_class;
    class_scope = save_cscope;
}

/* the initializer after a declarator: = x, (x), {x}, and for functions = 0 / default / delete */
static void parse_init_declarator_tail(Node *id) {
    Node *fd = declarator_function(id->a);
    if (at_p(P_ASSIGN)) {
        pos++;
        if (fd) {
            if (T0->kind == K_NUM && strcmp(T0->text, "0") == 0) {
                fd->flags |= FQ_PURE;
                pos++;
                return;
            }
            if (at_kw(KW_default)) {
                fd->flags |= FQ_DEFAULT;
                pos++;
                return;
            }
            if (at_kw(KW_delete)) {
                fd->flags |= FQ_DELETE;
                pos++;
                return;
            }
        }
        id->b = parse_initializer_clause();
        return;
    }
    if (at_p(P_LBRACE) && !fd) {
        id->b = parse_braced_init();
        id->flags |= 1;
        return;
    }
    if (at_p(P_LPAREN) && !fd) {
        pos++;
        id->b = parse_arg_list();
        expect_p(P_RPAREN);
        if (!id->b) id->b = new_at(N_INITLIST);
        id->flags |= 2;
        return;
    }
}

/* is d a function declarator?  int *f() is D_PTR(D_FUNC(D_NAME)): the pointer wraps the function */
static int is_function_declarator(Node *d) {
    Node *x = d;
    while (x && (x->kind == N_D_PTR || x->kind == N_D_REF || x->kind == N_D_RREF || x->kind == N_D_MEMPTR)) x = x->a;
    while (x && x->kind == N_D_PACK) x = x->a;
    return x && x->kind == N_D_FUNC;
}

static Node *parse_declaration(int ctx);
static Node *parse_template_declaration(int ctx);
static Node *parse_namespace(void);
static Node *parse_using(int ctx);

/* enters the scope of the class that a qualified declarator-id names (A::f) for the body of f */
static void enter_member_scope(Node *nid) {
    Node *q = nid->a;
    Scope *sc = (nid->flags & ID_GLOBAL) ? global_scope : 0;
    Sym *y = 0;
    int first = 1;
    for (; q && q->next; q = q->next) {
        if (first && !sc) y = lookup_chain(cur_scope, q->name);
        else y = sc ? lookup_member(sc, q->name, 0) : 0;
        first = 0;
        sc = y ? y->members : 0;
        if (!sc) break;
    }
    if (sc) {
        cur_scope = sc;
        if (y && sc->kind == SC_CLASS) {
            class_name = y->name;
            class_scope = sc;
        }
    }
}

/* a simple-declaration or a function definition */
static Node *parse_simple_declaration(int ctx) {
    Node *d = new_at(N_DECL);
    Node *tail = 0;
    int saved_ctx = decl_ctx;
    decl_ctx = ctx;
    d->a = parse_decl_specs(ctx);
    if (at_p(P_SEMI)) {
        pos++;
        decl_ctx = saved_ctx;
        in_template_decl = 0;
        return d;
    }
    if ((d->a->val & BT_AUTO) && (at_p(P_LBRACK) || ((at_p(P_AMP) || at_p(P_ANDAND)) && is_p(T1, P_LBRACK)))) {
        /* a structured binding declaration: auto [a, b] = e; */
        Node *b = new_at(N_BINDING), *ntail = 0;
        b->a = d->a;
        if (!at_p(P_LBRACK)) {
            b->op = T0->id;
            pos++;
        }
        expect_p(P_LBRACK);
        for (;;) {
            Token *nt = expect_ident();
            Node *idn = node_new(N_ID, nt), *part = node_new(N_PART, nt);
            part->name = nt->name;
            idn->a = part;
            list_add(&b->c, &ntail, idn);
            declare(cur_scope, nt->name, SY_VAR);
            if (!accept_p(P_COMMA)) break;
        }
        expect_p(P_RBRACK);
        if (accept_p(P_ASSIGN)) b->b = parse_initializer_clause();
        else if (at_p(P_LBRACE)) b->b = parse_braced_init();
        else if (at_p(P_LPAREN)) {
            pos++;
            b->b = parse_assign();
            expect_p(P_RPAREN);
        } else perr("expected an initializer for the structured binding");
        expect_p(P_SEMI);
        decl_ctx = saved_ctx;
        return b;
    }
    for (;;) {
        Node *id = new_at(N_INITDECL);
        id->a = parse_declarator(DM_NAMED);
        if (at_p(P_COLON) && ctx == CX_CLASS && !is_function_declarator(id->a)) {
            pos++;
            id->c = parse_cond();                           /* a bit-field */
        }
        if (is_function_declarator(id->a) && (at_p(P_LBRACE) || at_p(P_COLON) || at_kw(KW_try))
            && ctx != CX_PARAM && ctx != CX_CONDITION) {
            /* a function definition */
            Node *fn = new_at(N_FUNCDEF);
            Node *nid = declarator_name_id(id->a);
            fn->a = d->a;
            fn->b = id->a;
            declare_declarator(id->a, d->a, d->a->flags & DS_TYPEDEF);
            if (ctx == CX_CLASS) {
                Deferred *df = anew(sizeof(Deferred));
                df->fn = fn;
                df->pos = pos;
                df->scope = cur_scope;
                df->class_name = class_name;
                df->class_scope = class_scope;
                if (deferred_tail) deferred_tail->next = df;
                else deferred_head = df;
                deferred_tail = df;
                skip_function_body();
            } else {
                /* a member defined outside its class: the class scope is visible in the body */
                Scope *saved = cur_scope;
                Name *saved_class = class_name;
                Scope *saved_cscope = class_scope;
                if (nid && part_count(nid) > 1) enter_member_scope(nid);
                parse_function_body(fn);
                cur_scope = saved;
                class_name = saved_class;
                class_scope = saved_cscope;
            }
            decl_ctx = saved_ctx;
            in_template_decl = 0;
            return fn;
        }
        parse_init_declarator_tail(id);
        declare_declarator(id->a, d->a, d->a->flags & DS_TYPEDEF);
        list_add(&d->b, &tail, id);
        if (!accept_p(P_COMMA)) break;
    }
    in_template_decl = 0;
    expect_p(P_SEMI);
    decl_ctx = saved_ctx;
    return d;
}

static Node *parse_static_assert(void) {
    Node *n = new_at(N_STATIC_ASSERT);
    pos++;
    expect_p(P_LPAREN);
    n->a = parse_cond();
    if (accept_p(P_COMMA)) n->b = parse_primary();
    expect_p(P_RPAREN);
    expect_p(P_SEMI);
    return n;
}

static Node *parse_declaration(int ctx) {
    Token *t = T0;
    if (is_p(t, P_SEMI)) {
        Node *n = new_at(N_EMPTY_DECL);
        pos++;
        return n;
    }
    if (t->kind == K_IDENT && t->id) {
        switch (t->id) {
        case KW_template:
            return parse_template_declaration(ctx);
        case KW_namespace:
            return parse_namespace();
        case KW_inline:
            if (is_kw(T1, KW_namespace)) return parse_namespace();
            break;
        case KW_using:
            return parse_using(ctx);
        case KW_static_assert:
            return parse_static_assert();
        case KW_asm:
            {
                Node *n = new_at(N_ASM);
                int depth = 1;
                pos++;
                expect_p(P_LPAREN);
                n->text = T0->text;
                while (depth > 0) {
                    if (T0->kind == K_EOF) perr("unterminated asm");
                    if (at_p(P_LPAREN)) depth++;
                    if (at_p(P_RPAREN)) depth--;
                    pos++;
                }
                expect_p(P_SEMI);
                return n;
            }
        case KW_extern:
            if (T1->kind == K_STR) {
                Node *n = new_at(N_LINKAGE);
                Node *tail = 0;
                pos++;
                n->text = T0->text;
                pos++;
                if (at_p(P_LBRACE)) {
                    pos++;
                    while (!at_p(P_RBRACE)) {
                        if (T0->kind == K_EOF) perr("expected '}' at the end of the input");
                        list_add(&n->a, &tail, parse_declaration(ctx));
                    }
                    pos++;
                } else n->a = parse_declaration(ctx);
                return n;
            }
            if (is_kw(T1, KW_template)) {
                Node *n = new_at(N_INSTANTIATION);
                pos++;
                n->flags = 1;
                n->a = parse_declaration(ctx);
                return n;
            }
            break;
        }
    }
    if (is_p(t, P_LBRACK) && is_p(T1, P_LBRACK)) {
        /* an attribute-declaration [[...]]; or attributes before a declaration */
        Node *attrs = parse_attributes();
        if (at_p(P_SEMI)) {
            pos++;
            return attrs;
        }
        {
            Node *d = parse_declaration(ctx);
            if (d && d->kind == N_DECL && d->a) append_list(&d->a->d, attrs);
            return d;
        }
    }
    return parse_simple_declaration(ctx);
}

/* ------------------------------------------------------------ namespaces and using */

static Node *parse_namespace(void) {
    Node *n = new_at(N_NAMESPACE);
    Node *tail = 0;
    Scope *saved = cur_scope, *ns;
    Sym *y = 0;
    if (accept_kw(KW_inline)) n->flags |= NS_INLINE;
    expect_kw(KW_namespace);
    append_list(&n->d, parse_attributes());
    if (is_ident(T0)) {
        Token *nt = T0;
        pos++;
        n->name = nt->name;
        if (accept_p(P_ASSIGN)) {
            /* namespace alias */
            Node *id = parse_id(0);
            n->kind = N_NSALIAS;
            n->a = id;
            expect_p(P_SEMI);
            y = declare(decl_target(), nt->name, SY_NAMESPACE);
            y->members = id->sym ? id->sym->members : 0;
            return n;
        }
        y = declare(decl_target(), nt->name, SY_NAMESPACE);
        if (!y->members) y->members = scope_new(SC_NAMESPACE, decl_target(), nt->name);
        ns = y->members;
        if (n->flags & NS_INLINE) add_using(decl_target(), ns);
        if (at_p(P_SCOPE)) {
            /* namespace a::b { ... } is namespace a { namespace b { ... } } */
            Node *inner;
            pos++;
            n->scope = ns;
            cur_scope = ns;
            tk[pos - 1] = tok_copy(tk[pos - 1]);
            tk[pos - 1]->kind = K_IDENT;
            tk[pos - 1]->id = KW_namespace;
            tk[pos - 1]->name = intern_str("namespace");
            pos--;
            inner = parse_namespace();
            cur_scope = saved;
            n->a = inner;
            return n;
        }
    } else {
        /* an unnamed namespace: its names are visible in the enclosing scope */
        ns = scope_new(SC_NAMESPACE, cur_scope, 0);
        add_using(decl_target(), ns);
    }
    n->scope = ns;
    expect_p(P_LBRACE);
    cur_scope = ns;
    while (!at_p(P_RBRACE)) {
        if (T0->kind == K_EOF) perr("expected '}' at the end of the input");
        list_add(&n->a, &tail, parse_declaration(CX_NAMESPACE));
    }
    pos++;
    cur_scope = saved;
    return n;
}

static Node *parse_using(int ctx) {
    Node *n = new_at(N_USING_DECL);
    expect_kw(KW_using);
    if (at_kw(KW_namespace)) {
        pos++;
        n->kind = N_USING_DIR;
        n->a = parse_id(0);
        if (n->a->sym && n->a->sym->members) add_using(decl_target(), n->a->sym->members);
        expect_p(P_SEMI);
        return n;
    }
    if (is_ident(T0) && is_p(T1, P_ASSIGN)) {
        /* alias-declaration */
        Token *nt = T0;
        Sym *y;
        n->kind = N_ALIAS;
        n->name = nt->name;
        pos += 2;
        y = declare(decl_target(), nt->name, in_template_decl ? SY_TEMPLATE : SY_TYPE);
        y->type_template = in_template_decl;
        if (in_dependent_context()) y->dependent = 1;
        in_template_decl = 0;
        n->a = parse_type_id();
        if (n->a->a && n->a->a->a && n->a->a->a->kind == N_ID && n->a->a->a->sym && !y->type_template)
            y->members = n->a->a->a->sym->members;
        append_list(&n->d, parse_attributes());
        expect_p(P_SEMI);
        return n;
    }
    /* using-declaration */
    {
        Node *id;
        Sym *y;
        int tn = accept_kw(KW_typename);
        if (tn) n->flags = 1;
        id = parse_id(tn);
        n->a = id;
        expect_p(P_SEMI);
        if (id->a) {
            Node *lp = last_part(id);
            if (lp && lp->name && !(lp->flags & (PART_OPERATOR | PART_CONV | PART_DTOR))) {
                y = declare(ctx == CX_CLASS ? cur_scope : decl_target(), lp->name,
                            id->sym ? id->sym->kind : (tn ? SY_TYPE : SY_VAR));
                if (id->sym) {
                    y->members = id->sym->members;
                    y->type_template = id->sym->type_template;
                    y->dependent = id->sym->dependent;
                }
            }
        }
    }
    return n;
}

/* ------------------------------------------------------------ templates */

static Node *parse_template_params(void) {
    Node *head = 0, *tail = 0;
    expect_p(P_LT);
    no_gt++;
    while (!at_p(P_GT)) {
        Node *p = new_at(N_TPARAM);
        if (at_kw(KW_class) || at_kw(KW_typename)) {
            p->op = OPKW + T0->id;
            pos++;
            if (accept_p(P_ELLIPSIS)) p->flags |= TPF_PACK;
            if (is_ident(T0)) {
                p->name = T0->name;
                pos++;
                declare(cur_scope, p->name, SY_TYPE)->dependent = 1;
            }
            if (accept_p(P_ASSIGN)) p->a = parse_type_id();
        } else if (at_kw(KW_template)) {
            Scope *saved = cur_scope;
            pos++;
            cur_scope = scope_new(SC_TEMPLATE, saved, 0);
            no_gt--;
            p->b = parse_template_params();
            no_gt++;
            cur_scope = saved;
            if (at_kw(KW_class) || at_kw(KW_typename)) {
                p->op = OPKW + T0->id;
                pos++;
            }
            if (accept_p(P_ELLIPSIS)) p->flags |= TPF_PACK;
            if (is_ident(T0)) {
                Sym *y;
                p->name = T0->name;
                pos++;
                y = declare(cur_scope, p->name, SY_TEMPLATE);
                y->type_template = 1;
                y->dependent = 1;
            }
            if (accept_p(P_ASSIGN)) p->a = parse_id(0);
        } else {
            /* a non-type parameter */
            Node *par = new_at(N_PARAM);
            par->a = parse_decl_specs(CX_PARAM);
            if (at_p(P_ELLIPSIS)) {
                pos++;
                p->flags |= TPF_PACK;
            }
            if (!at_p(P_COMMA) && !at_p(P_GT) && !at_p(P_ASSIGN)) par->b = parse_declarator(DM_EITHER);
            if (accept_p(P_ASSIGN)) par->c = parse_cond();
            p->a = par;
            {
                Node *nid = declarator_name_id(par->b);
                if (nid && nid->a) {
                    Node *lp = last_part(nid);
                    if (lp && lp->name) {
                        p->name = lp->name;
                        declare(cur_scope, lp->name, SY_VAR)->dependent = 1;
                    }
                }
            }
        }
        list_add(&head, &tail, p);
        if (!accept_p(P_COMMA)) break;
    }
    no_gt--;
    expect_gt();
    return head;
}

static Node *parse_template_declaration(int ctx) {
    Node *n = new_at(N_TEMPLATE);
    Scope *saved = cur_scope;
    expect_kw(KW_template);
    if (!at_p(P_LT)) {
        /* explicit instantiation: template class X<int>; template void f<int>(int); */
        n->kind = N_INSTANTIATION;
        n->a = parse_declaration(ctx);
        return n;
    }
    cur_scope = scope_new(SC_TEMPLATE, saved, 0);
    n->scope = cur_scope;
    if (is_p(T1, P_GT)) {
        pos += 2;
        n->flags = TP_SPEC;
        in_template_decl = 0;
        n->b = parse_declaration(ctx);
        cur_scope = saved;
        return n;
    }
    n->a = parse_template_params();
    in_template_decl = 1;
    n->b = parse_declaration(ctx);
    in_template_decl = 0;
    cur_scope = saved;
    return n;
}

/* ------------------------------------------------------------ classes */

/* a class-key [attributes] [name] [final] [: bases] [{ members }] */
static Node *parse_class_specifier(int key) {
    Node *c = new_at(N_CLASS);
    Scope *target = decl_target();
    int is_tmpl = in_template_decl;
    int is_spec = 0;
    Sym *y = 0;
    Name *nm = 0;
    c->op = OPKW + key;
    pos++;
    append_list(&c->d, parse_attributes());
    if (is_ident(T0) || at_p(P_SCOPE)) {
        Node *lp;
        c->a = parse_id(0);
        lp = last_part(c->a);
        nm = lp->name;
        is_spec = (lp->flags & PART_ARGS) != 0;
    }
    if (is_ident(T0) && T0->name == nm_final) {
        c->flags |= CL_FINAL;
        pos++;
    }
    if (at_p(P_COLON) || at_p(P_LBRACE)) {
        Scope *cs;
        Node *tail = 0;
        c->flags |= CL_DEFINITION;
        /* the symbol of the class */
        if (c->a && part_count(c->a) == 1 && !(c->a->flags & ID_GLOBAL) && !is_spec) {
            y = declare(target, nm, is_tmpl ? SY_TEMPLATE : SY_TYPE);
            y->type_template = is_tmpl;
            if (!y->members || y->members->kind != SC_CLASS || is_tmpl) y->members = scope_new(SC_CLASS, cur_scope, nm);   /* a template: the scope has the parameters as its parent */
            cs = y->members;
        } else if (c->a && c->a->sym && !is_spec && c->a->sym->members) {
            cs = c->a->sym->members;
        } else {
            cs = scope_new(SC_CLASS, cur_scope, nm);       /* a specialization or an anonymous class */
        }
        in_template_decl = 0;
        if (is_tmpl || in_dependent_context()) cs->is_template = 1;
        c->scope = cs;
        /* the base classes */
        if (accept_p(P_COLON)) {
            for (;;) {
                Node *b = new_at(N_BASE);
                for (;;) {
                    if (at_kw(KW_virtual)) { b->flags |= 1; pos++; }
                    else if (at_kw(KW_public) || at_kw(KW_protected) || at_kw(KW_private)) {
                        b->op = OPKW + T0->id;
                        pos++;
                    } else break;
                }
                b->a = parse_id(0);
                if (accept_p(P_ELLIPSIS)) b->flags |= 2;
                if (b->a->sym && b->a->sym->members && !(b->a->val & IDV_DEPENDENT) && !b->a->sym->dependent)
                    add_base(cs, b->a->sym->members);
                else cs->dependent_base = 1;
                list_add(&c->b, &tail, b);
                if (!accept_p(P_COMMA)) break;
            }
        }
        expect_p(P_LBRACE);
        {
            Scope *s_scope = cur_scope;
            Name *s_name = class_name;
            Scope *s_cscope = class_scope;
            cur_scope = cs;
            class_name = nm;
            class_scope = cs;
            class_depth++;
            parse_member_list(c);
            expect_p(P_RBRACE);
            class_depth--;
            cur_scope = s_scope;
            class_name = s_name;
            class_scope = s_cscope;
        }
        if (class_depth == 0) run_deferred();
        append_list(&c->d, parse_attributes());
        return c;
    }
    /* an elaborated-type-specifier: class X; or class X x; */
    if (c->a && part_count(c->a) == 1 && !(c->a->flags & ID_GLOBAL) && !is_spec) {
        /* declares the class if it is not known (a forward declaration) */
        Sym *known = lookup_chain(cur_scope, nm);
        if (at_p(P_SEMI) || !known) {
            y = declare(target, nm, is_tmpl ? SY_TEMPLATE : SY_TYPE);
            y->type_template = is_tmpl;
            if (!y->members) y->members = scope_new(SC_CLASS, target, nm);
            if (is_tmpl) y->members->is_template = 1;
            in_template_decl = 0;
        } else c->a->sym = known;
    }
    return c;
}

static void parse_member_list(Node *cls) {
    Node *tail = 0;
    while (!at_p(P_RBRACE)) {
        Node *m;
        if (T0->kind == K_EOF) perr("expected '}' at the end of the input");
        if ((at_kw(KW_public) || at_kw(KW_protected) || at_kw(KW_private)) && is_p(T1, P_COLON)) {
            m = new_at(N_ACCESS);
            m->op = OPKW + T0->id;
            pos += 2;
            list_add(&cls->c, &tail, m);
            continue;
        }
        m = parse_declaration(CX_CLASS);
        list_add(&cls->c, &tail, m);
    }
}

/* ------------------------------------------------------------ the entry point */

Node *parse_file(int file) {
    Node *head = 0, *tail = 0;
    int cap = 1024, i;
    Token *t;
    tk = xalloc(sizeof(Token *) * cap);
    ntk = 0;
    pp_begin(file);
    for (;;) {
        t = pp_next();
        if (ntk + 8 >= cap) {
            cap *= 2;
            tk = xrealloc(tk, sizeof(Token *) * cap);
        }
        tk[ntk++] = t;
        if (t->kind == K_EOF) break;
    }
    for (i = 0; i < 6; i++) tk[ntk++] = t;
    pos = 0;
    nm_override = intern_str("override");
    nm_final = intern_str("final");
    global_scope = scope_new(SC_GLOBAL, 0, 0);
    cur_scope = global_scope;
    while (T0->kind != K_EOF) list_add(&head, &tail, parse_declaration(CX_NAMESPACE));
    return head;
}
