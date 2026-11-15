/*
 * parser.c - recursive-descent parser for VB.NET.
 *
 * It follows the syntactic grammar of vb.g4 (sections 4 to 11); the rule
 * names of vb.g4 are given in the comments.  Every parse function leaves the
 * statement terminator (end of line or ':') unconsumed; the loops that read
 * statements and members check and skip it.  The parser stops at the first
 * error with "file:line: error: message", using the messages of the VB
 * compiler where they exist.
 *
 * Blocks: the open blocks (If, For, Sub, Class, ...) are kept on a stack,
 * so that a missing or misplaced closing statement can be reported either
 * at the block that is not closed ("'If' must end with a matching 'End
 * If'.") or at the closing statement that has no block ("'Next' must be
 * preceded by a matching 'For'.").
 */

static TokVec *ptv;
static int pi;
static int in_lineif = 0;       /* inside the statements of a single-line If */
static int pending_next = 0;    /* "Next j, i": loops still to be closed */
static int allow_to = 0;        /* "a To b" bounds in the next argument list */
static int in_sub_lambda = 0;   /* the statement of a single-line Sub lambda: it may end at ) , } */

/* contextual keywords */
static Name *w_aggregate, *w_ascending, *w_assembly, *w_async, *w_await, *w_binary, *w_by,
    *w_compare, *w_custom, *w_descending, *w_distinct, *w_equals, *w_explicit, *w_from,
    *w_group, *w_infer, *w_into, *w_isfalse, *w_istrue, *w_iterator, *w_join, *w_key,
    *w_nameof, *w_off, *w_order, *w_out, *w_preserve, *w_skip, *w_strict, *w_take,
    *w_text, *w_until, *w_where, *w_yield;

void parser_init(void) {
    w_aggregate = intern_word("aggregate");
    w_ascending = intern_word("ascending");
    w_assembly = intern_word("assembly");
    w_async = intern_word("async");
    w_await = intern_word("await");
    w_binary = intern_word("binary");
    w_by = intern_word("by");
    w_compare = intern_word("compare");
    w_custom = intern_word("custom");
    w_descending = intern_word("descending");
    w_distinct = intern_word("distinct");
    w_equals = intern_word("equals");
    w_explicit = intern_word("explicit");
    w_from = intern_word("from");
    w_group = intern_word("group");
    w_infer = intern_word("infer");
    w_into = intern_word("into");
    w_isfalse = intern_word("isfalse");
    w_istrue = intern_word("istrue");
    w_iterator = intern_word("iterator");
    w_join = intern_word("join");
    w_key = intern_word("key");
    w_nameof = intern_word("nameof");
    w_off = intern_word("off");
    w_order = intern_word("order");
    w_out = intern_word("out");
    w_preserve = intern_word("preserve");
    w_skip = intern_word("skip");
    w_strict = intern_word("strict");
    w_take = intern_word("take");
    w_text = intern_word("text");
    w_until = intern_word("until");
    w_where = intern_word("where");
    w_yield = intern_word("yield");
}

/* ------------------------------------------------------------ tokens */

static Token *T(int k) {
    int j = pi + k;
    if (j >= ptv->n) j = ptv->n - 1;
    return &ptv->v[j];
}

static int tk(void) { return ptv->v[pi].kind; }
static int tk1(void) { return T(1)->kind; }

static Token *adv(void) {
    Token *t = &ptv->v[pi];
    if (t->kind != T_EOF) pi++;
    return t;
}

static int is_w(Token *t, Name *w) {
    return t->kind == T_IDENT && !(t->flags & TF_ESCAPED) && !t->tchar && t->name == w;
}

static int at_w(Name *w) { return is_w(T(0), w); }

static void perr_at(Token *t, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(t->file, t->line, fmt, ap);
    va_end(ap);
}

static void perr(const char *fmt, ...) {
    va_list ap;
    Token *t = T(0);
    va_start(ap, fmt);
    verror_at(t->file, t->line, fmt, ap);
    va_end(ap);
}

static Node *mk(int kind, Token *t) { return new_node(kind, t); }

static void set_name(Node *n, Token *t) {
    n->name = t->name;
    n->text = t->text;
    n->tchar = t->tchar;
    if (t->flags & TF_ESCAPED) n->flags |= F_ESCAPED;
}

static void expect(int kind, const char *msg) {
    if (tk() != kind) perr("%s", msg);
    adv();
}

static int end_of_stmt(void) {
    int k = tk();
    return k == T_NEWLINE || k == T_COLON || k == T_EOF || (in_lineif && k == K_Else) ||
           (in_sub_lambda && (k == P_RPAREN || k == P_COMMA || k == P_RBRACE));
}

static void expect_eos(void) {
    if (!end_of_stmt()) perr("End of statement expected.");
}

static void skip_terms(void) {
    while (tk() == T_NEWLINE || tk() == T_COLON) adv();
}

static int at_line_start(void) {
    return pi == 0 || ptv->v[pi - 1].kind == T_NEWLINE;
}

static Token *ident(void) {
    if (tk() != T_IDENT) perr("Identifier expected.");
    return adv();
}

static int is_ident_or_kw(int k) { return k == T_IDENT || k >= K_AddHandler; }

static int is_prim_kw(int k) {
    switch (k) {
    case K_Boolean: case K_Byte: case K_Char: case K_Date: case K_Decimal: case K_Double:
    case K_Integer: case K_Long: case K_Object: case K_SByte: case K_Short: case K_Single:
    case K_String: case K_UInteger: case K_ULong: case K_UShort:
        return 1;
    }
    return 0;
}

static int is_cast_kw(int k) {
    switch (k) {
    case K_CBool: case K_CByte: case K_CChar: case K_CDate: case K_CDec: case K_CDbl:
    case K_CInt: case K_CLng: case K_CObj: case K_CSByte: case K_CShort: case K_CSng:
    case K_CStr: case K_CUInt: case K_CULng: case K_CUShort:
        return 1;
    }
    return 0;
}

static int is_relop(int k) {
    return k == P_EQ || k == P_NE || k == P_LT || k == P_GT || k == P_LE || k == P_GE;
}

