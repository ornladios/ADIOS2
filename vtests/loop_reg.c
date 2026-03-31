/*
 * Tests for virtual register allocation across loop back-edges.
 *
 * These tests exercise the global pre-assignment path (do_global_assign)
 * which pins cross-block virtual registers to callee-saved physical
 * registers so they survive loop iterations without spill/reload.
 */
#include "dill.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

static int verbose = 0;

/*
 * Test 1: Tight array computation loop — the ADIOS2 pattern.
 *
 * Generates: out[i] = A[i]*A[i] + B[i]*B[i] + C[i]*C[i]
 * for i in [0, N).
 *
 * Cross-block vregs: pA, pB, pC, pOut (pointers), off (byte offset), N.
 * Float temporaries are block-local.
 */
static int
test_array_loop(void)
{
    dill_stream s = dill_create_stream();
    dill_exec_handle handle;
    void (*func)(double*, double*, double*, double*, long);
    dill_reg pA, pB, pC, pOut, off, limit;
    dill_reg va, vb, vc, t1, t2;
    int L_top, L_end;
    int i, failure = 0;
    const int N = 64;
    double A[64], B[64], C[64], out[64];

    /* Set up input data */
    for (i = 0; i < N; i++) {
        A[i] = (double)(i + 1);
        B[i] = (double)(i + 2);
        C[i] = (double)(i + 3);
        out[i] = -1.0;
    }

    dill_start_proc(s, "array_loop", DILL_V, "%p%p%p%p%l");
    pA = dill_vparam(s, 0);
    pB = dill_vparam(s, 1);
    pC = dill_vparam(s, 2);
    pOut = dill_vparam(s, 3);
    limit = dill_vparam(s, 4);

    off = dill_getreg(s, DILL_L);
    va = dill_getreg(s, DILL_D);
    vb = dill_getreg(s, DILL_D);
    vc = dill_getreg(s, DILL_D);
    t1 = dill_getreg(s, DILL_D);
    t2 = dill_getreg(s, DILL_D);

    L_top = dill_alloc_label(s, "loop_top");
    L_end = dill_alloc_label(s, "loop_end");

    /* Convert element count to byte limit: limit *= 8 */
    dill_mulli(s, limit, limit, (IMM_TYPE)8);
    dill_setl(s, off, (IMM_TYPE)0);

    /* Loop top */
    dill_mark_label(s, L_top);
    dill_bgel(s, off, limit, L_end);

    /* Load A[i], B[i], C[i] via base+offset */
    dill_ldd(s, va, pA, off);
    dill_ldd(s, vb, pB, off);
    dill_ldd(s, vc, pC, off);

    /* Compute A*A + B*B + C*C */
    dill_muld(s, t1, va, va);
    dill_muld(s, t2, vb, vb);
    dill_addd(s, t1, t1, t2);
    dill_muld(s, t2, vc, vc);
    dill_addd(s, t1, t1, t2);

    /* Store result */
    dill_std(s, t1, pOut, off);

    /* Advance offset by sizeof(double) */
    dill_addli(s, off, off, (IMM_TYPE)8);
    dill_jv(s, L_top);
    dill_mark_label(s, L_end);
    dill_retii(s, 0);

    handle = dill_finalize(s);
    func =
        (void (*)(double*, double*, double*, double*, long))dill_get_fp(handle);

    if (verbose)
        dill_dump(s);

    (*func)(A, B, C, out, (long)N);

    /* Verify results */
    for (i = 0; i < N; i++) {
        double expected = A[i] * A[i] + B[i] * B[i] + C[i] * C[i];
        if (out[i] != expected) {
            if (verbose)
                printf("  array_loop: out[%d] = %g, expected %g\n", i, out[i],
                       expected);
            failure = 1;
        }
    }

    dill_free_handle(handle);
    dill_free_stream(s);
    return failure;
}

