typedef struct Node { int val; struct Node *next; } Node;
struct P { int x, y; };
union U { int i; double d; char c[3]; };
enum Color { RED, GREEN = 5, BLUE };
typedef int (*binop)(int, char *);
int (*fp)(int, char *);
int a[3][4];
double arr[] = { 1.5, 2, [4] = 3 };
struct P pts[2] = { {1, 2}, 3, 4 };
char s[] = "hi";
char *msg = "hello";
int sizes[] = { sizeof(Node), sizeof(union U), sizeof a, sizeof(a[0]), BLUE };
int f(int n, char **v) {
    Node nd = { 7, 0 }, *p = &nd;
    int i, sum = 0;
    for (i = 0; i < n; i++) {
        if (i % 2) continue; else sum += i;
    }
    while (sum > 100) sum--;
    do { sum++; } while (sum < 3);
    switch (n) { case 1: sum = 1; break; case RED: default: sum = 2; }
    p->val = p->next ? 1 : 2;
    a[1][2] = *v[0] + (int)3.7;
    goto out;
out:
    return sum + nd.val + (p - &nd) + RED;
}
