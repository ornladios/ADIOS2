#include "../config.h"
#include "stdio.h"
#ifdef HAVE_MALLOC_H
#include "malloc.h"
#endif
#include <stdlib.h>
#include <stdint.h>
#ifdef HAVE_UNISTD_H
#include "unistd.h"
#endif

#include "dill.h"
#ifdef USE_MMAP_CODE_SEG
#include "sys/mman.h"
#endif

static int verbose = 0;

void a () {
    dill_stream s = dill_create_stream();
    dill_reg a;
    dill_exec_ctx ec;
    dill_exec_handle handle;
    int (*ip)(dill_exec_ctx, int);
    int result;
    dill_start_proc(s, "a_gen", DILL_I, "%EC%i");
    
    a = dill_getreg(s, DILL_I);
    dill_seti(s, a, 0);
    dill_addii(s, a, a, 5);
    dill_addii(s, a, a, 15);
    dill_addi(s, a, a, dill_vparam(s, 1));
    dill_addii(s, a, a, 20);
    dill_reti(s, a);
    handle = dill_finalize(s);
    ip = (int(*)(dill_exec_ctx, int))dill_get_fp(handle);
    
    if (verbose) dill_dump(s);
     
    ec = dill_get_exec_context(s);
    result = (*ip)(ec, 1);
    if (result != 41) {
	printf("A failed.  Expected 41, got %d\n", result);
    }
    dill_free_exec_context(ec);
    dill_free_stream(s);
    dill_free_handle(handle);
}

void b () {
    dill_stream s = dill_create_stream();
    dill_reg a;
    dill_exec_ctx ec;
    dill_exec_handle handle;
    int (*ip)(dill_exec_ctx, int);
    int result;
    dill_start_proc(s, "a_gen", DILL_I, "%EC%i");
    
    a = dill_getreg(s, DILL_I);
    dill_seti(s, a, 0);
    dill_addii(s, a, a, 5);
    dill_addii(s, a, a, 15);
    dill_addii(s, a, a, 20);
    dill_addii(s, a, a, 1);
    dill_reti(s, a);
    handle = dill_finalize(s);
    ip = (int(*)(dill_exec_ctx, int))dill_get_fp(handle);
    
    if (verbose) dill_dump(s);
     
    ec = dill_get_exec_context(s);
    result = (*ip)(ec, 1);
    if (result != 41) {
	printf("B failed.  Expected 41, got %d\n", result);
    }
    dill_free_exec_context(ec);
    dill_free_stream(s);
    dill_free_handle(handle);
}

/* muli by powers of two is strength-reduced to shifts at emit time; verify
 * the values are right across types, including x1 (becomes mov), a non-power
 * (stays mul), negative operands, and a count at the edge of the type. */
static void
mulshift()
{
    static struct {
        intptr_t imm;
        intptr_t in;
        intptr_t want_l;
        int want_i;
    } cases[] = {
        {1, 42, 42, 42},
        {2, -3, -6, -6},
        {8, 7, 56, 56},
        {4096, -5, -20480, -20480},
        {6, 7, 42, 42}, /* non-power: real mul */
#if INTPTR_MAX > INT32_MAX
        /* shift count above the 32-bit type's width: reduced for l, left as
         * a (truncating) mul for i.  64-bit targets only -- the constant
         * itself does not exist on a 32-bit intptr_t. */
        {((intptr_t)1) << 33, 3, ((intptr_t)3) << 33, 0},
#endif
    };
    size_t i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        dill_stream s = dill_create_stream();
        dill_exec_handle h;
        dill_reg a;
        intptr_t lres;
        int ires;
        intptr_t (*lp)(intptr_t);
        int (*iap)(int);

        dill_start_proc(s, "mull_gen", DILL_L, "%l");
        a = dill_getreg(s, DILL_L);
        dill_mulli(s, a, dill_vparam(s, 0), cases[i].imm);
        dill_retl(s, a);
        h = dill_finalize(s);
        lp = (intptr_t(*)(intptr_t))dill_get_fp(h);
        lres = lp(cases[i].in);
        if (lres != cases[i].want_l) {
            printf("mulshift L case %d: got %lld, expected %lld\n", (int)i,
                   (long long)lres, (long long)cases[i].want_l);
        }
        dill_free_handle(h);
        dill_free_stream(s);

        s = dill_create_stream();
        dill_start_proc(s, "muli_gen", DILL_I, "%i");
        a = dill_getreg(s, DILL_I);
        dill_mulii(s, a, dill_vparam(s, 0), (int)cases[i].imm);
        dill_reti(s, a);
        h = dill_finalize(s);
        iap = (int (*)(int))dill_get_fp(h);
        ires = iap((int)cases[i].in);
        if (ires != cases[i].want_i) {
            printf("mulshift I case %d: got %d, expected %d\n", (int)i, ires,
                   cases[i].want_i);
        }
        dill_free_handle(h);
        dill_free_stream(s);
    }
}

int 
main(int argc, char **argv)
{
    if (argc > 1) verbose++;
    if (verbose) printf("########## A\n");
    a();
    if (verbose) printf("########## B\n");
    b();
    if (verbose) printf("########## C\n");
    mulshift();
    if (verbose) printf("########## end\n");
    return 0;
}
