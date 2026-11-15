/*
 * vb_strings.c - String methods, the Microsoft.VisualBasic string and
 * conversion functions, Char tests, parsing, Like, Rnd and Math (included
 * by vb_rt.c).
 *
 * VB strings are 0-based in .NET methods (Substring, IndexOf) and 1-based
 * in the VB functions (Mid, InStr).  Nothing counts as "" for the VB
 * functions; the .NET instance methods of Nothing throw
 * NullReferenceException.
 */

static long *need(long *s) {
    if (!s) vbrt_throw(E_NULLREF);
    return s;
}

static void arg_error(const char *msg) { fail("ArgumentOutOfRangeException", msg); }

static long *sub(long *s, long start, long len) {
    long *r, i;
    if (start == 0 && len == slen(s) && s) return s;
    r = vbrt_str_new(len);
    for (i = 0; i < len; i++) r[1 + i] = s[1 + start + i];
    return r;
}

static long *empty_str(void) { return vbrt_str_new(0); }

/* ------------------------------------------------------------ characters */

static long upper_c(long c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    if (c >= 0xE0 && c <= 0xFE && c != 0xF7) return c - 32;
    if (c >= 0x100 && c <= 0x17F && (c & 1)) return c - 1;
    if (c >= 0x3B1 && c <= 0x3C9 && c != 0x3C2) return c - 32;
    if (c >= 0x430 && c <= 0x44F) return c - 32;
    if (c >= 0x450 && c <= 0x45F) return c - 80;
    return c;
}

static long lower_c(long c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    if (c >= 0xC0 && c <= 0xDE && c != 0xD7) return c + 32;
    if (c >= 0x100 && c <= 0x17F && !(c & 1)) return c + 1;
    if (c >= 0x391 && c <= 0x3A9 && c != 0x3A2) return c + 32;
    if (c >= 0x410 && c <= 0x42F) return c + 32;
    if (c >= 0x400 && c <= 0x40F) return c + 80;
    return c;
}

static int is_letter_c(long c) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return 1;
    if (c < 0xAA) return 0;
    if (c == 0xAA || c == 0xB5 || c == 0xBA) return 1;
    if (c >= 0xC0 && c <= 0x24F) return c != 0xD7 && c != 0xF7;
    if (c >= 0x370 && c <= 0x3FF) return c != 0x37E && c != 0x387;
    if (c >= 0x400 && c <= 0x52F) return c < 0x482 || c > 0x489;
    if (c >= 0x4E00 && c <= 0x9FFF) return 1;
    return c >= 0x5D0 && c <= 0x6FF;
}

static int is_space_c(long c) {
    return c == ' ' || (c >= 9 && c <= 13) || c == 0x85 || c == 0xA0 || c == 0x1680 ||
           (c >= 0x2000 && c <= 0x200A) || c == 0x2028 || c == 0x2029 || c == 0x202F || c == 0x205F || c == 0x3000;
}

static int is_punct_c(long c) {
    return (c >= '!' && c <= '#') || (c >= '%' && c <= '*') || (c >= ',' && c <= '/') || c == ':' || c == ';' ||
           c == '?' || c == '@' || (c >= '[' && c <= ']') || c == '_' || c == '{' || c == '}' || c == 0xA1 ||
           c == 0xA7 || c == 0xAB || c == 0xB6 || c == 0xB7 || c == 0xBB || c == 0xBF;
}

static long b(int v) { return v ? -1 : 0; }

long vbrt_char_isdigit(long c) { return b(c >= '0' && c <= '9'); }
long vbrt_char_isletter(long c) { return b(is_letter_c(c)); }
long vbrt_char_isletterordigit(long c) { return b(is_letter_c(c) || (c >= '0' && c <= '9')); }
long vbrt_char_iswhitespace(long c) { return b(is_space_c(c)); }
long vbrt_char_isupper(long c) { return b(is_letter_c(c) && lower_c(c) != c); }
long vbrt_char_islower(long c) { return b(is_letter_c(c) && upper_c(c) != c); }
long vbrt_char_ispunctuation(long c) { return b(is_punct_c(c)); }
long vbrt_char_toupper(long c) { return upper_c(c); }
long vbrt_char_tolower(long c) { return lower_c(c); }

/* ------------------------------------------------------------ .NET String methods */

long *vbrt_s_toupper(long *s) {
    long n = need(s)[0], i, *r = vbrt_str_new(n);
    for (i = 0; i < n; i++) r[1 + i] = upper_c(s[1 + i]);
    return r;
}

long *vbrt_s_tolower(long *s) {
    long n = need(s)[0], i, *r = vbrt_str_new(n);
    for (i = 0; i < n; i++) r[1 + i] = lower_c(s[1 + i]);
    return r;
}

