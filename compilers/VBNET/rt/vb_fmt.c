/*
 * vb_fmt.c - .NET numeric format strings (included by vb_rt.c).
 *
 * ToString(format), String.Format("{0:N2}", ...) and Format(value, style)
 * for the standard formats D, X, F, N, E, G, P, C and R and the custom
 * formats (0 # . , % ; \ and quoted literals), with the invariant culture:
 * "." decimal point, "," group separator, "Infinity", "NaN", "¤" currency.
 * Numbers are rounded on their exact decimal digits, with midpoints away
 * from zero, as .NET Core 3.0 and later do.
 */

/* a number as decimal digits: value = 0.dg * 10^pt (dg without leading
   zeros; "" is zero) */
typedef struct {
    int neg;
    char dg[400];
    int nd;
    int pt;
} Num;

static void num_trim(Num *n) {
    int i = 0, k;
    while (i < n->nd && n->dg[i] == '0') i++;
    if (i) {
        for (k = i; k < n->nd; k++) n->dg[k - i] = n->dg[k];
        n->nd -= i;
        n->pt -= i;
    }
    while (n->nd > 0 && n->dg[n->nd - 1] == '0') n->nd--;
    if (n->nd == 0) n->pt = 0;
    n->dg[n->nd] = 0;
}

static void num_from_int(Num *n, long v, int is_unsigned) {
    char buf[32];
    if (is_unsigned) sprintf(buf, "%llu", (unsigned long long)v);
    else sprintf(buf, "%lld", (long long)v);
    n->neg = buf[0] == '-';
    strcpy(n->dg, buf + n->neg);
    n->nd = strlen(n->dg);
    n->pt = n->nd;
    num_trim(n);
}

static void num_from_double(Num *n, double d) {
    char buf[120], *p;
    int k = 0;
    n->neg = signbit(d) != 0;
    if (d == 0.0) {
        n->nd = 0;
        n->pt = 0;
        n->dg[0] = 0;
        return;
    }
    snprintf(buf, sizeof buf, "%.60e", fabs(d));
    for (p = buf; *p && *p != 'e'; p++) if (*p != '.') n->dg[k++] = *p;
    n->dg[k] = 0;
    n->nd = k;
    n->pt = atoi(p + 1) + 1;
    num_trim(n);
}

/* keeps the digits before position `keep` (counted from the start of dg),
   rounding the midpoint away from zero */
static void num_round_at(Num *n, int keep) {
    int i;
    if (keep >= n->nd) return;
    if (keep < 0) {
        n->nd = 0;
        n->pt = 0;
        n->dg[0] = 0;
        return;
    }
    if (n->dg[keep] >= '5') {
        n->nd = keep;
        for (i = keep - 1; i >= 0; i--) {
            if (n->dg[i] < '9') { n->dg[i]++; break; }
            n->dg[i] = '0';
        }
        if (i < 0) {
            /* 999 -> 1000 */
            for (i = n->nd; i > 0; i--) n->dg[i] = n->dg[i - 1];
            n->dg[0] = '1';
            n->nd++;
            n->pt++;
        }
    } else n->nd = keep;
    n->dg[n->nd] = 0;
    num_trim(n);
}

/* rounds to `dec` digits after the decimal point */
static void num_round_dec(Num *n, int dec) { num_round_at(n, n->pt + dec); }

/* the digit at position k of the integer/fraction layout (k = 0 is the
   first digit before the point, -1 the first after it, ...) */
static char digit_at(Num *n, int pos) {
    int i = n->pt - 1 - pos;
    return (i >= 0 && i < n->nd) ? n->dg[i] : '0';
}

typedef struct {
    char *s;
    int len, cap;
} Out;

static void out_c(Out *o, int c) {
    if (o->len + 2 > o->cap) {
        o->cap = o->cap ? o->cap * 2 : 64;
        o->s = realloc(o->s, o->cap);
    }
    o->s[o->len++] = (char)c;
    o->s[o->len] = 0;
}