static int is_compound(int k) {
    switch (k) {
    case P_PLUSEQ: case P_MINUSEQ: case P_STAREQ: case P_SLASHEQ: case P_BACKSLASHEQ:
    case P_CARETEQ: case P_AMPEQ: case P_SHLEQ: case P_SHREQ:
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------ open blocks */

typedef struct {
    int kind;           /* the opening keyword: K_If, K_For, K_Sub, K_Class, ... */
    Token *tok;
} OpenBlock;

static OpenBlock opens[512];
static int nopen = 0;

static void open_push(int kind, Token *t) {
    if (nopen == 512) perr_at(t, "Statements are nested too deeply.");
    opens[nopen].kind = kind;
    opens[nopen].tok = t;
    nopen++;
}

static void open_pop(void) { nopen--; }

/* keywords that can follow End to close a block */
static int is_end_kw(int k) {
    switch (k) {
    case K_Sub: case K_Function: case K_If: case K_Select: case K_While: case K_With:
    case K_SyncLock: case K_Try: case K_Using: case K_Class: case K_Structure: case K_Module:
    case K_Interface: case K_Enum: case K_Namespace: case K_Property: case K_Get: case K_Set:
    case K_Event: case K_Operator: case K_AddHandler: case K_RemoveHandler: case K_RaiseEvent:
        return 1;
    }
    return 0;
}

/* the block that the current token closes (its opening keyword), or 0 */
static int closer_kind(void) {
    switch (tk()) {
    case K_End: return is_end_kw(tk1()) ? tk1() : 0;
    case K_Next: return K_For;
    case K_Loop: return K_Do;
    case K_Else: case K_ElseIf: return K_If;
    case K_Case: return K_Select;
    case K_Catch: case K_Finally: return K_Try;
    }
    return 0;
}

static const char *block_name(int k) {
    switch (k) {
    case K_Select: return "Select Case";
    case K_Event: return "Custom Event";
    }
    return tok_spelling_of(k);
}

static void unclosed(OpenBlock *o) {
    const char *nm = block_name(o->kind);
    switch (o->kind) {
    case K_For: perr_at(o->tok, "'For' must end with a matching 'Next'.");
    case K_Do: perr_at(o->tok, "'Do' must end with a matching 'Loop'.");
    case K_Class: case K_Structure: case K_Module: case K_Interface: case K_Namespace:
    case K_SyncLock: case K_Get: case K_Set:
        perr_at(o->tok, "'%s' statement must end with a matching 'End %s'.", nm, tok_spelling_of(o->kind));
    case K_AddHandler: case K_RemoveHandler: case K_RaiseEvent:
        perr_at(o->tok, "'%s' declaration must end with a matching 'End %s'.", nm, nm);
    }
    perr_at(o->tok, "'%s' must end with a matching 'End %s'.", nm, tok_spelling_of(o->kind));
}

static void stray_closer(void) {
    switch (tk()) {
    case K_Next: perr("'Next' must be preceded by a matching 'For'.");
    case K_Loop: perr("'Loop' must be preceded by a matching 'Do'.");
    case K_Else: perr("'Else' must be preceded by a matching 'If' or 'ElseIf'.");
    case K_ElseIf: perr("'ElseIf' must be preceded by a matching 'If' or 'ElseIf'.");
    case K_Case: perr("'Case' can only appear inside a 'Select Case' statement.");
    case K_Catch: perr("'Catch' cannot appear outside a 'Try' statement.");
    case K_Finally: perr("'Finally' can only appear inside a 'Try' statement.");
    }
    perr("'End %s' must be preceded by a matching '%s'.", tok_spelling_of(tk1()), block_name(tk1()));
}

/* the current token is not the closer of the innermost open block: reports
   the innermost block as not closed if an outer block (or the end of the
   file) explains the token, else the token as a closer without a block */
static void closer_error(void) {
    int ck = closer_kind(), i;
    if (nopen > 0) {
        if (tk() == T_EOF) unclosed(&opens[nopen - 1]);
        for (i = nopen - 2; i >= 0; i--)
            if (opens[i].kind == ck) unclosed(&opens[nopen - 1]);
    }
    if (ck == 0) perr("Syntax error.");
    stray_closer();
}

/* the innermost open block (kind) must be closed here by "End kind" */
static void close_end(int kind) {
    if (tk() == K_End && tk1() == kind) {
        adv();
        adv();
        open_pop();
        return;
    }
    closer_error();
}

/* ------------------------------------------------------------ forward */

static Node *parse_expr(void);
static Node *parse_binary(int min);
static Node *parse_term(void);
static Node *parse_type(int allow_array);
static Node *parse_nonarray_type(void);
static NodeVec *parse_block(void);
static Node *parse_statement(void);
static NodeVec *parse_params(void);
static NodeVec *parse_var_declarators(void);
static Node *parse_member(int ctx);
static void parse_attributes(void);

/* ------------------------------------------------------------ types */

/* TypeArgumentList, after "(Of"; an empty argument (open type) is TOPEN */
static NodeVec *parse_type_args_after_of(void) {
    NodeVec *v = nv_new();
    for (;;) {
        if (tk() == P_COMMA || tk() == P_RPAREN) nv_push(v, mk(N_TOPEN, T(0)));
        else nv_push(v, parse_type(1));
        if (tk() != P_COMMA) break;
        adv();
    }
    expect(P_RPAREN, "')' expected.");
    return v;
}

/* NonArrayTypeName: BuiltInTypeName or QualifiedTypeName */
static Node *parse_nonarray_type(void) {
    Token *t = T(0);
    Node *n;
    if (is_prim_kw(t->kind)) {
        adv();
        n = mk(N_TPRIM, t);
        n->op = t->kind;
        return n;
    }
    n = mk(N_TNAME, t);
    n->list = nv_new();
    if (t->kind == K_Global) {
        adv();
        n->flags |= F_GLOBAL;
        expect(P_DOT, "'.' expected.");
    } else if (t->kind != T_IDENT) perr("Type expected.");
    for (;;) {
        Token *p = T(0);
        Node *part;
        if (!is_ident_or_kw(p->kind)) perr("Identifier expected.");
        adv();
        part = mk(N_TPART, p);
        set_name(part, p);
        if (tk() == P_LPAREN && tk1() == K_Of) {
            adv();
            adv();
            part->list = parse_type_args_after_of();
        }
        nv_push(n->list, part);
        if (tk() != P_DOT) break;
        adv();
    }
    return n;
}

/* "(" ","* ")" */
static int at_array_mod(void) {
    int k = 1;
    if (tk() != P_LPAREN) return 0;
    while (T(k)->kind == P_COMMA) k++;
    return T(k)->kind == P_RPAREN;
}

static Node *array_mod(void) {
    Node *n = mk(N_TARRAY, T(0));
    n->ival = 1;
    adv();
    while (tk() == P_COMMA) {
        adv();
        n->ival++;
    }
    expect(P_RPAREN, "')' expected.");
    return n;
}

/* ArrayTypeModifiers after a type: the leftmost is the outermost array */
static Node *wrap_array_mods(Node *elem, NodeVec *mods) {
    int i;
    for (i = mods->n - 1; i >= 0; i--) {
        mods->v[i]->a = elem;
        elem = mods->v[i];
    }
    return elem;
}

/* TypeName */
static Node *parse_type(int allow_array) {
    Node *t = parse_nonarray_type();
    if (tk() == P_QUESTION) {
        Node *nn = mk(N_TNULLABLE, T(0));
        adv();
        nn->a = t;
        t = nn;
    }
    if (allow_array && at_array_mod()) {
        NodeVec *mods = nv_new();
        while (at_array_mod()) nv_push(mods, array_mod());
        t = wrap_array_mods(t, mods);
    }
    return t;
}

/* ------------------------------------------------------------ arguments */

/* ArgumentList, after "(", through ")"; omitted arguments are ARG nodes
   without an expression */
static NodeVec *parse_args(void) {
    NodeVec *v = nv_new();
    int to_ok = allow_to;
    allow_to = 0;
    if (tk() == P_RPAREN) {
        adv();
        return v;
    }
    for (;;) {
        Token *t = T(0);
        Node *arg = mk(N_ARG, t);
        if (tk() == P_COMMA || tk() == P_RPAREN) {
            /* omitted */
        } else if (is_ident_or_kw(t->kind) && tk1() == P_COLONEQ) {
            adv();
            adv();
            set_name(arg, t);
            arg->a = parse_expr();
        } else {
            arg->a = parse_expr();
            if (to_ok && tk() == K_To) {
                Node *b = mk(N_BOUND, T(0));
                adv();
                b->a = arg->a;
                b->b = parse_expr();
                arg->a = b;
            }
        }
        nv_push(v, arg);
        if (tk() == P_COMMA) {
            adv();
            continue;
        }
        expect(P_RPAREN, "')' expected.");
        return v;
    }
}

/* the bounds of an ArraySizeInitializationModifier: the expressions of a
   parsed argument list (no omitted or named arguments) */
static NodeVec *args_to_bounds(NodeVec *args, Token *at) {
    NodeVec *v = nv_new();
    int i;
    for (i = 0; i < args->n; i++) {
        Node *a = args->v[i];
        if (!a->a) perr_at(at, "Array bounds expected.");
        if (a->name) perr_at(at, "Named arguments are not valid as array bounds.");
        nv_push(v, a->a);
    }
    return v;
}

/* "(" Bound { "," Bound } ")"  with  Bound: Expression | Expression To Expression */
static NodeVec *parse_bounds(void) {
    Token *t = T(0);
    NodeVec *args;
    adv();
    allow_to = 1;
    args = parse_args();
    return args_to_bounds(args, t);
}

/* IdentifierModifiers: [?] [ArraySizeInitializationModifier | ArrayTypeModifiers] */
static void parse_id_mods(Node *v, int allow_bounds) {
    if (tk() == P_QUESTION) {
        adv();
        v->flags |= F_NULLABLE;
    }
    if (tk() == P_LPAREN && !at_array_mod()) {
        Node *rank;
        if (!allow_bounds) perr("Array bounds cannot appear in type specifiers.");
        rank = mk(N_TARRAY, T(0));
        v->list = parse_bounds();
        rank->ival = v->list->n;
        v->list2 = nv_new();
        nv_push(v->list2, rank);
    }
    while (at_array_mod()) {
        if (!v->list2) v->list2 = nv_new();
        nv_push(v->list2, array_mod());
    }
}

/* ------------------------------------------------------------ expressions */

#define PREC_XOR    1
#define PREC_OR     2
#define PREC_AND    3
#define PREC_NOT    4
#define PREC_REL    5
#define PREC_SHIFT  6
#define PREC_CONCAT 7
#define PREC_ADD    8
#define PREC_MOD    9
#define PREC_IDIV   10
#define PREC_MUL    11
#define PREC_NEG    12
#define PREC_POW    13

static int binop_prec(int k) {
    switch (k) {
    case P_CARET: return PREC_POW;
    case P_STAR: case P_SLASH: return PREC_MUL;
    case P_BACKSLASH: return PREC_IDIV;
    case K_Mod: return PREC_MOD;
    case P_PLUS: case P_MINUS: return PREC_ADD;
    case P_AMP: return PREC_CONCAT;
    case P_SHL: case P_SHR: return PREC_SHIFT;
    case P_EQ: case P_NE: case P_LT: case P_GT: case P_LE: case P_GE:
    case K_Is: case K_IsNot: case K_Like:
        return PREC_REL;
    case K_And: case K_AndAlso: return PREC_AND;
    case K_Or: case K_OrElse: return PREC_OR;
    case K_Xor: return PREC_XOR;
    }
    return 0;
}

static Node *parse_lambda(void);
static Node *parse_query(void);
static Node *parse_new(void);
static Node *parse_interp(Token *t);

/* CollectionInitializer: "{" [ element { "," element } ] "}" */
static Node *parse_coll_init(void) {
    Node *n = mk(N_COLLINIT, T(0));
    n->list = nv_new();
    expect(P_LBRACE, "'{' expected.");
    if (tk() != P_RBRACE) {
        for (;;) {
            nv_push(n->list, parse_expr());
            if (tk() != P_COMMA) break;
            adv();
        }
    }
    expect(P_RBRACE, "'}' expected.");
    return n;
}

/* ObjectMemberInitializer: With "{" FieldInitializer { "," ... } "}" */
static Node *parse_obj_init(int anonymous) {
    Node *n = mk(N_OBJINIT, T(0));
    n->list = nv_new();
    adv();          /* With */
    expect(P_LBRACE, "'{' expected.");
    for (;;) {
        Token *t = T(0);
        Node *fi = mk(N_FIELDINIT, t);
        if (anonymous && at_w(w_key)) {
            adv();
            fi->flags |= F_KEY;
        }
        if (tk() == P_DOT) {
            Token *id;
            adv();
            id = T(0);
            if (!is_ident_or_kw(id->kind)) perr("Identifier expected.");
            adv();
            set_name(fi, id);
            expect(P_EQ, "'=' expected.");
        } else if (!anonymous) {
            perr("'.' expected.");
        }
        fi->a = parse_expr();
        nv_push(n->list, fi);
        if (tk() != P_COMMA) break;
        adv();
    }
    expect(P_RBRACE, "'}' expected.");
    return n;
}

/* NewExpression: ObjectCreationExpression, ArrayCreationExpression or
   AnonymousObjectCreationExpression */
static Node *parse_new(void) {
    Token *t = adv();
    Node *type, *n;
    NodeVec *args = 0;
    Token *lp = 0;
    if (tk() == K_With) {
        n = mk(N_ANONNEW, t);
        n->b = parse_obj_init(1);
        return n;
    }
    type = parse_nonarray_type();
    if (tk() == P_QUESTION) {
        Node *nn = mk(N_TNULLABLE, T(0));
        adv();
        nn->a = type;
        type = nn;
    }
    if (tk() == P_LPAREN) {
        lp = adv();
        allow_to = 1;
        args = parse_args();
    }
    if (args && (tk() == P_LBRACE || at_array_mod())) {
        /* array creation: New T(bounds) {elements} or New T() {elements} */
        Node *rank = mk(N_TARRAY, lp);
        int i, omitted = 0;
        n = mk(N_NEWARRAY, t);
        n->a = type;
        for (i = 0; i < args->n; i++) if (!args->v[i]->a) omitted++;
        if (omitted == args->n) rank->ival = args->n ? args->n : 1;
        else {
            n->list = args_to_bounds(args, lp);
            rank->ival = n->list->n;
        }
        n->list2 = nv_new();
        nv_push(n->list2, rank);
        while (at_array_mod()) nv_push(n->list2, array_mod());
        if (tk() != P_LBRACE) perr("'{' expected.");
        n->b = parse_coll_init();
        return n;
    }
    n = mk(N_NEW, t);
    n->a = type;
    if (args) {
        int i;
        for (i = 0; i < args->n; i++)
            if (args->v[i]->a && args->v[i]->a->kind == N_BOUND) perr_at(lp, "Array bounds cannot appear here.");
        n->list = args;
        n->flags |= F_HASARGS;
    }
    if (tk() == K_With && tk1() == P_LBRACE) n->b = parse_obj_init(0);
    else if (at_w(w_from) && tk1() == P_LBRACE) {
        adv();
        n->b = parse_coll_init();
    }
    return n;
}

/* the postfix operators: member access, ?., dictionary access, invocation */
static Node *parse_postfix(Node *e) {
    for (;;) {
        Token *t = T(0);
        Node *n;
        if (t->kind == P_DOT || t->kind == P_QDOT) {
            Token *m;
            adv();
            m = T(0);
            if (!is_ident_or_kw(m->kind)) perr("Identifier expected.");
            adv();
            n = mk(N_MEMBER, m);
            n->a = e;
            set_name(n, m);
            if (t->kind == P_QDOT) n->flags |= F_QDOT;
            if (tk() == P_LPAREN && tk1() == K_Of) {
                adv();
                adv();
                n->list = parse_type_args_after_of();
            }
            e = n;
        } else if (t->kind == P_BANG && is_ident_or_kw(tk1())) {
            Token *m;
            adv();
            m = adv();
            n = mk(N_DICT, m);
            n->a = e;
            set_name(n, m);
            e = n;
        } else if (t->kind == P_LPAREN) {
            adv();
            n = mk(N_INVOKE, t);
            n->line = e->line;
            n->a = e;
            n->list = parse_args();
            e = n;
        } else if (t->kind == P_QUESTION && tk1() == P_LPAREN) {
            adv();
            adv();
            n = mk(N_INVOKE, t);
            n->line = e->line;
            n->a = e;
            n->flags |= F_QDOT;
            n->list = parse_args();
            e = n;
        } else break;
    }
    return e;
}

static int at_query_start(void) {
    return (at_w(w_from) || at_w(w_aggregate)) && tk1() == T_IDENT &&
           (T(2)->kind == K_In || T(2)->kind == K_As);
}

/* SimpleExpression and the other primary expressions */
static Node *parse_primary(void) {
    Token *t = T(0);
    Node *n;
    switch (t->kind) {
    case T_INT:
        adv();
        n = mk(N_INT, t);
        n->ival = t->ival;
        n->tchar = t->tchar;
        n->aux = t->base;
        return n;
    case T_FLOAT:
        adv();
        n = mk(N_FLOAT, t);
        n->dval = t->dval;
        n->tchar = t->tchar;
        return n;
    case T_STRING:
        adv();
        n = mk(N_STRING, t);
        n->text = t->text;
        return n;
    case T_CHAR:
        adv();
        n = mk(N_CHAR, t);
        n->ival = t->ival;
        return n;
    case T_INTERP:
        adv();
        return parse_interp(t);
    case K_True: case K_False:
        adv();
        n = mk(N_BOOL, t);
        n->ival = t->kind == K_True;
        return n;
    case K_Nothing:
        adv();
        return mk(N_NOTHING, t);
    case K_Me:
        adv();
        return mk(N_ME, t);
    case K_MyBase:
        adv();
        if (tk() != P_DOT) perr("'MyBase' must be followed by '.' and an identifier.");
        return mk(N_MYBASE, t);
    case K_MyClass:
        adv();
        if (tk() != P_DOT) perr("'MyClass' must be followed by '.' and an identifier.");
        return mk(N_MYCLASS, t);
    case K_Global:
        adv();
        if (tk() != P_DOT) perr("'Global' must be followed by '.' and an identifier.");
        return mk(N_GLOBAL, t);
    case P_LPAREN:
        adv();
        n = mk(N_PAREN, t);
        n->a = parse_expr();
        expect(P_RPAREN, "')' expected.");
        return n;
    case P_DOT:         /* .member inside a With block */
    case P_BANG:
        if (!is_ident_or_kw(tk1())) perr("Identifier expected.");
        return 0;       /* parse_postfix applies it to an empty base */
    case P_LBRACE:
        return parse_coll_init();
    case K_New:
        return parse_new();
    case K_CType: case K_DirectCast: case K_TryCast:
        adv();
        n = mk(N_CAST, t);
        n->op = t->kind;
        expect(P_LPAREN, "'(' expected.");
        n->a = parse_expr();
        expect(P_COMMA, "',' expected.");
        n->b = parse_type(1);
        expect(P_RPAREN, "')' expected.");
        return n;
    case K_TypeOf:
        adv();
        n = mk(N_TYPEOF, t);
        n->a = parse_binary(PREC_SHIFT);
        if (tk() == K_IsNot) n->flags |= F_ISNOT;
        else if (tk() != K_Is) perr("'Is' expected.");
        adv();
        n->b = parse_type(1);
        return n;
    case K_GetType:
        adv();
        n = mk(N_GETTYPE, t);
        expect(P_LPAREN, "'(' expected.");
        n->a = parse_type(1);
        expect(P_RPAREN, "')' expected.");
        return n;
    case K_AddressOf:
        adv();
        n = mk(N_ADDRESSOF, t);
        n->a = parse_term();
        return n;
    case K_If:
        adv();
        n = mk(N_IIF, t);
        expect(P_LPAREN, "'(' expected.");
        n->a = parse_expr();
        expect(P_COMMA, "',' expected.");
        n->b = parse_expr();
        if (tk() == P_COMMA) {
            adv();
            n->c = parse_expr();
        }
        expect(P_RPAREN, "')' expected.");
        return n;
    case K_Function: case K_Sub:
        return parse_lambda();
    case K_GetXmlNamespace:
        perr("XML literals are not supported.");
    case P_LT:
        perr("XML literals are not supported.");
    case T_IDENT:
        if (at_query_start()) return parse_query();
        if (at_w(w_nameof) && tk1() == P_LPAREN) {
            adv();
            adv();
            n = mk(N_NAMEOF, t);
            n->a = parse_expr();
            expect(P_RPAREN, "')' expected.");
            return n;
        }
        if ((at_w(w_async) || at_w(w_iterator)) && (tk1() == K_Function || tk1() == K_Sub))
            perr("'%s' lambdas are not supported.", at_w(w_async) ? "Async" : "Iterator");
        adv();
        n = mk(N_NAME, t);
        set_name(n, t);
        if (tk() == P_LPAREN && tk1() == K_Of) {
            adv();
            adv();
            n->list = parse_type_args_after_of();
        }
        return n;
    }
    if (is_cast_kw(t->kind)) {
        adv();
        n = mk(N_CAST, t);
        n->op = t->kind;
        expect(P_LPAREN, "'(' expected.");
        n->a = parse_expr();
        expect(P_RPAREN, "')' expected.");
        return n;
    }
    if (is_prim_kw(t->kind)) {      /* Integer.Parse(...), String.Empty */
        n = mk(N_TYPEEXPR, t);
        n->a = parse_nonarray_type();
        if (tk() != P_DOT) perr("'.' expected.");
        return n;
    }
    perr("Expression expected.");
    return 0;
}

/* a term: a primary expression and its postfix operators */
static Node *parse_term(void) {
    Token *t = T(0);
    Node *base = parse_primary();
    if (!base) {
        /* ".name" or "!name" with the With object as the base */
        Node *n;
        Token *m;
        adv();
        m = adv();
        n = mk(t->kind == P_DOT ? N_MEMBER : N_DICT, m);
        set_name(n, m);
        if (t->kind == P_DOT && tk() == P_LPAREN && tk1() == K_Of) {
            adv();
            adv();
            n->list = parse_type_args_after_of();
        }
        base = n;
    }
    return parse_postfix(base);
}

/* OperatorExpression by precedence climbing; the unary operators parse
   their operand at their own precedence (as -2 ^ 2 = -4 and
   Not a = b is Not (a = b)) */
static Node *parse_binary(int min) {
    Node *left;
    Token *t = T(0);
    if (t->kind == P_MINUS || t->kind == P_PLUS) {
        adv();
        left = mk(N_UNARY, t);
        left->op = t->kind;
        left->a = parse_binary(PREC_NEG);
    } else if (t->kind == K_Not) {
        adv();
        left = mk(N_UNARY, t);
        left->op = t->kind;
        left->a = parse_binary(PREC_NOT);
    } else if (is_w(t, w_await) && (tk1() == T_IDENT || tk1() == K_Me || tk1() == K_New)) {
        perr("'Await' is not supported.");
        left = 0;
    } else left = parse_term();
    for (;;) {
        Token *o = T(0);
        int p = binop_prec(o->kind);
        Node *n;
        if (p == 0 || p < min) break;
        adv();
        n = mk(N_BINARY, o);
        n->line = left->line;
        n->op = o->kind;
        n->a = left;
        n->b = parse_binary(p + 1);
        left = n;
    }
    return left;
}

static Node *parse_expr(void) {
    return parse_binary(PREC_XOR);
}

/* ------------------------------------------------------------ lambdas */

/* LambdaExpression: Function/Sub [( params )] expression | statement, or
   the multi-line form ending with End Function / End Sub */
static Node *parse_lambda(void) {
    Token *t = adv();
    Node *n = mk(N_LAMBDA, t);
    n->op = t->kind;
    expect(P_LPAREN, "'(' expected.");
    n->list2 = parse_params();
    if (t->kind == K_Function && tk() == K_As) {
        adv();
        n->a = parse_type(1);
        if (tk() != T_NEWLINE) perr("End of statement expected.");
    }
    if (tk() == T_NEWLINE) {
        int save_if = in_lineif, save_next = pending_next, save_sl = in_sub_lambda;
        n->flags |= F_MULTILINE;
        in_lineif = 0;
        in_sub_lambda = 0;
        pending_next = 0;
        open_push(t->kind, t);
        n->list3 = parse_block();
        close_end(t->kind);
        in_lineif = save_if;
        in_sub_lambda = save_sl;
        pending_next = save_next;
    } else if (t->kind == K_Function) {
        n->b = parse_expr();
    } else {
        int save_sl = in_sub_lambda;
        in_sub_lambda = 1;
        n->list3 = nv_new();
        nv_push(n->list3, parse_statement());
        in_sub_lambda = save_sl;
    }
    return n;
}

/* ------------------------------------------------------------ queries */

/* the start of a query operator at token j (after a line break, the query
   continues on the next line) */
static int is_query_op_at(int j) {
    Token *t = T(j), *n = T(j + 1);
    if (t->kind == K_Select) return n->kind != K_Case;
    if (t->kind == K_Let) return n->kind == T_IDENT;
    if (t->kind != T_IDENT || (t->flags & TF_ESCAPED) || t->tchar) return 0;
    if (n->kind == P_EQ || n->kind == P_DOT || n->kind == P_LPAREN || is_compound(n->kind)) {
        /* "Where (x > 1)" and "Take (5)" are query operators too */
        if (!(n->kind == P_LPAREN && (t->name == w_where || t->name == w_take || t->name == w_skip)))
            return 0;
    }
    if (t->name == w_from || t->name == w_aggregate || t->name == w_join) return n->kind == T_IDENT;
    if (t->name == w_order) return is_w(n, w_by);
    if (t->name == w_group) return is_w(n, w_by) || is_w(n, w_join) || n->kind != T_NEWLINE;
    if (t->name == w_where || t->name == w_take || t->name == w_skip || t->name == w_distinct)
        return 1;
    return 0;
}

static void query_continuation(void) {
    if (tk() == T_NEWLINE && is_query_op_at(1)) adv();
}

/* a line break before On, Into or By (vb.g4: LineTerminator? 'Into') */
static void query_nl_before(Name *w, int kind) {
    if (tk() == T_NEWLINE && ((w && is_w(T(1), w)) || (kind && tk1() == kind))) adv();
}

/* CollectionRangeVariableDeclaration: name [As type] In expression */
static Node *parse_range_in(void) {
    Token *id = ident();
    Node *r = mk(N_RANGEVAR, id);
    set_name(r, id);
    if (tk() == K_As) {
        adv();
        r->a = parse_type(1);
    }
    expect(K_In, "'In' expected.");
    r->b = parse_expr();
    return r;
}

static NodeVec *parse_range_in_list(void) {
    NodeVec *v = nv_new();
    for (;;) {
        nv_push(v, parse_range_in());
        if (tk() != P_COMMA) break;
        adv();
    }
    return v;
}

/* ExpressionRangeVariableDeclaration list; the name is optional where the
   query can infer it (Select, Group By, Into) */
static NodeVec *parse_expr_range_list(int name_required) {
    NodeVec *v = nv_new();
    for (;;) {
        Token *t = T(0);
        Node *r = mk(N_RANGEVAR, t);
        if (t->kind == T_IDENT && (tk1() == P_EQ || tk1() == K_As)) {
            adv();
            set_name(r, t);
            if (tk() == K_As) {
                adv();
                r->a = parse_type(1);
            }
            expect(P_EQ, "'=' expected.");
        } else if (name_required) perr("Identifier expected.");
        r->b = parse_expr();
        nv_push(v, r);
        if (tk() != P_COMMA) break;
        adv();
    }
    return v;
}

static Node *qop(Token *t, const char *name) {
    Node *n = mk(N_QOP, t);
    n->text = name;
    return n;
}

static Node *parse_query_op(int first);

/* JoinQueryOperator / GroupJoinQueryOperator, after Join */
static Node *parse_join(Token *t, int group) {
    Node *n = qop(t, group ? "Group Join" : "Join");
    n->a = parse_range_in();
    n->list = nv_new();
    for (;;) {
        query_continuation();
        if (at_w(w_join)) {
            Token *j = adv();
            nv_push(n->list, parse_join(j, 0));
        } else if (at_w(w_group) && is_w(T(1), w_join)) {
            Token *j = adv();
            adv();
            nv_push(n->list, parse_join(j, 1));
        } else break;
    }
    query_continuation();
    query_nl_before(0, K_On);
    expect(K_On, "'On' expected.");
    n->list2 = nv_new();
    for (;;) {
        Node *c = mk(N_JOINCOND, T(0));
        c->a = parse_binary(PREC_NOT);
        if (!at_w(w_equals)) perr("'Equals' expected.");
        adv();
        c->b = parse_binary(PREC_NOT);
        nv_push(n->list2, c);
        if (tk() != K_And) break;
        adv();
    }
    if (group) {
        query_nl_before(w_into, 0);
        if (!at_w(w_into)) perr("'Into' expected.");
        adv();
        n->list3 = parse_expr_range_list(0);
    }
    return n;
}

static Node *parse_query_op(int first) {
    Token *t;
    Node *n;
    if (!first) query_continuation();
    t = T(0);
    if (at_w(w_from) && tk1() == T_IDENT) {
        adv();
        n = qop(t, "From");
        n->list = parse_range_in_list();
        return n;
    }
    if (at_w(w_aggregate) && tk1() == T_IDENT) {
        adv();
        n = qop(t, "Aggregate");
        n->list = parse_range_in_list();
        n->list2 = nv_new();
        for (;;) {
            Node *op;
            query_continuation();
            query_nl_before(w_into, 0);
            if (at_w(w_into)) break;
            op = parse_query_op(0);
            if (!op) perr("'Into' expected.");
            nv_push(n->list2, op);
        }
        adv();
        n->list3 = parse_expr_range_list(0);
        return n;
    }
    if (first) return 0;
    if (t->kind == K_Select && tk1() != K_Case) {
        adv();
        n = qop(t, "Select");
        n->list = parse_expr_range_list(0);
        return n;
    }
    if (t->kind == K_Let) {
        adv();
        n = qop(t, "Let");
        n->list = parse_expr_range_list(1);
        return n;
    }
    if (t->kind != T_IDENT || (t->flags & TF_ESCAPED)) return 0;
    if (t->name == w_distinct) {
        adv();
        return qop(t, "Distinct");
    }
    if (t->name == w_where) {
        adv();
        n = qop(t, "Where");
        n->b = parse_expr();
        return n;
    }
    if (t->name == w_order && is_w(T(1), w_by)) {
        adv();
        adv();
        n = qop(t, "Order By");
        n->list = nv_new();
        for (;;) {
            Node *k = mk(N_ORDERKEY, T(0));
            k->a = parse_expr();
            if (at_w(w_descending)) {
                adv();
                k->flags |= F_DESC;
            } else if (at_w(w_ascending)) {
                adv();
                k->flags |= F_ASC;
            }
            nv_push(n->list, k);
            if (tk() != P_COMMA) break;
            adv();
        }
        return n;
    }
    if (t->name == w_take || t->name == w_skip) {
        int take = t->name == w_take;
        adv();
        if (tk() == K_While) {
            adv();
            n = qop(t, take ? "Take While" : "Skip While");
        } else n = qop(t, take ? "Take" : "Skip");
        n->b = parse_expr();
        return n;
    }
    if (t->name == w_join) {
        adv();
        return parse_join(t, 0);
    }
    if (t->name == w_group) {
        adv();
        if (at_w(w_join)) {
            adv();
            return parse_join(t, 1);
        }
        n = qop(t, "Group By");
        if (!at_w(w_by)) n->list = parse_expr_range_list(0);
        query_nl_before(w_by, 0);
        if (!at_w(w_by)) perr("'By' expected.");
        adv();
        n->list2 = parse_expr_range_list(0);
        query_nl_before(w_into, 0);
        if (!at_w(w_into)) perr("'Into' expected.");
        adv();
        n->list3 = parse_expr_range_list(0);
        return n;
    }
    return 0;
}

/* QueryExpression: FromOrAggregateQueryOperator QueryOperator* */
static Node *parse_query(void) {
    Node *q = mk(N_QUERY, T(0));
    q->list = nv_new();
    nv_push(q->list, parse_query_op(1));
    for (;;) {
        Node *op = parse_query_op(0);
        if (!op) break;
        nv_push(q->list, op);
    }
    return q;
}

/* ------------------------------------------------------------ interpolated strings */

static int is_dq(int c) { return c == '"' || c == 0x201C || c == 0x201D; }

/* parses codepoints [s, e) of file f as one expression */
static Node *parse_sub_expr(int f, int s, int e, int line, int col, Token *at) {
    TokVec *save_tv = ptv, *tv;
    int save_pi = pi;
    Node *n;
    tv = lex_range(f, s, e, line, col);
    ptv = tv;
    pi = 0;
    if (tk() == T_EOF) perr_at(at, "Expression expected.");
    n = parse_expr();
    if (tk() != T_EOF) perr("Syntax error.");
    ptv = save_tv;
    pi = save_pi;
    return n;
}

static Node *lit_part(Token *t, StrBuf *b, int line) {
    Node *n = mk(N_STRING, t);
    n->line = line;
    n->text = sb_take(b);
    return n;
}

/* $"..." : the literal parts and the holes { expression [, alignment] [: format] } */
static Node *parse_interp(Token *t) {
    const int *s = files[t->file].text;
    int i = t->start + 2, end = t->end - 1;
    int line = t->line, line_start = -1;
    Node *n = mk(N_INTERP, t);
    StrBuf lit;
    sb_init(&lit);
    n->list = nv_new();
    while (i < end) {
        int c = s[i];
        if (c == '{' && s[i + 1] == '{') { sb_putc(&lit, '{'); i += 2; continue; }
        if (c == '}' && s[i + 1] == '}') { sb_putc(&lit, '}'); i += 2; continue; }
        if (is_dq(c)) { sb_putc(&lit, '"'); i += 2; continue; }
        if (c == 10 || c == 13) {
            if (c == 10 || s[i + 1] != 10) line++, line_start = i + 1;
            sb_put_cp(&lit, c);
            i++;
            continue;
        }
        if (c == '{') {
            int hs = i + 1, j = hs, depth = 0, comma = -1, colon = -1, hline = line, hcol;
            Node *h;
            if (lit.len) nv_push(n->list, lit_part(t, &lit, line));
            hcol = line_start < 0 ? t->col + (hs - t->start) : hs - line_start + 1;
            for (;;) {
                int d = s[j];
                if (j >= end) perr_at(t, "'}' expected.");
                if (colon >= 0) {
                    if (d == '}') break;
                    j++;
                    continue;
                }
                if (is_dq(d)) {
                    j++;
                    while (j < end && !(is_dq(s[j]) && !is_dq(s[j + 1]))) j += is_dq(s[j]) ? 2 : 1;
                    j++;
                    continue;
                }
                if (d == 10 || (d == 13 && s[j + 1] != 10)) line++, line_start = j + 1;
                if (d == '(' || d == '[' || d == '{') depth++;
                else if (d == ')' || d == ']') depth--;
                else if (d == '}') {
                    if (depth == 0) break;
                    depth--;
                } else if (depth == 0 && d == ',' && comma < 0) comma = j;
                else if (depth == 0 && d == ':') colon = j;
                j++;
            }
            h = mk(N_INTERPHOLE, t);
            h->line = hline;
            {
                int ee = comma >= 0 ? comma : colon >= 0 ? colon : j;
                h->a = parse_sub_expr(t->file, hs, ee, hline, hcol, t);
            }
            if (comma >= 0)
                h->b = parse_sub_expr(t->file, comma + 1, colon >= 0 ? colon : j, hline, hcol, t);
            if (colon >= 0) {
                StrBuf fb;
                int k;
                sb_init(&fb);
                for (k = colon + 1; k < j; k++) sb_put_cp(&fb, s[k]);
                h->text = fb.s;
            }
            nv_push(n->list, h);
            i = j + 1;
            continue;
        }
        sb_put_cp(&lit, c);
        i++;
    }
    if (lit.len) nv_push(n->list, lit_part(t, &lit, line));
    return n;
}

/* ------------------------------------------------------------ parameters, attributes */

/* Attributes: parsed and ignored */
static void parse_attributes(void) {
    while (tk() == P_LT) {
        adv();
        for (;;) {
            if ((at_w(w_assembly) || tk() == K_Module) && tk1() == T_COLON) {
                adv();
                adv();
            }
            parse_nonarray_type();
            if (tk() == P_LPAREN) {
                adv();
                parse_args();
            }
            if (tk() != P_COMMA) break;
            adv();
        }
        expect(P_GT, "'>' expected.");
    }
}

/* ParameterList, after "(", through ")" */
static NodeVec *parse_params(void) {
    NodeVec *v = nv_new();
    if (tk() == P_RPAREN) {
        adv();
        return v;
    }
    for (;;) {
        Token *id;
        Node *p;
        int mods = 0;
        parse_attributes();
        for (;;) {
            int b = 0;
            switch (tk()) {
            case K_ByVal: b = M_BYVAL; break;
            case K_ByRef: b = M_BYREF; break;
            case K_Optional: b = M_OPTIONAL; break;
            case K_ParamArray: b = M_PARAMARRAY; break;
            }
            if (!b) break;
            if (mods & b) perr("Duplicate specifier.");
            mods |= b;
            adv();
        }
        if ((mods & M_BYVAL) && (mods & M_BYREF)) perr("'ByVal' and 'ByRef' cannot be combined.");
        id = ident();
        p = mk(N_PARAM, id);
        set_name(p, id);
        p->mods = mods;
        parse_id_mods(p, 0);
        if (tk() == K_As) {
            adv();
            p->a = parse_type(1);
        }
        if (tk() == P_EQ) {
            adv();
            p->b = parse_expr();
        }
        nv_push(v, p);
        if (tk() == P_COMMA) {
            adv();
            continue;
        }
        expect(P_RPAREN, "')' expected.");
        return v;
    }
}

/* TypeParameterList, after "(Of" */
static NodeVec *parse_type_params(void) {
    NodeVec *v = nv_new();
    for (;;) {
        Token *id;
        Node *tp;
        int fl = 0;
        if (tk() == K_In) {
            adv();
            fl = F_IN;
        } else if (at_w(w_out) && tk1() == T_IDENT) {
            adv();
            fl = F_OUT;
        }
        id = ident();
        tp = mk(N_TYPEPARAM, id);
        set_name(tp, id);
        tp->flags |= fl;
        if (tk() == K_As) {
            int braces;
            adv();
            tp->list = nv_new();
            braces = tk() == P_LBRACE;
            if (braces) adv();
            for (;;) {
                Token *c = T(0);
                if (c->kind == K_New || c->kind == K_Structure || c->kind == K_Class) {
                    Node *cn = mk(N_CONSTRAINT, c);
                    cn->op = c->kind;
                    adv();
                    nv_push(tp->list, cn);
                } else nv_push(tp->list, parse_type(1));
                if (!braces || tk() != P_COMMA) break;
                adv();
            }
            if (braces) expect(P_RBRACE, "'}' expected.");
        }
        nv_push(v, tp);
        if (tk() == P_COMMA) {
            adv();
            continue;
        }
        expect(P_RPAREN, "')' expected.");
        return v;
    }
}

/* HandlesClause or ImplementsClause */
static Node *parse_handles(void) {
    Token *t = adv();
    Node *n = mk(N_HANDLES, t);
    n->op = t->kind;
    n->list = nv_new();
    for (;;) {
        if (t->kind == K_Implements) {
            Node *ty = parse_nonarray_type();
            if (ty->kind != N_TNAME || ty->list->n < 2) perr("'.' expected.");
            nv_push(n->list, ty);
        } else {
            Token *b = T(0), *m;
            Node *base, *mem;
            if (b->kind == K_Me) base = mk(N_ME, b);
            else if (b->kind == K_MyBase) base = mk(N_MYBASE, b);
            else if (b->kind == K_MyClass) base = mk(N_MYCLASS, b);
            else if (b->kind == T_IDENT) {
                base = mk(N_NAME, b);
                set_name(base, b);
            } else {
                perr("Identifier expected.");
                base = 0;
            }
            adv();
            expect(P_DOT, "'.' expected.");
            m = T(0);
            if (!is_ident_or_kw(m->kind)) perr("Identifier expected.");
            adv();
            mem = mk(N_MEMBER, m);
            set_name(mem, m);
            mem->a = base;
            nv_push(n->list, mem);
        }
        if (tk() != P_COMMA) break;
        adv();
    }
    return n;
}

/* ------------------------------------------------------------ statements */

static int block_end(void) {
    if (tk() == K_EndIf) perr("'EndIf' statements are no longer supported; use 'End If' instead.");
    if (tk() == K_Wend) perr("'Wend' statements are no longer supported; use 'End While' statements instead.");
    return tk() == T_EOF || closer_kind() != 0;
}

/* Block: statements up to a closing statement (not consumed) */
static NodeVec *parse_block(void) {
    NodeVec *v = nv_new();
    for (;;) {
        Node *s;
        skip_terms();
        if (block_end()) return v;
        if (at_line_start() && (tk() == T_IDENT || tk() == T_INT) && tk1() == T_COLON) {
            /* LabelDeclarationStatement */
            Token *t = adv();
            Node *l = mk(N_LABEL, t);
            if (t->kind == T_IDENT) set_name(l, t);
            else l->ival = t->ival;
            adv();
            nv_push(v, l);
            continue;
        }
        s = parse_statement();
        if (s) nv_push(v, s);
        if (pending_next) return v;
        expect_eos();
    }
}

/* the statements of a single-line If, up to the end of the line or Else */
static NodeVec *parse_line_stmts(void) {
    NodeVec *v = nv_new();
    int save = in_lineif;
    in_lineif = 1;
    for (;;) {
        while (tk() == T_COLON) adv();
        if (tk() == T_NEWLINE || tk() == T_EOF || tk() == K_Else) break;
        nv_push(v, parse_statement());
        expect_eos();
    }
    in_lineif = save;
    return v;
}

/* IfStatement: BlockIfStatement or LineIfThenStatement */
static Node *parse_if(void) {
    Token *kw = adv();
    Node *n = mk(N_IF, kw);
    n->a = parse_expr();
    if (tk() == K_Then) {
        adv();
        if (tk() != T_NEWLINE && tk() != T_EOF) {
            n->flags |= F_LINEIF;
            n->list = parse_line_stmts();
            if (tk() == K_Else) {
                adv();
                n->flags |= F_HASELSE;
                n->list3 = parse_line_stmts();
            }
            return n;
        }
    } else if (!end_of_stmt()) perr("'Then' expected.");
    open_push(K_If, kw);
    n->list = parse_block();
    n->list2 = nv_new();
    for (;;) {
        if (tk() == K_ElseIf || (tk() == K_Else && tk1() == K_If)) {
            Token *e = adv();
            Node *ei = mk(N_ELSEIF, e);
            if (n->flags & F_HASELSE) perr_at(e, "'ElseIf' must be preceded by a matching 'If' or 'ElseIf'.");
            if (e->kind == K_Else) adv();
            ei->a = parse_expr();
            if (tk() == K_Then) adv();
            expect_eos();
            ei->list = parse_block();
            nv_push(n->list2, ei);
            continue;
        }
        if (tk() == K_Else) {
            if (n->flags & F_HASELSE) perr("'Else' must be preceded by a matching 'If' or 'ElseIf'.");
            adv();
            n->flags |= F_HASELSE;
            expect_eos();
            n->list3 = parse_block();
            continue;
        }
        break;
    }
    close_end(K_If);
    return n;
}

/* SelectStatement */
static Node *parse_select(void) {
    Token *kw = adv();
    Node *n = mk(N_SELECT, kw);
    if (tk() == K_Case) adv();
    n->a = parse_expr();
    expect_eos();
    open_push(K_Select, kw);
    n->list = nv_new();
    skip_terms();
    if (tk() != K_Case && !block_end())
        perr("Statements and labels are not valid between 'Select Case' and first 'Case'.");
    while (tk() == K_Case) {
        Token *c = adv();
        Node *cs = mk(N_CASE, c);
        if (n->list->n && (n->list->v[n->list->n - 1]->flags & F_ELSE))
            perr_at(c, "'Case' cannot follow a 'Case Else' in the same 'Select' statement.");
        if (tk() == K_Else) {
            adv();
            cs->flags |= F_ELSE;
        } else {
            cs->list = nv_new();
            for (;;) {
                Token *ct = T(0);
                Node *cl;
                if (tk() == K_Is || is_relop(tk())) {
                    if (tk() == K_Is) adv();
                    if (!is_relop(tk())) perr("Relational operator expected.");
                    cl = mk(N_CASEIS, ct);
                    cl->op = adv()->kind;
                    cl->a = parse_expr();
                } else {
                    Node *e = parse_expr();
                    if (tk() == K_To) {
                        adv();
                        cl = mk(N_CASERANGE, ct);
                        cl->a = e;
                        cl->b = parse_expr();
                    } else {
                        cl = mk(N_CASEVAL, ct);
                        cl->a = e;
                    }
                }
                nv_push(cs->list, cl);
                if (tk() != P_COMMA) break;
                adv();
            }
        }
        expect_eos();
        cs->list2 = parse_block();
        nv_push(n->list, cs);
    }
    close_end(K_Select);
    return n;
}

static Node *parse_while(void) {
    Token *kw = adv();
    Node *n = mk(N_WHILE, kw);
    n->a = parse_expr();
    expect_eos();
    open_push(K_While, kw);
    n->list = parse_block();
    close_end(K_While);
    return n;
}

/* DoLoopStatement */
static Node *parse_do(void) {
    Token *kw = adv();
    Node *n = mk(N_DO, kw);
    if (tk() == K_While || at_w(w_until)) {
        n->flags |= F_TOP | (tk() == K_While ? F_WHILE : F_UNTIL);
        adv();
        n->a = parse_expr();
    }
    expect_eos();
    open_push(K_Do, kw);
    n->list = parse_block();
    if (tk() != K_Loop) closer_error();
    adv();
    open_pop();
    if (tk() == K_While || at_w(w_until)) {
        if (n->flags & F_TOP) perr("'Loop' cannot have a condition if matching 'Do' has one.");
        n->flags |= F_BOTTOM | (tk() == K_While ? F_WHILE : F_UNTIL);
        adv();
        n->a = parse_expr();
    }
    return n;
}

/* LoopControlVariable: Identifier [IdentifierModifiers As TypeName] | Expression */
static Node *parse_loop_var(void) {
    if (tk() == T_IDENT && (tk1() == K_As || (tk1() == P_QUESTION && T(2)->kind == K_As))) {
        Token *id = adv();
        Node *v = mk(N_LOOPVAR, id);
        set_name(v, id);
        if (tk() == P_QUESTION) {
            adv();
            v->flags |= F_NULLABLE;
        }
        adv();
        v->a = parse_type(1);
        return v;
    }
    return parse_term();
}

/* the body of a For or For Each loop and its Next */
static void finish_for(Node *n, Token *kw) {
    open_push(K_For, kw);
    n->list = parse_block();
    if (pending_next > 0) {
        pending_next--;
        open_pop();
        return;
    }
    if (tk() != K_Next) closer_error();
    adv();
    if (!end_of_stmt()) {
        int i;
        n->list2 = nv_new();
        for (;;) {
            nv_push(n->list2, parse_term());
            if (tk() != P_COMMA) break;
            adv();
        }
        pending_next = n->list2->n - 1;
        for (i = 1; i <= pending_next; i++)
            if (nopen - 1 - i < 0 || opens[nopen - 1 - i].kind != K_For)
                perr("'Next' must be preceded by a matching 'For'.");
    }
    open_pop();
}

/* ForStatement and ForEachStatement */
static Node *parse_for(void) {
    Token *kw = adv();
    Node *n;
    if (tk() == K_Each) {
        adv();
        n = mk(N_FOREACH, kw);
        n->a = parse_loop_var();
        expect(K_In, "'In' expected.");
        n->b = parse_expr();
    } else {
        n = mk(N_FOR, kw);
        n->a = parse_loop_var();
        expect(P_EQ, "'=' expected.");
        n->b = parse_expr();
        expect(K_To, "'To' expected.");
        n->c = parse_expr();
        if (tk() == K_Step) {
            adv();
            n->d = parse_expr();
        }
    }
    expect_eos();
    finish_for(n, kw);
    return n;
}

/* TryStatement */
static Node *parse_try(void) {
    Token *kw = adv();
    Node *n = mk(N_TRY, kw);
    expect_eos();
    open_push(K_Try, kw);
    n->list = parse_block();
    n->list2 = nv_new();
    for (;;) {
        if (tk() == K_Catch) {
            Token *c = adv();
            Node *cn = mk(N_CATCH, c);
            if (n->flags & F_HASFINALLY) perr_at(c, "'Catch' cannot appear after 'Finally' within a 'Try' statement.");
            if (tk() == T_IDENT) {
                set_name(cn, adv());
                if (tk() == K_As) {
                    adv();
                    cn->a = parse_nonarray_type();
                }
            }
            if (tk() == K_When) {
                adv();
                cn->b = parse_expr();
            }
            expect_eos();
            cn->list = parse_block();
            nv_push(n->list2, cn);
            continue;
        }
        if (tk() == K_Finally) {
            if (n->flags & F_HASFINALLY) perr("'Finally' can only appear once in a 'Try' statement.");
            adv();
            n->flags |= F_HASFINALLY;
            expect_eos();
            n->list3 = parse_block();
            continue;
        }
        break;
    }
    close_end(K_Try);
    return n;
}

/* With, SyncLock: keyword Expression Block End keyword */
static Node *parse_expr_block(int kind) {
    Token *kw = adv();
    Node *n = mk(kind, kw);
    n->a = parse_expr();
    expect_eos();
    open_push(kw->kind, kw);
    n->list = parse_block();
    close_end(kw->kind);
    return n;
}

static Node *parse_using(void) {
    Token *kw = adv();
    Node *n = mk(N_USING, kw);
    if (tk() == T_IDENT && (tk1() == K_As || tk1() == P_EQ)) n->list = parse_var_declarators();
    else n->a = parse_expr();
    expect_eos();
    open_push(K_Using, kw);
    n->list2 = parse_block();
    close_end(K_Using);
    return n;
}

/* LocalDeclarationStatement: Dim / Static / Const declarators */
static Node *parse_local(void) {
    Token *kw = T(0);
    Node *n = mk(N_LOCAL, kw);
    for (;;) {
        int b = tk() == K_Dim ? M_DIM : tk() == K_Static ? M_STATIC : tk() == K_Const ? M_CONST : 0;
        if (!b) break;
        if (n->mods & b) perr("Duplicate specifier.");
        n->mods |= b;
        adv();
    }
    n->list = parse_var_declarators();
    return n;
}

/* VariableDeclarators */
static NodeVec *parse_var_declarators(void) {
    NodeVec *v = nv_new();
    for (;;) {
        Node *d = mk(N_VARDECL, T(0));
        d->list = nv_new();
        for (;;) {
            Token *id = ident();
            Node *vi = mk(N_VARID, id);
            set_name(vi, id);
            parse_id_mods(vi, 1);
            nv_push(d->list, vi);
            if (tk() == P_COMMA && tk1() == T_IDENT) {
                adv();
                continue;
            }
            break;
        }
        if (tk() == K_As) {
            adv();
            if (tk() == K_New) {
                d->flags |= F_ASNEW;
                d->b = parse_new();
            } else d->a = parse_type(1);
        }
        if (tk() == P_EQ) {
            if (d->flags & F_ASNEW) perr("End of statement expected.");
            adv();
            d->b = parse_expr();
        }
        nv_push(v, d);
        if (tk() == P_COMMA) {
            adv();
            continue;
        }
        break;
    }
    return v;
}

/* ReDim [Preserve] target(bounds) {, target(bounds)} */
static Node *parse_redim(void) {
    Token *kw = adv();
    Node *n = mk(N_REDIM, kw);
    if (at_w(w_preserve)) {
        adv();
        n->flags |= F_PRESERVE;
    }
    n->list = nv_new();
    for (;;) {
        Token *t = T(0);
        Node *e, *c;
        allow_to = 1;
        e = parse_term();
        allow_to = 0;
        if (e->kind != N_INVOKE)
            perr_at(t, "'ReDim' statements require a parenthesized list of the new bounds of each dimension of the array.");
        c = mk(N_REDIMCLAUSE, t);
        c->a = e->a;
        c->list = args_to_bounds(e->list, t);
        nv_push(n->list, c);
        if (tk() != P_COMMA) break;
        adv();
    }
    return n;
}

static int is_invocable(Node *e) {
    return e->kind == N_INVOKE || e->kind == N_NAME || e->kind == N_MEMBER || e->kind == N_DICT;
}

/* AssignmentStatement or InvocationStatement */
static Node *parse_expr_statement(void) {
    Token *t = T(0);
    Node *lhs = parse_term(), *n;
    int k = tk();
    if (k == P_EQ || is_compound(k)) {
        n = mk(N_ASSIGN, T(0));
        n->line = lhs->line;
        n->op = k;
        adv();
        n->a = lhs;
        n->b = parse_expr();
        return n;
    }
    if (!is_invocable(lhs)) perr_at(t, "Syntax error.");
    if (!end_of_stmt()) perr("End of statement expected.");
    n = mk(N_CALLSTMT, t);
    n->a = lhs;
    return n;
}

/* a label name after GoTo */
static Node *label_ref(Node *n) {
    Token *t = T(0);
    if (t->kind == T_IDENT) set_name(n, t);
    else if (t->kind == T_INT) n->ival = t->ival;
    else perr("Label expected.");
    adv();
    return n;
}

static int is_decl_kw(int k) {
    switch (k) {
    case K_Sub: case K_Function: case K_Property: case K_Event: case K_Operator: case K_Class:
    case K_Structure: case K_Module: case K_Interface: case K_Enum: case K_Namespace:
    case K_Delegate: case K_Declare: case K_Public: case K_Private: case K_Protected:
    case K_Friend: case K_Shared: case K_Overridable: case K_Overrides: case K_MustOverride:
    case K_NotOverridable: case K_MustInherit: case K_NotInheritable: case K_Overloads:
    case K_Shadows: case K_Partial: case K_ReadOnly: case K_WriteOnly: case K_WithEvents:
    case K_Implements: case K_Inherits: case K_Option: case K_Imports:
        return 1;
    }
    return 0;
}

/* Statement */
static Node *parse_statement(void) {
    Token *t = T(0);
    Node *n;
    switch (t->kind) {
    case K_Dim: case K_Static: case K_Const:
        return parse_local();
    case K_If: return parse_if();
    case K_Select: return parse_select();
    case K_While: return parse_while();
    case K_Do: return parse_do();
    case K_For: return parse_for();
    case K_Try: return parse_try();
    case K_With: return parse_expr_block(N_WITH);
    case K_SyncLock: return parse_expr_block(N_SYNCLOCK);
    case K_Using: return parse_using();
    case K_Throw: case K_Return:
        adv();
        n = mk(t->kind == K_Throw ? N_THROW : N_RETURN, t);
        if (!end_of_stmt()) n->a = parse_expr();
        return n;
    case K_GoTo:
        adv();
        return label_ref(mk(N_GOTO, t));
    case K_Exit:
        adv();
        n = mk(N_EXIT, t);
        switch (tk()) {
        case K_Do: case K_For: case K_While: case K_Select: case K_Sub: case K_Function:
        case K_Property: case K_Try:
            n->op = adv()->kind;
            return n;
        }
        perr("'Exit' must be followed by 'Sub', 'Function', 'Property', 'Do', 'For', 'While', 'Select', or 'Try'.");
    case K_Continue:
        adv();
        n = mk(N_CONTINUE, t);
        if (tk() == K_Do || tk() == K_For || tk() == K_While) {
            n->op = adv()->kind;
            return n;
        }
        perr("'Continue' must be followed by 'Do', 'For' or 'While'.");
    case K_Stop:
        adv();
        return mk(N_STOP, t);
    case K_End:
        adv();
        return mk(N_END, t);
    case K_ReDim:
        return parse_redim();
    case K_Erase:
        adv();
        n = mk(N_ERASE, t);
        n->list = nv_new();
        for (;;) {
            nv_push(n->list, parse_term());
            if (tk() != P_COMMA) break;
            adv();
        }
        return n;
    case K_RaiseEvent: {
        Token *id;
        adv();
        id = T(0);
        if (!is_ident_or_kw(id->kind)) perr("Identifier expected.");
        adv();
        n = mk(N_RAISEEVENT, t);
        set_name(n, id);
        if (tk() == P_LPAREN) {
            adv();
            n->list = parse_args();
        }
        return n;
    }
    case K_AddHandler: case K_RemoveHandler:
        adv();
        n = mk(t->kind == K_AddHandler ? N_ADDHANDLER : N_REMOVEHANDLER, t);
        n->a = parse_expr();
        expect(P_COMMA, "',' expected.");
        n->b = parse_expr();
        return n;
    case K_Call:
        adv();
        n = mk(N_CALLSTMT, t);
        n->flags |= F_CALLKW;
        n->a = parse_term();
        return n;
    case K_Error:
        perr("'Error' statements are not supported.");
    case K_On:
        if (tk1() == K_Error) perr("'On Error' statements are not supported.");
        break;
    case K_Resume:
        perr("'Resume' statements are not supported.");
    case K_GoSub:
        perr("'GoSub' statements are no longer supported.");
    case K_Let: case K_Set:
        perr("'Let' and 'Set' assignment statements are no longer supported.");
    }
    if (is_decl_kw(t->kind) && !((t->kind == K_Sub || t->kind == K_Function) && tk1() == P_LPAREN))
        perr("Statement cannot appear within a method body.");
    if ((is_w(t, w_await) || is_w(t, w_yield)) &&
        (tk1() == T_IDENT || tk1() == K_Me || tk1() == K_New || tk1() == T_INT || tk1() == T_STRING))
        perr("'%s' is not supported.", is_w(t, w_await) ? "Await" : "Yield");
    return parse_expr_statement();
}

/* ------------------------------------------------------------ declarations */

enum { CTX_FILE, CTX_NAMESPACE, CTX_MODULE, CTX_CLASS, CTX_STRUCT, CTX_INTERFACE };

static int mod_bit(Token *t) {
    switch (t->kind) {
    case K_Public: return M_PUBLIC;
    case K_Private: return M_PRIVATE;
    case K_Protected: return M_PROTECTED;
    case K_Friend: return M_FRIEND;
    case K_Shadows: return M_SHADOWS;
    case K_Shared: return M_SHARED;
    case K_Overridable: return M_OVERRIDABLE;
    case K_NotOverridable: return M_NOTOVERRIDABLE;
    case K_MustOverride: return M_MUSTOVERRIDE;
    case K_Overrides: return M_OVERRIDES;
    case K_Overloads: return M_OVERLOADS;
    case K_Partial: return M_PARTIAL;
    case K_MustInherit: return M_MUSTINHERIT;
    case K_NotInheritable: return M_NOTINHERITABLE;
    case K_ReadOnly: return M_READONLY;
    case K_WriteOnly: return M_WRITEONLY;
    case K_WithEvents: return M_WITHEVENTS;
    case K_Dim: return M_DIM;
    case K_Static: return M_STATIC;
    case K_Default: return M_DEFAULT;
    case K_Widening: return M_WIDENING;
    case K_Narrowing: return M_NARROWING;
    }
    if (is_w(t, w_async) || is_w(t, w_iterator)) {
        Token *n = t + 1;
        if (n->kind == K_Sub || n->kind == K_Function || n->kind == K_Property || mod_bit(n))
            return is_w(t, w_async) ? M_ASYNC : M_ITERATOR;
    }
    return 0;
}

static int parse_mods(void) {
    int m = 0, b;
    while ((b = mod_bit(T(0))) != 0) {
        if (m & b) perr("Duplicate specifier.");
        m |= b;
        adv();
    }
    if (m & M_ASYNC) perr("'Async' methods are not supported.");
    if (m & M_ITERATOR) perr("'Iterator' methods are not supported.");
    return m;
}

/* the name after Sub/Function/Property/Event/Class... */
static Token *decl_name(void) {
    if (tk() != T_IDENT) perr("Identifier expected.");
    return adv();
}

/* SubSignature / FunctionSignature after the name */
static void parse_signature(Node *n, int is_function) {
    if (tk() == P_LPAREN && tk1() == K_Of) {
        adv();
        adv();
        n->list = parse_type_params();
    }
    if (tk() == P_LPAREN) {
        adv();
        n->flags |= F_PARENS;
        n->list2 = parse_params();
    } else n->list2 = nv_new();
    if (tk() == K_As) {
        if (!is_function) perr("End of statement expected.");
        adv();
        parse_attributes();
        n->a = parse_type(1);
    }
}

/* SubDeclaration, FunctionDeclaration and their MustOverride and interface forms */
static Node *parse_method(int mods, int ctx) {
    Token *kw = adv();
    Token *id = decl_name();
    Node *n = mk(N_METHOD, kw);
    n->op = kw->kind;
    n->mods = mods;
    set_name(n, id);
    parse_signature(n, kw->kind == K_Function);
    if (tk() == K_Handles || tk() == K_Implements) n->b = parse_handles();
    expect_eos();
    if ((mods & M_MUSTOVERRIDE) || ctx == CTX_INTERFACE) return n;
    open_push(kw->kind, kw);
    n->list3 = parse_block();
    close_end(kw->kind);
    return n;
}

/* ConstructorMemberDeclaration: Sub New [(params)] Block End Sub */
static Node *parse_ctor(int mods) {
    Token *kw = adv();
    Node *n = mk(N_CTOR, kw);
    Token *nw = adv();
    n->mods = mods;
    set_name(n, nw);
    if (tk() == P_LPAREN) {
        adv();
        n->list2 = parse_params();
    } else n->list2 = nv_new();
    expect_eos();
    open_push(K_Sub, kw);
    n->list3 = parse_block();
    close_end(K_Sub);
    return n;
}

/* accessor blocks of a property or custom event */
static Node *parse_accessor(int allow_params) {
    Token *a = adv();
    Node *acc = mk(N_ACCESSOR, a);
    acc->op = a->kind;
    if (tk() == P_LPAREN) {
        if (!allow_params) perr("End of statement expected.");
        adv();
        acc->list2 = parse_params();
    }
    expect_eos();
    open_push(a->kind, a);
    acc->list3 = parse_block();
    close_end(a->kind);
    expect_eos();
    return acc;
}

/* PropertyMemberDeclaration: regular, MustOverride, interface and auto-implemented */
static Node *parse_property(int mods, int ctx) {
    Token *kw = adv();
    Token *id = decl_name();
    Node *n = mk(N_PROPERTY, kw);
    int j;
    n->mods = mods;
    set_name(n, id);
    if (tk() == P_LPAREN) {
        adv();
        n->list2 = parse_params();
    }
    if (tk() == K_As) {
        adv();
        parse_attributes();
        if (tk() == K_New) {
            n->flags |= F_ASNEW;
            n->c = parse_new();
        } else n->a = parse_type(1);
    }
    if (tk() == P_EQ) {
        if (n->flags & F_ASNEW) perr("End of statement expected.");
        adv();
        n->c = parse_expr();
    }
    if (tk() == K_Implements) n->b = parse_handles();
    expect_eos();
    if (ctx == CTX_INTERFACE || (mods & M_MUSTOVERRIDE)) return n;
    /* a regular property if the next line starts with [access] Get or Set */
    j = 0;
    while (T(j)->kind == T_NEWLINE || T(j)->kind == T_COLON) j++;
    while (T(j)->kind == K_Public || T(j)->kind == K_Private || T(j)->kind == K_Protected ||
           T(j)->kind == K_Friend)
        j++;
    if (T(j)->kind != K_Get && T(j)->kind != K_Set) {
        n->flags |= F_AUTO;
        return n;
    }
    if (n->c) perr_at(kw, "Initializers are valid only for auto-implemented properties.");
    open_push(K_Property, kw);
    n->list3 = nv_new();
    for (;;) {
        int am;
        skip_terms();
        parse_attributes();
        am = parse_mods();
        if (tk() == K_Get || tk() == K_Set) {
            Node *acc = parse_accessor(tk() == K_Set);
            acc->mods = am;
            nv_push(n->list3, acc);
            continue;
        }
        if (am) perr("'Get' or 'Set' expected.");
        break;
    }
    close_end(K_Property);
    return n;
}

/* EventMemberDeclaration: regular or Custom */
static Node *parse_event(int mods, int custom) {
    Token *kw = adv();
    Token *id = decl_name();
    Node *n = mk(N_EVENT, kw);
    n->mods = mods;
    set_name(n, id);
    if (custom) n->flags |= F_CUSTOM;
    if (tk() == P_LPAREN) {
        adv();
        n->list2 = parse_params();
    } else if (tk() == K_As) {
        adv();
        n->a = parse_nonarray_type();
    }
    if (custom && !n->a) perr("'As' expected.");
    if (tk() == K_Implements) n->b = parse_handles();
    expect_eos();
    if (!custom) return n;
    open_push(K_Event, kw);
    n->list3 = nv_new();
    for (;;) {
        skip_terms();
        parse_attributes();
        if (tk() == K_AddHandler || tk() == K_RemoveHandler || tk() == K_RaiseEvent) {
            if (tk1() != P_LPAREN) perr_at(T(1), "'(' expected.");
            nv_push(n->list3, parse_accessor(1));
            continue;
        }
        break;
    }
    close_end(K_Event);
    return n;
}

/* OperatorDeclaration */
static Node *parse_operator(int mods) {
    Token *kw = adv();
    Token *op = T(0);
    Node *n = mk(N_OPERATOR, kw);
    n->mods = mods;
    switch (op->kind) {
    case P_PLUS: case P_MINUS: case P_STAR: case P_SLASH: case P_BACKSLASH: case P_AMP:
    case K_Like: case K_Mod: case K_And: case K_Or: case K_Xor: case P_CARET: case P_SHL:
    case P_SHR: case P_EQ: case P_NE: case P_GT: case P_LT: case P_GE: case P_LE: case K_Not:
    case K_CType:
        n->op = op->kind;
        break;
    default:
        if (is_w(op, w_istrue) || is_w(op, w_isfalse)) set_name(n, op);
        else perr("Operator expected.");
    }
    adv();
    expect(P_LPAREN, "'(' expected.");
    n->list2 = parse_params();
    if (tk() == K_As) {
        adv();
        parse_attributes();
        n->a = parse_type(1);
    }
    expect_eos();
    open_push(K_Operator, kw);
    n->list3 = parse_block();
    close_end(K_Operator);
    return n;
}

/* DelegateDeclaration */
static Node *parse_delegate(int mods) {
    Token *kw = adv();
    Token *k = T(0), *id;
    Node *n = mk(N_DELEGATE, kw);
    n->mods = mods;
    if (k->kind != K_Sub && k->kind != K_Function) perr("'Sub' or 'Function' expected.");
    adv();
    n->op = k->kind;
    id = decl_name();
    set_name(n, id);
    parse_signature(n, k->kind == K_Function);
    return n;
}

/* EnumDeclaration */
static Node *parse_enum(int mods) {
    Token *kw = adv();
    Token *id = decl_name();
    Node *n = mk(N_ENUM, kw);
    n->mods = mods;
    set_name(n, id);
    if (tk() == K_As) {
        adv();
        n->a = parse_nonarray_type();
    }
    expect_eos();
    open_push(K_Enum, kw);
    n->list = nv_new();
    for (;;) {
        Token *m;
        Node *em;
        skip_terms();
        if (block_end()) break;
        parse_attributes();
        m = ident();
        em = mk(N_ENUMMEMBER, m);
        set_name(em, m);
        if (tk() == P_EQ) {
            adv();
            em->a = parse_expr();
        }
        expect_eos();
        nv_push(n->list, em);
    }
    close_end(K_Enum);
    if (n->list->n == 0) perr_at(kw, "'Enum' must contain at least one member.");
    return n;
}

/* VariableMemberDeclaration */
static Node *parse_field(int mods) {
    Node *n = mk(N_FIELD, T(0));
    n->mods = mods;
    n->list = parse_var_declarators();
    return n;
}

/* ConstantMemberDeclaration */
static Node *parse_const_member(int mods) {
    Node *n = mk(N_CONSTDECL, adv());
    n->mods = mods;
    n->list = parse_var_declarators();
    return n;
}

/* the members up to End <kind> */
static void parse_members(NodeVec *v, int ctx) {
    for (;;) {
        Node *m;
        skip_terms();
        if (block_end()) return;
        m = parse_member(ctx);
        if (m) nv_push(v, m);
        expect_eos();
    }
}

/* ClassDeclaration, StructureDeclaration, ModuleDeclaration, InterfaceDeclaration */
static Node *parse_type_decl(int nk, int mods) {
    Token *kw = adv();
    Token *id = decl_name();
    Node *n = mk(nk, kw);
    int ctx = nk == N_CLASS ? CTX_CLASS : nk == N_STRUCTURE ? CTX_STRUCT :
              nk == N_MODULE ? CTX_MODULE : CTX_INTERFACE;
    const char *what = tok_spelling_of(kw->kind);
    n->mods = mods;
    set_name(n, id);
    if (nk != N_MODULE && tk() == P_LPAREN && tk1() == K_Of) {
        adv();
        adv();
        n->list = parse_type_params();
    }
    expect_eos();
    open_push(kw->kind, kw);
    n->list2 = nv_new();
    n->list3 = nv_new();
    for (;;) {
        skip_terms();
        if (tk() == K_Inherits) {
            Token *t = adv();
            if (nk == N_CLASS) {
                if (n->a || n->list2->n)
                    perr_at(t, "'Inherits' can appear only once within a 'Class' statement and can only be followed by 'Implements'.");
                n->a = parse_nonarray_type();
            } else if (nk == N_INTERFACE) {
                for (;;) {
                    nv_push(n->list2, parse_nonarray_type());
                    if (tk() != P_COMMA) break;
                    adv();
                }
            } else perr_at(t, "'Inherits' statements are not valid in a '%s'.", what);
            expect_eos();
            continue;
        }
        if (tk() == K_Implements) {
            Token *t = adv();
            if (nk != N_CLASS && nk != N_STRUCTURE)
                perr_at(t, "'Implements' statements are not valid in a '%s'.", what);
            for (;;) {
                nv_push(n->list2, parse_nonarray_type());
                if (tk() != P_COMMA) break;
                adv();
            }
            expect_eos();
            continue;
        }
        break;
    }
    for (;;) {
        parse_members(n->list3, ctx);
        if (tk() == K_Inherits && nk == N_CLASS)
            perr("'Inherits' statements must precede all declarations in a class.");
        if (tk() == K_Implements && (nk == N_CLASS || nk == N_STRUCTURE))
            perr("'Implements' statements must follow any 'Inherits' statement and precede all declarations in a class.");
        break;
    }
    close_end(kw->kind);
    return n;
}

static Node *parse_namespace(void) {
    Token *kw = adv();
    Node *n = mk(N_NAMESPACE, kw);
    n->a = parse_nonarray_type();
    expect_eos();
    open_push(K_Namespace, kw);
    n->list = nv_new();
    parse_members(n->list, CTX_NAMESPACE);
    close_end(K_Namespace);
    return n;
}

/* a member declaration (NamespaceMemberDeclaration, ClassMemberDeclaration,
   ...); NULL for an attribute statement */
static Node *parse_member(int ctx) {
    Token *t;
    int mods, top = ctx == CTX_FILE || ctx == CTX_NAMESPACE;
    parse_attributes();
    if (end_of_stmt()) return 0;
    mods = parse_mods();
    t = T(0);
    switch (t->kind) {
    case K_Namespace:
        if (mods || !top) perr("'Namespace' statements can occur only at file or namespace level.");
        return parse_namespace();
    case K_Class: return parse_type_decl(N_CLASS, mods);
    case K_Structure: return parse_type_decl(N_STRUCTURE, mods);
    case K_Interface: return parse_type_decl(N_INTERFACE, mods);
    case K_Module:
        if (!top) perr("'Module' statements can occur only at file or namespace level.");
        return parse_type_decl(N_MODULE, mods);
    case K_Enum: return parse_enum(mods);
    case K_Delegate: return parse_delegate(mods);
    case K_Option: perr("'Option' statements must precede any declarations or 'Imports' statements.");
    case K_Imports: perr("'Imports' statements must precede any declarations.");
    case K_Inherits:
        perr("'Inherits' statements must precede all declarations in a class.");
    case K_Implements:
        perr("'Implements' statements must follow any 'Inherits' statement and precede all declarations in a class.");
    }
    if (top) {
        if (mods || t->kind == K_Sub || t->kind == K_Function || t->kind == K_Property ||
            t->kind == K_Const || t->kind == K_Event || t->kind == K_Operator)
            perr("Statement is not valid in a namespace.");
        perr("Declaration expected.");
    }
    if (ctx == CTX_INTERFACE && (t->kind == K_Operator || t->kind == K_Const ||
                                 (is_w(t, w_custom) && tk1() == K_Event)))
        perr("Statement is not valid in an interface.");
    if (ctx == CTX_MODULE && t->kind == K_Operator) perr("Operators cannot be declared in modules.");
    switch (t->kind) {
    case K_Sub:
        if (tk1() == K_New) {
            if (ctx == CTX_INTERFACE) perr("'Sub New' cannot be declared in an interface.");
            return parse_ctor(mods);
        }
        return parse_method(mods, ctx);
    case K_Function: return parse_method(mods, ctx);
    case K_Property: return parse_property(mods, ctx);
    case K_Event: return parse_event(mods, 0);
    case K_Operator: return parse_operator(mods);
    case K_Const: return parse_const_member(mods);
    case K_Declare: perr("'Declare' statements are not supported.");
    }
    if (is_w(t, w_custom) && tk1() == K_Event) {
        adv();
        return parse_event(mods, 1);
    }
    if (t->kind == T_IDENT && mods && ctx != CTX_INTERFACE) return parse_field(mods);
    perr("Declaration expected.");
    return 0;
}

/* ------------------------------------------------------------ files */

/* OptionStatement */
static Node *parse_option(void) {
    Token *kw = adv(), *w = T(0);
    Node *n = mk(N_OPTION, kw);
    if (is_w(w, w_explicit) || is_w(w, w_strict) || is_w(w, w_infer)) {
        adv();
        n->name = w->name;
        n->text = "On";
        if (tk() == K_On) adv();
        else if (at_w(w_off)) {
            adv();
            n->text = "Off";
        }
    } else if (is_w(w, w_compare)) {
        adv();
        n->name = w->name;
        if (at_w(w_binary)) n->text = "Binary";
        else if (at_w(w_text)) n->text = "Text";
        else perr("'Binary' or 'Text' expected.");
        adv();
    } else perr("'Explicit', 'Strict', 'Compare' or 'Infer' expected.");
    return n;
}

/* ImportsStatement: its clauses are added to v */
static void parse_imports(NodeVec *v) {
    adv();
    for (;;) {
        Token *t = T(0);
        Node *im = mk(N_IMPORT, t);
        if (tk() == P_LT) perr("XML namespace imports are not supported.");
        if (tk() == T_IDENT && tk1() == P_EQ) {
            adv();
            adv();
            set_name(im, t);
        }
        im->a = parse_nonarray_type();
        nv_push(v, im);
        if (tk() != P_COMMA) break;
        adv();
    }
}

/* Start: OptionStatement* ImportsStatement* AttributesStatement* NamespaceMemberDeclaration* */
Node *parse_file(TokVec *tv) {
    Node *f;
    ptv = tv;
    pi = 0;
    nopen = 0;
    pending_next = 0;
    in_lineif = 0;
    f = mk(N_FILE, T(0));
    f->list = nv_new();
    f->list2 = nv_new();
    skip_terms();
    while (tk() == K_Option) {
        nv_push(f->list, parse_option());
        expect_eos();
        skip_terms();
    }
    while (tk() == K_Imports) {
        parse_imports(f->list);
        expect_eos();
        skip_terms();
    }
    for (;;) {
        Node *m;
        skip_terms();
        if (tk() == T_EOF) break;
        if (block_end()) closer_error();
        m = parse_member(CTX_FILE);
        if (m) nv_push(f->list2, m);
        expect_eos();
    }
    return f;
}