/* what: 1 start, 2 end, 3 both; chars: Char() to trim (or NULL: white space) */
static long *trim(long *s, int what, long *chars) {
    long a = 0, z = slen(s);
    int (*in)(long *, long);
    if (!s) return s;
    while (a < z) {
        long c = s[1 + a], k, hit = 0;
        if (!(what & 1)) break;
        if (chars && chars[0]) { for (k = 0; k < chars[0]; k++) if (chars[3 + k] == c) hit = 1; }
        else hit = is_space_c(c);
        if (!hit) break;
        a++;
    }
    while (z > a) {
        long c = s[z], k, hit = 0;
        if (!(what & 2)) break;
        if (chars && chars[0]) { for (k = 0; k < chars[0]; k++) if (chars[3 + k] == c) hit = 1; }
        else hit = is_space_c(c);
        if (!hit) break;
        z--;
    }
    (void)in;
    return sub(s, a, z - a);
}

long *vbrt_s_trim(long *s) { return trim(need(s), 3, 0); }
long *vbrt_s_trimc(long *s, long *chars) { return trim(need(s), 3, chars); }
long *vbrt_s_trimstart(long *s) { return trim(need(s), 1, 0); }
long *vbrt_s_trimend(long *s) { return trim(need(s), 2, 0); }
long *vbrt_s_trimstartc(long *s, long *chars) { return trim(need(s), 1, chars); }
long *vbrt_s_trimendc(long *s, long *chars) { return trim(need(s), 2, chars); }

long *vbrt_s_substring(long *s, long start) {
    if (start < 0 || start > need(s)[0]) arg_error("startIndex cannot be larger than length of string. (Parameter 'startIndex')");
    return sub(s, start, s[0] - start);
}

long *vbrt_s_substring2(long *s, long start, long len) {
    if (start < 0 || start > need(s)[0]) arg_error("startIndex cannot be larger than length of string. (Parameter 'startIndex')");
    if (len < 0 || start + len > s[0]) arg_error("Index and length must refer to a location within the string. (Parameter 'length')");
    return sub(s, start, len);
}

static long index_of(long *s, long *t, long from) {
    long n = slen(s), m = slen(t), i, j;
    if (from < 0 || from > n) arg_error("Index was out of range. Must be non-negative and less than or equal to the size of the collection. (Parameter 'startIndex')");
    for (i = from; i + m <= n; i++) {
        for (j = 0; j < m && s[1 + i + j] == t[1 + j]; j++) ;
        if (j == m) return i;
    }
    return -1;
}

static long last_index_of(long *s, long *t) {
    long n = slen(s), m = slen(t), i, j;
    for (i = n - m; i >= 0; i--) {
        for (j = 0; j < m && s[1 + i + j] == t[1 + j]; j++) ;
        if (j == m) return i;
    }
    return -1;
}

long vbrt_s_indexof(long *s, long *t) { need(s); need(t); return index_of(s, t, 0); }
long vbrt_s_indexof2(long *s, long *t, long from) { need(s); need(t); return index_of(s, t, from); }
long vbrt_s_indexofc(long *s, long c) { return index_of(need(s), vbrt_str_char(c), 0); }
long vbrt_s_indexofc2(long *s, long c, long from) { return index_of(need(s), vbrt_str_char(c), from); }
long vbrt_s_lastindexof(long *s, long *t) { need(s); need(t); return last_index_of(s, t); }
long vbrt_s_lastindexofc(long *s, long c) { return last_index_of(need(s), vbrt_str_char(c)); }
long vbrt_s_contains(long *s, long *t) { need(s); need(t); return b(index_of(s, t, 0) >= 0); }
long vbrt_s_containsc(long *s, long c) { return b(index_of(need(s), vbrt_str_char(c), 0) >= 0); }

long vbrt_s_startswith(long *s, long *t) {
    long m = need(t)[0], i;
    if (m > need(s)[0]) return 0;
    for (i = 0; i < m; i++) if (s[1 + i] != t[1 + i]) return 0;
    return -1;
}

long vbrt_s_endswith(long *s, long *t) {
    long m = need(t)[0], n = need(s)[0], i;
    if (m > n) return 0;
    for (i = 0; i < m; i++) if (s[1 + n - m + i] != t[1 + i]) return 0;
    return -1;
}

long *vbrt_s_replace(long *s, long *a, long *r) {
    long n = need(s)[0], m = need(a)[0], len = 0, i, j, k, cap;
    long *buf, *out;
    if (m == 0) fail("ArgumentException", "String cannot be of zero length. (Parameter 'oldValue')");
    cap = n + 16;
    buf = malloc(sizeof(long) * cap);
    for (i = 0; i < n;) {
        for (j = 0; j < m && i + j < n && s[1 + i + j] == a[1 + j]; j++) ;
        if (j == m) {
            for (k = 0; k < slen(r); k++) {
                if (len == cap) buf = realloc(buf, sizeof(long) * (cap *= 2));
                buf[len++] = r[1 + k];
            }
            i += m;
        } else {
            if (len == cap) buf = realloc(buf, sizeof(long) * (cap *= 2));
            buf[len++] = s[1 + i++];
        }
    }
    out = vbrt_str_new(len);
    for (i = 0; i < len; i++) out[1 + i] = buf[i];
    free(buf);
    return out;
}