static void out_s(Out *o, const char *s) { while (*s) out_c(o, *s++); }

/* the integer part with optional grouping, then dec decimals */
static void put_fixed(Out *o, Num *n, int dec, int group) {
    int ip = n->pt > 0 ? n->pt : 1, i;
    for (i = ip - 1; i >= 0; i--) {
        out_c(o, digit_at(n, i));
        if (group && i > 0 && i % 3 == 0) out_c(o, ',');
    }
    if (dec > 0) {
        out_c(o, '.');
        for (i = 1; i <= dec; i++) out_c(o, digit_at(n, -i));
    }
}

static void put_exp(Out *o, int e, int mindig, int ch) {
    char buf[16];
    out_c(o, ch);
    out_c(o, e < 0 ? '-' : '+');
    sprintf(buf, "%0*d", mindig, e < 0 ? -e : e);
    out_s(o, buf);
}

/* scientific: d.ddd (dec decimals, trailing zeros kept unless trim) */
static void put_sci(Out *o, Num *n, int dec, int trim, int mindig, int ch) {
    int e = n->nd ? n->pt - 1 : 0, i, last = dec;
    out_c(o, n->nd ? n->dg[0] : '0');
    if (trim) {
        last = n->nd - 1 < dec ? n->nd - 1 : dec;
        if (last < 0) last = 0;
    }
    if (last > 0) {
        out_c(o, '.');
        for (i = 1; i <= last; i++) out_c(o, i < n->nd ? n->dg[i] : '0');
    }
    put_exp(o, e, mindig, ch);
}

static long *out_str(Out *o) {
    long *s = str_from_c(o->s ? o->s : "");
    free(o->s);
    return s;
}

static int is_int_kind(long k) { return k >= K_SBYTE && k <= K_ULONG; }

static void format_error(void) {
    fail("FormatException", "Format specifier was invalid.");
}

/* ------------------------------------------------------------ custom formats */