/*
 * Test 2: Register pressure overflow.
 *
 * Uses 20 integer vregs that are all live across the loop back-edge.
 * Most architectures have fewer than 20 callee-saved registers,
 * so some vregs must fall back to on-demand allocation with spills.
 * Verifies correctness when pre-assignment runs out of registers.
 *
 * Computes: sum = v0 + v1 + ... + v19, where each vi is initialized
 * to (i+1) and incremented each iteration. After N iterations,
 * vi = (i+1) + N, and sum = sum((i+1)+N for i in 0..19) = 20*N + 210.
 */
static int
test_pressure_overflow(void)
{
    dill_stream s = dill_create_stream();
    dill_exec_handle handle;
    long (*func)(long);
    dill_reg v[20], sum, cnt, limit;
    int L_top, L_end;
    int i, failure = 0;
    const int NREGS = 20;
    const long NITER = 10;

    dill_start_proc(s, "pressure", DILL_L, "%l");
    limit = dill_vparam(s, 0);

    for (i = 0; i < NREGS; i++) {
        v[i] = dill_getreg(s, DILL_L);
    }
    sum = dill_getreg(s, DILL_L);
    cnt = dill_getreg(s, DILL_L);

    L_top = dill_alloc_label(s, "loop_top");
    L_end = dill_alloc_label(s, "loop_end");

    /* Initialize: v[i] = i + 1 */
    for (i = 0; i < NREGS; i++) {
        dill_setl(s, v[i], (IMM_TYPE)(i + 1));
    }
    dill_setl(s, cnt, (IMM_TYPE)0);

    /* Loop: increment all vregs each iteration */
    dill_mark_label(s, L_top);
    dill_bgel(s, cnt, limit, L_end);
    for (i = 0; i < NREGS; i++) {
        dill_addli(s, v[i], v[i], (IMM_TYPE)1);
    }
    dill_addli(s, cnt, cnt, (IMM_TYPE)1);
    dill_jv(s, L_top);
    dill_mark_label(s, L_end);

    /* Sum all vregs */
    dill_movl(s, sum, v[0]);
    for (i = 1; i < NREGS; i++) {
        dill_addl(s, sum, sum, v[i]);
    }
    dill_retl(s, sum);

    handle = dill_finalize(s);
    func = (long (*)(long))dill_get_fp(handle);

    if (verbose)
        dill_dump(s);

    {
        long result = (*func)(NITER);
        /* sum = sum_{i=0}^{19} ((i+1) + NITER) = 20*NITER + 20*21/2 */
        long expected = NREGS * NITER + (NREGS * (NREGS + 1)) / 2;
        if (result != expected) {
            if (verbose)
                printf("  pressure: result = %ld, expected %ld\n", result,
                       expected);
            failure = 1;
        }
    }

    dill_free_handle(handle);
    dill_free_stream(s);
    return failure;
}

/*
 * Test 3: Mixed int + float cross-block registers.
 *
 * A loop where both integer vregs (pointers, index) and float vregs
 * (running sums) are live across the back-edge.
 *
 * Computes: sumA = sum(A[i]), sumB = sum(B[i]) for i in [0, N).
 * Returns sumA + sumB as a double.
 */