long *vbrt_s_replacec(long *s, long a, long r) {
    long n = need(s)[0], i, *out = vbrt_str_new(n);
    for (i = 0; i < n; i++) out[1 + i] = s[1 + i] == a ? r : s[1 + i];
    return out;
}

long *vbrt_s_insert(long *s, long at, long *t) {
    long n = need(s)[0], m = need(t)[0], i, *r;
    if (at < 0 || at > n) arg_error("Index was out of range. (Parameter 'startIndex')");
    r = vbrt_str_new(n + m);
    for (i = 0; i < at; i++) r[1 + i] = s[1 + i];
    for (i = 0; i < m; i++) r[1 + at + i] = t[1 + i];
    for (i = at; i < n; i++) r[1 + m + i] = s[1 + i];
    return r;
}

long *vbrt_s_remove2(long *s, long at, long cnt) {
    long n = need(s)[0], i, *r;
    if (at < 0 || cnt < 0 || at + cnt > n) arg_error("Index and count must refer to a location within the string. (Parameter 'count')");
    r = vbrt_str_new(n - cnt);
    for (i = 0; i < at; i++) r[1 + i] = s[1 + i];
    for (i = at + cnt; i < n; i++) r[1 + i - cnt] = s[1 + i];
    return r;
}

long *vbrt_s_remove(long *s, long at) {
    if (at < 0 || at > need(s)[0]) arg_error("startIndex must be less than length of string. (Parameter 'startIndex')");
    return vbrt_s_remove2(s, at, s[0] - at);
}

static long *pad(long *s, long width, long c, int left) {
    long n = need(s)[0], i, *r;
    if (width < 0) arg_error("Non-negative number required. (Parameter 'totalWidth')");
    if (width <= n) return s;
    r = vbrt_str_new(width);
    for (i = 0; i < width; i++) r[1 + i] = c;
    for (i = 0; i < n; i++) r[1 + (left ? width - n : 0) + i] = s[1 + i];
    return r;
}

long *vbrt_s_padleft(long *s, long w) { return pad(s, w, ' ', 1); }
long *vbrt_s_padleftc(long *s, long w, long c) { return pad(s, w, c, 1); }
long *vbrt_s_padright(long *s, long w) { return pad(s, w, ' ', 0); }
long *vbrt_s_padrightc(long *s, long w, long c) { return pad(s, w, c, 0); }

static long *char_array(long n) {
    long lens[1];
    lens[0] = n;
    return new_array(ARR_TAG(K_CHAR, 1), 1, lens);
}

static long *string_array(long n) {
    long lens[1];
    lens[0] = n;
    return new_array(ARR_TAG(K_STRING, 1), 1, lens);
}

long *vbrt_s_tochararray(long *s) {
    long n = need(s)[0], i, *a = char_array(n);
    for (i = 0; i < n; i++) a[3 + i] = s[1 + i];
    return a;
}

long *vbrt_chars_to_str(long *a) {
    long n, i, *s;
    if (!a) return 0;
    n = a[0];
    s = vbrt_str_new(n);
    for (i = 0; i < n; i++) s[1 + i] = a[3 + i];
    return s;
}

/* splits at any of the separators (Char()), or at the string sep */
static long *split(long *s, long *seps, long *sep, long limit) {
    long n = slen(s), i, start = 0, cnt = 1, k = 0, *a, m = sep ? slen(sep) : 0;
    /* count */
    for (i = 0; i < n; i++) {
        int hit = 0;
        if (sep) hit = m > 0 && i + m <= n && index_of(s, sep, i) == i;
        else if (seps && seps[0]) { long j; for (j = 0; j < seps[0]; j++) if (seps[3 + j] == s[1 + i]) hit = 1; }
        else hit = is_space_c(s[1 + i]);
        if (hit) {
            cnt++;
            if (sep) i += m - 1;
        }
    }
    if (limit > 0 && cnt > limit) cnt = limit;
    a = string_array(cnt);
    for (i = 0; i < n && k < cnt - 1; i++) {
        int hit = 0;
        if (sep) hit = m > 0 && i + m <= n && index_of(s, sep, i) == i;
        else if (seps && seps[0]) { long j; for (j = 0; j < seps[0]; j++) if (seps[3 + j] == s[1 + i]) hit = 1; }
        else hit = is_space_c(s[1 + i]);
        if (hit) {
            a[3 + k++] = (long)sub(s, start, i - start);
            start = i + (sep ? m : 1);
            if (sep) i += m - 1;
        }
    }
    a[3 + k] = (long)sub(s, start, n - start);
    return a;
}

long *vbrt_s_split(long *s, long *seps) { return split(need(s), seps, 0, 0); }
long *vbrt_s_splitc(long *s, long c) { long *a = char_array(1); a[3] = c; return split(need(s), a, 0, 0); }
long *vbrt_s_splits(long *s, long *sep) { return split(need(s), 0, sep, 0); }

