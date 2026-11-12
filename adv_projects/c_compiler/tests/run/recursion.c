#include <stdio.h>
/* Towers of Hanoi, Ackermann and mutual recursion */
int moves;
void hanoi(int n, char from, char to, char via) {
    if (n == 0) return;
    hanoi(n - 1, from, via, to);
    moves++;
    if (n == 3) printf("move disk 3 %c->%c\n", from, to);
    hanoi(n - 1, via, to, from);
}
int ack(int m, int n) {
    if (m == 0) return n + 1;
    if (n == 0) return ack(m - 1, 1);
    return ack(m - 1, ack(m, n - 1));
}
int is_odd(int n);
int is_even(int n) { return n == 0 ? 1 : is_odd(n - 1); }
int is_odd(int n) { return n == 0 ? 0 : is_even(n - 1); }
long sum_to(long n) { return n == 0 ? 0 : n + sum_to(n - 1); }
int main(void) {
    hanoi(10, 'A', 'C', 'B');
    printf("moves=%d\n", moves);
    printf("ack(2,3)=%d ack(3,3)=%d\n", ack(2, 3), ack(3, 3));
    printf("%d %d\n", is_even(1000), is_odd(777));
    printf("%ld\n", sum_to(50000));
    return 0;
}