static int
test_mixed_loop(void)
{
    dill_stream s = dill_create_stream();
    dill_exec_handle handle;
    double (*func)(double*, double*, long);
    dill_reg pA, pB, off, limit;
    dill_reg sumA, sumB, total, tmp;
    int L_top, L_end;
    int i, failure = 0;
    const int N = 32;
    double A[32], B[32];

    for (i = 0; i < N; i++) {
        A[i] = (double)(i + 1);       /* 1, 2, ..., 32 */
        B[i] = (double)(i + 1) * 0.5; /* 0.5, 1.0, ..., 16.0 */
    }

    dill_start_proc(s, "mixed_loop", DILL_D, "%p%p%l");
    pA = dill_vparam(s, 0);
    pB = dill_vparam(s, 1);
    limit = dill_vparam(s, 2);

    off = dill_getreg(s, DILL_L);
    sumA = dill_getreg(s, DILL_D);
    sumB = dill_getreg(s, DILL_D);
    total = dill_getreg(s, DILL_D);
    tmp = dill_getreg(s, DILL_D);

    L_top = dill_alloc_label(s, "loop_top");
    L_end = dill_alloc_label(s, "loop_end");

    dill_mulli(s, limit, limit, (IMM_TYPE)8);
    dill_setl(s, off, (IMM_TYPE)0);
    dill_setd(s, sumA, 0.0);
    dill_setd(s, sumB, 0.0);

    dill_mark_label(s, L_top);
    dill_bgel(s, off, limit, L_end);

    dill_ldd(s, tmp, pA, off);
    dill_addd(s, sumA, sumA, tmp);

    dill_ldd(s, tmp, pB, off);
    dill_addd(s, sumB, sumB, tmp);

    dill_addli(s, off, off, (IMM_TYPE)8);
    dill_jv(s, L_top);
    dill_mark_label(s, L_end);

    dill_addd(s, total, sumA, sumB);
    dill_retd(s, total);

    handle = dill_finalize(s);
    func = (double (*)(double*, double*, long))dill_get_fp(handle);

    if (verbose)
        dill_dump(s);

    {
        double result = (*func)(A, B, (long)N);
        /* sumA = 32*33/2 = 528, sumB = 528*0.5 = 264, total = 792 */
        double expected = 792.0;
        if (result != expected) {
            if (verbose)
                printf("  mixed_loop: result = %g, expected %g\n", result,
                       expected);
            failure = 1;
        }
    }

    dill_free_handle(handle);
    dill_free_stream(s);
    return failure;
}

/*
 * Test 4: Nested loops.
 *
 * Outer loop iterates over rows, inner loop over columns.
 * Computes: out[i] = sum(M[i][j] for j in [0, COLS))
 * M is stored as a flat array M[i*COLS + j].
 *
 * Outer-loop vregs (row index, output pointer, matrix pointer)
 * must survive across ALL inner-loop basic blocks.
 */
static int
test_nested_loop(void)
{
    dill_stream s = dill_create_stream();
    dill_exec_handle handle;
    void (*func)(double*, double*, long, long);
    dill_reg pM, pOut, nrows, ncols;
    dill_reg row, col, row_off, elem_off;
    dill_reg sum, tmp;
    int L_outer, L_outer_end, L_inner, L_inner_end;
    int i, j, failure = 0;
    const int ROWS = 8, COLS = 8;
    double M[64], out[8];

    for (i = 0; i < ROWS; i++)
        for (j = 0; j < COLS; j++)
            M[i * COLS + j] = (double)(i * COLS + j + 1);
    memset(out, 0, sizeof(out));

    dill_start_proc(s, "nested_loop", DILL_V, "%p%p%l%l");
    pM = dill_vparam(s, 0);
    pOut = dill_vparam(s, 1);
    nrows = dill_vparam(s, 2);
    ncols = dill_vparam(s, 3);

    row = dill_getreg(s, DILL_L);
    col = dill_getreg(s, DILL_L);
    row_off = dill_getreg(s, DILL_L);
    elem_off = dill_getreg(s, DILL_L);
    sum = dill_getreg(s, DILL_D);
    tmp = dill_getreg(s, DILL_D);

    L_outer = dill_alloc_label(s, "outer_top");
    L_outer_end = dill_alloc_label(s, "outer_end");
    L_inner = dill_alloc_label(s, "inner_top");
    L_inner_end = dill_alloc_label(s, "inner_end");

    /* Convert counts to byte counts */
    dill_mulli(s, nrows, nrows, (IMM_TYPE)8);
    dill_mulli(s, ncols, ncols, (IMM_TYPE)8);

    dill_setl(s, row, (IMM_TYPE)0);
    dill_setl(s, row_off, (IMM_TYPE)0);

    /* Outer loop: rows */
    dill_mark_label(s, L_outer);
    dill_bgel(s, row, nrows, L_outer_end);

    dill_setd(s, sum, 0.0);
    dill_setl(s, col, (IMM_TYPE)0);

    /* Inner loop: columns */
    dill_mark_label(s, L_inner);
    dill_bgel(s, col, ncols, L_inner_end);

    /* elem_off = row_off + col (both in bytes already) */
    dill_addl(s, elem_off, row_off, col);
    dill_ldd(s, tmp, pM, elem_off);
    dill_addd(s, sum, sum, tmp);

    dill_addli(s, col, col, (IMM_TYPE)8);
    dill_jv(s, L_inner);
    dill_mark_label(s, L_inner_end);

    /* Store row sum: out[row] = sum */
    dill_std(s, sum, pOut, row);

    dill_addli(s, row, row, (IMM_TYPE)8);
    /* row_off += ncols (advance by one row of bytes) */
    dill_addl(s, row_off, row_off, ncols);
    dill_jv(s, L_outer);
    dill_mark_label(s, L_outer_end);
    dill_retii(s, 0);

    handle = dill_finalize(s);
    func = (void (*)(double*, double*, long, long))dill_get_fp(handle);

    if (verbose)
        dill_dump(s);

    (*func)(M, out, (long)ROWS, (long)COLS);

    for (i = 0; i < ROWS; i++) {
        double expected = 0.0;
        for (j = 0; j < COLS; j++)
            expected += M[i * COLS + j];
        if (out[i] != expected) {
            if (verbose)
                printf("  nested_loop: out[%d] = %g, expected %g\n", i, out[i],
                       expected);
            failure = 1;
        }
    }

    dill_free_handle(handle);
    dill_free_stream(s);
    return failure;
}