long vbrt_s_equals(long *s, long *t) { need(s); return b(vbrt_strcmp(s, t, 0) == 0 && t != 0); }
long vbrt_s_compareto(long *s, long *t) { need(s); return t ? vbrt_strcmp(s, t, 0) : 1; }

/* String.Compare: a culture-like order: ignoring case first, then lower
   before upper case */
long vbrt_str_compare(long *a, long *b2, long ignore_case) {
    long c;
    if (!a || !b2) return a == b2 ? 0 : a ? 1 : -1;
    c = vbrt_strcmp(a, b2, 1);
    if (c || ignore_case) return c;
    {
        long n = a[0] < b2[0] ? a[0] : b2[0], i;
        for (i = 0; i < n; i++)
            if (a[1 + i] != b2[1 + i]) return lower_c(a[1 + i]) == a[1 + i] ? -1 : 1;
    }
    return 0;
}

long vbrt_str_isnullorempty(long *s) { return b(!s || s[0] == 0); }

long vbrt_str_isnullorwhitespace(long *s) {
    long i;
    if (!s) return -1;
    for (i = 0; i < s[0]; i++) if (!is_space_c(s[1 + i])) return 0;
    return -1;
}

/* String.Join(sep, String()) / (sep, Object()) */
long *vbrt_str_join(long *sep, long *arr) {
    long i, *r = empty_str();
    if (!arr) vbrt_throw(E_NULLREF);
    for (i = 0; i < arr[0]; i++) {
        long *e = (long *)arr[3 + i];
        if (i) r = vbrt_concat(r, sep);
        if (e && tag_of(e) != K_STRING) e = vbrt_obj_str(e);
        r = vbrt_concat(r, e);
    }
    return r;
}

long *vbrt_str_concat_arr(long *arr) { return vbrt_str_join(0, arr); }

long *vbrt_str_new_char(long c, long n) {
    long i, *s;
    if (n < 0) arg_error("Count cannot be less than zero. (Parameter 'count')");
    s = vbrt_str_new(n);
    for (i = 0; i < n; i++) s[1 + i] = c;
    return s;
}

/* ------------------------------------------------------------ VB string functions */

long vbrt_vb_len(long *s) { return slen(s); }

long *vbrt_vb_left(long *s, long n) {
    if (n < 0) fail("ArgumentException", "Argument 'Length' must be greater or equal to zero.");
    if (!s) return empty_str();
    return sub(s, 0, n < s[0] ? n : s[0]);
}

long *vbrt_vb_right(long *s, long n) {
    if (n < 0) fail("ArgumentException", "Argument 'Length' must be greater or equal to zero.");
    if (!s) return empty_str();
    if (n > s[0]) n = s[0];
    return sub(s, s[0] - n, n);
}

long *vbrt_vb_mid2(long *s, long start, long len) {
    long n = slen(s);
    if (start <= 0) fail("ArgumentException", "Argument 'Start' must be greater than zero.");
    if (len < 0) fail("ArgumentException", "Argument 'Length' must be greater or equal to zero.");
    if (!s || start > n) return empty_str();
    if (len > n - start + 1) len = n - start + 1;
    return sub(s, start - 1, len);
}

long *vbrt_vb_mid(long *s, long start) { return vbrt_vb_mid2(s, start, 2147483647); }

/* InStr([start,] s, t): 1-based, 0 if not found */
long vbrt_vb_instr2(long start, long *s, long *t) {
    long n = slen(s);
    if (start < 1) fail("ArgumentException", "Argument 'Start' must be greater than zero.");
    if (start > n + 1 || (!t || t[0] == 0)) return start > n + 1 ? 0 : start;
    if (!s) return 0;
    return index_of(s, t, start - 1) + 1;
}

long vbrt_vb_instr(long *s, long *t) { return vbrt_vb_instr2(1, s, t); }

long vbrt_vb_instrrev(long *s, long *t) {
    if (!t || t[0] == 0) return slen(s);
    return last_index_of(s, t) + 1;
}

long *vbrt_vb_ucase(long *s) { return s ? vbrt_s_toupper(s) : empty_str(); }
long *vbrt_vb_lcase(long *s) { return s ? vbrt_s_tolower(s) : empty_str(); }

/* VB's Trim functions remove spaces only */
long *vbrt_vb_trim(long *s) {
    long *sp = char_array(1);
    sp[3] = ' ';
    return s ? trim(s, 3, sp) : empty_str();
}

long *vbrt_vb_ltrim(long *s) {
    long *sp = char_array(1);
    sp[3] = ' ';
    return s ? trim(s, 1, sp) : empty_str();
}

long *vbrt_vb_rtrim(long *s) {
    long *sp = char_array(1);
    sp[3] = ' ';
    return s ? trim(s, 2, sp) : empty_str();
}

