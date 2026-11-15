/*
 * Tower_of_Hanoi.c
 *
 * Solves the Tower of Hanoi puzzle for n disks recursively and prints the
 * pegs about ten times along the way (and at the end).  The pegs are an
 * n x 3 array: slot 0 is the top of a peg, 0 means no disk.
 * C11 version of documentation/projects/Tower_of_Hanoi.asm.
 *
 * Usage:
 *     Tower_of_Hanoi <disks>
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <stdlib.h>

static long num_discs;
static long move_number;
static long print_mod = 1;          /* the pegs are printed after every print_mod moves */
static long *pegs;                  /* pegs[peg * num_discs + slot] */

static long get_disk(long peg, long slot) { return pegs[peg * num_discs + slot]; }
static void set_disk(long peg, long slot, long v) { pegs[peg * num_discs + slot] = v; }

static void print_pegs(void)
{
    long i, j;
    printf("Move: %ld\n", move_number);
    for (i = 0; i < num_discs; i++) {
        for (j = 0; j < 3; j++) {
            long disk = get_disk(j, i);
            if (disk == 0) fputs("|", stdout);
            else printf("%ld", disk);
            fputs("   ", stdout);
        }
        putchar('\n');
    }
    fputs("=========\n", stdout);
    putchar('\n');
}

/* moves the top disk of src to the top of dst */
static void make_move(long src, long dst)
{
    long i, disk = 0, pos = num_discs - 1;
    move_number++;
    for (i = 0; i < num_discs; i++) {
        if (get_disk(src, i) != 0) {
            disk = get_disk(src, i);
            set_disk(src, i, 0);
            break;
        }
    }
    for (i = 0; i < num_discs; i++) {
        if (get_disk(dst, i) != 0) {
            pos = i - 1;
            break;
        }
    }
    set_disk(dst, pos, disk);
    if (move_number % print_mod == 0) print_pegs();
}

static void solve(long n, long src, long dst, long aux)
{
    if (n == 1) make_move(src, dst);
    else {
        solve(n - 1, src, aux, dst);
        make_move(src, dst);
        solve(n - 1, aux, dst, src);
    }
}

int main(int argc, char **argv)
{
    long n, i, steps, pm;
    if (argc != 2) {
        puts("Syntax: Hanoi [disks]");
        exit(1);
    }
    n = atol(argv[1]);
    if (n <= 0) n = 3;
    num_discs = n;
    pegs = calloc(n * 3, sizeof(long));
    if (!pegs) {
        puts("Allocation failed!");
        exit(1);
    }
    for (i = 0; i < n; i++) set_disk(0, i, i + 1);
    /* print about 10 times: every steps/10 moves, rounded down to a multiple of 5 (100 for n > 10) */
    steps = (1L << n) - 1;
    pm = steps / 10 + 2;
    pm -= pm % (n > 10 ? 100 : 5);
    if (pm < 1) pm = 1;
    print_mod = pm;
    print_pegs();
    solve(n, 0, 2, 1);
    if (move_number % print_mod != 0) print_pegs();
    free(pegs);
    return 0;
}