/*
 * Test 5: Diamond control flow (if/else merge).
 *
 * Computes: if (sel) result = a*a + b*b; else result = a + b;
 * Where a, b are pre-assigned vregs that survive across both branches.
 *
 * BB0: define a, b, sel → branch
 * BB1 (then): t = a*a + b*b → jump merge
 * BB2 (else): t = a + b → fall through
 * BB3 (merge): return t + a (a must still be valid here)
 */
static int
test_diamond(void)
{
    dill_stream s = dill_create_stream();
    dill_exec_handle handle;
    long (*func)(long, long, long);
    dill_reg a, b, sel, t;
    int L_else, L_merge;
    int failure = 0;

    dill_start_proc(s, "diamond", DILL_L, "%l%l%l");
    a = dill_vparam(s, 0);
    b = dill_vparam(s, 1);
    sel = dill_vparam(s, 2);

    t = dill_getreg(s, DILL_L);

    L_else = dill_alloc_label(s, "else");
    L_merge = dill_alloc_label(s, "merge");

    /* BB0: branch on sel */
    dill_beqli(s, sel, (IMM_TYPE)0, L_else);

    /* BB1 (then): t = a*a + b*b */
    dill_mull(s, t, a, a);
    {
        dill_reg tmp = dill_getreg(s, DILL_L);
        dill_mull(s, tmp, b, b);
        dill_addl(s, t, t, tmp);
    }
    dill_jv(s, L_merge);

    /* BB2 (else): t = a + b */
    dill_mark_label(s, L_else);
    dill_addl(s, t, a, b);

    /* BB3 (merge): return t + a */
    dill_mark_label(s, L_merge);
    dill_addl(s, t, t, a);
    dill_retl(s, t);

    handle = dill_finalize(s);
    func = (long (*)(long, long, long))dill_get_fp(handle);

    if (verbose)
        dill_dump(s);

    {
        /* sel=1: t = 3*3 + 4*4 = 25, result = 25 + 3 = 28 */
        long r1 = (*func)(3, 4, 1);
        /* sel=0: t = 3 + 4 = 7, result = 7 + 3 = 10 */
        long r2 = (*func)(3, 4, 0);
        if (r1 != 28) {
            if (verbose)
                printf("  diamond(sel=1): result = %ld, expected 28\n", r1);
            failure = 1;
        }
        if (r2 != 10) {
            if (verbose)
                printf("  diamond(sel=0): result = %ld, expected 10\n", r2);
            failure = 1;
        }
    }

    dill_free_handle(handle);
    dill_free_stream(s);
    return failure;
}