long vbrt_vb_chrw(long c) {
    if (c < -32768 || c > 65535) fail("ArgumentException", "Procedure call or argument is not valid.");
    return c < 0 ? c + 65536 : c;
}

long vbrt_vb_chr(long c) {
    if (c < -32768 || c > 65535) fail("ArgumentException", "Procedure call or argument is not valid.");
    return c < 0 ? c + 65536 : c;
}

long vbrt_vb_asc(long *s) {
    if (!s || s[0] == 0) fail("ArgumentException", "Argument 'String' cannot be zero-length or Nothing.");
    return s[1];
}

long *vbrt_vb_space(long n) {
    if (n < 0) fail("ArgumentException", "Argument 'Number' must be greater or equal to zero.");
    return vbrt_str_new_char(' ', n);
}

long *vbrt_vb_strdup(long n, long c) { return vbrt_str_new_char(c, n); }

long *vbrt_vb_strreverse(long *s) {
    long n = slen(s), i, *r = vbrt_str_new(n);
    for (i = 0; i < n; i++) r[1 + i] = s[n - i];
    return r;
}

long *vbrt_vb_replace(long *s, long *a, long *r) {
    if (!s) return 0;
    if (!a || a[0] == 0) return s;
    return vbrt_s_replace(s, a, r ? r : empty_str());
}

long *vbrt_vb_split(long *s, long *sep) {
    if (!s || s[0] == 0) {
        long *a = string_array(1);
        a[3] = (long)empty_str();
        return a;
    }
    if (!sep || sep[0] == 0) {
        long *a = string_array(1);
        a[3] = (long)s;
        return a;
    }
    return split(s, 0, sep, 0);
}

long *vbrt_vb_split1(long *s) { return vbrt_vb_split(s, vbrt_str_char(' ')); }
long *vbrt_vb_join(long *arr, long *sep) { return vbrt_str_join(sep, arr); }
long *vbrt_vb_join1(long *arr) { return vbrt_str_join(vbrt_str_char(' '), arr); }

long vbrt_vb_strcomp(long *a, long *b2) { return vbrt_strcmp(a, b2, 0); }

long *vbrt_vb_hex(long v) {
    char buf[32];
    sprintf(buf, "%llX", (unsigned long long)v);
    return str_from_c(buf);
}

long *vbrt_vb_oct(long v) {
    char buf[32];
    sprintf(buf, "%llo", (unsigned long long)v);
    return str_from_c(buf);
}

/* Val(s): the number at the start of the string, ignoring spaces */
double vbrt_vb_val(long *s) {
    char buf[400];
    long n = slen(s), i = 0, k = 0;
    while (i < n && (s[1 + i] == ' ' || s[1 + i] == '\t' || s[1 + i] == '\n' || s[1 + i] == '\r')) i++;
    if (i + 1 < n && s[1 + i] == '&') {
        long base = (s[2 + i] == 'H' || s[2 + i] == 'h') ? 16 : (s[2 + i] == 'O' || s[2 + i] == 'o') ? 8 : 0;
        long v = 0;
        if (base) {
            for (i += 2; i < n; i++) {
                long c = s[1 + i], d;
                if (c >= '0' && c <= '9') d = c - '0';
                else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
                else if (c == ' ') continue;
                else break;
                if (d >= base) break;
                v = v * base + d;
            }
            return (double)v;
        }
    }
    for (; i < n && k < 390; i++) {
        long c = s[1 + i];
        if (c == ' ' || c == '\t') continue;
        if ((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' || ((c == '+' || c == '-') &&
            (k == 0 || buf[k - 1] == 'e' || buf[k - 1] == 'E'))) buf[k++] = (char)c;
        else break;
    }
    buf[k] = 0;
    return strtod(buf, 0);
}

long vbrt_vb_isnumeric(long *o) {
    double d;
    long iv;
    int isint;
    if (!o) return 0;
    if (tag_of(o) == K_STRING) return b(!parse_num(o, &d, &iv, &isint));
    return b(tag_of(o) >= K_SBYTE && tag_of(o) <= K_DOUBLE);
}

/* ------------------------------------------------------------ Parse, TryParse */

/* parses an integer (Integer.Parse): white space, sign, digits; kind for
   the range */