static void custom_format(Out *o, Num *n0, const char *f, int fmtlen) {
    /* the sections: positive;negative;zero */
    const char *sec[3];
    int seclen[3], ns = 0, i, s;
    int intp = 0, first0 = -1, decp = 0, dec0 = 0, group = 0, pct = 0, seen_pt = 0;
    char istr[400], dstr[400];
    int il = 0, dl = 0, k, minint, ph;
    Num n = *n0;
    sec[0] = f;
    for (i = 0; i < fmtlen && ns < 2; i++) {
        if (f[i] == '\\') { i++; continue; }
        if (f[i] == '\'' || f[i] == '"') {
            int q = f[i];
            for (i++; i < fmtlen && f[i] != q; i++) ;
            continue;
        }
        if (f[i] == ';') {
            seclen[ns] = (int)(&f[i] - sec[ns]);
            sec[++ns] = &f[i + 1];
        }
    }
    seclen[ns] = (int)(f + fmtlen - sec[ns]);
    s = 0;
    if (n.neg && n.nd && ns >= 1 && seclen[1] > 0) s = 1;
    else if (n.nd == 0 && ns >= 2 && seclen[2] > 0) s = 2;
    f = sec[s];
    fmtlen = seclen[s];
    /* the placeholders */
    for (i = 0; i < fmtlen; i++) {
        int c = f[i];
        if (c == '\\') { i++; continue; }
        if (c == '\'' || c == '"') { for (i++; i < fmtlen && f[i] != c; i++) ; continue; }
        if (c == '0' || c == '#') {
            if (seen_pt) {
                decp++;
                if (c == '0') dec0 = decp;
            } else {
                if (c == '0' && first0 < 0) first0 = intp;
                intp++;
            }
        } else if (c == '.') seen_pt = 1;
        else if (c == ',' && !seen_pt && intp > 0) group = 1;
        else if (c == '%') pct++;
    }
    if (n.nd) n.pt += 2 * pct;
    num_round_dec(&n, decp);
    /* the integer digits, at least the placeholders from the first 0 */
    minint = first0 >= 0 ? intp - first0 : 0;
    for (k = (n.pt > minint ? n.pt : minint) - 1; k >= 0; k--) istr[il++] = digit_at(&n, k);
    /* the decimals, without trailing zeros in # places */
    for (k = 1; k <= decp; k++) dstr[dl++] = digit_at(&n, -k);
    while (dl > dec0 && dstr[dl - 1] == '0') dl--;
    if (s == 0 && n.neg && (n.nd || dl)) out_c(o, '-');
    ph = 0;
    seen_pt = 0;
    k = 0;
    for (i = 0; i < fmtlen; i++) {
        int c = f[i];
        if (c == '\\') {
            if (i + 1 < fmtlen) out_c(o, f[++i]);
            continue;
        }
        if (c == '\'' || c == '"') {
            for (i++; i < fmtlen && f[i] != c; i++) out_c(o, f[i]);
            continue;
        }
        if ((c == '0' || c == '#') && !seen_pt) {
            /* integer placeholder ph of intp gets the digits from istr index
               il - intp + ph (the first one also gets the extra digits) */
            int from = ph == 0 ? 0 : il - intp + ph, to = il - intp + ph, d;
            if (from < 0) from = 0;
            for (d = from; d <= to; d++) {
                if (d < 0) continue;
                out_c(o, istr[d]);
                if (group && il - 1 - d > 0 && (il - 1 - d) % 3 == 0) out_c(o, ',');
            }
            ph++;
            continue;
        }
        if (c == '.') {
            if (!seen_pt && dl > 0) out_c(o, '.');
            seen_pt = 1;
            continue;
        }
        if ((c == '0' || c == '#') && seen_pt) {
            if (k < dl) out_c(o, dstr[k]);
            k++;
            continue;
        }
        if (c == ',') continue;
        out_c(o, c);
    }
    if (intp == 0 && il > 0 && !seen_pt) {
        /* no placeholders at all: .NET prints the format as it is */
    }
}

/* ------------------------------------------------------------ standard formats */

