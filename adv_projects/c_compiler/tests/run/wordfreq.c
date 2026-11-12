/* Word frequency counter using a hash table of linked lists. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUCKETS 31

typedef struct Entry {
    char *word;
    int count;
    struct Entry *next;
} Entry;

static Entry *table[BUCKETS];

static unsigned hash(const char *s) {
    unsigned h = 5381;
    while (*s) h = h * 33 + (unsigned)*s++;
    return h % BUCKETS;
}

static void add_word(const char *w) {
    unsigned h = hash(w);
    for (Entry *e = table[h]; e; e = e->next)
        if (strcmp(e->word, w) == 0) { e->count++; return; }
    Entry *e = malloc(sizeof *e);
    e->word = strdup(w);
    e->count = 1;
    e->next = table[h];
    table[h] = e;
}

static int cmp_entry(const void *a, const void *b) {
    const Entry *x = *(const Entry **)a, *y = *(const Entry **)b;
    if (x->count != y->count) return y->count - x->count;
    return strcmp(x->word, y->word);
}

int main(void) {
    const char *text =
        "the quick brown fox jumps over the lazy dog. The dog barks; "
        "the fox runs. A quick brown dog? Yes, the quick one!";
    char word[32];
    int n = 0, total = 0;
    for (const char *p = text;; p++) {
        if (isalpha(*p)) {
            if (n < 31) word[n++] = tolower(*p);
        } else {
            if (n > 0) { word[n] = '\0'; add_word(word); total++; n = 0; }
            if (!*p) break;
        }
    }
    Entry *all[64];
    int count = 0;
    for (int i = 0; i < BUCKETS; i++)
        for (Entry *e = table[i]; e; e = e->next) all[count++] = e;
    qsort(all, count, sizeof(Entry *), cmp_entry);
    printf("%d words, %d distinct\n", total, count);
    for (int i = 0; i < 5; i++) printf("%-6s %d\n", all[i]->word, all[i]->count);
    for (int i = 0; i < count; i++) { free(all[i]->word); free(all[i]); }
    return 0;
}