static int parse_int(long *s, long kind, long *out) {
    long n = slen(s), i = 0, neg = 0, nd = 0;
    unsigned long long v = 0;
    if (!s) return 1;
    while (i < n && is_space_c(s[1 + i])) i++;
    while (n > i && is_space_c(s[n])) n--;
    if (i < n && (s[1 + i] == '-' || s[1 + i] == '+')) neg = s[1 + i++] == '-';
    for (; i < n; i++) {
        long c = s[1 + i];
        if (c < '0' || c > '9') return 1;
        if (v > 1844674407370955161ULL || (v == 1844674407370955161ULL && c > '5')) return 2;
        v = v * 10 + (c - '0');
        nd++;
    }
    if (!nd) return 1;
    if (kind == K_ULONG) {
        if (neg && v) return 2;
        *out = (long)v;
        return 0;
    }
    if (v > 9223372036854775808ULL || (!neg && v > 9223372036854775807ULL)) return 2;
    *out = neg ? -(long)v : (long)v;
    if (kind == K_UINTEGER || kind == K_USHORT || kind == K_BYTE) {
        if (*out < 0 && !(neg && v == 0)) return 2;
    }
    switch (kind) {
    case K_SBYTE: return *out < -128 || *out > 127 ? 2 : 0;
    case K_BYTE: return *out > 255 ? 2 : 0;
    case K_SHORT: return *out < -32768 || *out > 32767 ? 2 : 0;
    case K_USHORT: return *out > 65535 ? 2 : 0;
    case K_INTEGER: return *out < -2147483647 - 1 || *out > 2147483647 ? 2 : 0;
    case K_UINTEGER: return *out > 4294967295 ? 2 : 0;
    }
    return 0;
}

/* parses a floating point number (Double.Parse): white space, sign,
   digits with "," groups, ".", exponent, Infinity, NaN */
