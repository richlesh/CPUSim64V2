/* <stdatomic.h>: single-threaded behaviour of every operation */
#include <stdio.h>
#include <stdatomic.h>

struct big { long a, b, c; };

static atomic_int g = ATOMIC_VAR_INIT(7);
static atomic_flag gflag = ATOMIC_FLAG_INIT;

int main(void)
{
    atomic_long x;
    atomic_uint u;
    atomic_bool b;
    atomic_int i;
    long old, expected;
    int arr[5] = {10, 20, 30, 40, 50};
    struct big bs[4];
    int *volatile ip;
    atomic_intptr_t ap;
    struct big *sp;
    atomic_flag f = ATOMIC_FLAG_INIT;

    printf("g=%d lf=%d %d %d\n", (int)atomic_load(&g), ATOMIC_INT_LOCK_FREE,
           ATOMIC_POINTER_LOCK_FREE, ATOMIC_BOOL_LOCK_FREE);
    atomic_init(&x, 5);
    printf("load %ld\n", atomic_load(&x));
    atomic_store(&x, 42);
    printf("store %ld\n", atomic_load(&x));
    printf("exchange old=%ld", atomic_exchange(&x, 100));
    printf(" now=%ld\n", atomic_load(&x));

    /* fetch_* return the OLD value */
    old = atomic_fetch_add(&x, 5);  printf("add old=%ld now=%ld\n", old, atomic_load(&x));
    old = atomic_fetch_sub(&x, 15); printf("sub old=%ld now=%ld\n", old, atomic_load(&x));
    atomic_store(&x, 12);
    old = atomic_fetch_and(&x, 10); printf("and old=%ld now=%ld\n", old, atomic_load(&x));
    old = atomic_fetch_or(&x, 5);   printf("or old=%ld now=%ld\n", old, atomic_load(&x));
    old = atomic_fetch_xor(&x, 3);  printf("xor old=%ld now=%ld\n", old, atomic_load(&x));
    old = atomic_fetch_add(&x, -20); printf("add neg old=%ld now=%ld\n", old, atomic_load(&x));

    /* compare_exchange */
    atomic_store(&x, 50);
    expected = 50;
    old = atomic_compare_exchange_strong(&x, &expected, 60);
    printf("cas1=%ld expected=%ld now=%ld\n", old, expected, atomic_load(&x));
    expected = 1;
    old = atomic_compare_exchange_strong(&x, &expected, 70);
    printf("cas2=%ld expected=%ld now=%ld\n", old, expected, atomic_load(&x));
    old = atomic_compare_exchange_weak(&x, &expected, 70);
    printf("cas3=%ld now=%ld\n", old, atomic_load(&x));
    expected = 3;
    old = atomic_compare_exchange_weak_explicit(&x, &expected, 9, memory_order_acq_rel, memory_order_acquire);
    printf("cas4=%ld expected=%ld\n", old, expected);

    /* _explicit forms and every memory order */
    atomic_store_explicit(&x, 1, memory_order_relaxed);
    old = atomic_fetch_add_explicit(&x, 1, memory_order_acquire);
    old += atomic_fetch_sub_explicit(&x, 1, memory_order_release) * 10;
    old += atomic_fetch_or_explicit(&x, 8, memory_order_consume) * 100;
    old += atomic_fetch_and_explicit(&x, 12, memory_order_seq_cst) * 1000;
    old += atomic_fetch_xor_explicit(&x, 4, memory_order_acq_rel) * 10000;
    printf("explicit %ld now=%ld", old, atomic_load_explicit(&x, memory_order_acquire));
    printf(" load=%ld", atomic_load_explicit(&x, memory_order_seq_cst));
    printf(" exch=%ld\n", atomic_exchange_explicit(&x, 0, memory_order_relaxed));
    printf("orders %d %d %d %d %d %d\n", memory_order_relaxed, memory_order_consume, memory_order_acquire,
           memory_order_release, memory_order_acq_rel, memory_order_seq_cst);

    /* unsigned, bool, int, size_t typedefs */
    atomic_init(&u, 4000000000U);
    atomic_fetch_add(&u, 5);
    printf("uint %lu\n", (unsigned long)atomic_load(&u));
    atomic_init(&b, 0);
    printf("bool was %d", atomic_exchange(&b, 1));
    printf(" now %d\n", (int)atomic_load(&b));
    atomic_init(&i, -3);
    printf("int %d", (int)atomic_fetch_add(&i, 10));
    printf(" %d\n", (int)atomic_load(&i));
    {
        atomic_size_t sz; atomic_ptrdiff_t pd; atomic_uintmax_t um; atomic_char ch; atomic_wchar_t wc;
        atomic_int_fast32_t f32; atomic_uint_least8_t l8; atomic_char16_t c16; atomic_char32_t c32;
        atomic_schar sc; atomic_uchar uc; atomic_short sh; atomic_ushort us; atomic_llong ll; atomic_ullong ull;
        atomic_init(&sz, 1); atomic_init(&pd, -1); atomic_init(&um, 3); atomic_init(&ch, 'a');
        atomic_init(&wc, 'w'); atomic_init(&f32, 32); atomic_init(&l8, 8); atomic_init(&c16, 16);
        atomic_init(&c32, 33); atomic_init(&sc, -2); atomic_init(&uc, 2); atomic_init(&sh, -5);
        atomic_init(&us, 5); atomic_init(&ll, -6); atomic_init(&ull, 6);
        printf("types %ld %ld %ld %c %c", (long)atomic_load(&sz), (long)atomic_load(&pd),
               (long)atomic_load(&um), (char)atomic_load(&ch), (char)atomic_load(&wc));
        printf(" %ld %ld %ld %ld", (long)atomic_load(&f32), (long)atomic_load(&l8),
               (long)atomic_load(&c16), (long)atomic_load(&c32));
        printf(" %ld %ld %ld %ld %ld %ld\n", (long)atomic_load(&sc), (long)atomic_load(&uc),
               (long)atomic_load(&sh), (long)atomic_load(&us), (long)atomic_load(&ll), (long)atomic_load(&ull));
    }

    /* lock-free query, kill_dependency */
    printf("is_lock_free %d", atomic_is_lock_free(&x));
    printf(" %d\n", atomic_is_lock_free(&b));
    printf("kill_dependency %d\n", kill_dependency(3) + 1);

    /* pointers: fetch_add scales by the size of the pointed-to type */
    ip = arr;
    atomic_init(&ap, (intptr_t)arr);
    printf("ptr start %d\n", *(int *)atomic_load(&ap));
    {
        int *volatile p = arr;
        int n1;
        int *old_p = (int *)atomic_fetch_add(&p, 2);
        printf("ptr add old=%d now=%d\n", *old_p, *p);
        old_p = (int *)atomic_fetch_sub(&p, 1);
        printf("ptr sub old=%d now=%d\n", *old_p, *p);
        old_p = (int *)atomic_exchange(&p, arr + 4);
        printf("ptr exch old=%d now=%d\n", *old_p, *p);
        old_p = arr + 4;
        n1 = atomic_compare_exchange_strong(&p, &old_p, arr);
        printf("ptr cas=%d now=%d\n", n1, *p);
        old_p = arr + 3;
        n1 = atomic_compare_exchange_strong(&p, &old_p, arr);
        printf("ptr cas=%d cur=%d now=%d\n", n1, *old_p, *p);
        atomic_store(&p, arr + 1);
        printf("ptr load %d\n", *(int *)atomic_load(&p));
    }
    {
        struct big *volatile q = bs;
        struct big *old_q;
        bs[0].a = 1; bs[1].a = 2; bs[2].a = 3; bs[3].a = 4;
        old_q = (struct big *)atomic_fetch_add(&q, 3);
        printf("struct ptr old=%ld now=%ld\n", old_q->a, q->a);
        old_q = (struct big *)atomic_fetch_sub(&q, 2);
        printf("struct ptr old=%ld now=%ld\n", old_q->a, q->a);
        sp = q;
        printf("same %d\n", sp == &bs[1]);
    }

    /* atomic_flag */
    printf("flag %d", atomic_flag_test_and_set(&f));
    printf(" %d", atomic_flag_test_and_set(&f));
    atomic_flag_clear(&f);
    printf(" %d", atomic_flag_test_and_set_explicit(&f, memory_order_acquire));
    atomic_flag_clear_explicit(&f, memory_order_release);
    printf(" %d\n", atomic_flag_test_and_set(&f));
    printf("gflag %d", atomic_flag_test_and_set(&gflag));
    printf(" %d\n", atomic_flag_test_and_set(&gflag));

    /* fences */
    atomic_thread_fence(memory_order_seq_cst);
    atomic_signal_fence(memory_order_acquire);
    printf("fences ok\n");

    /* arguments evaluated once */
    {
        int n = 0;
        atomic_long arrl[3];
        atomic_long *pl = arrl;
        atomic_init(&arrl[0], 0); atomic_init(&arrl[1], 0); atomic_init(&arrl[2], 0);
        atomic_fetch_add(pl++, n++ + 5);
        atomic_store(pl++, n++ + 7);
        printf("once n=%d idx=%d", n, (int)(pl - arrl));
        printf(" %ld %ld %ld\n", atomic_load(&arrl[0]), atomic_load(&arrl[1]), atomic_load(&arrl[2]));
    }
    return 0;
}
