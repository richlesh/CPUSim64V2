#include <stdio.h>
#include <stdlib.h>
typedef struct Node { int val; struct Node *next; } Node;
struct Point { int x, y; };
typedef struct { char name[8]; int age; double score; } Person;
union Value { int i; double d; char c; };
enum Color { RED, GREEN = 5, BLUE };
typedef enum Color Color;
typedef int (*Cmp)(const void *, const void *);
Node *push(Node *head, int v) { Node *n = malloc(sizeof(Node)); n->val = v; n->next = head; return n; }
int length(Node *n) { int k = 0; for (; n; n = n->next) k++; return k; }
struct Point make_point(int x, int y) { struct Point p = { x, y }; return p; }
struct Point add_points(struct Point a, struct Point b) { struct Point r; r.x = a.x + b.x; r.y = a.y + b.y; return r; }
int by_age(const void *a, const void *b) { return ((const Person *)a)->age - ((const Person *)b)->age; }
int cmp_int(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }
const char *color_name(Color c) {
    switch (c) { case RED: return "red"; case GREEN: return "green"; case BLUE: return "blue"; }
    return "?";
}
int apply(int (*f)(int, int), int a, int b) { return f(a, b); }
int mul(int a, int b) { return a * b; }
int sub2(int a, int b) { return a - b; }
struct Point gpt = { 3, 4 };
struct Point *gpp = &gpt;
int (*gfn)(int, int) = mul;
Person team[3] = { { "ann", 31, 1.5 }, { "bob", 25, 2.5 }, { "cy", 40, 3.5 } };
int main(void) {
    Node *list = 0;
    for (int i = 1; i <= 5; i++) list = push(list, i * 10);
    printf("len=%d first=%d second=%d\n", length(list), list->val, list->next->val);
    int total = 0;
    while (list) { Node *n = list; total += n->val; list = n->next; free(n); }
    printf("total=%d\n", total);
    struct Point p = make_point(1, 2), q = { 10, 20 };
    struct Point r = add_points(p, q);
    printf("r=(%d,%d) size=%d\n", r.x, r.y, (int)sizeof r);
    struct Point copy = r; copy.x = 0;
    printf("copy=(%d,%d) r.x=%d\n", copy.x, copy.y, r.x);
    printf("gpt=(%d,%d) via ptr %d\n", gpt.x, gpt.y, gpp->y);
    union Value u; u.d = 1.5;
    printf("union size=%d d=%.1f\n", (int)sizeof(union Value), u.d);
    u.i = 65; printf("u.c=%c\n", u.c);
    printf("%s %s %s %d\n", color_name(RED), color_name(GREEN), color_name(BLUE), BLUE);
    printf("%d %d %d\n", apply(mul, 6, 7), apply(sub2, 6, 7), gfn(3, 3));
    int (*ops[2])(int, int) = { mul, sub2 };
    printf("%d %d\n", ops[0](2, 5), (*ops[1])(2, 5));
    qsort(team, 3, sizeof(Person), by_age);
    for (int i = 0; i < 3; i++) printf("%s:%d:%.1f ", team[i].name, team[i].age, team[i].score);
    printf("\n");
    int nums[] = { 5, 3, 9, 1, 7 };
    Cmp c = cmp_int;
    qsort(nums, 5, sizeof(int), c);
    for (int i = 0; i < 5; i++) printf("%d ", nums[i]);
    printf("\n");
    Person *pp = &team[1];
    pp->age++;
    (*pp).score *= 2;
    printf("%s %d %.1f\n", pp->name, team[1].age, team[1].score);
    struct { int a[3]; struct Point pt; } nested = { { 1, 2, 3 }, { 4, 5 } };
    printf("%d %d %d\n", nested.a[2], nested.pt.y, (int)sizeof nested);
    struct Point arr[2] = { [1] = { .y = 9, .x = 8 } };
    printf("%d %d %d %d\n", arr[0].x, arr[0].y, arr[1].x, arr[1].y);
    struct Point *cl = &(struct Point){ 11, 12 };
    printf("%d\n", cl->x + cl->y);
    return 0;
}
