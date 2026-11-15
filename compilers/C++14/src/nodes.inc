/*
 * nodes.inc - the AST node kinds.  N(KIND) -> N_KIND.  The fields that a
 * kind uses are listed in parser.c where the node is built; the dump
 * (--ast) prints every non-empty field generically.
 */
/* names */
N(ID)            /* an id-expression or type name: a = list of N_PART, flags ID_xxx */
N(PART)          /* one component: name or op, b = template arguments (N_TARG list) */
N(TARG)          /* a template argument: a = N_TYPEID or an expression */
/* types and declarations */
N(TYPE)          /* decl-specifier-seq: flags DS_xxx, val BT_xxx, a = type name / class / enum / decltype */
N(TYPEID)        /* a type-id: a = N_TYPE, b = abstract declarator */
N(PARAM)         /* a = N_TYPE, b = declarator, c = default argument */
N(DECL)          /* a = N_TYPE, b = list of N_INITDECL */
N(INITDECL)      /* a = declarator, b = initializer, c = bit-field width */
N(BINDING)       /* auto [a, b] = e: a = N_TYPE, b = initializer, c = N_ID list of the names, op = 0 / P_AMP / P_ANDAND */
N(FUNCDEF)       /* a = N_TYPE, b = declarator, c = constructor initializers, d = body */
N(CTORINIT)      /* a = member / base id, b = arguments, flags = braces */
N(D_NAME)        /* a = N_ID or 0 */
N(D_PTR)         /* a = inner, flags = cv */
N(D_REF)         /* a = inner */
N(D_RREF)        /* a = inner */
N(D_MEMPTR)      /* a = inner, b = class N_ID, flags = cv */
N(D_ARRAY)       /* a = inner, b = size */
N(D_FUNC)        /* a = inner, b = N_PARAM list, c = trailing return N_TYPEID, d = noexcept expr, flags FQ_xxx */
N(D_PACK)        /* a = inner (a parameter pack) */
N(CLASS)         /* op = KW_class/struct/union, a = N_ID name or 0, b = N_BASE list, c = members, flags CL_xxx */
N(BASE)          /* a = N_ID, flags BASE_xxx, op = access */
N(ENUM)          /* flags EN_xxx, a = N_ID name, b = underlying N_TYPEID, c = enumerators */
N(ENUMERATOR)    /* name, a = value */
N(NAMESPACE)     /* name, a = declarations, flags NS_INLINE */
N(NSALIAS)       /* name, a = N_ID */
N(USING_DIR)     /* a = N_ID */
N(USING_DECL)    /* a = N_ID, flags = typename */
N(ALIAS)         /* name, a = N_TYPEID, b = template parameters if an alias template */
N(TEMPLATE)      /* a = N_TPARAM list, b = declaration; flags TP_SPEC for template<> */
N(INSTANTIATION) /* a = declaration (explicit instantiation) */
N(TPARAM)        /* name, op = KW_class / KW_typename / 0 (non-type), a = default / N_PARAM, b = template template params, flags TPF_PACK */
N(STATIC_ASSERT) /* a = condition, b = message */
N(ASM)           /* text */
N(LINKAGE)       /* text = language, a = declarations */
N(ACCESS)        /* op = KW_public / protected / private */
N(FRIEND)        /* a = declaration */
N(EMPTY_DECL)
N(ATTR)          /* an attribute-specifier: text */
/* statements */
N(BLOCK)         /* a = statements */
N(EXPR_STMT)     /* a = expression */
N(NULL_STMT)
N(DECL_STMT)     /* a = N_DECL */
N(IF)            /* a = condition, b = then, c = else, flags IF_CONSTEXPR, d = init */
N(SWITCH)        /* a = condition, b = body */
N(WHILE)         /* a = condition, b = body */
N(DO)            /* a = body, b = condition */
N(FOR)           /* a = init, b = condition, c = increment, d = body */
N(RANGEFOR)      /* a = N_DECL (the variable), b = range expression, d = body */
N(BREAK)
N(CONTINUE)
N(RETURN)        /* a = expression */
N(GOTO)          /* name */
N(LABEL)         /* name, a = statement */
N(CASE)          /* a = value, b = statement */
N(DEFAULT)       /* a = statement */
N(TRY)           /* a = block, b = N_CATCH list */
N(CATCH)         /* a = N_PARAM or 0 for ..., b = block */
/* expressions */
N(NUM)           /* text */
N(CHARLIT)       /* text */
N(STRLIT)        /* text; adjacent literals are one node with a = next literal */
N(BOOLLIT)       /* val */
N(NULLPTR)
N(THIS)
N(PAREN)         /* a */
N(UNARY)         /* op = P_xxx (prefix), a */
N(POSTFIX)       /* op = P_INC / P_DEC, a */
N(BINARY)        /* op = P_xxx, a, b */
N(ASSIGN)        /* op = P_ASSIGN or a compound assignment, a, b */
N(COND)          /* a ? b : c */
N(COMMA)         /* a, b */
N(CALL)          /* a = callee, b = arguments */
N(INDEX)         /* a[b] */
N(MEMBER)        /* a.name / a->name: op = P_DOT / P_ARROW, b = N_ID */
N(PTRMEM)        /* a.*b / a->*b: op = P_DOTSTAR / P_ARROWSTAR */
N(CAST)          /* (type) a: b = N_TYPEID, a = operand */
N(NAMEDCAST)     /* op = KW_static_cast ..., b = N_TYPEID, a = operand */
N(FUNCCAST)      /* T(args) / T{args}: b = N_TYPEID, a = arguments, flags = braces */
N(SIZEOF)        /* a = expression or b = N_TYPEID; flags = pack (sizeof...) */
N(ALIGNOF)       /* b = N_TYPEID */
N(NOEXCEPT_EXPR) /* a */
N(TYPETRAIT)     /* __is_class(T) and the like: name = the trait, a = first N_TYPEID, b = second N_TYPEID */
N(TYPEID_EXPR)   /* a = expression or b = N_TYPEID */
N(NEW)           /* a = placement, b = N_TYPEID, c = initializer, d = array size, flags = global / braces */
N(DELETE)        /* a = operand, flags = global / array */
N(THROW)         /* a = operand or 0 */
N(LAMBDA)        /* a = captures, b = N_PARAM list, c = return type, d = body, flags LF_xxx */
N(CAPTURE)       /* name, op = P_ASSIGN / P_AMP / 0, a = initializer */
N(INITLIST)      /* a = elements: a braced-init-list */
N(DESIGNATED)    /* unused */
N(PACKEXP)       /* a ... */
N(TYPE_EXPR)     /* a type used as an expression (template argument): b = N_TYPEID */