static int parse_dbl(long *s, double *out) {
    char buf[400];
    long n = slen(s), i = 0, k = 0, digits = 0;
    char *end;
    if (!s) return 1;
    while (i < n && is_space_c(s[1 + i])) i++;
    while (n > i && is_space_c(s[n])) n--;
    for (; i < n && k < 390; i++) {
        long c = s[1 + i];
        if (c == ',') continue;
        if (c >= '0' && c <= '9') digits++;
        buf[k++] = (char)c;
    }
    buf[k] = 0;
    if (strcmp(buf, "Infinity") == 0 || strcmp(buf, "+Infinity") == 0 || strcmp(buf, "\xE2\x88\x9E") == 0) { *out = INFINITY; return 0; }
    if (strcmp(buf, "-Infinity") == 0) { *out = -INFINITY; return 0; }
    if (strcmp(buf, "NaN") == 0) { *out = NAN; return 0; }
    if (!digits) return 1;
    for (i = 0; i < k; i++) {
        int c = buf[i];
        if (!((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-')) return 1;
    }
    *out = strtod(buf, &end);
    return *end != 0;
}

static void parse_fail(long *s, int why, long kind) {
    char msg[300];
    long n = slen(s), i, k;
    if (!s) fail("ArgumentNullException", "Value cannot be null. (Parameter 's')");
    if (why == 2) {
        sprintf(msg, "Value was either too large or too small for %s %s.",
                (kind == K_INTEGER || kind == K_UINTEGER) ? "an" : "a", sys_name(kind));
        fail("OverflowException", msg);
    }
    k = sprintf(msg, "The input string '");
    for (i = 0; i < n && k < 250; i++) msg[k++] = (char)s[1 + i];
    sprintf(msg + k, "' was not in a correct format.");
    fail("FormatException", msg);
}

long vbrt_parse_int(long *s, long kind) {
    long v = 0;
    int r = parse_int(s, kind, &v);
    if (r) parse_fail(s, r, kind);
    return v;
}

long vbrt_tryparse_int(long *s, long *out, long kind) {
    long v = 0;
    if (parse_int(s, kind, &v)) {
        *out = 0;
        return 0;
    }
    *out = v;
    return -1;
}

double vbrt_parse_dbl(long *s, long single) {
    double d = 0;
    if (parse_dbl(s, &d)) parse_fail(s, 1, K_DOUBLE);
    return single ? vbrt_to_single(d) : d;
}

long vbrt_tryparse_dbl(long *s, double *out, long single) {
    double d = 0;
    if (parse_dbl(s, &d)) {
        *out = 0;
        return 0;
    }
    *out = single ? vbrt_to_single(d) : d;
    return -1;
}

long vbrt_tryparse_bool(long *s, long *out) {
    static const char *tf[] = { "true", "false" };
    int w;
    for (w = 0; w < 2; w++) {
        long n = slen(s), i, j = 0, ok = 1, a = 0;
        while (a < n && is_space_c(s[1 + a])) a++;
        while (n > a && is_space_c(s[n])) n--;
        if (n - a != (long)strlen(tf[w])) continue;
        for (i = a; i < n; i++, j++) if (lower_c(s[1 + i]) != tf[w][j]) ok = 0;
        if (ok) {
            *out = w == 0 ? -1 : 0;
            return -1;
        }
    }
    *out = 0;
    return 0;
}

long vbrt_parse_bool(long *s) {
    long v = 0;
    if (!s) fail("ArgumentNullException", "Value cannot be null. (Parameter 'value')");
    if (!vbrt_tryparse_bool(s, &v)) {
        char msg[300];
        long i, k = sprintf(msg, "String '");
        for (i = 0; i < s[0] && k < 250; i++) msg[k++] = (char)s[1 + i];
        sprintf(msg + k, "' was not recognized as a valid Boolean.");
        fail("FormatException", msg);
    }
    return v;
}

/* Convert.ToInt32(String) and the like: Nothing is 0, else Parse */
long vbrt_convert_str_int(long *s, long kind) { return s ? vbrt_parse_int(s, kind) : 0; }
double vbrt_convert_str_dbl(long *s) { return s ? vbrt_parse_dbl(s, 0) : 0.0; }

/* ------------------------------------------------------------ Like */

static int like_at(long *s, long si, long *p, long pi, long text) {
    long n = slen(s), m = slen(p);
    while (pi < m) {
        long c = p[1 + pi];
        if (c == '*') {
            long k;
            while (pi < m && p[1 + pi] == '*') pi++;
            if (pi == m) return 1;
            for (k = si; k <= n; k++) if (like_at(s, k, p, pi, text)) return 1;
            return 0;
        }
        if (si >= n) return 0;
        if (c == '?') {
            si++;
            pi++;
            continue;
        }
        if (c == '#') {
            if (s[1 + si] < '0' || s[1 + si] > '9') return 0;
            si++;
            pi++;
            continue;
        }
        if (c == '[') {
            long q = pi + 1, neg = 0, hit = 0, x = s[1 + si];
            if (q < m && p[1 + q] == '!') { neg = 1; q++; }
            while (q < m && p[1 + q] != ']') {
                long lo = p[1 + q], hi = lo;
                if (q + 2 < m && p[2 + q] == '-' && p[3 + q] != ']') { hi = p[3 + q]; q += 2; }
                if (text ? (fold(x) >= fold(lo) && fold(x) <= fold(hi)) : (x >= lo && x <= hi)) hit = 1;
                q++;
            }
            if (q >= m) fail("ArgumentException", "Pattern string is not valid.");
            if (hit == neg) return 0;
            si++;
            pi = q + 1;
            continue;
        }
        if (text ? fold(c) != fold(s[1 + si]) : c != s[1 + si]) return 0;
        si++;
        pi++;
    }
    return si == n;
}

long vbrt_like(long *s, long *p, long text) { return b(like_at(s, 0, p, 0, text)); }

/* ------------------------------------------------------------ Math */

long vbrt_math_absi(long v, long kind) {
    if (v < 0) {
        if ((kind == K_INTEGER && v == -2147483647 - 1) || (kind == K_LONG && v == -9223372036854775807 - 1) ||
            (kind == K_SHORT && v == -32768) || (kind == K_SBYTE && v == -128))
            fail("OverflowException", "Negating the minimum value of a twos complement number is invalid.");
        return -v;
    }
    return v;
}

double vbrt_math_absd(double d) { return fabs(d); }
double vbrt_math_round(double d) { return round_even(d); }

/* Math.Round(value, digits), as .NET does it */
double vbrt_math_round2(double d, long digits) {
    double p = 1;
    long i;
    if (digits < 0 || digits > 15) arg_error("Rounding digits must be between 0 and 15, inclusive. (Parameter 'digits')");
    if (fabs(d) >= 1e16) return d;
    for (i = 0; i < digits; i++) p *= 10;
    return round_even(d * p) / p;
}

double vbrt_math_truncate(double d) { return d < 0 ? ceil(d) : floor(d); }
long vbrt_math_signd(double d) {
    if (isnan(d)) fail("ArithmeticException", "Function does not accept floating point Not-a-Number values.");
    return d > 0 ? 1 : d < 0 ? -1 : 0;
}
long vbrt_math_signi(long v) { return v > 0 ? 1 : v < 0 ? -1 : 0; }
double vbrt_math_log10(double d) { return log10(d); }
double vbrt_math_logb(double d, double base) { return log(d) / log(base); }
long vbrt_math_maxi(long a, long b2) { return a > b2 ? a : b2; }
long vbrt_math_mini(long a, long b2) { return a < b2 ? a : b2; }
double vbrt_math_maxd(double a, double b2) { return isnan(a) || isnan(b2) ? NAN : a > b2 ? a : b2; }
double vbrt_math_mind(double a, double b2) { return isnan(a) || isnan(b2) ? NAN : a < b2 ? a : b2; }
long vbrt_math_maxu(long a, long b2) { return vbrt_ul_cmp(a, b2) > 0 ? a : b2; }
long vbrt_math_minu(long a, long b2) { return vbrt_ul_cmp(a, b2) < 0 ? a : b2; }

/* Int(x) and Fix(x) of a Double */
double vbrt_vb_int(double d) { return floor(d); }
double vbrt_vb_fix(double d) { return d < 0 ? ceil(d) : floor(d); }

/* ------------------------------------------------------------ Rnd, Randomize */

static long rnd_seed = 0x50000;
static double rnd_last;

/* Rnd(): the 24-bit generator of VB */
double vbrt_vb_rnd(void) {
    rnd_seed = (rnd_seed * 0x43FD43FD + 0xC39EC3) & 0xFFFFFF;
    rnd_last = (double)rnd_seed / 16777216.0;
    return rnd_last;
}

/* Rnd(n): n < 0 the same number for each n, 0 the last number, > 0 the next */
double vbrt_vb_rnd1(double n) {
    if (n < 0) {
        long bits;
        double f = vbrt_to_single(n);
        /* the binary32 bits of n */
        int e;
        double m = frexp(fabs(f), &e);
        long mant = (long)(m * 16777216.0) & 0x7FFFFF;
        bits = ((n < 0 ? 1L : 0L) << 31) | ((long)(e + 126) << 23) | mant;
        rnd_seed = (bits + (bits >> 24)) & 0xFFFFFF;
        return vbrt_vb_rnd();
    }
    if (n == 0) return rnd_last;
    return vbrt_vb_rnd();
}

/* Randomize(): seeds from the time */
void vbrt_vb_randomize(void) {
    long t = (long)time(0);
    long v = (t & 0xFFFF) ^ (t >> 16);
    rnd_seed = (rnd_seed & 0xFF0000FF) | ((v << 8) & 0xFFFF00);
}

void vbrt_vb_randomize1(double n) {
    long bits;
    memcpy(&bits, &n, sizeof bits);
    bits = (bits >> 32) & 0xFFFFFFFF;
    bits = ((bits & 0xFFFF) ^ (bits >> 16)) << 8;
    rnd_seed = (rnd_seed & 0xFF0000FF) | (bits & 0xFFFF00);
}

/* ------------------------------------------------------------ Environment */

void vbrt_env_exit(long code) {
    fflush(stdout);
    exit((int)code);
}

/* Environment.GetCommandLineArgs(): with the program name */
long *vbrt_env_args(void) {
    long i, *a = string_array(vb_argc);
    for (i = 0; i < vb_argc; i++) a[3 + i] = (long)str_from_c(vb_argv[i]);
    return a;
}

/* ------------------------------------------------------------ more conversions */

double vbrt_unbox_d(long *o) {
    long v = vbrt_unbox(o, K_DOUBLE);
    return bits_d(v);
}

long *vbrt_vb_hex32(long v) { return vbrt_vb_hex(v & 0xFFFFFFFF); }
long *vbrt_vb_oct32(long v) { return vbrt_vb_oct(v & 0xFFFFFFFF); }
long vbrt_vb_isnothing(long *o) { return b(o == 0); }

/* CompareTo and Equals of the primitive types */
long vbrt_cmp_i(long a, long b2) { return a < b2 ? -1 : a > b2 ? 1 : 0; }
long vbrt_cmp_d(double a, double b2) {
    if (a < b2) return -1;
    if (a > b2) return 1;
    if (a == b2) return 0;
    return isnan(a) ? (isnan(b2) ? 0 : -1) : 1;
}
long vbrt_eq_i(long a, long b2) { return a == b2 ? -1 : 0; }
long vbrt_eq_d(double a, double b2) { return (a == b2 || (isnan(a) && isnan(b2))) ? -1 : 0; }

/* the characters chars[start .. start+length-1] as a String */
long *vbrt_chars_sub(long *a, long start, long len) {
    long i, *s;
    if (!a) vbrt_throw(E_NULLREF);
    if (start < 0 || len < 0 || start + len > a[0]) vbrt_throw(E_INDEX);
    s = vbrt_str_new(len);
    for (i = 0; i < len; i++) s[1 + i] = a[3 + start + i];
    return s;
}

/* the default comparison of two objects that are not IComparable: strings,
   and boxed values of the same primitive kind or enum */
long vbrt_obj_compare(long *a, long *b2) {
    long ta, tb, k;
    if (!a || !b2) return a ? 1 : b2 ? -1 : 0;
    ta = tag_of(a);
    tb = tag_of(b2);
    if (ta == K_STRING && tb == K_STRING) return vbrt_str_compare(a, b2, 0);
    if (ta != tb) fail("InvalidOperationException", "Failed to compare two elements in the array.");
    k = ta;
    if (ta > 255) {
        long *d = (long *)ta;
        if (d[0] != D_ENUM) fail("InvalidOperationException", "Failed to compare two elements in the array.");
        k = d[4];
    }
    if (k == K_SINGLE || k == K_DOUBLE) return vbrt_cmp_d(bits_d(a[0]), bits_d(b2[0]));
    if (k == K_ULONG) return vbrt_ul_cmp(a[0], b2[0]);
    if (k == K_BOOLEAN) return a[0] == b2[0] ? 0 : a[0] ? 1 : -1;      /* False < True */
    if (k <= K_CHAR) return vbrt_cmp_i(a[0], b2[0]);
    fail("InvalidOperationException", "Failed to compare two elements in the array.");
    return 0;
}

/* a seed for New Random(): from the clock */
long vbrt_tick_seed(void) {
    long t = (long)clock() ^ ((long)time(0) * 2654435761L);
    return t & 0x7FFFFFFF;
}
