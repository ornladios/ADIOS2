#include "stdio.h"
#include "dill.h"

static int errors = 0;

static void check_i(int got, int expected) {
    if (got != expected) {
	printf("FAIL: got %d expected %d\n", got, expected);
	errors++;
    }
}

static void check_l(long got, long expected) {
    if (got != expected) {
	printf("FAIL: got %ld expected %ld\n", got, expected);
	errors++;
    }
}

static void noop7(void *a, void *b, void *c, int d, int e, int f, void *g) {
    volatile int x = d + e + f;
    (void)a; (void)b; (void)c; (void)g; (void)x;
}

int main() {
    dill_stream s;
    dill_exec_handle h;
    dill_reg blk_reg, tmp, d1, d2;
    int blk;

    /* Test 1: local block survives a function call */
    printf("Test 1: local block across call\n");
    s = dill_create_stream();
    dill_start_simple_proc(s, "t1", DILL_V);
    blk = dill_getvblock(s, 32);
    blk_reg = dill_getreg(s, DILL_P);
    tmp = dill_getreg(s, DILL_I);
    dill_virtual_lea(s, blk_reg, blk);
    dill_seti(s, tmp, 42);
    dill_stii(s, tmp, blk_reg, 0);
    dill_seti(s, tmp, 99);
    dill_stii(s, tmp, blk_reg, 4);

    d1 = dill_getreg(s, DILL_P);
    d2 = dill_getreg(s, DILL_P);
    dill_setp(s, d1, 0);
    dill_setp(s, d2, 0);
    dill_scallv(s, (void*)noop7, "noop7",
		"%p%p%p%I%I%I%p", blk_reg, d1, d2, 1, 2, 3, blk_reg);

    dill_ldii(s, tmp, blk_reg, 0);
    dill_scallv(s, (void*)check_i, "post", "%i%I", tmp, 42);
    dill_ldii(s, tmp, blk_reg, 4);
    dill_scallv(s, (void*)check_i, "chk", "%i%I", tmp, 99);

    h = dill_finalize(s);
    ((void(*)())dill_get_fp(h))();
    dill_free_handle(h);
    dill_free_stream(s);

    /* Test 2: multiple local blocks, all survive calls */
    printf("Test 2: multiple local blocks across calls\n");
    s = dill_create_stream();
    dill_start_simple_proc(s, "t2", DILL_V);
    {
	int blk1 = dill_getvblock(s, 16);
	int blk2 = dill_getvblock(s, 16);
	dill_reg r1, r2, t;
	r1 = dill_getreg(s, DILL_P);
	r2 = dill_getreg(s, DILL_P);
	t = dill_getreg(s, DILL_I);
	dill_virtual_lea(s, r1, blk1);
	dill_virtual_lea(s, r2, blk2);

	dill_seti(s, t, 111);
	dill_stii(s, t, r1, 0);
	dill_seti(s, t, 222);
	dill_stii(s, t, r2, 0);

	d1 = dill_getreg(s, DILL_P);
	dill_setp(s, d1, 0);
	dill_scallv(s, (void*)noop7, "noop7",
		    "%p%p%p%I%I%I%p", r1, d1, d1, 0, 0, 0, r2);

	dill_ldii(s, t, r1, 0);
	dill_scallv(s, (void*)check_i, "chk", "%i%I", t, 111);
	dill_ldii(s, t, r2, 0);
	dill_scallv(s, (void*)check_i, "chk", "%i%I", t, 222);
    }
    h = dill_finalize(s);
    ((void(*)())dill_get_fp(h))();
    dill_free_handle(h);
    dill_free_stream(s);

    /* Test 3: long (8-byte) values in local block across call */
    printf("Test 3: long values in local block across call\n");
    s = dill_create_stream();
    dill_start_simple_proc(s, "t3", DILL_V);
    {
	int b = dill_getvblock(s, 16);
	dill_reg br, t;
	br = dill_getreg(s, DILL_P);
	t = dill_getreg(s, DILL_L);
	dill_virtual_lea(s, br, b);
	dill_setl(s, t, (IMM_TYPE)0x123456789ABCDEF0LL);
	dill_stli(s, t, br, 0);

	d1 = dill_getreg(s, DILL_P);
	dill_setp(s, d1, 0);
	dill_scallv(s, (void*)noop7, "noop7",
		    "%p%p%p%I%I%I%p", br, d1, d1, 0, 0, 0, d1);

	dill_ldli(s, t, br, 0);
	dill_scallv(s, (void*)check_l, "chk", "%l%L",
		    t, (IMM_TYPE)0x123456789ABCDEF0LL);
    }
    h = dill_finalize(s);
    ((void(*)())dill_get_fp(h))();
    dill_free_handle(h);
    dill_free_stream(s);

    if (errors == 0) {
	printf("No errors!\n");
    } else {
	printf("%d errors\n", errors);
    }
    return errors != 0;
}