/* the text of a value of a primitive kind with a .NET format string */
static long *format_num(long kind, long bits, long *fmt) {
    char f[200];
    int i, fl = 0, spec, prec = -1;
    Num n;
    Out o;
    double d = 0;
    int isint = is_int_kind(kind);
    memset(&o, 0, sizeof o);
    for (i = 0; fmt && i < fmt[0] && fl < 190; i++) f[fl++] = (char)fmt[1 + i];
    f[fl] = 0;
    if (kind == K_SINGLE || kind == K_DOUBLE) {
        memcpy(&d, &bits, sizeof d);
        if (isnan(d)) return str_from_c("NaN");
        if (isinf(d)) return str_from_c(d > 0 ? "Infinity" : "-Infinity");
    }
    if (fl == 0) {
        char buf[64];
        fmt_value(buf, kind, bits);
        return str_from_c(buf);
    }
    spec = f[0];
    if (((spec >= 'A' && spec <= 'Z') || (spec >= 'a' && spec <= 'z')) &&
        (fl == 1 || (fl <= 3 && f[1] >= '0' && f[1] <= '9' && (fl == 2 || (f[2] >= '0' && f[2] <= '9'))))) {
        if (fl > 1) prec = atoi(f + 1);
    } else spec = 0;
    if (isint) num_from_int(&n, bits, kind == K_BYTE || kind == K_USHORT || kind == K_UINTEGER || kind == K_ULONG);
    else num_from_double(&n, d);
    switch (spec) {
    case 'D': case 'd':
        if (!isint) format_error();
        if (n.neg) out_c(&o, '-');
        for (i = n.pt; i < prec; i++) out_c(&o, '0');
        put_fixed(&o, &n, 0, 0);
        return out_str(&o);
    case 'X': case 'x': {
        char buf[40];
        int w;
        unsigned long long u = (unsigned long long)bits;
        if (!isint) format_error();
        switch (kind) {
        case K_SBYTE: case K_BYTE: u &= 0xFF; break;
        case K_SHORT: case K_USHORT: u &= 0xFFFF; break;
        case K_INTEGER: case K_UINTEGER: u &= 0xFFFFFFFFULL; break;
        }
        w = prec < 0 ? 0 : prec;
        sprintf(buf, spec == 'X' ? "%0*llX" : "%0*llx", w, u);
        return str_from_c(buf);
    }
    case 'F': case 'f': case 'N': case 'n':
        if (prec < 0) prec = 2;
        num_round_dec(&n, prec);
        if (n.neg && (n.nd || !isint)) out_c(&o, '-');
        put_fixed(&o, &n, prec, spec == 'N' || spec == 'n');
        return out_str(&o);
    case 'P': case 'p':
        if (prec < 0) prec = 2;
        if (n.nd) n.pt += 2;
        num_round_dec(&n, prec);
        if (n.neg && (n.nd || !isint)) out_c(&o, '-');
        put_fixed(&o, &n, prec, 1);
        out_s(&o, " %");
        return out_str(&o);
    case 'C': case 'c':
        if (prec < 0) prec = 2;
        num_round_dec(&n, prec);
        if (n.neg && n.nd) out_c(&o, '(');
        out_s(&o, "\xA4");
        put_fixed(&o, &n, prec, 1);
        if (n.neg && n.nd) out_c(&o, ')');
        return out_str(&o);
    case 'E': case 'e':
        if (prec < 0) prec = 6;
        num_round_at(&n, prec + 1);
        if (n.neg && (n.nd || !isint)) out_c(&o, '-');
        put_sci(&o, &n, prec, 0, 3, spec);
        return out_str(&o);
    case 'G': case 'g': case 'R': case 'r':
        if (prec <= 0 || spec == 'R' || spec == 'r') {
            char buf[64];
            fmt_value(buf, kind, bits);
            return str_from_c(buf);
        }
        num_round_at(&n, prec);
        if (n.neg && (n.nd || !isint)) out_c(&o, '-');
        if (n.nd && (n.pt - 1 >= prec || n.pt - 1 < -4)) put_sci(&o, &n, prec - 1, 1, 2, spec == 'g' ? 'e' : 'E');
        else {
            int dec = n.nd - n.pt;
            put_fixed(&o, &n, dec > 0 ? dec : 0, 0);
        }
        return out_str(&o);
    }
    custom_format(&o, &n, f, fl);
    return out_str(&o);
}

/* x.ToString(format) */
long *vbrt_format_value(long kind, long bits, long *fmt) {
    if (kind == K_STRING) return (long *)bits;
    if (kind == K_CHAR || kind == K_BOOLEAN) return vbrt_to_str(kind, bits);
    return format_num(kind, bits, fmt);
}

/* a boxed value with a format (String.Format "{0:fmt}") */
static long *format_obj(long *o, long *fmt) {
    long t;
    if (!o) return 0;
    t = tag_of(o);
    if (t >= K_BOOLEAN && t <= K_CHAR) return vbrt_format_value(t, o[0], fmt);
    return vbrt_obj_str(o);
}

/* VB's Format(expression, style): the named formats, else a .NET one */
long *vbrt_vb_format(long *o, long *style) {
    static const char *names[] = { "general number", "G", "currency", "C", "fixed", "F", "standard", "N",
                                   "percent", "P", "scientific", "0.00E+00", 0 };
    int i, j;
    if (style) {
        for (i = 0; names[i]; i += 2) {
            int ok = (long)strlen(names[i]) == style[0];
            for (j = 0; ok && j < style[0]; j++) if (fold(style[1 + j]) != names[i][j]) ok = 0;
            if (ok) return format_obj(o, str_from_c(names[i + 1]));
        }
    }
    return format_obj(o, style);
}