/*
 * Test 6: Loop with function call.
 *
 * A loop that calls an external function each iteration.
 * Pre-assigned callee-saved registers (pointers, index) must
 * survive the call. Verifies values are correct after each call.
 *
 * Uses int/DILL_I for the call and accumulator (not long) because
 * C 'long' is 32-bit on Windows but DILL_L is 64-bit.
 *
 * Computes: sum += scale_val(A[i]) for i in [0, N)
 * where scale_val(x) returns x * 2.
 */
static int
scale_val(int x)
{
    return x * 2;
}

static int
test_loop_with_call(void)
{
    dill_stream s = dill_create_stream();
    dill_exec_handle handle;
    int (*func)(int*, int);
    dill_reg pA, limit, off, sum, tmp, ret;
    int L_top, L_end;
    int i, failure = 0;
    const int N = 16;
    int A[16];

    for (i = 0; i < N; i++)
        A[i] = i + 1;

    dill_start_proc(s, "loop_call", DILL_I, "%p%i");
    pA = dill_vparam(s, 0);
    limit = dill_vparam(s, 1);

    off = dill_getreg(s, DILL_L);
    sum = dill_getreg(s, DILL_I);
    tmp = dill_getreg(s, DILL_I);

    L_top = dill_alloc_label(s, "loop_top");
    L_end = dill_alloc_label(s, "loop_end");

    dill_mulli(s, limit, limit, (IMM_TYPE)4);
    dill_setl(s, off, (IMM_TYPE)0);
    dill_seti(s, sum, 0);

    dill_mark_label(s, L_top);
    dill_bgel(s, off, limit, L_end);

    /* tmp = A[off] */
    dill_ldi(s, tmp, pA, off);

    /* call scale_val(tmp) */
    dill_push_init(s);
    dill_push_argi(s, tmp);
    ret = dill_calli(s, (void*)scale_val, "scale_val");

    /* sum += return value */
    dill_addi(s, sum, sum, ret);

    dill_addli(s, off, off, (IMM_TYPE)4);
    dill_jv(s, L_top);
    dill_mark_label(s, L_end);

    dill_reti(s, sum);

    handle = dill_finalize(s);
    func = (int (*)(int*, int))dill_get_fp(handle);

    if (verbose)
        dill_dump(s);

    {
        int result = (*func)(A, N);
        /* sum = 2 * sum(1..16) = 2 * 136 = 272 */
        int expected = 272;
        if (result != expected) {
            if (verbose)
                printf("  loop_call: result = %d, expected %d\n", result,
                       expected);
            failure = 1;
        }
    }

    dill_free_handle(handle);
    dill_free_stream(s);
    return failure;
}

int
main(int argc, char** argv)
{
    int failure = 0;

    if (argc > 1)
        verbose++;

    printf("Test 1: array loop (A*A+B*B+C*C)... ");
    if (test_array_loop()) {
        printf("FAILED\n");
        failure = 1;
    } else {
        printf("passed\n");
    }

    printf("Test 2: register pressure overflow (20 cross-block vregs)... ");
    if (test_pressure_overflow()) {
        printf("FAILED\n");
        failure = 1;
    } else {
        printf("passed\n");
    }

    printf("Test 3: mixed int+float loop... ");
    if (test_mixed_loop()) {
        printf("FAILED\n");
        failure = 1;
    } else {
        printf("passed\n");
    }

    printf("Test 4: nested loops... ");
    if (test_nested_loop()) {
        printf("FAILED\n");
        failure = 1;
    } else {
        printf("passed\n");
    }

    printf("Test 5: diamond control flow (if/else merge)... ");
    if (test_diamond()) {
        printf("FAILED\n");
        failure = 1;
    } else {
        printf("passed\n");
    }

    printf("Test 6: loop with function call... ");
    if (test_loop_with_call()) {
        printf("FAILED\n");
        failure = 1;
    } else {
        printf("passed\n");
    }

    if (!failure) {
        printf("No errors!\n");
    }
    return failure;
}
