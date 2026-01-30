/* This file is generated from general.ops.  Do not edit directly. */

#include "config.h"
#include "dill.h"
#include "stdio.h"
#include "math.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <stdint.h>

#ifdef _MSC_VER
#include <stdlib.h>
#define srand48(s) srand(s)
#define drand48() (rand()*(1./RAND_MAX))
#define lrand48() ((long long)rand() << 32 | rand())
#define kill(x,y)
#else
extern double drand48();
extern IMM_TYPE lrand48();
void srand48(IMM_TYPE seedval);
#endif

/* Global test data - initialized in main() */
static int verbose;
static float rand1_f;
static float rand2_f;
static float src1f_vals[2];
static float src2f_vals[2];
static float br_srcf_vals[6];
static short rand1_s;
static short rand2_s;
static short src1s_vals[2];
static short src2s_vals[2];
static short br_srcs_vals[6];
static IMM_TYPE rand1_l;
static IMM_TYPE rand2_l;
static IMM_TYPE src1l_vals[2];
static IMM_TYPE src2l_vals[2];
static IMM_TYPE br_srcl_vals[6];
static unsigned char rand1_uc;
static unsigned char rand2_uc;
static unsigned char src1uc_vals[2];
static unsigned char src2uc_vals[2];
static unsigned char br_srcuc_vals[6];
static unsigned short rand1_us;
static unsigned short rand2_us;
static unsigned short src1us_vals[2];
static unsigned short src2us_vals[2];
static unsigned short br_srcus_vals[6];
static UIMM_TYPE rand1_ul;
static UIMM_TYPE rand2_ul;
static UIMM_TYPE src1ul_vals[2];
static UIMM_TYPE src2ul_vals[2];
static UIMM_TYPE br_srcul_vals[6];
static signed char rand1_c;
static signed char rand2_c;
static signed char src1c_vals[2];
static signed char src2c_vals[2];
static signed char br_srcc_vals[6];
static double rand1_d;
static double rand2_d;
static double src1d_vals[2];
static double src2d_vals[2];
static double br_srcd_vals[6];
static unsigned int rand1_u;
static unsigned int rand2_u;
static unsigned int src1u_vals[2];
static unsigned int src2u_vals[2];
static unsigned int br_srcu_vals[6];
static int rand1_i;
static int rand2_i;
static int src1i_vals[2];
static int src2i_vals[2];
static int br_srci_vals[6];
static char* rand1_p;
static char* rand2_p;
static char* src1p_vals[2];
static char* src2p_vals[2];
static char* br_srcp_vals[6];
static int sh_src2_vals[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

static UIMM_TYPE bit_pattern_vals[] = { 0x1, 0x0,  0x2, 0x1,  0x4, 0x3,  0x8, 0x7,  0x10, 0xf,  0x20, 0x1f,  0x40, 0x3f,  0x80, 0x7f,  0x100, 0xff,  0x200, 0x1ff,  0x400, 0x3ff,  0x800, 0x7ff,  0x1000, 0xfff,  0x2000, 0x1fff,  0x4000, 0x3fff,  0x8000, 0x7fff,  0x10000, 0xffff,  0x20000, 0x1ffff,  0x40000, 0x3ffff,  0x80000, 0x7ffff,  0x100000, 0xfffff,  0x200000, 0x1fffff,  0x400000, 0x3fffff,  0x800000, 0x7fffff,  0x1000000, 0xffffff,  0x2000000, 0x1ffffff,  0x4000000, 0x3ffffff,  0x8000000, 0x7ffffff,  0x10000000, 0xfffffff,  0x20000000, 0x1fffffff,  0x40000000, 0x3fffffff,  0x80000000, 0x7fffffff,  0xffffffff};


static int
test_arith_int(dill_stream c)
{
    int failed = 0;
    size_t i, j;
    (void)i; (void)j;  /* suppress unused warnings */

    /* test for dill_addi */
    if (verbose) printf("test for dill_addi");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = (int) src1i_vals[i];
        for (j=0 ; j < sizeof(src2i_vals)/sizeof(src2i_vals[0]) ; j++) {
            int source2_i = (int) src2i_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_I);
	    dill_addi(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx ec, int a, int b)) dill_get_fp(h);
	    expected_result = source1_i + source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_addi test, expected %d, got %d, for %d + %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_addu */
    if (verbose) printf("test for dill_addu");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = (unsigned int) src1u_vals[i];
        for (j=0 ; j < sizeof(src2u_vals)/sizeof(src2u_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int) src2u_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_U);
	    dill_addu(c, dest, opnd1, opnd2);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx ec, unsigned int a, unsigned int b)) dill_get_fp(h);
	    expected_result = source1_u + source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_addu test, expected %u, got %u, for %u + %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_addul */
    if (verbose) printf("test for dill_addul");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = (UIMM_TYPE) src1ul_vals[i];
        for (j=0 ; j < sizeof(src2ul_vals)/sizeof(src2ul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE) src2ul_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_UL);
	    dill_addul(c, dest, opnd1, opnd2);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_ul + source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_addul test, expected %zu, got %zu, for %zu + %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_addl */
    if (verbose) printf("test for dill_addl");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = (IMM_TYPE) src1l_vals[i];
        for (j=0 ; j < sizeof(src2l_vals)/sizeof(src2l_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE) src2l_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_L);
	    dill_addl(c, dest, opnd1, opnd2);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_l + source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_addl test, expected %zx, got %zx, for %zx + %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_subi */
    if (verbose) printf("test for dill_subi");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = (int) src1i_vals[i];
        for (j=0 ; j < sizeof(src2i_vals)/sizeof(src2i_vals[0]) ; j++) {
            int source2_i = (int) src2i_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_I);
	    dill_subi(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx ec, int a, int b)) dill_get_fp(h);
	    expected_result = source1_i - source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_subi test, expected %d, got %d, for %d - %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_subu */
    if (verbose) printf("test for dill_subu");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = (unsigned int) src1u_vals[i];
        for (j=0 ; j < sizeof(src2u_vals)/sizeof(src2u_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int) src2u_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_U);
	    dill_subu(c, dest, opnd1, opnd2);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx ec, unsigned int a, unsigned int b)) dill_get_fp(h);
	    expected_result = source1_u - source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_subu test, expected %u, got %u, for %u - %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_subul */
    if (verbose) printf("test for dill_subul");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = (UIMM_TYPE) src1ul_vals[i];
        for (j=0 ; j < sizeof(src2ul_vals)/sizeof(src2ul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE) src2ul_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_UL);
	    dill_subul(c, dest, opnd1, opnd2);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_ul - source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_subul test, expected %zu, got %zu, for %zu - %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_subl */
    if (verbose) printf("test for dill_subl");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = (IMM_TYPE) src1l_vals[i];
        for (j=0 ; j < sizeof(src2l_vals)/sizeof(src2l_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE) src2l_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_L);
	    dill_subl(c, dest, opnd1, opnd2);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_l - source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_subl test, expected %zx, got %zx, for %zx - %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_muli */
    if (verbose) printf("test for dill_muli");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = (int) src1i_vals[i];
        for (j=0 ; j < sizeof(src2i_vals)/sizeof(src2i_vals[0]) ; j++) {
            int source2_i = (int) src2i_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_I);
	    dill_muli(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx ec, int a, int b)) dill_get_fp(h);
	    expected_result = source1_i * source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_muli test, expected %d, got %d, for %d * %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_mulu */
    if (verbose) printf("test for dill_mulu");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = (unsigned int) src1u_vals[i];
        for (j=0 ; j < sizeof(src2u_vals)/sizeof(src2u_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int) src2u_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_U);
	    dill_mulu(c, dest, opnd1, opnd2);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx ec, unsigned int a, unsigned int b)) dill_get_fp(h);
	    expected_result = source1_u * source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_mulu test, expected %u, got %u, for %u * %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_mulul */
    if (verbose) printf("test for dill_mulul");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = (UIMM_TYPE) src1ul_vals[i];
        for (j=0 ; j < sizeof(src2ul_vals)/sizeof(src2ul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE) src2ul_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_UL);
	    dill_mulul(c, dest, opnd1, opnd2);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_ul * source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_mulul test, expected %zu, got %zu, for %zu * %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_mull */
    if (verbose) printf("test for dill_mull");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = (IMM_TYPE) src1l_vals[i];
        for (j=0 ; j < sizeof(src2l_vals)/sizeof(src2l_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE) src2l_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_L);
	    dill_mull(c, dest, opnd1, opnd2);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_l * source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_mull test, expected %zx, got %zx, for %zx * %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_divi */
    if (verbose) printf("test for dill_divi");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = (int) src1i_vals[i];
        for (j=0 ; j < sizeof(src2i_vals)/sizeof(src2i_vals[0]) ; j++) {
            int source2_i = (int) src2i_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_i == 0) goto skip;
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_I);
	    dill_divi(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx ec, int a, int b)) dill_get_fp(h);
	    expected_result = source1_i / source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_divi test, expected %d, got %d, for %d / %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
skip: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_divu */
    if (verbose) printf("test for dill_divu");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = (unsigned int) src1u_vals[i];
        for (j=0 ; j < sizeof(src2u_vals)/sizeof(src2u_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int) src2u_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_u == 0) goto skip1;
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_U);
	    dill_divu(c, dest, opnd1, opnd2);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx ec, unsigned int a, unsigned int b)) dill_get_fp(h);
	    expected_result = source1_u / source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_divu test, expected %u, got %u, for %u / %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
skip1: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_divul */
    if (verbose) printf("test for dill_divul");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = (UIMM_TYPE) src1ul_vals[i];
        for (j=0 ; j < sizeof(src2ul_vals)/sizeof(src2ul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE) src2ul_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_ul == 0) goto skip2;
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_UL);
	    dill_divul(c, dest, opnd1, opnd2);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_ul / source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_divul test, expected %zu, got %zu, for %zu / %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
skip2: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_divl */
    if (verbose) printf("test for dill_divl");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = (IMM_TYPE) src1l_vals[i];
        for (j=0 ; j < sizeof(src2l_vals)/sizeof(src2l_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE) src2l_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_l == 0) goto skip3;
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_L);
	    dill_divl(c, dest, opnd1, opnd2);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_l / source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_divl test, expected %zx, got %zx, for %zx / %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
skip3: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_modi */
    if (verbose) printf("test for dill_modi");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = (int) src1i_vals[i];
        for (j=0 ; j < sizeof(src2i_vals)/sizeof(src2i_vals[0]) ; j++) {
            int source2_i = (int) src2i_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_i <= 0) goto skip4;if (source2_i  > 0) goto skip4;
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_I);
	    dill_modi(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx ec, int a, int b)) dill_get_fp(h);
	    expected_result = source1_i % source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_modi test, expected %d, got %d, for %d %% %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
skip4: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_modu */
    if (verbose) printf("test for dill_modu");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = (unsigned int) src1u_vals[i];
        for (j=0 ; j < sizeof(src2u_vals)/sizeof(src2u_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int) src2u_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_u <= 0) goto skip5;if (source2_u  > 0) goto skip5;
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_U);
	    dill_modu(c, dest, opnd1, opnd2);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx ec, unsigned int a, unsigned int b)) dill_get_fp(h);
	    expected_result = source1_u % source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_modu test, expected %u, got %u, for %u %% %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
skip5: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_modul */
    if (verbose) printf("test for dill_modul");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = (UIMM_TYPE) src1ul_vals[i];
        for (j=0 ; j < sizeof(src2ul_vals)/sizeof(src2ul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE) src2ul_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_ul <= 0) goto skip6;if (source2_ul  > 0) goto skip6;
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_UL);
	    dill_modul(c, dest, opnd1, opnd2);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_ul % source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_modul test, expected %zu, got %zu, for %zu %% %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
skip6: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_modl */
    if (verbose) printf("test for dill_modl");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = (IMM_TYPE) src1l_vals[i];
        for (j=0 ; j < sizeof(src2l_vals)/sizeof(src2l_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE) src2l_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_l <= 0) goto skip7;if (source2_l  > 0) goto skip7;
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_L);
	    dill_modl(c, dest, opnd1, opnd2);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_l % source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_modl test, expected %zx, got %zx, for %zx %% %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
skip7: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_andi */
    if (verbose) printf("test for dill_andi");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = (int) src1i_vals[i];
        for (j=0 ; j < sizeof(src2i_vals)/sizeof(src2i_vals[0]) ; j++) {
            int source2_i = (int) src2i_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_I);
	    dill_andi(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx ec, int a, int b)) dill_get_fp(h);
	    expected_result = source1_i & source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_andi test, expected %d, got %d, for %d & %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_andu */
    if (verbose) printf("test for dill_andu");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = (unsigned int) src1u_vals[i];
        for (j=0 ; j < sizeof(src2u_vals)/sizeof(src2u_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int) src2u_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_U);
	    dill_andu(c, dest, opnd1, opnd2);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx ec, unsigned int a, unsigned int b)) dill_get_fp(h);
	    expected_result = source1_u & source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_andu test, expected %u, got %u, for %u & %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_andul */
    if (verbose) printf("test for dill_andul");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = (UIMM_TYPE) src1ul_vals[i];
        for (j=0 ; j < sizeof(src2ul_vals)/sizeof(src2ul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE) src2ul_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_UL);
	    dill_andul(c, dest, opnd1, opnd2);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_ul & source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_andul test, expected %zu, got %zu, for %zu & %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_andl */
    if (verbose) printf("test for dill_andl");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = (IMM_TYPE) src1l_vals[i];
        for (j=0 ; j < sizeof(src2l_vals)/sizeof(src2l_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE) src2l_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_L);
	    dill_andl(c, dest, opnd1, opnd2);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_l & source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_andl test, expected %zx, got %zx, for %zx & %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ori */
    if (verbose) printf("test for dill_ori");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = (int) src1i_vals[i];
        for (j=0 ; j < sizeof(src2i_vals)/sizeof(src2i_vals[0]) ; j++) {
            int source2_i = (int) src2i_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_I);
	    dill_ori(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx ec, int a, int b)) dill_get_fp(h);
	    expected_result = source1_i | source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ori test, expected %d, got %d, for %d | %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_oru */
    if (verbose) printf("test for dill_oru");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = (unsigned int) src1u_vals[i];
        for (j=0 ; j < sizeof(src2u_vals)/sizeof(src2u_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int) src2u_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_U);
	    dill_oru(c, dest, opnd1, opnd2);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx ec, unsigned int a, unsigned int b)) dill_get_fp(h);
	    expected_result = source1_u | source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_oru test, expected %u, got %u, for %u | %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_orul */
    if (verbose) printf("test for dill_orul");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = (UIMM_TYPE) src1ul_vals[i];
        for (j=0 ; j < sizeof(src2ul_vals)/sizeof(src2ul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE) src2ul_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_UL);
	    dill_orul(c, dest, opnd1, opnd2);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_ul | source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_orul test, expected %zu, got %zu, for %zu | %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_orl */
    if (verbose) printf("test for dill_orl");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = (IMM_TYPE) src1l_vals[i];
        for (j=0 ; j < sizeof(src2l_vals)/sizeof(src2l_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE) src2l_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_L);
	    dill_orl(c, dest, opnd1, opnd2);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_l | source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_orl test, expected %zx, got %zx, for %zx | %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_xori */
    if (verbose) printf("test for dill_xori");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = (int) src1i_vals[i];
        for (j=0 ; j < sizeof(src2i_vals)/sizeof(src2i_vals[0]) ; j++) {
            int source2_i = (int) src2i_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_I);
	    dill_xori(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx ec, int a, int b)) dill_get_fp(h);
	    expected_result = source1_i ^ source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_xori test, expected %d, got %d, for %d ^ %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_xoru */
    if (verbose) printf("test for dill_xoru");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = (unsigned int) src1u_vals[i];
        for (j=0 ; j < sizeof(src2u_vals)/sizeof(src2u_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int) src2u_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_U);
	    dill_xoru(c, dest, opnd1, opnd2);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx ec, unsigned int a, unsigned int b)) dill_get_fp(h);
	    expected_result = source1_u ^ source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_xoru test, expected %u, got %u, for %u ^ %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_xorul */
    if (verbose) printf("test for dill_xorul");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = (UIMM_TYPE) src1ul_vals[i];
        for (j=0 ; j < sizeof(src2ul_vals)/sizeof(src2ul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE) src2ul_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_UL);
	    dill_xorul(c, dest, opnd1, opnd2);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_ul ^ source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_xorul test, expected %zu, got %zu, for %zu ^ %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_xorl */
    if (verbose) printf("test for dill_xorl");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = (IMM_TYPE) src1l_vals[i];
        for (j=0 ; j < sizeof(src2l_vals)/sizeof(src2l_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE) src2l_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_L);
	    dill_xorl(c, dest, opnd1, opnd2);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_l ^ source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_xorl test, expected %zx, got %zx, for %zx ^ %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lshi */
    if (verbose) printf("test for dill_lshi");
    {
        int source1_i = rand1_i;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            int source2_i = (int) sh_src2_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_i >= sizeof(IMM_TYPE)) goto skip8;
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_I);
	    dill_lshi(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx ec, int a, int b)) dill_get_fp(h);
	    expected_result = source1_i << source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lshi test, expected %d, got %d, for %d << %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
skip8: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lshu */
    if (verbose) printf("test for dill_lshu");
    {
        unsigned int source1_u = rand1_u;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int) sh_src2_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_u >= sizeof(IMM_TYPE)) goto skip9;
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_U);
	    dill_lshu(c, dest, opnd1, opnd2);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx ec, unsigned int a, unsigned int b)) dill_get_fp(h);
	    expected_result = source1_u << source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lshu test, expected %u, got %u, for %u << %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
skip9: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lshul */
    if (verbose) printf("test for dill_lshul");
    {
        UIMM_TYPE source1_ul = rand1_ul;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE) sh_src2_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_ul >= sizeof(IMM_TYPE)) goto skip10;
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_UL);
	    dill_lshul(c, dest, opnd1, opnd2);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_ul << source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lshul test, expected %zu, got %zu, for %zu << %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
skip10: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lshl */
    if (verbose) printf("test for dill_lshl");
    {
        IMM_TYPE source1_l = rand1_l;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE) sh_src2_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_l >= sizeof(IMM_TYPE)) goto skip11;
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_L);
	    dill_lshl(c, dest, opnd1, opnd2);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_l << source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lshl test, expected %zx, got %zx, for %zx << %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
skip11: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_rshi */
    if (verbose) printf("test for dill_rshi");
    {
        int source1_i = rand1_i;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            int source2_i = (int) sh_src2_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_i >= sizeof(IMM_TYPE)) goto skip12;
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_I);
	    dill_rshi(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx ec, int a, int b)) dill_get_fp(h);
	    expected_result = source1_i >> source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_rshi test, expected %d, got %d, for %d >> %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
skip12: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_rshu */
    if (verbose) printf("test for dill_rshu");
    {
        unsigned int source1_u = rand1_u;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int) sh_src2_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_u >= sizeof(IMM_TYPE)) goto skip13;
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_U);
	    dill_rshu(c, dest, opnd1, opnd2);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx ec, unsigned int a, unsigned int b)) dill_get_fp(h);
	    expected_result = source1_u >> source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_rshu test, expected %u, got %u, for %u >> %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
skip13: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_rshul */
    if (verbose) printf("test for dill_rshul");
    {
        UIMM_TYPE source1_ul = rand1_ul;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE) sh_src2_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_ul >= sizeof(IMM_TYPE)) goto skip14;
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_UL);
	    dill_rshul(c, dest, opnd1, opnd2);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_ul >> source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_rshul test, expected %zu, got %zu, for %zu >> %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
skip14: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_rshl */
    if (verbose) printf("test for dill_rshl");
    {
        IMM_TYPE source1_l = rand1_l;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE) sh_src2_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_l >= sizeof(IMM_TYPE)) goto skip15;
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_L);
	    dill_rshl(c, dest, opnd1, opnd2);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b)) dill_get_fp(h);
	    expected_result = source1_l >> source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_rshl test, expected %zx, got %zx, for %zx >> %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
skip15: ;
	}
    }
    if (verbose) printf(" done\n");
    return failed;
}

static int
test_arith_ptr(dill_stream c)
{
    int failed = 0;
    size_t i, j;
    (void)i; (void)j;  /* suppress unused warnings */

    /* test for dill_addp */
    if (verbose) printf("test for dill_addp");
    {
        char* source1_p = rand1_p;
        for (j=0 ; j < sizeof(src2l_vals)/sizeof(src2l_vals[0]) ; j++) {
            IMM_TYPE source2_l = src2l_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    char* result;
	    char* expected_result;
	    char* (*proc)(dill_exec_ctx ec, char* a, IMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_P, "%EC%p%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_P);
	    dill_addp(c, dest, opnd1, opnd2);
	    dill_retp(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (char*(*)(dill_exec_ctx, char*, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_p + source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_addp test, expected %p, got %p, for %p + %zx\n",
		       (void*) expected_result, (void*) result, (void*) source1_p, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_subp */
    if (verbose) printf("test for dill_subp");
    {
        char* source1_p = rand1_p;
        for (j=0 ; j < sizeof(src2l_vals)/sizeof(src2l_vals[0]) ; j++) {
            IMM_TYPE source2_l = src2l_vals[j];

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    char* result;
	    char* expected_result;
	    char* (*proc)(dill_exec_ctx ec, char* a, IMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_P, "%EC%p%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_P);
	    dill_subp(c, dest, opnd1, opnd2);
	    dill_retp(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (char*(*)(dill_exec_ctx, char*, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_p - source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_subp test, expected %p, got %p, for %p - %zx\n",
		       (void*) expected_result, (void*) result, (void*) source1_p, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");
    return failed;
}

static int
test_arith_float(dill_stream c)
{
    int failed = 0;
    size_t i, j;
    (void)i; (void)j;  /* suppress unused warnings */

    /* test for dill_addf */
    if (verbose) printf("test for dill_addf");
    {
        float source1_f = rand1_f;
        {
            float source2_f = rand2_f;

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;
	    double range = 0.000001 * (fabs((double)source1_f) + fabs((double)source2_f));
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_F, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_F);
	    dill_addf(c, dest, opnd1, opnd2);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx ec, float a, float b)) dill_get_fp(h);
	    expected_result = source1_f + source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_addf test, expected %g, got %g, for %g + %g\n",
		       expected_result, result, source1_f, source2_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_addd */
    if (verbose) printf("test for dill_addd");
    {
        double source1_d = rand1_d;
        {
            double source2_d = rand2_d;

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;
	    double range = 0.000001 * (fabs((double)source1_d) + fabs((double)source2_d));
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_D, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_D);
	    dill_addd(c, dest, opnd1, opnd2);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx ec, double a, double b)) dill_get_fp(h);
	    expected_result = source1_d + source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_addd test, expected %g, got %g, for %g + %g\n",
		       expected_result, result, source1_d, source2_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_subf */
    if (verbose) printf("test for dill_subf");
    {
        float source1_f = rand1_f;
        {
            float source2_f = rand2_f;

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;
	    double range = 0.000001 * (fabs((double)source1_f) + fabs((double)source2_f));
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_F, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_F);
	    dill_subf(c, dest, opnd1, opnd2);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx ec, float a, float b)) dill_get_fp(h);
	    expected_result = source1_f - source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_subf test, expected %g, got %g, for %g - %g\n",
		       expected_result, result, source1_f, source2_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_subd */
    if (verbose) printf("test for dill_subd");
    {
        double source1_d = rand1_d;
        {
            double source2_d = rand2_d;

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;
	    double range = 0.000001 * (fabs((double)source1_d) + fabs((double)source2_d));
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_D, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_D);
	    dill_subd(c, dest, opnd1, opnd2);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx ec, double a, double b)) dill_get_fp(h);
	    expected_result = source1_d - source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_subd test, expected %g, got %g, for %g - %g\n",
		       expected_result, result, source1_d, source2_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_mulf */
    if (verbose) printf("test for dill_mulf");
    {
        float source1_f = rand1_f;
        {
            float source2_f = rand2_f;

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;
	    double range = 0.000001 * (fabs((double)source1_f * (double)source2_f));
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_F, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_F);
	    dill_mulf(c, dest, opnd1, opnd2);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx ec, float a, float b)) dill_get_fp(h);
	    expected_result = source1_f * source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_mulf test, expected %g, got %g, for %g * %g\n",
		       expected_result, result, source1_f, source2_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_muld */
    if (verbose) printf("test for dill_muld");
    {
        double source1_d = rand1_d;
        {
            double source2_d = rand2_d;

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;
	    double range = 0.000001 * (fabs((double)source1_d * (double)source2_d));
	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_D, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_D);
	    dill_muld(c, dest, opnd1, opnd2);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx ec, double a, double b)) dill_get_fp(h);
	    expected_result = source1_d * source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_muld test, expected %g, got %g, for %g * %g\n",
		       expected_result, result, source1_d, source2_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_divf */
    if (verbose) printf("test for dill_divf");
    {
        float source1_f = rand1_f;
        {
            float source2_f = rand2_f;

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;
	    double range = 0.000001 * (fabs((double)source1_f / (double)source2_f));
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_f == 0) goto skip16;
	    dill_start_proc(c, "no name", DILL_F, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_F);
	    dill_divf(c, dest, opnd1, opnd2);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx ec, float a, float b)) dill_get_fp(h);
	    expected_result = source1_f / source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_divf test, expected %g, got %g, for %g / %g\n",
		       expected_result, result, source1_f, source2_f);
		dill_dump(c);
		failed++;
	    }
skip16: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_divd */
    if (verbose) printf("test for dill_divd");
    {
        double source1_d = rand1_d;
        {
            double source2_d = rand2_d;

	    dill_reg opnd1, opnd2, dest;
	    dill_exec_ctx ec;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;
	    double range = 0.000001 * (fabs((double)source1_d / (double)source2_d));
	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_d == 0) goto skip17;
	    dill_start_proc(c, "no name", DILL_D, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dest = dill_getreg(c, DILL_D);
	    dill_divd(c, dest, opnd1, opnd2);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx ec, double a, double b)) dill_get_fp(h);
	    expected_result = source1_d / source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_divd test, expected %g, got %g, for %g / %g\n",
		       expected_result, result, source1_d, source2_d);
		dill_dump(c);
		failed++;
	    }
skip17: ;
	}
    }
    if (verbose) printf(" done\n");
    return failed;
}

static int
test_arith2_int(dill_stream c)
{
    int failed = 0;
    size_t i, j;
    (void)i; (void)j;  /* suppress unused warnings */

    /* test for dill_comi */
    if (verbose) printf("test for dill_comi");
    {
        int source1_i = rand1_i;
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_comi(c, dest, opnd1);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = ~ source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_comi test, expected %d, got %d, for ~ %d\n",
		       expected_result, result, source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_comu */
    if (verbose) printf("test for dill_comu");
    {
        unsigned int source1_u = rand1_u;
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_U, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_comu(c, dest, opnd1);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = ~ source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_comu test, expected %u, got %u, for ~ %u\n",
		       expected_result, result, source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_comul */
    if (verbose) printf("test for dill_comul");
    {
        UIMM_TYPE source1_ul = rand1_ul;
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_comul(c, dest, opnd1);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = ~ source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_comul test, expected %zu, got %zu, for ~ %zu\n",
		       expected_result, result, source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_coml */
    if (verbose) printf("test for dill_coml");
    {
        IMM_TYPE source1_l = rand1_l;
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_L, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_coml(c, dest, opnd1);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = ~ source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_coml test, expected %zx, got %zx, for ~ %zx\n",
		       expected_result, result, source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_noti */
    if (verbose) printf("test for dill_noti");
    {
        int source1_i = rand1_i;
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_noti(c, dest, opnd1);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = ! source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_noti test, expected %d, got %d, for ! %d\n",
		       expected_result, result, source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_notu */
    if (verbose) printf("test for dill_notu");
    {
        unsigned int source1_u = rand1_u;
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_U, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_notu(c, dest, opnd1);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = ! source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_notu test, expected %u, got %u, for ! %u\n",
		       expected_result, result, source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_notul */
    if (verbose) printf("test for dill_notul");
    {
        UIMM_TYPE source1_ul = rand1_ul;
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_notul(c, dest, opnd1);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = ! source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_notul test, expected %zu, got %zu, for ! %zu\n",
		       expected_result, result, source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_notl */
    if (verbose) printf("test for dill_notl");
    {
        IMM_TYPE source1_l = rand1_l;
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_L, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_notl(c, dest, opnd1);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = ! source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_notl test, expected %zx, got %zx, for ! %zx\n",
		       expected_result, result, source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_negi */
    if (verbose) printf("test for dill_negi");
    {
        int source1_i = rand1_i;
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_negi(c, dest, opnd1);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = - source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_negi test, expected %d, got %d, for - %d\n",
		       expected_result, result, source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_negu */
    if (verbose) printf("test for dill_negu");
    {
        unsigned int source1_u = rand1_u;
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_U, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_negu(c, dest, opnd1);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (unsigned) - (int) source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_negu test, expected %u, got %u, for - %u\n",
		       expected_result, result, source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_negul */
    if (verbose) printf("test for dill_negul");
    {
        UIMM_TYPE source1_ul = rand1_ul;
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_negul(c, dest, opnd1);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = (uintptr_t) - (intptr_t) source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_negul test, expected %zu, got %zu, for - %zu\n",
		       expected_result, result, source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_negl */
    if (verbose) printf("test for dill_negl");
    {
        IMM_TYPE source1_l = rand1_l;
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_L, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_negl(c, dest, opnd1);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = - source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_negl test, expected %zx, got %zx, for - %zx\n",
		       expected_result, result, source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");
    return failed;
}

static int
test_arith2_float(dill_stream c)
{
    int failed = 0;
    size_t i, j;
    (void)i; (void)j;  /* suppress unused warnings */

    /* test for dill_negf */
    if (verbose) printf("test for dill_negf");
    {
        float source1_f = rand1_f;
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec, float a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_F, "%EC%f");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_F);
	    dill_negf(c, dest, opnd1);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx, float)) dill_get_fp(h);
	    expected_result = - source1_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_negf test, expected %g, got %g, for - %g\n",
		       expected_result, result, source1_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_negd */
    if (verbose) printf("test for dill_negd");
    {
        double source1_d = rand1_d;
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec, double a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_D, "%EC%d");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_D);
	    dill_negd(c, dest, opnd1);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx, double)) dill_get_fp(h);
	    expected_result = - source1_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_negd test, expected %g, got %g, for - %g\n",
		       expected_result, result, source1_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");
    return failed;
}

static int
test_arithi(dill_stream c)
{
    int failed = 0;
    size_t i, j;
    (void)i; (void)j;  /* suppress unused warnings */

    /* test for dill_addii (immediate) */
    if (verbose) printf("test for dill_addii (immediate)");
    {
        int source1_i = rand1_i;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            int source2_i = (int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_addii(c, dest, opnd1, source2_i);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i + source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_addii (immediate) test, expected %d, got %d, for %d + %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_addui (immediate) */
    if (verbose) printf("test for dill_addui (immediate)");
    {
        unsigned int source1_u = rand1_u;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_addui(c, dest, opnd1, source2_u);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u + source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_addui (immediate) test, expected %u, got %u, for %u + %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_adduli (immediate) */
    if (verbose) printf("test for dill_adduli (immediate)");
    {
        UIMM_TYPE source1_ul = rand1_ul;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_adduli(c, dest, opnd1, source2_ul);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul + source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_adduli (immediate) test, expected %zu, got %zu, for %zu + %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_addli (immediate) */
    if (verbose) printf("test for dill_addli (immediate)");
    {
        IMM_TYPE source1_l = rand1_l;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_addli(c, dest, opnd1, source2_l);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l + source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_addli (immediate) test, expected %zx, got %zx, for %zx + %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_subii (immediate) */
    if (verbose) printf("test for dill_subii (immediate)");
    {
        int source1_i = rand1_i;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            int source2_i = (int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_subii(c, dest, opnd1, source2_i);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i - source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_subii (immediate) test, expected %d, got %d, for %d - %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_subui (immediate) */
    if (verbose) printf("test for dill_subui (immediate)");
    {
        unsigned int source1_u = rand1_u;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_subui(c, dest, opnd1, source2_u);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u - source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_subui (immediate) test, expected %u, got %u, for %u - %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_subuli (immediate) */
    if (verbose) printf("test for dill_subuli (immediate)");
    {
        UIMM_TYPE source1_ul = rand1_ul;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_subuli(c, dest, opnd1, source2_ul);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul - source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_subuli (immediate) test, expected %zu, got %zu, for %zu - %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_subli (immediate) */
    if (verbose) printf("test for dill_subli (immediate)");
    {
        IMM_TYPE source1_l = rand1_l;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_subli(c, dest, opnd1, source2_l);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l - source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_subli (immediate) test, expected %zx, got %zx, for %zx - %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_mulii (immediate) */
    if (verbose) printf("test for dill_mulii (immediate)");
    {
        int source1_i = rand1_i;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            int source2_i = (int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_mulii(c, dest, opnd1, source2_i);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i * source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_mulii (immediate) test, expected %d, got %d, for %d * %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_mului (immediate) */
    if (verbose) printf("test for dill_mului (immediate)");
    {
        unsigned int source1_u = rand1_u;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_mului(c, dest, opnd1, source2_u);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u * source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_mului (immediate) test, expected %u, got %u, for %u * %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_mululi (immediate) */
    if (verbose) printf("test for dill_mululi (immediate)");
    {
        UIMM_TYPE source1_ul = rand1_ul;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_mululi(c, dest, opnd1, source2_ul);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul * source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_mululi (immediate) test, expected %zu, got %zu, for %zu * %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_mulli (immediate) */
    if (verbose) printf("test for dill_mulli (immediate)");
    {
        IMM_TYPE source1_l = rand1_l;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_mulli(c, dest, opnd1, source2_l);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l * source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_mulli (immediate) test, expected %zx, got %zx, for %zx * %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_divii (immediate) */
    if (verbose) printf("test for dill_divii (immediate)");
    {
        int source1_i = rand1_i;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            int source2_i = (int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_i == 0) goto skip18;
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_divii(c, dest, opnd1, source2_i);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i / source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_divii (immediate) test, expected %d, got %d, for %d / %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
skip18: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_divui (immediate) */
    if (verbose) printf("test for dill_divui (immediate)");
    {
        unsigned int source1_u = rand1_u;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_u == 0) goto skip19;
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_divui(c, dest, opnd1, source2_u);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u / source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_divui (immediate) test, expected %u, got %u, for %u / %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
skip19: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_divuli (immediate) */
    if (verbose) printf("test for dill_divuli (immediate)");
    {
        UIMM_TYPE source1_ul = rand1_ul;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_ul == 0) goto skip20;
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_divuli(c, dest, opnd1, source2_ul);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul / source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_divuli (immediate) test, expected %zu, got %zu, for %zu / %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
skip20: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_divli (immediate) */
    if (verbose) printf("test for dill_divli (immediate)");
    {
        IMM_TYPE source1_l = rand1_l;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_l == 0) goto skip21;
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_divli(c, dest, opnd1, source2_l);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l / source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_divli (immediate) test, expected %zx, got %zx, for %zx / %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
skip21: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_modii (immediate) */
    if (verbose) printf("test for dill_modii (immediate)");
    {
        int source1_i = rand1_i;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            int source2_i = (int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_i <= 0) goto skip22;if (source2_i  > 0) goto skip22;
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_modii(c, dest, opnd1, source2_i);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i % source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_modii (immediate) test, expected %d, got %d, for %d %% %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
skip22: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_modui (immediate) */
    if (verbose) printf("test for dill_modui (immediate)");
    {
        unsigned int source1_u = rand1_u;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_u <= 0) goto skip23;if (source2_u  > 0) goto skip23;
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_modui(c, dest, opnd1, source2_u);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u % source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_modui (immediate) test, expected %u, got %u, for %u %% %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
skip23: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_moduli (immediate) */
    if (verbose) printf("test for dill_moduli (immediate)");
    {
        UIMM_TYPE source1_ul = rand1_ul;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_ul <= 0) goto skip24;if (source2_ul  > 0) goto skip24;
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_moduli(c, dest, opnd1, source2_ul);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul % source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_moduli (immediate) test, expected %zu, got %zu, for %zu %% %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
skip24: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_modli (immediate) */
    if (verbose) printf("test for dill_modli (immediate)");
    {
        IMM_TYPE source1_l = rand1_l;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (source2_l <= 0) goto skip25;if (source2_l  > 0) goto skip25;
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_modli(c, dest, opnd1, source2_l);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l % source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_modli (immediate) test, expected %zx, got %zx, for %zx %% %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
skip25: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_andii (immediate) */
    if (verbose) printf("test for dill_andii (immediate)");
    {
        int source1_i = rand1_i;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            int source2_i = (int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_andii(c, dest, opnd1, source2_i);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i & source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_andii (immediate) test, expected %d, got %d, for %d & %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_andui (immediate) */
    if (verbose) printf("test for dill_andui (immediate)");
    {
        unsigned int source1_u = rand1_u;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_andui(c, dest, opnd1, source2_u);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u & source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_andui (immediate) test, expected %u, got %u, for %u & %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_anduli (immediate) */
    if (verbose) printf("test for dill_anduli (immediate)");
    {
        UIMM_TYPE source1_ul = rand1_ul;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_anduli(c, dest, opnd1, source2_ul);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul & source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_anduli (immediate) test, expected %zu, got %zu, for %zu & %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_andli (immediate) */
    if (verbose) printf("test for dill_andli (immediate)");
    {
        IMM_TYPE source1_l = rand1_l;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_andli(c, dest, opnd1, source2_l);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l & source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_andli (immediate) test, expected %zx, got %zx, for %zx & %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_orii (immediate) */
    if (verbose) printf("test for dill_orii (immediate)");
    {
        int source1_i = rand1_i;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            int source2_i = (int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_orii(c, dest, opnd1, source2_i);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i | source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_orii (immediate) test, expected %d, got %d, for %d | %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_orui (immediate) */
    if (verbose) printf("test for dill_orui (immediate)");
    {
        unsigned int source1_u = rand1_u;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_orui(c, dest, opnd1, source2_u);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u | source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_orui (immediate) test, expected %u, got %u, for %u | %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_oruli (immediate) */
    if (verbose) printf("test for dill_oruli (immediate)");
    {
        UIMM_TYPE source1_ul = rand1_ul;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_oruli(c, dest, opnd1, source2_ul);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul | source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_oruli (immediate) test, expected %zu, got %zu, for %zu | %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_orli (immediate) */
    if (verbose) printf("test for dill_orli (immediate)");
    {
        IMM_TYPE source1_l = rand1_l;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_orli(c, dest, opnd1, source2_l);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l | source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_orli (immediate) test, expected %zx, got %zx, for %zx | %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_xorii (immediate) */
    if (verbose) printf("test for dill_xorii (immediate)");
    {
        int source1_i = rand1_i;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            int source2_i = (int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_xorii(c, dest, opnd1, source2_i);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i ^ source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_xorii (immediate) test, expected %d, got %d, for %d ^ %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_xorui (immediate) */
    if (verbose) printf("test for dill_xorui (immediate)");
    {
        unsigned int source1_u = rand1_u;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_xorui(c, dest, opnd1, source2_u);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u ^ source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_xorui (immediate) test, expected %u, got %u, for %u ^ %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_xoruli (immediate) */
    if (verbose) printf("test for dill_xoruli (immediate)");
    {
        UIMM_TYPE source1_ul = rand1_ul;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_xoruli(c, dest, opnd1, source2_ul);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul ^ source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_xoruli (immediate) test, expected %zu, got %zu, for %zu ^ %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_xorli (immediate) */
    if (verbose) printf("test for dill_xorli (immediate)");
    {
        IMM_TYPE source1_l = rand1_l;
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE)bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_xorli(c, dest, opnd1, source2_l);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l ^ source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_xorli (immediate) test, expected %zx, got %zx, for %zx ^ %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lshii (immediate) */
    if (verbose) printf("test for dill_lshii (immediate)");
    {
        int source1_i = rand1_i;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            int source2_i = (int)sh_src2_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if ((source2_i >= sizeof(IMM_TYPE)) || (source2_i <= 0)) goto skip26;
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_lshii(c, dest, opnd1, source2_i);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i << source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lshii (immediate) test, expected %d, got %d, for %d << %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
skip26: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lshui (immediate) */
    if (verbose) printf("test for dill_lshui (immediate)");
    {
        unsigned int source1_u = rand1_u;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int)sh_src2_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if ((source2_u >= sizeof(IMM_TYPE)) || (source2_u <= 0)) goto skip27;
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_lshui(c, dest, opnd1, source2_u);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u << source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lshui (immediate) test, expected %u, got %u, for %u << %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
skip27: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lshuli (immediate) */
    if (verbose) printf("test for dill_lshuli (immediate)");
    {
        UIMM_TYPE source1_ul = rand1_ul;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE)sh_src2_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if ((source2_ul >= sizeof(IMM_TYPE)) || (source2_ul <= 0)) goto skip28;
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_lshuli(c, dest, opnd1, source2_ul);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul << source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lshuli (immediate) test, expected %zu, got %zu, for %zu << %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
skip28: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lshli (immediate) */
    if (verbose) printf("test for dill_lshli (immediate)");
    {
        IMM_TYPE source1_l = rand1_l;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE)sh_src2_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if ((source2_l >= sizeof(IMM_TYPE)) || (source2_l <= 0)) goto skip29;
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_lshli(c, dest, opnd1, source2_l);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l << source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lshli (immediate) test, expected %zx, got %zx, for %zx << %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
skip29: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_rshii (immediate) */
    if (verbose) printf("test for dill_rshii (immediate)");
    {
        int source1_i = rand1_i;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            int source2_i = (int)sh_src2_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if ((source2_i >= sizeof(IMM_TYPE)) || (source2_i <= 0)) goto skip30;
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_rshii(c, dest, opnd1, source2_i);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i >> source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_rshii (immediate) test, expected %d, got %d, for %d >> %d\n",
		       expected_result, result, source1_i, source2_i);
		dill_dump(c);
		failed++;
	    }
skip30: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_rshui (immediate) */
    if (verbose) printf("test for dill_rshui (immediate)");
    {
        unsigned int source1_u = rand1_u;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            unsigned int source2_u = (unsigned int)sh_src2_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if ((source2_u >= sizeof(IMM_TYPE)) || (source2_u <= 0)) goto skip31;
	    dill_start_proc(c, "no name", DILL_U, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_rshui(c, dest, opnd1, source2_u);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u >> source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_rshui (immediate) test, expected %u, got %u, for %u >> %u\n",
		       expected_result, result, source1_u, source2_u);
		dill_dump(c);
		failed++;
	    }
skip31: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_rshuli (immediate) */
    if (verbose) printf("test for dill_rshuli (immediate)");
    {
        UIMM_TYPE source1_ul = rand1_ul;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = (UIMM_TYPE)sh_src2_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if ((source2_ul >= sizeof(IMM_TYPE)) || (source2_ul <= 0)) goto skip32;
	    dill_start_proc(c, "no name", DILL_UL, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_rshuli(c, dest, opnd1, source2_ul);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul >> source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_rshuli (immediate) test, expected %zu, got %zu, for %zu >> %zu\n",
		       expected_result, result, source1_ul, source2_ul);
		dill_dump(c);
		failed++;
	    }
skip32: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_rshli (immediate) */
    if (verbose) printf("test for dill_rshli (immediate)");
    {
        IMM_TYPE source1_l = rand1_l;
        for (j=0 ; j < sizeof(sh_src2_vals)/sizeof(sh_src2_vals[0]) ; j++) {
            IMM_TYPE source2_l = (IMM_TYPE)sh_src2_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if ((source2_l >= sizeof(IMM_TYPE)) || (source2_l <= 0)) goto skip33;
	    dill_start_proc(c, "no name", DILL_L, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_rshli(c, dest, opnd1, source2_l);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l >> source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_rshli (immediate) test, expected %zx, got %zx, for %zx >> %zx\n",
		       expected_result, result, source1_l, source2_l);
		dill_dump(c);
		failed++;
	    }
skip33: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_addpi (immediate) */
    if (verbose) printf("test for dill_addpi (immediate)");
    {
        char* source1_p = rand1_p;
        for (j=0 ; j < sizeof(src2l_vals)/sizeof(src2l_vals[0]) ; j++) {
            IMM_TYPE source2_l = src2l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    char* result;
	    char* expected_result;
	    char* (*proc)(dill_exec_ctx, char*);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_P, "%EC%p%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_P);
	    dill_addpi(c, dest, opnd1, source2_l);
	    dill_retp(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (char*(*)(dill_exec_ctx, char*)) dill_get_fp(h);
	    expected_result = source1_p + source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_addpi (immediate) test, expected %p, got %p, for %p + %zx\n",
		       (void*) expected_result, (void*) result, (void*) source1_p, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_subpi (immediate) */
    if (verbose) printf("test for dill_subpi (immediate)");
    {
        char* source1_p = rand1_p;
        for (j=0 ; j < sizeof(src2l_vals)/sizeof(src2l_vals[0]) ; j++) {
            IMM_TYPE source2_l = src2l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    char* result;
	    char* expected_result;
	    char* (*proc)(dill_exec_ctx, char*);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_P, "%EC%p%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_P);
	    dill_subpi(c, dest, opnd1, source2_l);
	    dill_retp(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (char*(*)(dill_exec_ctx, char*)) dill_get_fp(h);
	    expected_result = source1_p - source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_subpi (immediate) test, expected %p, got %p, for %p - %zx\n",
		       (void*) expected_result, (void*) result, (void*) source1_p, source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");
    return failed;
}

static int
test_branch(dill_stream c)
{
    int failed = 0;
    size_t i, j;
    (void)i; (void)j;  /* suppress unused warnings */

    /* test for dill_beqc */
    if (verbose) printf("test for dill_beqc");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_beqc(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_c == source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c, source2_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_beqc test, expected %d, got %d, for %d == %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bequc */
    if (verbose) printf("test for dill_bequc");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bequc(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc == source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc, source2_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bequc test, expected %d, got %d, for %u == %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_beqs */
    if (verbose) printf("test for dill_beqs");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_beqs(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_s == source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s, source2_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_beqs test, expected %d, got %d, for %d == %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bequs */
    if (verbose) printf("test for dill_bequs");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bequs(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us == source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us, source2_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bequs test, expected %d, got %d, for %u == %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_beqi */
    if (verbose) printf("test for dill_beqi");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_beqi(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int, int)) dill_get_fp(h);
	    expected_result = source1_i == source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_beqi test, expected %d, got %d, for %d == %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bequ */
    if (verbose) printf("test for dill_bequ");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bequ(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u == source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bequ test, expected %d, got %d, for %u == %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bequl */
    if (verbose) printf("test for dill_bequl");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bequl(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul == source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bequl test, expected %d, got %d, for %zu == %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_beql */
    if (verbose) printf("test for dill_beql");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_beql(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l == source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_beql test, expected %d, got %d, for %zx == %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_beqp */
    if (verbose) printf("test for dill_beqp");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, char* a, char* b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip34;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p%p");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_beqp(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*, char*)) dill_get_fp(h);
	    expected_result = source1_p == source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p, source2_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_beqp test, expected %d, got %d, for %p == %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip34: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_beqd */
    if (verbose) printf("test for dill_beqd");
    for (i=0 ; i < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; i++) {
        double source1_d = br_srcd_vals[i];
        for (j=0 ; j < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; j++) {
            double source2_d = br_srcd_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_beqd(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, double, double)) dill_get_fp(h);
	    expected_result = source1_d == source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_beqd test, expected %d, got %d, for %g == %g\n",
		       expected_result, result,  source1_d,  source2_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_beqf */
    if (verbose) printf("test for dill_beqf");
    for (i=0 ; i < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; i++) {
        float source1_f = br_srcf_vals[i];
        for (j=0 ; j < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; j++) {
            float source2_f = br_srcf_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_beqf(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, float, float)) dill_get_fp(h);
	    expected_result = source1_f == source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_beqf test, expected %d, got %d, for %g == %g\n",
		       expected_result, result,  source1_f,  source2_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgec */
    if (verbose) printf("test for dill_bgec");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgec(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_c >= source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c, source2_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgec test, expected %d, got %d, for %d >= %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgeuc */
    if (verbose) printf("test for dill_bgeuc");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgeuc(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc >= source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc, source2_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgeuc test, expected %d, got %d, for %u >= %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bges */
    if (verbose) printf("test for dill_bges");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bges(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_s >= source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s, source2_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bges test, expected %d, got %d, for %d >= %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgeus */
    if (verbose) printf("test for dill_bgeus");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgeus(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us >= source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us, source2_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgeus test, expected %d, got %d, for %u >= %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgei */
    if (verbose) printf("test for dill_bgei");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgei(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int, int)) dill_get_fp(h);
	    expected_result = source1_i >= source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgei test, expected %d, got %d, for %d >= %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgeu */
    if (verbose) printf("test for dill_bgeu");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgeu(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u >= source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgeu test, expected %d, got %d, for %u >= %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgeul */
    if (verbose) printf("test for dill_bgeul");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgeul(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul >= source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgeul test, expected %d, got %d, for %zu >= %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgel */
    if (verbose) printf("test for dill_bgel");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgel(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l >= source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgel test, expected %d, got %d, for %zx >= %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgep */
    if (verbose) printf("test for dill_bgep");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, char* a, char* b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip35;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p%p");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgep(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*, char*)) dill_get_fp(h);
	    expected_result = source1_p >= source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p, source2_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgep test, expected %d, got %d, for %p >= %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip35: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bged */
    if (verbose) printf("test for dill_bged");
    for (i=0 ; i < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; i++) {
        double source1_d = br_srcd_vals[i];
        for (j=0 ; j < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; j++) {
            double source2_d = br_srcd_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bged(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, double, double)) dill_get_fp(h);
	    expected_result = source1_d >= source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bged test, expected %d, got %d, for %g >= %g\n",
		       expected_result, result,  source1_d,  source2_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgef */
    if (verbose) printf("test for dill_bgef");
    for (i=0 ; i < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; i++) {
        float source1_f = br_srcf_vals[i];
        for (j=0 ; j < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; j++) {
            float source2_f = br_srcf_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgef(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, float, float)) dill_get_fp(h);
	    expected_result = source1_f >= source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgef test, expected %d, got %d, for %g >= %g\n",
		       expected_result, result,  source1_f,  source2_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtc */
    if (verbose) printf("test for dill_bgtc");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtc(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_c > source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c, source2_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtc test, expected %d, got %d, for %d > %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtuc */
    if (verbose) printf("test for dill_bgtuc");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtuc(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc > source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc, source2_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtuc test, expected %d, got %d, for %u > %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgts */
    if (verbose) printf("test for dill_bgts");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgts(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_s > source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s, source2_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgts test, expected %d, got %d, for %d > %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtus */
    if (verbose) printf("test for dill_bgtus");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtus(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us > source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us, source2_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtus test, expected %d, got %d, for %u > %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgti */
    if (verbose) printf("test for dill_bgti");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgti(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int, int)) dill_get_fp(h);
	    expected_result = source1_i > source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgti test, expected %d, got %d, for %d > %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtu */
    if (verbose) printf("test for dill_bgtu");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtu(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u > source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtu test, expected %d, got %d, for %u > %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtul */
    if (verbose) printf("test for dill_bgtul");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtul(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul > source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtul test, expected %d, got %d, for %zu > %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtl */
    if (verbose) printf("test for dill_bgtl");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtl(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l > source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtl test, expected %d, got %d, for %zx > %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtp */
    if (verbose) printf("test for dill_bgtp");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, char* a, char* b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip36;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p%p");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtp(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*, char*)) dill_get_fp(h);
	    expected_result = source1_p > source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p, source2_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtp test, expected %d, got %d, for %p > %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip36: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtd */
    if (verbose) printf("test for dill_bgtd");
    for (i=0 ; i < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; i++) {
        double source1_d = br_srcd_vals[i];
        for (j=0 ; j < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; j++) {
            double source2_d = br_srcd_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtd(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, double, double)) dill_get_fp(h);
	    expected_result = source1_d > source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtd test, expected %d, got %d, for %g > %g\n",
		       expected_result, result,  source1_d,  source2_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtf */
    if (verbose) printf("test for dill_bgtf");
    for (i=0 ; i < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; i++) {
        float source1_f = br_srcf_vals[i];
        for (j=0 ; j < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; j++) {
            float source2_f = br_srcf_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtf(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, float, float)) dill_get_fp(h);
	    expected_result = source1_f > source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtf test, expected %d, got %d, for %g > %g\n",
		       expected_result, result,  source1_f,  source2_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_blec */
    if (verbose) printf("test for dill_blec");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_blec(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_c <= source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c, source2_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_blec test, expected %d, got %d, for %d <= %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bleuc */
    if (verbose) printf("test for dill_bleuc");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bleuc(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc <= source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc, source2_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bleuc test, expected %d, got %d, for %u <= %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bles */
    if (verbose) printf("test for dill_bles");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bles(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_s <= source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s, source2_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bles test, expected %d, got %d, for %d <= %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bleus */
    if (verbose) printf("test for dill_bleus");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bleus(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us <= source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us, source2_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bleus test, expected %d, got %d, for %u <= %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_blei */
    if (verbose) printf("test for dill_blei");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_blei(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int, int)) dill_get_fp(h);
	    expected_result = source1_i <= source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_blei test, expected %d, got %d, for %d <= %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bleu */
    if (verbose) printf("test for dill_bleu");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bleu(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u <= source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bleu test, expected %d, got %d, for %u <= %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bleul */
    if (verbose) printf("test for dill_bleul");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bleul(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul <= source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bleul test, expected %d, got %d, for %zu <= %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_blel */
    if (verbose) printf("test for dill_blel");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_blel(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l <= source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_blel test, expected %d, got %d, for %zx <= %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_blep */
    if (verbose) printf("test for dill_blep");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, char* a, char* b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip37;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p%p");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_blep(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*, char*)) dill_get_fp(h);
	    expected_result = source1_p <= source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p, source2_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_blep test, expected %d, got %d, for %p <= %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip37: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bled */
    if (verbose) printf("test for dill_bled");
    for (i=0 ; i < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; i++) {
        double source1_d = br_srcd_vals[i];
        for (j=0 ; j < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; j++) {
            double source2_d = br_srcd_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bled(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, double, double)) dill_get_fp(h);
	    expected_result = source1_d <= source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bled test, expected %d, got %d, for %g <= %g\n",
		       expected_result, result,  source1_d,  source2_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_blef */
    if (verbose) printf("test for dill_blef");
    for (i=0 ; i < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; i++) {
        float source1_f = br_srcf_vals[i];
        for (j=0 ; j < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; j++) {
            float source2_f = br_srcf_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_blef(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, float, float)) dill_get_fp(h);
	    expected_result = source1_f <= source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_blef test, expected %d, got %d, for %g <= %g\n",
		       expected_result, result,  source1_f,  source2_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltc */
    if (verbose) printf("test for dill_bltc");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltc(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_c < source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c, source2_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltc test, expected %d, got %d, for %d < %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltuc */
    if (verbose) printf("test for dill_bltuc");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltuc(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc < source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc, source2_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltuc test, expected %d, got %d, for %u < %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_blts */
    if (verbose) printf("test for dill_blts");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_blts(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_s < source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s, source2_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_blts test, expected %d, got %d, for %d < %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltus */
    if (verbose) printf("test for dill_bltus");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltus(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us < source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us, source2_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltus test, expected %d, got %d, for %u < %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_blti */
    if (verbose) printf("test for dill_blti");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_blti(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int, int)) dill_get_fp(h);
	    expected_result = source1_i < source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_blti test, expected %d, got %d, for %d < %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltu */
    if (verbose) printf("test for dill_bltu");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltu(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u < source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltu test, expected %d, got %d, for %u < %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltul */
    if (verbose) printf("test for dill_bltul");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltul(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul < source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltul test, expected %d, got %d, for %zu < %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltl */
    if (verbose) printf("test for dill_bltl");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltl(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l < source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltl test, expected %d, got %d, for %zx < %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltp */
    if (verbose) printf("test for dill_bltp");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, char* a, char* b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip38;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p%p");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltp(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*, char*)) dill_get_fp(h);
	    expected_result = source1_p < source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p, source2_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltp test, expected %d, got %d, for %p < %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip38: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltd */
    if (verbose) printf("test for dill_bltd");
    for (i=0 ; i < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; i++) {
        double source1_d = br_srcd_vals[i];
        for (j=0 ; j < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; j++) {
            double source2_d = br_srcd_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltd(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, double, double)) dill_get_fp(h);
	    expected_result = source1_d < source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltd test, expected %d, got %d, for %g < %g\n",
		       expected_result, result,  source1_d,  source2_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltf */
    if (verbose) printf("test for dill_bltf");
    for (i=0 ; i < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; i++) {
        float source1_f = br_srcf_vals[i];
        for (j=0 ; j < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; j++) {
            float source2_f = br_srcf_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltf(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, float, float)) dill_get_fp(h);
	    expected_result = source1_f < source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltf test, expected %d, got %d, for %g < %g\n",
		       expected_result, result,  source1_f,  source2_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bnec */
    if (verbose) printf("test for dill_bnec");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bnec(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_c != source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c, source2_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bnec test, expected %d, got %d, for %d != %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bneuc */
    if (verbose) printf("test for dill_bneuc");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bneuc(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc != source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc, source2_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bneuc test, expected %d, got %d, for %u != %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bnes */
    if (verbose) printf("test for dill_bnes");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bnes(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_s != source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s, source2_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bnes test, expected %d, got %d, for %d != %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bneus */
    if (verbose) printf("test for dill_bneus");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bneus(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us != source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us, source2_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bneus test, expected %d, got %d, for %u != %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bnei */
    if (verbose) printf("test for dill_bnei");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bnei(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int, int)) dill_get_fp(h);
	    expected_result = source1_i != source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bnei test, expected %d, got %d, for %d != %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bneu */
    if (verbose) printf("test for dill_bneu");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bneu(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u != source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bneu test, expected %d, got %d, for %u != %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bneul */
    if (verbose) printf("test for dill_bneul");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bneul(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul != source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bneul test, expected %d, got %d, for %zu != %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bnel */
    if (verbose) printf("test for dill_bnel");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bnel(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l != source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bnel test, expected %d, got %d, for %zx != %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bnep */
    if (verbose) printf("test for dill_bnep");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, char* a, char* b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip39;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p%p");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bnep(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*, char*)) dill_get_fp(h);
	    expected_result = source1_p != source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p, source2_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bnep test, expected %d, got %d, for %p != %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip39: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bned */
    if (verbose) printf("test for dill_bned");
    for (i=0 ; i < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; i++) {
        double source1_d = br_srcd_vals[i];
        for (j=0 ; j < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; j++) {
            double source2_d = br_srcd_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bned(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, double, double)) dill_get_fp(h);
	    expected_result = source1_d != source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bned test, expected %d, got %d, for %g != %g\n",
		       expected_result, result,  source1_d,  source2_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bnef */
    if (verbose) printf("test for dill_bnef");
    for (i=0 ; i < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; i++) {
        float source1_f = br_srcf_vals[i];
        for (j=0 ; j < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; j++) {
            float source2_f = br_srcf_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bnef(c, opnd1, opnd2, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, float, float)) dill_get_fp(h);
	    expected_result = source1_f != source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bnef test, expected %d, got %d, for %g != %g\n",
		       expected_result, result,  source1_f,  source2_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");
    return failed;
}

static int
test_branchi(dill_stream c)
{
    int failed = 0;
    size_t i, j;
    (void)i; (void)j;  /* suppress unused warnings */

    /* test for dill_beqci */
    if (verbose) printf("test for dill_beqci");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, signed int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_beqci(c, opnd1,  source2_c, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = source1_c == source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_beqci test, expected %d, got %d, for %d == %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bequci */
    if (verbose) printf("test for dill_bequci");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bequci(c, opnd1,  source2_uc, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc == source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bequci test, expected %d, got %d, for %u == %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_beqsi */
    if (verbose) printf("test for dill_beqsi");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, signed int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_beqsi(c, opnd1,  source2_s, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = source1_s == source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_beqsi test, expected %d, got %d, for %d == %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bequsi */
    if (verbose) printf("test for dill_bequsi");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bequsi(c, opnd1,  source2_us, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us == source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bequsi test, expected %d, got %d, for %u == %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_beqii */
    if (verbose) printf("test for dill_beqii");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_beqii(c, opnd1,  source2_i, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i == source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_beqii test, expected %d, got %d, for %d == %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bequi */
    if (verbose) printf("test for dill_bequi");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bequi(c, opnd1,  source2_u, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u == source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bequi test, expected %d, got %d, for %u == %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bequli */
    if (verbose) printf("test for dill_bequli");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bequli(c, opnd1,  source2_ul, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul == source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bequli test, expected %d, got %d, for %zu == %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_beqli */
    if (verbose) printf("test for dill_beqli");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_beqli(c, opnd1,  source2_l, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l == source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_beqli test, expected %d, got %d, for %zx == %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_beqpi */
    if (verbose) printf("test for dill_beqpi");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, char* a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip40;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_beqpi(c, opnd1, (IMM_TYPE) source2_p, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*)) dill_get_fp(h);
	    expected_result = source1_p == source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_beqpi test, expected %d, got %d, for %p == %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip40: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgeci */
    if (verbose) printf("test for dill_bgeci");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, signed int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgeci(c, opnd1,  source2_c, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = source1_c >= source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgeci test, expected %d, got %d, for %d >= %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgeuci */
    if (verbose) printf("test for dill_bgeuci");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgeuci(c, opnd1,  source2_uc, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc >= source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgeuci test, expected %d, got %d, for %u >= %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgesi */
    if (verbose) printf("test for dill_bgesi");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, signed int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgesi(c, opnd1,  source2_s, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = source1_s >= source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgesi test, expected %d, got %d, for %d >= %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgeusi */
    if (verbose) printf("test for dill_bgeusi");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgeusi(c, opnd1,  source2_us, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us >= source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgeusi test, expected %d, got %d, for %u >= %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgeii */
    if (verbose) printf("test for dill_bgeii");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgeii(c, opnd1,  source2_i, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i >= source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgeii test, expected %d, got %d, for %d >= %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgeui */
    if (verbose) printf("test for dill_bgeui");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgeui(c, opnd1,  source2_u, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u >= source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgeui test, expected %d, got %d, for %u >= %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgeuli */
    if (verbose) printf("test for dill_bgeuli");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgeuli(c, opnd1,  source2_ul, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul >= source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgeuli test, expected %d, got %d, for %zu >= %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgeli */
    if (verbose) printf("test for dill_bgeli");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgeli(c, opnd1,  source2_l, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l >= source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgeli test, expected %d, got %d, for %zx >= %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgepi */
    if (verbose) printf("test for dill_bgepi");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, char* a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip41;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgepi(c, opnd1, (IMM_TYPE) source2_p, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*)) dill_get_fp(h);
	    expected_result = source1_p >= source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgepi test, expected %d, got %d, for %p >= %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip41: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtci */
    if (verbose) printf("test for dill_bgtci");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, signed int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtci(c, opnd1,  source2_c, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = source1_c > source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtci test, expected %d, got %d, for %d > %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtuci */
    if (verbose) printf("test for dill_bgtuci");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtuci(c, opnd1,  source2_uc, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc > source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtuci test, expected %d, got %d, for %u > %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtsi */
    if (verbose) printf("test for dill_bgtsi");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, signed int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtsi(c, opnd1,  source2_s, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = source1_s > source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtsi test, expected %d, got %d, for %d > %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtusi */
    if (verbose) printf("test for dill_bgtusi");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtusi(c, opnd1,  source2_us, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us > source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtusi test, expected %d, got %d, for %u > %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtii */
    if (verbose) printf("test for dill_bgtii");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtii(c, opnd1,  source2_i, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i > source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtii test, expected %d, got %d, for %d > %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtui */
    if (verbose) printf("test for dill_bgtui");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtui(c, opnd1,  source2_u, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u > source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtui test, expected %d, got %d, for %u > %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtuli */
    if (verbose) printf("test for dill_bgtuli");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtuli(c, opnd1,  source2_ul, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul > source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtuli test, expected %d, got %d, for %zu > %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtli */
    if (verbose) printf("test for dill_bgtli");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtli(c, opnd1,  source2_l, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l > source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtli test, expected %d, got %d, for %zx > %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bgtpi */
    if (verbose) printf("test for dill_bgtpi");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, char* a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip42;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bgtpi(c, opnd1, (IMM_TYPE) source2_p, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*)) dill_get_fp(h);
	    expected_result = source1_p > source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bgtpi test, expected %d, got %d, for %p > %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip42: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bleci */
    if (verbose) printf("test for dill_bleci");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, signed int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bleci(c, opnd1,  source2_c, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = source1_c <= source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bleci test, expected %d, got %d, for %d <= %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bleuci */
    if (verbose) printf("test for dill_bleuci");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bleuci(c, opnd1,  source2_uc, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc <= source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bleuci test, expected %d, got %d, for %u <= %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_blesi */
    if (verbose) printf("test for dill_blesi");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, signed int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_blesi(c, opnd1,  source2_s, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = source1_s <= source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_blesi test, expected %d, got %d, for %d <= %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bleusi */
    if (verbose) printf("test for dill_bleusi");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bleusi(c, opnd1,  source2_us, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us <= source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bleusi test, expected %d, got %d, for %u <= %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bleii */
    if (verbose) printf("test for dill_bleii");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bleii(c, opnd1,  source2_i, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i <= source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bleii test, expected %d, got %d, for %d <= %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bleui */
    if (verbose) printf("test for dill_bleui");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bleui(c, opnd1,  source2_u, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u <= source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bleui test, expected %d, got %d, for %u <= %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bleuli */
    if (verbose) printf("test for dill_bleuli");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bleuli(c, opnd1,  source2_ul, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul <= source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bleuli test, expected %d, got %d, for %zu <= %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bleli */
    if (verbose) printf("test for dill_bleli");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bleli(c, opnd1,  source2_l, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l <= source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bleli test, expected %d, got %d, for %zx <= %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_blepi */
    if (verbose) printf("test for dill_blepi");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, char* a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip43;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_blepi(c, opnd1, (IMM_TYPE) source2_p, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*)) dill_get_fp(h);
	    expected_result = source1_p <= source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_blepi test, expected %d, got %d, for %p <= %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip43: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltci */
    if (verbose) printf("test for dill_bltci");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, signed int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltci(c, opnd1,  source2_c, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = source1_c < source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltci test, expected %d, got %d, for %d < %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltuci */
    if (verbose) printf("test for dill_bltuci");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltuci(c, opnd1,  source2_uc, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc < source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltuci test, expected %d, got %d, for %u < %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltsi */
    if (verbose) printf("test for dill_bltsi");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, signed int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltsi(c, opnd1,  source2_s, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = source1_s < source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltsi test, expected %d, got %d, for %d < %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltusi */
    if (verbose) printf("test for dill_bltusi");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltusi(c, opnd1,  source2_us, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us < source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltusi test, expected %d, got %d, for %u < %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltii */
    if (verbose) printf("test for dill_bltii");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltii(c, opnd1,  source2_i, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i < source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltii test, expected %d, got %d, for %d < %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltui */
    if (verbose) printf("test for dill_bltui");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltui(c, opnd1,  source2_u, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u < source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltui test, expected %d, got %d, for %u < %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltuli */
    if (verbose) printf("test for dill_bltuli");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltuli(c, opnd1,  source2_ul, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul < source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltuli test, expected %d, got %d, for %zu < %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltli */
    if (verbose) printf("test for dill_bltli");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltli(c, opnd1,  source2_l, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l < source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltli test, expected %d, got %d, for %zx < %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bltpi */
    if (verbose) printf("test for dill_bltpi");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, char* a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip44;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bltpi(c, opnd1, (IMM_TYPE) source2_p, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*)) dill_get_fp(h);
	    expected_result = source1_p < source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bltpi test, expected %d, got %d, for %p < %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip44: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bneci */
    if (verbose) printf("test for dill_bneci");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, signed int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bneci(c, opnd1,  source2_c, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = source1_c != source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bneci test, expected %d, got %d, for %d != %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bneuci */
    if (verbose) printf("test for dill_bneuci");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bneuci(c, opnd1,  source2_uc, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc != source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bneuci test, expected %d, got %d, for %u != %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bnesi */
    if (verbose) printf("test for dill_bnesi");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, signed int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bnesi(c, opnd1,  source2_s, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = source1_s != source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bnesi test, expected %d, got %d, for %d != %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bneusi */
    if (verbose) printf("test for dill_bneusi");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bneusi(c, opnd1,  source2_us, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us != source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bneusi test, expected %d, got %d, for %u != %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bneii */
    if (verbose) printf("test for dill_bneii");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bneii(c, opnd1,  source2_i, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = source1_i != source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bneii test, expected %d, got %d, for %d != %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bneui */
    if (verbose) printf("test for dill_bneui");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, unsigned int a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bneui(c, opnd1,  source2_u, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u != source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bneui test, expected %d, got %d, for %u != %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bneuli */
    if (verbose) printf("test for dill_bneuli");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, UIMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bneuli(c, opnd1,  source2_ul, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul != source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bneuli test, expected %d, got %d, for %zu != %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bneli */
    if (verbose) printf("test for dill_bneli");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, IMM_TYPE a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bneli(c, opnd1,  source2_l, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l != source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bneli test, expected %d, got %d, for %zx != %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_bnepi */
    if (verbose) printf("test for dill_bnepi");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1;
	    int label;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, char* a);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip45;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	
	    label = dill_alloc_label(c, NULL);
	    dill_bnepi(c, opnd1, (IMM_TYPE) source2_p, label);
	    dill_retii(c, 0);
	    dill_mark_label(c, label);	    
	    dill_retii(c, 1);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*)) dill_get_fp(h);
	    expected_result = source1_p != source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_bnepi test, expected %d, got %d, for %p != %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip45: ;
	}
    }
    if (verbose) printf(" done\n");
    return failed;
}

static int
test_compare(dill_stream c)
{
    int failed = 0;
    size_t i, j;
    (void)i; (void)j;  /* suppress unused warnings */

    /* test for dill_eqc */
    if (verbose) printf("test for dill_eqc");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_eqc(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_c == source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c, source2_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_eqc test, expected %d, got %d, for %d == %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_equc */
    if (verbose) printf("test for dill_equc");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_equc(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc == source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc, source2_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_equc test, expected %d, got %d, for %u == %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_eqs */
    if (verbose) printf("test for dill_eqs");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_eqs(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_s == source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s, source2_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_eqs test, expected %d, got %d, for %d == %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_equs */
    if (verbose) printf("test for dill_equs");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_equs(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us == source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us, source2_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_equs test, expected %d, got %d, for %u == %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_eqi */
    if (verbose) printf("test for dill_eqi");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_eqi(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int, int)) dill_get_fp(h);
	    expected_result = source1_i == source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_eqi test, expected %d, got %d, for %d == %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_equ */
    if (verbose) printf("test for dill_equ");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_equ(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u == source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_equ test, expected %d, got %d, for %u == %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_equl */
    if (verbose) printf("test for dill_equl");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_equl(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul == source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_equl test, expected %d, got %d, for %zu == %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_eql */
    if (verbose) printf("test for dill_eql");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_eql(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l == source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_eql test, expected %d, got %d, for %zx == %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_eqp */
    if (verbose) printf("test for dill_eqp");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, char* a, char* b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip46;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p%p");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_eqp(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*, char*)) dill_get_fp(h);
	    expected_result = source1_p == source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p, source2_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_eqp test, expected %d, got %d, for %p == %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip46: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_eqd */
    if (verbose) printf("test for dill_eqd");
    for (i=0 ; i < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; i++) {
        double source1_d = br_srcd_vals[i];
        for (j=0 ; j < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; j++) {
            double source2_d = br_srcd_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_eqd(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, double, double)) dill_get_fp(h);
	    expected_result = source1_d == source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_eqd test, expected %d, got %d, for %g == %g\n",
		       expected_result, result,  source1_d,  source2_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_eqf */
    if (verbose) printf("test for dill_eqf");
    for (i=0 ; i < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; i++) {
        float source1_f = br_srcf_vals[i];
        for (j=0 ; j < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; j++) {
            float source2_f = br_srcf_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_eqf(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, float, float)) dill_get_fp(h);
	    expected_result = source1_f == source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_eqf test, expected %d, got %d, for %g == %g\n",
		       expected_result, result,  source1_f,  source2_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gec */
    if (verbose) printf("test for dill_gec");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gec(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_c >= source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c, source2_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gec test, expected %d, got %d, for %d >= %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_geuc */
    if (verbose) printf("test for dill_geuc");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_geuc(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc >= source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc, source2_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_geuc test, expected %d, got %d, for %u >= %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ges */
    if (verbose) printf("test for dill_ges");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_ges(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_s >= source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s, source2_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ges test, expected %d, got %d, for %d >= %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_geus */
    if (verbose) printf("test for dill_geus");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_geus(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us >= source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us, source2_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_geus test, expected %d, got %d, for %u >= %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gei */
    if (verbose) printf("test for dill_gei");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gei(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int, int)) dill_get_fp(h);
	    expected_result = source1_i >= source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gei test, expected %d, got %d, for %d >= %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_geu */
    if (verbose) printf("test for dill_geu");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_geu(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u >= source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_geu test, expected %d, got %d, for %u >= %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_geul */
    if (verbose) printf("test for dill_geul");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_geul(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul >= source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_geul test, expected %d, got %d, for %zu >= %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gel */
    if (verbose) printf("test for dill_gel");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gel(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l >= source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gel test, expected %d, got %d, for %zx >= %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gep */
    if (verbose) printf("test for dill_gep");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, char* a, char* b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip47;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p%p");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gep(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*, char*)) dill_get_fp(h);
	    expected_result = source1_p >= source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p, source2_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gep test, expected %d, got %d, for %p >= %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip47: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ged */
    if (verbose) printf("test for dill_ged");
    for (i=0 ; i < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; i++) {
        double source1_d = br_srcd_vals[i];
        for (j=0 ; j < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; j++) {
            double source2_d = br_srcd_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_ged(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, double, double)) dill_get_fp(h);
	    expected_result = source1_d >= source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ged test, expected %d, got %d, for %g >= %g\n",
		       expected_result, result,  source1_d,  source2_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gef */
    if (verbose) printf("test for dill_gef");
    for (i=0 ; i < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; i++) {
        float source1_f = br_srcf_vals[i];
        for (j=0 ; j < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; j++) {
            float source2_f = br_srcf_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gef(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, float, float)) dill_get_fp(h);
	    expected_result = source1_f >= source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gef test, expected %d, got %d, for %g >= %g\n",
		       expected_result, result,  source1_f,  source2_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gtc */
    if (verbose) printf("test for dill_gtc");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gtc(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_c > source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c, source2_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gtc test, expected %d, got %d, for %d > %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gtuc */
    if (verbose) printf("test for dill_gtuc");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gtuc(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc > source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc, source2_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gtuc test, expected %d, got %d, for %u > %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gts */
    if (verbose) printf("test for dill_gts");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gts(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_s > source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s, source2_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gts test, expected %d, got %d, for %d > %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gtus */
    if (verbose) printf("test for dill_gtus");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gtus(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us > source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us, source2_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gtus test, expected %d, got %d, for %u > %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gti */
    if (verbose) printf("test for dill_gti");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gti(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int, int)) dill_get_fp(h);
	    expected_result = source1_i > source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gti test, expected %d, got %d, for %d > %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gtu */
    if (verbose) printf("test for dill_gtu");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gtu(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u > source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gtu test, expected %d, got %d, for %u > %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gtul */
    if (verbose) printf("test for dill_gtul");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gtul(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul > source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gtul test, expected %d, got %d, for %zu > %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gtl */
    if (verbose) printf("test for dill_gtl");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gtl(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l > source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gtl test, expected %d, got %d, for %zx > %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gtp */
    if (verbose) printf("test for dill_gtp");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, char* a, char* b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip48;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p%p");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gtp(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*, char*)) dill_get_fp(h);
	    expected_result = source1_p > source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p, source2_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gtp test, expected %d, got %d, for %p > %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip48: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gtd */
    if (verbose) printf("test for dill_gtd");
    for (i=0 ; i < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; i++) {
        double source1_d = br_srcd_vals[i];
        for (j=0 ; j < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; j++) {
            double source2_d = br_srcd_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gtd(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, double, double)) dill_get_fp(h);
	    expected_result = source1_d > source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gtd test, expected %d, got %d, for %g > %g\n",
		       expected_result, result,  source1_d,  source2_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_gtf */
    if (verbose) printf("test for dill_gtf");
    for (i=0 ; i < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; i++) {
        float source1_f = br_srcf_vals[i];
        for (j=0 ; j < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; j++) {
            float source2_f = br_srcf_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_gtf(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, float, float)) dill_get_fp(h);
	    expected_result = source1_f > source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_gtf test, expected %d, got %d, for %g > %g\n",
		       expected_result, result,  source1_f,  source2_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lec */
    if (verbose) printf("test for dill_lec");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_lec(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_c <= source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c, source2_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lec test, expected %d, got %d, for %d <= %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_leuc */
    if (verbose) printf("test for dill_leuc");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_leuc(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc <= source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc, source2_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_leuc test, expected %d, got %d, for %u <= %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_les */
    if (verbose) printf("test for dill_les");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_les(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_s <= source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s, source2_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_les test, expected %d, got %d, for %d <= %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_leus */
    if (verbose) printf("test for dill_leus");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_leus(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us <= source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us, source2_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_leus test, expected %d, got %d, for %u <= %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lei */
    if (verbose) printf("test for dill_lei");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_lei(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int, int)) dill_get_fp(h);
	    expected_result = source1_i <= source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lei test, expected %d, got %d, for %d <= %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_leu */
    if (verbose) printf("test for dill_leu");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_leu(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u <= source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_leu test, expected %d, got %d, for %u <= %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_leul */
    if (verbose) printf("test for dill_leul");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_leul(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul <= source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_leul test, expected %d, got %d, for %zu <= %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lel */
    if (verbose) printf("test for dill_lel");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_lel(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l <= source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lel test, expected %d, got %d, for %zx <= %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lep */
    if (verbose) printf("test for dill_lep");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, char* a, char* b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip49;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p%p");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_lep(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*, char*)) dill_get_fp(h);
	    expected_result = source1_p <= source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p, source2_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lep test, expected %d, got %d, for %p <= %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip49: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_led */
    if (verbose) printf("test for dill_led");
    for (i=0 ; i < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; i++) {
        double source1_d = br_srcd_vals[i];
        for (j=0 ; j < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; j++) {
            double source2_d = br_srcd_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_led(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, double, double)) dill_get_fp(h);
	    expected_result = source1_d <= source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_led test, expected %d, got %d, for %g <= %g\n",
		       expected_result, result,  source1_d,  source2_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lef */
    if (verbose) printf("test for dill_lef");
    for (i=0 ; i < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; i++) {
        float source1_f = br_srcf_vals[i];
        for (j=0 ; j < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; j++) {
            float source2_f = br_srcf_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_lef(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, float, float)) dill_get_fp(h);
	    expected_result = source1_f <= source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lef test, expected %d, got %d, for %g <= %g\n",
		       expected_result, result,  source1_f,  source2_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ltc */
    if (verbose) printf("test for dill_ltc");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_ltc(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_c < source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c, source2_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ltc test, expected %d, got %d, for %d < %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ltuc */
    if (verbose) printf("test for dill_ltuc");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_ltuc(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc < source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc, source2_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ltuc test, expected %d, got %d, for %u < %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lts */
    if (verbose) printf("test for dill_lts");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_lts(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_s < source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s, source2_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lts test, expected %d, got %d, for %d < %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ltus */
    if (verbose) printf("test for dill_ltus");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_ltus(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us < source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us, source2_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ltus test, expected %d, got %d, for %u < %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lti */
    if (verbose) printf("test for dill_lti");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_lti(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int, int)) dill_get_fp(h);
	    expected_result = source1_i < source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lti test, expected %d, got %d, for %d < %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ltu */
    if (verbose) printf("test for dill_ltu");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_ltu(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u < source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ltu test, expected %d, got %d, for %u < %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ltul */
    if (verbose) printf("test for dill_ltul");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_ltul(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul < source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ltul test, expected %d, got %d, for %zu < %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ltl */
    if (verbose) printf("test for dill_ltl");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_ltl(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l < source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ltl test, expected %d, got %d, for %zx < %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ltp */
    if (verbose) printf("test for dill_ltp");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, char* a, char* b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip50;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p%p");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_ltp(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*, char*)) dill_get_fp(h);
	    expected_result = source1_p < source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p, source2_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ltp test, expected %d, got %d, for %p < %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip50: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ltd */
    if (verbose) printf("test for dill_ltd");
    for (i=0 ; i < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; i++) {
        double source1_d = br_srcd_vals[i];
        for (j=0 ; j < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; j++) {
            double source2_d = br_srcd_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_ltd(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, double, double)) dill_get_fp(h);
	    expected_result = source1_d < source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ltd test, expected %d, got %d, for %g < %g\n",
		       expected_result, result,  source1_d,  source2_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ltf */
    if (verbose) printf("test for dill_ltf");
    for (i=0 ; i < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; i++) {
        float source1_f = br_srcf_vals[i];
        for (j=0 ; j < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; j++) {
            float source2_f = br_srcf_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_ltf(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, float, float)) dill_get_fp(h);
	    expected_result = source1_f < source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ltf test, expected %d, got %d, for %g < %g\n",
		       expected_result, result,  source1_f,  source2_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_nec */
    if (verbose) printf("test for dill_nec");
    for (i=0 ; i < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; i++) {
        signed char source1_c = br_srcc_vals[i];
        for (j=0 ; j < sizeof(br_srcc_vals)/sizeof(br_srcc_vals[0]) ; j++) {
            signed char source2_c = br_srcc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_nec(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_c != source2_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c, source2_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_nec test, expected %d, got %d, for %d != %d\n",
		       expected_result, result,  source1_c,  source2_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_neuc */
    if (verbose) printf("test for dill_neuc");
    for (i=0 ; i < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; i++) {
        unsigned char source1_uc = br_srcuc_vals[i];
        for (j=0 ; j < sizeof(br_srcuc_vals)/sizeof(br_srcuc_vals[0]) ; j++) {
            unsigned char source2_uc = br_srcuc_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_neuc(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_uc != source2_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_uc, source2_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_neuc test, expected %d, got %d, for %u != %u\n",
		       expected_result, result,  source1_uc,  source2_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_nes */
    if (verbose) printf("test for dill_nes");
    for (i=0 ; i < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; i++) {
        short source1_s = br_srcs_vals[i];
        for (j=0 ; j < sizeof(br_srcs_vals)/sizeof(br_srcs_vals[0]) ; j++) {
            short source2_s = br_srcs_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a, signed int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_nes(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int, signed int)) dill_get_fp(h);
	    expected_result = source1_s != source2_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s, source2_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_nes test, expected %d, got %d, for %d != %d\n",
		       expected_result, result,  source1_s,  source2_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_neus */
    if (verbose) printf("test for dill_neus");
    for (i=0 ; i < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; i++) {
        unsigned short source1_us = br_srcus_vals[i];
        for (j=0 ; j < sizeof(br_srcus_vals)/sizeof(br_srcus_vals[0]) ; j++) {
            unsigned short source2_us = br_srcus_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_neus(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_us != source2_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us, source2_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_neus test, expected %d, got %d, for %u != %u\n",
		       expected_result, result,  source1_us,  source2_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_nei */
    if (verbose) printf("test for dill_nei");
    for (i=0 ; i < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; i++) {
        int source1_i = br_srci_vals[i];
        for (j=0 ; j < sizeof(br_srci_vals)/sizeof(br_srci_vals[0]) ; j++) {
            int source2_i = br_srci_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, int a, int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%i%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_nei(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, int, int)) dill_get_fp(h);
	    expected_result = source1_i != source2_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i, source2_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_nei test, expected %d, got %d, for %d != %d\n",
		       expected_result, result,  source1_i,  source2_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_neu */
    if (verbose) printf("test for dill_neu");
    for (i=0 ; i < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; i++) {
        unsigned int source1_u = br_srcu_vals[i];
        for (j=0 ; j < sizeof(br_srcu_vals)/sizeof(br_srcu_vals[0]) ; j++) {
            unsigned int source2_u = br_srcu_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a, unsigned int b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%u%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_neu(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u != source2_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u, source2_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_neu test, expected %d, got %d, for %u != %u\n",
		       expected_result, result,  source1_u,  source2_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_neul */
    if (verbose) printf("test for dill_neul");
    for (i=0 ; i < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = br_srcul_vals[i];
        for (j=0 ; j < sizeof(br_srcul_vals)/sizeof(br_srcul_vals[0]) ; j++) {
            UIMM_TYPE source2_ul = br_srcul_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, UIMM_TYPE a, UIMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%ul%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_neul(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul != source2_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul, source2_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_neul test, expected %d, got %d, for %zu != %zu\n",
		       expected_result, result,  source1_ul,  source2_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_nel */
    if (verbose) printf("test for dill_nel");
    for (i=0 ; i < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; i++) {
        IMM_TYPE source1_l = br_srcl_vals[i];
        for (j=0 ; j < sizeof(br_srcl_vals)/sizeof(br_srcl_vals[0]) ; j++) {
            IMM_TYPE source2_l = br_srcl_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, IMM_TYPE a, IMM_TYPE b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%l%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_nel(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l != source2_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l, source2_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_nel test, expected %d, got %d, for %zx != %zx\n",
		       expected_result, result,  source1_l,  source2_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_nep */
    if (verbose) printf("test for dill_nep");
    for (i=0 ; i < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; i++) {
        char* source1_p = br_srcp_vals[i];
        for (j=0 ; j < sizeof(br_srcp_vals)/sizeof(br_srcp_vals[0]) ; j++) {
            char* source2_p = br_srcp_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, char* a, char* b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    if (((IMM_TYPE)source2_p < 0) || ((IMM_TYPE)source1_p < 0))goto skip51;
	    dill_start_proc(c, "no name", DILL_I, "%EC%p%p");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_nep(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, char*, char*)) dill_get_fp(h);
	    expected_result = source1_p != source2_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p, source2_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_nep test, expected %d, got %d, for %p != %p\n",
		       expected_result, result, (void*) source1_p, (void*) source2_p);
		dill_dump(c);
		failed++;
	    }
skip51: ;
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ned */
    if (verbose) printf("test for dill_ned");
    for (i=0 ; i < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; i++) {
        double source1_d = br_srcd_vals[i];
        for (j=0 ; j < sizeof(br_srcd_vals)/sizeof(br_srcd_vals[0]) ; j++) {
            double source2_d = br_srcd_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, double a, double b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%d%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_ned(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, double, double)) dill_get_fp(h);
	    expected_result = source1_d != source2_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d, source2_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ned test, expected %d, got %d, for %g != %g\n",
		       expected_result, result,  source1_d,  source2_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_nef */
    if (verbose) printf("test for dill_nef");
    for (i=0 ; i < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; i++) {
        float source1_f = br_srcf_vals[i];
        for (j=0 ; j < sizeof(br_srcf_vals)/sizeof(br_srcf_vals[0]) ; j++) {
            float source2_f = br_srcf_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, float a, float b);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    
	    dill_start_proc(c, "no name", DILL_I, "%EC%f%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	
	    dest = dill_getreg(c, DILL_I);
	    dill_nef(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, float, float)) dill_get_fp(h);
	    expected_result = source1_f != source2_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f, source2_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_nef test, expected %d, got %d, for %g != %g\n",
		       expected_result, result,  source1_f,  source2_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");
    return failed;
}

static int
test_convert(dill_stream c)
{
    int failed = 0;
    size_t i, j;
    (void)i; (void)j;  /* suppress unused warnings */

    /* test for dill_cvc2uc */
    if (verbose) printf("test for dill_cvc2uc");
    for (i=0 ; i < sizeof(src1c_vals)/sizeof(src1c_vals[0]) ; i++) {
        signed char source1_c = src1c_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned char result;
	    unsigned char expected_result;
	    unsigned char (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UC, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UC);
	    dill_cvc2uc(c, dest, opnd1);
	    dill_retuc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned char(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (unsigned char) source1_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvc2uc test, expected %u, got %u, for (unsigned char) %d\n",
		        expected_result,  result,  source1_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvc2s */
    if (verbose) printf("test for dill_cvc2s");
    for (i=0 ; i < sizeof(src1c_vals)/sizeof(src1c_vals[0]) ; i++) {
        signed char source1_c = src1c_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    short result;
	    short expected_result;
	    short (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_S, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_S);
	    dill_cvc2s(c, dest, opnd1);
	    dill_rets(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (short(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (short) source1_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvc2s test, expected %d, got %d, for (short) %d\n",
		        expected_result,  result,  source1_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvc2us */
    if (verbose) printf("test for dill_cvc2us");
    for (i=0 ; i < sizeof(src1c_vals)/sizeof(src1c_vals[0]) ; i++) {
        signed char source1_c = src1c_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned short result;
	    unsigned short expected_result;
	    unsigned short (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_US, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_US);
	    dill_cvc2us(c, dest, opnd1);
	    dill_retus(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned short(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (unsigned short) source1_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvc2us test, expected %u, got %u, for (unsigned short) %d\n",
		        expected_result,  result,  source1_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvc2i */
    if (verbose) printf("test for dill_cvc2i");
    for (i=0 ; i < sizeof(src1c_vals)/sizeof(src1c_vals[0]) ; i++) {
        signed char source1_c = src1c_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_cvc2i(c, dest, opnd1);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (int) source1_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvc2i test, expected %d, got %d, for (int) %d\n",
		        expected_result,  result,  source1_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvc2u */
    if (verbose) printf("test for dill_cvc2u");
    for (i=0 ; i < sizeof(src1c_vals)/sizeof(src1c_vals[0]) ; i++) {
        signed char source1_c = src1c_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_U, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_cvc2u(c, dest, opnd1);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (unsigned int) source1_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvc2u test, expected %u, got %u, for (unsigned int) %d\n",
		        expected_result,  result,  source1_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvc2ul */
    if (verbose) printf("test for dill_cvc2ul");
    for (i=0 ; i < sizeof(src1c_vals)/sizeof(src1c_vals[0]) ; i++) {
        signed char source1_c = src1c_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UL, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_cvc2ul(c, dest, opnd1);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (UIMM_TYPE) source1_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvc2ul test, expected %zu, got %zu, for (UIMM_TYPE) %d\n",
		        expected_result,  result,  source1_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvc2l */
    if (verbose) printf("test for dill_cvc2l");
    for (i=0 ; i < sizeof(src1c_vals)/sizeof(src1c_vals[0]) ; i++) {
        signed char source1_c = src1c_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_L, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_cvc2l(c, dest, opnd1);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (IMM_TYPE) source1_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvc2l test, expected %zx, got %zx, for (IMM_TYPE) %d\n",
		        expected_result,  result,  source1_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvd2c */
    if (verbose) printf("test for dill_cvd2c");
    for (i=0 ; i < sizeof(src1d_vals)/sizeof(src1d_vals[0]) ; i++) {
        double source1_d = src1d_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    signed char result;
	    signed char expected_result;
	    signed char (*proc)(dill_exec_ctx ec, double a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_C, "%EC%d");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_C);
	    dill_cvd2c(c, dest, opnd1);
	    dill_retc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (signed char(*)(dill_exec_ctx, double)) dill_get_fp(h);
	    expected_result = (signed char) source1_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvd2c test, expected %d, got %d, for (signed char) %g\n",
		        expected_result,  result,  source1_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvd2uc */
    if (verbose) printf("test for dill_cvd2uc");
    for (i=0 ; i < sizeof(src1d_vals)/sizeof(src1d_vals[0]) ; i++) {
        double source1_d = src1d_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned char result;
	    unsigned char expected_result;
	    unsigned char (*proc)(dill_exec_ctx ec, double a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UC, "%EC%d");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UC);
	    dill_cvd2uc(c, dest, opnd1);
	    dill_retuc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned char(*)(dill_exec_ctx, double)) dill_get_fp(h);
	    expected_result = (unsigned char) source1_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvd2uc test, expected %u, got %u, for (unsigned char) %g\n",
		        expected_result,  result,  source1_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvd2s */
    if (verbose) printf("test for dill_cvd2s");
    for (i=0 ; i < sizeof(src1d_vals)/sizeof(src1d_vals[0]) ; i++) {
        double source1_d = src1d_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    short result;
	    short expected_result;
	    short (*proc)(dill_exec_ctx ec, double a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_S, "%EC%d");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_S);
	    dill_cvd2s(c, dest, opnd1);
	    dill_rets(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (short(*)(dill_exec_ctx, double)) dill_get_fp(h);
	    expected_result = (short) source1_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvd2s test, expected %d, got %d, for (short) %g\n",
		        expected_result,  result,  source1_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvd2us */
    if (verbose) printf("test for dill_cvd2us");
    for (i=0 ; i < sizeof(src1d_vals)/sizeof(src1d_vals[0]) ; i++) {
        double source1_d = src1d_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned short result;
	    unsigned short expected_result;
	    unsigned short (*proc)(dill_exec_ctx ec, double a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_US, "%EC%d");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_US);
	    dill_cvd2us(c, dest, opnd1);
	    dill_retus(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned short(*)(dill_exec_ctx, double)) dill_get_fp(h);
	    expected_result = (unsigned short) source1_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvd2us test, expected %u, got %u, for (unsigned short) %g\n",
		        expected_result,  result,  source1_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvd2i */
    if (verbose) printf("test for dill_cvd2i");
    for (i=0 ; i < sizeof(src1d_vals)/sizeof(src1d_vals[0]) ; i++) {
        double source1_d = src1d_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, double a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_I, "%EC%d");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_cvd2i(c, dest, opnd1);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, double)) dill_get_fp(h);
	    expected_result = (int) source1_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvd2i test, expected %d, got %d, for (int) %g\n",
		        expected_result,  result,  source1_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvd2u */
    if (verbose) printf("test for dill_cvd2u");
    for (i=0 ; i < sizeof(src1d_vals)/sizeof(src1d_vals[0]) ; i++) {
        double source1_d = src1d_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, double a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_U, "%EC%d");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_cvd2u(c, dest, opnd1);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, double)) dill_get_fp(h);
	    expected_result = (unsigned int) source1_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvd2u test, expected %u, got %u, for (unsigned int) %g\n",
		        expected_result,  result,  source1_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvd2ul */
    if (verbose) printf("test for dill_cvd2ul");
    for (i=0 ; i < sizeof(src1d_vals)/sizeof(src1d_vals[0]) ; i++) {
        double source1_d = src1d_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, double a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UL, "%EC%d");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_cvd2ul(c, dest, opnd1);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, double)) dill_get_fp(h);
	    expected_result = (UIMM_TYPE) source1_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvd2ul test, expected %zu, got %zu, for (UIMM_TYPE) %g\n",
		        expected_result,  result,  source1_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvd2l */
    if (verbose) printf("test for dill_cvd2l");
    for (i=0 ; i < sizeof(src1d_vals)/sizeof(src1d_vals[0]) ; i++) {
        double source1_d = src1d_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, double a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_L, "%EC%d");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_cvd2l(c, dest, opnd1);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, double)) dill_get_fp(h);
	    expected_result = (IMM_TYPE) source1_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvd2l test, expected %zx, got %zx, for (IMM_TYPE) %g\n",
		        expected_result,  result,  source1_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvf2c */
    if (verbose) printf("test for dill_cvf2c");
    for (i=0 ; i < sizeof(src1f_vals)/sizeof(src1f_vals[0]) ; i++) {
        float source1_f = src1f_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    signed char result;
	    signed char expected_result;
	    signed char (*proc)(dill_exec_ctx ec, float a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_C, "%EC%f");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_C);
	    dill_cvf2c(c, dest, opnd1);
	    dill_retc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (signed char(*)(dill_exec_ctx, float)) dill_get_fp(h);
	    expected_result = (signed char) source1_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvf2c test, expected %d, got %d, for (signed char) %g\n",
		        expected_result,  result,  source1_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvf2uc */
    if (verbose) printf("test for dill_cvf2uc");
    for (i=0 ; i < sizeof(src1f_vals)/sizeof(src1f_vals[0]) ; i++) {
        float source1_f = src1f_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned char result;
	    unsigned char expected_result;
	    unsigned char (*proc)(dill_exec_ctx ec, float a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UC, "%EC%f");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UC);
	    dill_cvf2uc(c, dest, opnd1);
	    dill_retuc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned char(*)(dill_exec_ctx, float)) dill_get_fp(h);
	    expected_result = (unsigned char) source1_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvf2uc test, expected %u, got %u, for (unsigned char) %g\n",
		        expected_result,  result,  source1_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvf2s */
    if (verbose) printf("test for dill_cvf2s");
    for (i=0 ; i < sizeof(src1f_vals)/sizeof(src1f_vals[0]) ; i++) {
        float source1_f = src1f_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    short result;
	    short expected_result;
	    short (*proc)(dill_exec_ctx ec, float a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_S, "%EC%f");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_S);
	    dill_cvf2s(c, dest, opnd1);
	    dill_rets(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (short(*)(dill_exec_ctx, float)) dill_get_fp(h);
	    expected_result = (short) source1_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvf2s test, expected %d, got %d, for (short) %g\n",
		        expected_result,  result,  source1_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvf2us */
    if (verbose) printf("test for dill_cvf2us");
    for (i=0 ; i < sizeof(src1f_vals)/sizeof(src1f_vals[0]) ; i++) {
        float source1_f = src1f_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned short result;
	    unsigned short expected_result;
	    unsigned short (*proc)(dill_exec_ctx ec, float a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_US, "%EC%f");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_US);
	    dill_cvf2us(c, dest, opnd1);
	    dill_retus(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned short(*)(dill_exec_ctx, float)) dill_get_fp(h);
	    expected_result = (unsigned short) source1_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvf2us test, expected %u, got %u, for (unsigned short) %g\n",
		        expected_result,  result,  source1_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvf2i */
    if (verbose) printf("test for dill_cvf2i");
    for (i=0 ; i < sizeof(src1f_vals)/sizeof(src1f_vals[0]) ; i++) {
        float source1_f = src1f_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, float a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_I, "%EC%f");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_cvf2i(c, dest, opnd1);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, float)) dill_get_fp(h);
	    expected_result = (int) source1_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvf2i test, expected %d, got %d, for (int) %g\n",
		        expected_result,  result,  source1_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvf2u */
    if (verbose) printf("test for dill_cvf2u");
    for (i=0 ; i < sizeof(src1f_vals)/sizeof(src1f_vals[0]) ; i++) {
        float source1_f = src1f_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, float a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_U, "%EC%f");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_cvf2u(c, dest, opnd1);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, float)) dill_get_fp(h);
	    expected_result = (unsigned int) source1_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvf2u test, expected %u, got %u, for (unsigned int) %g\n",
		        expected_result,  result,  source1_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvf2ul */
    if (verbose) printf("test for dill_cvf2ul");
    for (i=0 ; i < sizeof(src1f_vals)/sizeof(src1f_vals[0]) ; i++) {
        float source1_f = src1f_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, float a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UL, "%EC%f");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_cvf2ul(c, dest, opnd1);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, float)) dill_get_fp(h);
	    expected_result = (UIMM_TYPE) source1_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvf2ul test, expected %zu, got %zu, for (UIMM_TYPE) %g\n",
		        expected_result,  result,  source1_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvf2l */
    if (verbose) printf("test for dill_cvf2l");
    for (i=0 ; i < sizeof(src1f_vals)/sizeof(src1f_vals[0]) ; i++) {
        float source1_f = src1f_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, float a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_L, "%EC%f");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_cvf2l(c, dest, opnd1);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, float)) dill_get_fp(h);
	    expected_result = (IMM_TYPE) source1_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvf2l test, expected %zx, got %zx, for (IMM_TYPE) %g\n",
		        expected_result,  result,  source1_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvi2c */
    if (verbose) printf("test for dill_cvi2c");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    signed char result;
	    signed char expected_result;
	    signed char (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_C, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_C);
	    dill_cvi2c(c, dest, opnd1);
	    dill_retc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (signed char(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = (signed char) source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvi2c test, expected %d, got %d, for (signed char) %d\n",
		        expected_result,  result,  source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvi2uc */
    if (verbose) printf("test for dill_cvi2uc");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned char result;
	    unsigned char expected_result;
	    unsigned char (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UC, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UC);
	    dill_cvi2uc(c, dest, opnd1);
	    dill_retuc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned char(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = (unsigned char) source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvi2uc test, expected %u, got %u, for (unsigned char) %d\n",
		        expected_result,  result,  source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvi2s */
    if (verbose) printf("test for dill_cvi2s");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    short result;
	    short expected_result;
	    short (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_S, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_S);
	    dill_cvi2s(c, dest, opnd1);
	    dill_rets(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (short(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = (short) source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvi2s test, expected %d, got %d, for (short) %d\n",
		        expected_result,  result,  source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvi2us */
    if (verbose) printf("test for dill_cvi2us");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned short result;
	    unsigned short expected_result;
	    unsigned short (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_US, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_US);
	    dill_cvi2us(c, dest, opnd1);
	    dill_retus(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned short(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = (unsigned short) source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvi2us test, expected %u, got %u, for (unsigned short) %d\n",
		        expected_result,  result,  source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvi2u */
    if (verbose) printf("test for dill_cvi2u");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_U, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_cvi2u(c, dest, opnd1);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = (unsigned int) source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvi2u test, expected %u, got %u, for (unsigned int) %d\n",
		        expected_result,  result,  source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvi2ul */
    if (verbose) printf("test for dill_cvi2ul");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UL, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_cvi2ul(c, dest, opnd1);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = (UIMM_TYPE) source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvi2ul test, expected %zu, got %zu, for (UIMM_TYPE) %d\n",
		        expected_result,  result,  source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvi2l */
    if (verbose) printf("test for dill_cvi2l");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_L, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_cvi2l(c, dest, opnd1);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = (IMM_TYPE) source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvi2l test, expected %zx, got %zx, for (IMM_TYPE) %d\n",
		        expected_result,  result,  source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvl2c */
    if (verbose) printf("test for dill_cvl2c");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    signed char result;
	    signed char expected_result;
	    signed char (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_C, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_C);
	    dill_cvl2c(c, dest, opnd1);
	    dill_retc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (signed char(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = (signed char) source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvl2c test, expected %d, got %d, for (signed char) %zx\n",
		        expected_result,  result,  source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvl2uc */
    if (verbose) printf("test for dill_cvl2uc");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned char result;
	    unsigned char expected_result;
	    unsigned char (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UC, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UC);
	    dill_cvl2uc(c, dest, opnd1);
	    dill_retuc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned char(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = (unsigned char) source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvl2uc test, expected %u, got %u, for (unsigned char) %zx\n",
		        expected_result,  result,  source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvl2s */
    if (verbose) printf("test for dill_cvl2s");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    short result;
	    short expected_result;
	    short (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_S, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_S);
	    dill_cvl2s(c, dest, opnd1);
	    dill_rets(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (short(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = (short) source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvl2s test, expected %d, got %d, for (short) %zx\n",
		        expected_result,  result,  source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvl2us */
    if (verbose) printf("test for dill_cvl2us");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned short result;
	    unsigned short expected_result;
	    unsigned short (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_US, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_US);
	    dill_cvl2us(c, dest, opnd1);
	    dill_retus(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned short(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = (unsigned short) source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvl2us test, expected %u, got %u, for (unsigned short) %zx\n",
		        expected_result,  result,  source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvl2i */
    if (verbose) printf("test for dill_cvl2i");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_I, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_cvl2i(c, dest, opnd1);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = (int) source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvl2i test, expected %d, got %d, for (int) %zx\n",
		        expected_result,  result,  source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvl2u */
    if (verbose) printf("test for dill_cvl2u");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_U, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_cvl2u(c, dest, opnd1);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = (unsigned int) source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvl2u test, expected %u, got %u, for (unsigned int) %zx\n",
		        expected_result,  result,  source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvl2ul */
    if (verbose) printf("test for dill_cvl2ul");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UL, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_cvl2ul(c, dest, opnd1);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = (UIMM_TYPE) source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvl2ul test, expected %zu, got %zu, for (UIMM_TYPE) %zx\n",
		        expected_result,  result,  source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvs2c */
    if (verbose) printf("test for dill_cvs2c");
    for (i=0 ; i < sizeof(src1s_vals)/sizeof(src1s_vals[0]) ; i++) {
        short source1_s = src1s_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    signed char result;
	    signed char expected_result;
	    signed char (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_C, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_C);
	    dill_cvs2c(c, dest, opnd1);
	    dill_retc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (signed char(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (signed char) source1_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvs2c test, expected %d, got %d, for (signed char) %d\n",
		        expected_result,  result,  source1_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvs2uc */
    if (verbose) printf("test for dill_cvs2uc");
    for (i=0 ; i < sizeof(src1s_vals)/sizeof(src1s_vals[0]) ; i++) {
        short source1_s = src1s_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned char result;
	    unsigned char expected_result;
	    unsigned char (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UC, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UC);
	    dill_cvs2uc(c, dest, opnd1);
	    dill_retuc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned char(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (unsigned char) source1_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvs2uc test, expected %u, got %u, for (unsigned char) %d\n",
		        expected_result,  result,  source1_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvs2us */
    if (verbose) printf("test for dill_cvs2us");
    for (i=0 ; i < sizeof(src1s_vals)/sizeof(src1s_vals[0]) ; i++) {
        short source1_s = src1s_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned short result;
	    unsigned short expected_result;
	    unsigned short (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_US, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_US);
	    dill_cvs2us(c, dest, opnd1);
	    dill_retus(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned short(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (unsigned short) source1_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvs2us test, expected %u, got %u, for (unsigned short) %d\n",
		        expected_result,  result,  source1_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvs2i */
    if (verbose) printf("test for dill_cvs2i");
    for (i=0 ; i < sizeof(src1s_vals)/sizeof(src1s_vals[0]) ; i++) {
        short source1_s = src1s_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_I, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_cvs2i(c, dest, opnd1);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (int) source1_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvs2i test, expected %d, got %d, for (int) %d\n",
		        expected_result,  result,  source1_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvs2u */
    if (verbose) printf("test for dill_cvs2u");
    for (i=0 ; i < sizeof(src1s_vals)/sizeof(src1s_vals[0]) ; i++) {
        short source1_s = src1s_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_U, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_cvs2u(c, dest, opnd1);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (unsigned int) source1_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvs2u test, expected %u, got %u, for (unsigned int) %d\n",
		        expected_result,  result,  source1_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvs2ul */
    if (verbose) printf("test for dill_cvs2ul");
    for (i=0 ; i < sizeof(src1s_vals)/sizeof(src1s_vals[0]) ; i++) {
        short source1_s = src1s_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UL, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_cvs2ul(c, dest, opnd1);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (UIMM_TYPE) source1_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvs2ul test, expected %zu, got %zu, for (UIMM_TYPE) %d\n",
		        expected_result,  result,  source1_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvs2l */
    if (verbose) printf("test for dill_cvs2l");
    for (i=0 ; i < sizeof(src1s_vals)/sizeof(src1s_vals[0]) ; i++) {
        short source1_s = src1s_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_L, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_cvs2l(c, dest, opnd1);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (IMM_TYPE) source1_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvs2l test, expected %zx, got %zx, for (IMM_TYPE) %d\n",
		        expected_result,  result,  source1_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvu2c */
    if (verbose) printf("test for dill_cvu2c");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    signed char result;
	    signed char expected_result;
	    signed char (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_C, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_C);
	    dill_cvu2c(c, dest, opnd1);
	    dill_retc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (signed char(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (signed char) source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvu2c test, expected %d, got %d, for (signed char) %u\n",
		        expected_result,  result,  source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvu2uc */
    if (verbose) printf("test for dill_cvu2uc");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned char result;
	    unsigned char expected_result;
	    unsigned char (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UC, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UC);
	    dill_cvu2uc(c, dest, opnd1);
	    dill_retuc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned char(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (unsigned char) source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvu2uc test, expected %u, got %u, for (unsigned char) %u\n",
		        expected_result,  result,  source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvu2s */
    if (verbose) printf("test for dill_cvu2s");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    short result;
	    short expected_result;
	    short (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_S, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_S);
	    dill_cvu2s(c, dest, opnd1);
	    dill_rets(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (short(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (short) source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvu2s test, expected %d, got %d, for (short) %u\n",
		        expected_result,  result,  source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvu2us */
    if (verbose) printf("test for dill_cvu2us");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned short result;
	    unsigned short expected_result;
	    unsigned short (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_US, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_US);
	    dill_cvu2us(c, dest, opnd1);
	    dill_retus(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned short(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (unsigned short) source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvu2us test, expected %u, got %u, for (unsigned short) %u\n",
		        expected_result,  result,  source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvu2i */
    if (verbose) printf("test for dill_cvu2i");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_cvu2i(c, dest, opnd1);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (int) source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvu2i test, expected %d, got %d, for (int) %u\n",
		        expected_result,  result,  source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvu2ul */
    if (verbose) printf("test for dill_cvu2ul");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UL, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_cvu2ul(c, dest, opnd1);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (UIMM_TYPE) source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvu2ul test, expected %zu, got %zu, for (UIMM_TYPE) %u\n",
		        expected_result,  result,  source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvu2l */
    if (verbose) printf("test for dill_cvu2l");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_L, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_cvu2l(c, dest, opnd1);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (IMM_TYPE) source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvu2l test, expected %zx, got %zx, for (IMM_TYPE) %u\n",
		        expected_result,  result,  source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvul2c */
    if (verbose) printf("test for dill_cvul2c");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    signed char result;
	    signed char expected_result;
	    signed char (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_C, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_C);
	    dill_cvul2c(c, dest, opnd1);
	    dill_retc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (signed char(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = (signed char) source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvul2c test, expected %d, got %d, for (signed char) %zu\n",
		        expected_result,  result,  source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvul2uc */
    if (verbose) printf("test for dill_cvul2uc");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned char result;
	    unsigned char expected_result;
	    unsigned char (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UC, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UC);
	    dill_cvul2uc(c, dest, opnd1);
	    dill_retuc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned char(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = (unsigned char) source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvul2uc test, expected %u, got %u, for (unsigned char) %zu\n",
		        expected_result,  result,  source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvul2s */
    if (verbose) printf("test for dill_cvul2s");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    short result;
	    short expected_result;
	    short (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_S, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_S);
	    dill_cvul2s(c, dest, opnd1);
	    dill_rets(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (short(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = (short) source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvul2s test, expected %d, got %d, for (short) %zu\n",
		        expected_result,  result,  source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvul2us */
    if (verbose) printf("test for dill_cvul2us");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned short result;
	    unsigned short expected_result;
	    unsigned short (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_US, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_US);
	    dill_cvul2us(c, dest, opnd1);
	    dill_retus(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned short(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = (unsigned short) source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvul2us test, expected %u, got %u, for (unsigned short) %zu\n",
		        expected_result,  result,  source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvul2i */
    if (verbose) printf("test for dill_cvul2i");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_I, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_cvul2i(c, dest, opnd1);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = (int) source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvul2i test, expected %d, got %d, for (int) %zu\n",
		        expected_result,  result,  source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvul2u */
    if (verbose) printf("test for dill_cvul2u");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_U, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_cvul2u(c, dest, opnd1);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = (unsigned int) source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvul2u test, expected %u, got %u, for (unsigned int) %zu\n",
		        expected_result,  result,  source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvul2l */
    if (verbose) printf("test for dill_cvul2l");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_L, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_cvul2l(c, dest, opnd1);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = (IMM_TYPE) source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvul2l test, expected %zx, got %zx, for (IMM_TYPE) %zu\n",
		        expected_result,  result,  source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvus2c */
    if (verbose) printf("test for dill_cvus2c");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    signed char result;
	    signed char expected_result;
	    signed char (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_C, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_C);
	    dill_cvus2c(c, dest, opnd1);
	    dill_retc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (signed char(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (signed char) source1_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvus2c test, expected %d, got %d, for (signed char) %u\n",
		        expected_result,  result,  source1_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvus2uc */
    if (verbose) printf("test for dill_cvus2uc");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned char result;
	    unsigned char expected_result;
	    unsigned char (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UC, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UC);
	    dill_cvus2uc(c, dest, opnd1);
	    dill_retuc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned char(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (unsigned char) source1_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvus2uc test, expected %u, got %u, for (unsigned char) %u\n",
		        expected_result,  result,  source1_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvus2s */
    if (verbose) printf("test for dill_cvus2s");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    short result;
	    short expected_result;
	    short (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_S, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_S);
	    dill_cvus2s(c, dest, opnd1);
	    dill_rets(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (short(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (short) source1_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvus2s test, expected %d, got %d, for (short) %u\n",
		        expected_result,  result,  source1_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvus2i */
    if (verbose) printf("test for dill_cvus2i");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_I, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_cvus2i(c, dest, opnd1);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (int) source1_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvus2i test, expected %d, got %d, for (int) %u\n",
		        expected_result,  result,  source1_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvus2u */
    if (verbose) printf("test for dill_cvus2u");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_U, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_cvus2u(c, dest, opnd1);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (unsigned int) source1_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvus2u test, expected %u, got %u, for (unsigned int) %u\n",
		        expected_result,  result,  source1_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvus2ul */
    if (verbose) printf("test for dill_cvus2ul");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UL, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_cvus2ul(c, dest, opnd1);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (UIMM_TYPE) source1_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvus2ul test, expected %zu, got %zu, for (UIMM_TYPE) %u\n",
		        expected_result,  result,  source1_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvus2l */
    if (verbose) printf("test for dill_cvus2l");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_L, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_cvus2l(c, dest, opnd1);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (IMM_TYPE) source1_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvus2l test, expected %zx, got %zx, for (IMM_TYPE) %u\n",
		        expected_result,  result,  source1_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvd2f */
    if (verbose) printf("test for dill_cvd2f");
    for (i=0 ; i < sizeof(src1d_vals)/sizeof(src1d_vals[0]) ; i++) {
        double source1_d = src1d_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec, double a);
	    dill_exec_handle h;
	    double range = 0.000001 * fabs((double)source1_d);
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_F, "%EC%d");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_F);
	    dill_cvd2f(c, dest, opnd1);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx, double)) dill_get_fp(h);
	    expected_result = (float) source1_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_cvd2f test, expected %g, got %g, for (float) %g\n",
		        expected_result,  result,  source1_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvf2d */
    if (verbose) printf("test for dill_cvf2d");
    for (i=0 ; i < sizeof(src1f_vals)/sizeof(src1f_vals[0]) ; i++) {
        float source1_f = src1f_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec, float a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_D, "%EC%f");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_D);
	    dill_cvf2d(c, dest, opnd1);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx, float)) dill_get_fp(h);
	    expected_result = (double) source1_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvf2d test, expected %g, got %g, for (double) %g\n",
		        expected_result,  result,  source1_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvc2f */
    if (verbose) printf("test for dill_cvc2f");
    for (i=0 ; i < sizeof(src1c_vals)/sizeof(src1c_vals[0]) ; i++) {
        signed char source1_c = src1c_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    double range = 0.000001 * fabs((double)source1_c);
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_F, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_F);
	    dill_cvc2f(c, dest, opnd1);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (float) source1_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_cvc2f test, expected %g, got %g, for (float) %d\n",
		        expected_result,  result,  source1_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvc2d */
    if (verbose) printf("test for dill_cvc2d");
    for (i=0 ; i < sizeof(src1c_vals)/sizeof(src1c_vals[0]) ; i++) {
        signed char source1_c = src1c_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_D, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_D);
	    dill_cvc2d(c, dest, opnd1);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (double) source1_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvc2d test, expected %g, got %g, for (double) %d\n",
		        expected_result,  result,  source1_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvus2f */
    if (verbose) printf("test for dill_cvus2f");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    double range = 0.000001 * fabs((double)source1_us);
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_F, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_F);
	    dill_cvus2f(c, dest, opnd1);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (float) source1_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_cvus2f test, expected %g, got %g, for (float) %u\n",
		        expected_result,  result,  source1_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvus2d */
    if (verbose) printf("test for dill_cvus2d");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_D, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_D);
	    dill_cvus2d(c, dest, opnd1);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (double) source1_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvus2d test, expected %g, got %g, for (double) %u\n",
		        expected_result,  result,  source1_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvs2f */
    if (verbose) printf("test for dill_cvs2f");
    for (i=0 ; i < sizeof(src1s_vals)/sizeof(src1s_vals[0]) ; i++) {
        short source1_s = src1s_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    double range = 0.000001 * fabs((double)source1_s);
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_F, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_F);
	    dill_cvs2f(c, dest, opnd1);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (float) source1_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_cvs2f test, expected %g, got %g, for (float) %d\n",
		        expected_result,  result,  source1_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvs2d */
    if (verbose) printf("test for dill_cvs2d");
    for (i=0 ; i < sizeof(src1s_vals)/sizeof(src1s_vals[0]) ; i++) {
        short source1_s = src1s_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec, signed int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_D, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_D);
	    dill_cvs2d(c, dest, opnd1);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx, signed int)) dill_get_fp(h);
	    expected_result = (double) source1_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvs2d test, expected %g, got %g, for (double) %d\n",
		        expected_result,  result,  source1_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvus2f */
    if (verbose) printf("test for dill_cvus2f");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    double range = 0.000001 * fabs((double)source1_us);
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_F, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_F);
	    dill_cvus2f(c, dest, opnd1);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (float) source1_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_cvus2f test, expected %g, got %g, for (float) %u\n",
		        expected_result,  result,  source1_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvus2d */
    if (verbose) printf("test for dill_cvus2d");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_D, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_D);
	    dill_cvus2d(c, dest, opnd1);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (double) source1_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvus2d test, expected %g, got %g, for (double) %u\n",
		        expected_result,  result,  source1_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvi2f */
    if (verbose) printf("test for dill_cvi2f");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;
	    double range = 0.000001 * fabs((double)source1_i);
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_F, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_F);
	    dill_cvi2f(c, dest, opnd1);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = (float) source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_cvi2f test, expected %g, got %g, for (float) %d\n",
		        expected_result,  result,  source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvi2d */
    if (verbose) printf("test for dill_cvi2d");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_D, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_D);
	    dill_cvi2d(c, dest, opnd1);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = (double) source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvi2d test, expected %g, got %g, for (double) %d\n",
		        expected_result,  result,  source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvl2f */
    if (verbose) printf("test for dill_cvl2f");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;
	    double range = 0.000001 * fabs((double)source1_l);
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_F, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_F);
	    dill_cvl2f(c, dest, opnd1);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = (float) source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_cvl2f test, expected %g, got %g, for (float) %zx\n",
		        expected_result,  result,  source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvl2d */
    if (verbose) printf("test for dill_cvl2d");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_D, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_D);
	    dill_cvl2d(c, dest, opnd1);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = (double) source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvl2d test, expected %g, got %g, for (double) %zx\n",
		        expected_result,  result,  source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvu2f */
    if (verbose) printf("test for dill_cvu2f");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    double range = 0.000001 * fabs((double)source1_u);
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_F, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_F);
	    dill_cvu2f(c, dest, opnd1);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (float) source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_cvu2f test, expected %g, got %g, for (float) %u\n",
		        expected_result,  result,  source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvu2d */
    if (verbose) printf("test for dill_cvu2d");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_D, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_D);
	    dill_cvu2d(c, dest, opnd1);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (double) source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvu2d test, expected %g, got %g, for (double) %u\n",
		        expected_result,  result,  source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvul2f */
    if (verbose) printf("test for dill_cvul2f");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;
	    double range = 0.000001 * fabs((double)source1_ul);
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_F, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_F);
	    dill_cvul2f(c, dest, opnd1);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = (float) source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if ((result > (expected_result + range)) || (result < (expected_result - range))) {
		printf("Failed dill_cvul2f test, expected %g, got %g, for (float) %zu\n",
		        expected_result,  result,  source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvul2d */
    if (verbose) printf("test for dill_cvul2d");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_D, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_D);
	    dill_cvul2d(c, dest, opnd1);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = (double) source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvul2d test, expected %g, got %g, for (double) %zu\n",
		        expected_result,  result,  source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvi2c */
    if (verbose) printf("test for dill_cvi2c");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    signed char result;
	    signed char expected_result;
	    signed char (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_C, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_C);
	    dill_cvi2c(c, dest, opnd1);
	    dill_retc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (signed char(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = (signed char) source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvi2c test, expected %d, got %d, for (signed char) %d\n",
		        expected_result,  result,  source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvi2s */
    if (verbose) printf("test for dill_cvi2s");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    short result;
	    short expected_result;
	    short (*proc)(dill_exec_ctx ec, int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_S, "%EC%i");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_S);
	    dill_cvi2s(c, dest, opnd1);
	    dill_rets(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (short(*)(dill_exec_ctx, int)) dill_get_fp(h);
	    expected_result = (short) source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvi2s test, expected %d, got %d, for (short) %d\n",
		        expected_result,  result,  source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvl2c */
    if (verbose) printf("test for dill_cvl2c");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    signed char result;
	    signed char expected_result;
	    signed char (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_C, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_C);
	    dill_cvl2c(c, dest, opnd1);
	    dill_retc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (signed char(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = (signed char) source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvl2c test, expected %d, got %d, for (signed char) %zx\n",
		        expected_result,  result,  source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvl2s */
    if (verbose) printf("test for dill_cvl2s");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    short result;
	    short expected_result;
	    short (*proc)(dill_exec_ctx ec, IMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_S, "%EC%l");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_S);
	    dill_cvl2s(c, dest, opnd1);
	    dill_rets(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (short(*)(dill_exec_ctx, IMM_TYPE)) dill_get_fp(h);
	    expected_result = (short) source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvl2s test, expected %d, got %d, for (short) %zx\n",
		        expected_result,  result,  source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvu2c */
    if (verbose) printf("test for dill_cvu2c");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    signed char result;
	    signed char expected_result;
	    signed char (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_C, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_C);
	    dill_cvu2c(c, dest, opnd1);
	    dill_retc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (signed char(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (signed char) source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvu2c test, expected %d, got %d, for (signed char) %u\n",
		        expected_result,  result,  source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvu2s */
    if (verbose) printf("test for dill_cvu2s");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    short result;
	    short expected_result;
	    short (*proc)(dill_exec_ctx ec, unsigned int a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_S, "%EC%u");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_S);
	    dill_cvu2s(c, dest, opnd1);
	    dill_rets(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (short(*)(dill_exec_ctx, unsigned int)) dill_get_fp(h);
	    expected_result = (short) source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvu2s test, expected %d, got %d, for (short) %u\n",
		        expected_result,  result,  source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvul2c */
    if (verbose) printf("test for dill_cvul2c");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    signed char result;
	    signed char expected_result;
	    signed char (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_C, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_C);
	    dill_cvul2c(c, dest, opnd1);
	    dill_retc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (signed char(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = (signed char) source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvul2c test, expected %d, got %d, for (signed char) %zu\n",
		        expected_result,  result,  source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvul2s */
    if (verbose) printf("test for dill_cvul2s");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    short result;
	    short expected_result;
	    short (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_S, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_S);
	    dill_cvul2s(c, dest, opnd1);
	    dill_rets(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (short(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = (short) source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvul2s test, expected %d, got %d, for (short) %zu\n",
		        expected_result,  result,  source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvp2ul */
    if (verbose) printf("test for dill_cvp2ul");
    for (i=0 ; i < sizeof(src1p_vals)/sizeof(src1p_vals[0]) ; i++) {
        char* source1_p = src1p_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, char* a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_UL, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_cvp2ul(c, dest, opnd1);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, char*)) dill_get_fp(h);
	    expected_result = (UIMM_TYPE) source1_p;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvp2ul test, expected %zu, got %zu, for (UIMM_TYPE) %p\n",
		        expected_result,  result, (void*) source1_p);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_cvul2p */
    if (verbose) printf("test for dill_cvul2p");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
	{

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    char* result;
	    char* expected_result;
	    char* (*proc)(dill_exec_ctx ec, UIMM_TYPE a);
	    dill_exec_handle h;
	    
	    if (verbose) {printf(".");fflush(stdout);}

	    dill_start_proc(c, "no name", DILL_P, "%EC%ul");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_P);
	    dill_cvul2p(c, dest, opnd1);
	    dill_retp(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (char*(*)(dill_exec_ctx, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = (char*) source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_cvul2p test, expected %p, got %p, for (char*) %zu\n",
		       (void*) expected_result, (void*) result,  source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");
    return failed;
}

static int
test_load(dill_stream c)
{
    int failed = 0;
    size_t i, j;
    (void)i; (void)j;  /* suppress unused warnings */

    /* test for dill_ldc */
    if (verbose) printf("test for dill_ldc");
    for (i=0 ; i < sizeof(src1c_vals)/sizeof(src1c_vals[0]) ; i++) {
        signed char source1_c = src1c_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    signed char result;
	    signed char expected_result;
	    signed char (*proc)(dill_exec_ctx, void*);
            void *p = ((char*)&source1_c) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_C, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_C);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_ldc(c, dest, opnd1, opnd2);
	    dill_retc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (signed char(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ldc test, expected %d, got %d, for loading %d\n",
		       expected_result, result, source1_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lduc */
    if (verbose) printf("test for dill_lduc");
    for (i=0 ; i < sizeof(src1uc_vals)/sizeof(src1uc_vals[0]) ; i++) {
        unsigned char source1_uc = src1uc_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    unsigned char result;
	    unsigned char expected_result;
	    unsigned char (*proc)(dill_exec_ctx, void*);
            void *p = ((char*)&source1_uc) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_UC, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UC);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_lduc(c, dest, opnd1, opnd2);
	    dill_retuc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned char(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lduc test, expected %u, got %u, for loading %u\n",
		       expected_result, result, source1_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lds */
    if (verbose) printf("test for dill_lds");
    for (i=0 ; i < sizeof(src1s_vals)/sizeof(src1s_vals[0]) ; i++) {
        short source1_s = src1s_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    short result;
	    short expected_result;
	    short (*proc)(dill_exec_ctx, void*);
            void *p = ((char*)&source1_s) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_S, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_S);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_lds(c, dest, opnd1, opnd2);
	    dill_rets(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (short(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lds test, expected %d, got %d, for loading %d\n",
		       expected_result, result, source1_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ldus */
    if (verbose) printf("test for dill_ldus");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    unsigned short result;
	    unsigned short expected_result;
	    unsigned short (*proc)(dill_exec_ctx, void*);
            void *p = ((char*)&source1_us) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_US, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_US);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_ldus(c, dest, opnd1, opnd2);
	    dill_retus(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned short(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ldus test, expected %u, got %u, for loading %u\n",
		       expected_result, result, source1_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ldi */
    if (verbose) printf("test for dill_ldi");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx, void*);
            void *p = ((char*)&source1_i) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_I, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_ldi(c, dest, opnd1, opnd2);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ldi test, expected %d, got %d, for loading %d\n",
		       expected_result, result, source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ldu */
    if (verbose) printf("test for dill_ldu");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx, void*);
            void *p = ((char*)&source1_u) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_U, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_ldu(c, dest, opnd1, opnd2);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ldu test, expected %u, got %u, for loading %u\n",
		       expected_result, result, source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ldul */
    if (verbose) printf("test for dill_ldul");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx, void*);
            void *p = ((char*)&source1_ul) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_UL, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_ldul(c, dest, opnd1, opnd2);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ldul test, expected %zu, got %zu, for loading %zu\n",
		       expected_result, result, source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ldl */
    if (verbose) printf("test for dill_ldl");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx, void*);
            void *p = ((char*)&source1_l) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_L, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_ldl(c, dest, opnd1, opnd2);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ldl test, expected %zx, got %zx, for loading %zx\n",
		       expected_result, result, source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ldf */
    if (verbose) printf("test for dill_ldf");
    for (i=0 ; i < sizeof(src1f_vals)/sizeof(src1f_vals[0]) ; i++) {
        float source1_f = src1f_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx, void*);
            void *p = ((char*)&source1_f) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_F, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_F);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_ldf(c, dest, opnd1, opnd2);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ldf test, expected %g, got %g, for loading %g\n",
		       expected_result, result, source1_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ldd */
    if (verbose) printf("test for dill_ldd");
    for (i=0 ; i < sizeof(src1d_vals)/sizeof(src1d_vals[0]) ; i++) {
        double source1_d = src1d_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, dest;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx, void*);
            void *p = ((char*)&source1_d) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_D, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_D);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_ldd(c, dest, opnd1, opnd2);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ldd test, expected %g, got %g, for loading %g\n",
		       expected_result, result, source1_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ldci */
    if (verbose) printf("test for dill_ldci");
    for (i=0 ; i < sizeof(src1c_vals)/sizeof(src1c_vals[0]) ; i++) {
        signed char source1_c = src1c_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    signed char result;
	    signed char expected_result;
	    signed char (*proc)(dill_exec_ctx ec, void *p);
	    dill_exec_handle h;
            void *p;

	    offset &= ~3;
	    p = ((char*)&source1_c) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_C, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_C);
	    dill_ldci(c, dest, opnd1, offset);
	    dill_retc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (signed char(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ldci test, expected %d, got %d, for loading %d at offset %zx\n",
		       expected_result, result, source1_c, offset);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lduci */
    if (verbose) printf("test for dill_lduci");
    for (i=0 ; i < sizeof(src1uc_vals)/sizeof(src1uc_vals[0]) ; i++) {
        unsigned char source1_uc = src1uc_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned char result;
	    unsigned char expected_result;
	    unsigned char (*proc)(dill_exec_ctx ec, void *p);
	    dill_exec_handle h;
            void *p;

	    offset &= ~3;
	    p = ((char*)&source1_uc) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_UC, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UC);
	    dill_lduci(c, dest, opnd1, offset);
	    dill_retuc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned char(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lduci test, expected %u, got %u, for loading %u at offset %zx\n",
		       expected_result, result, source1_uc, offset);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ldsi */
    if (verbose) printf("test for dill_ldsi");
    for (i=0 ; i < sizeof(src1s_vals)/sizeof(src1s_vals[0]) ; i++) {
        short source1_s = src1s_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    short result;
	    short expected_result;
	    short (*proc)(dill_exec_ctx ec, void *p);
	    dill_exec_handle h;
            void *p;

	    offset &= ~3;
	    p = ((char*)&source1_s) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_S, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_S);
	    dill_ldsi(c, dest, opnd1, offset);
	    dill_rets(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (short(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ldsi test, expected %d, got %d, for loading %d at offset %zx\n",
		       expected_result, result, source1_s, offset);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ldusi */
    if (verbose) printf("test for dill_ldusi");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned short result;
	    unsigned short expected_result;
	    unsigned short (*proc)(dill_exec_ctx ec, void *p);
	    dill_exec_handle h;
            void *p;

	    offset &= ~3;
	    p = ((char*)&source1_us) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_US, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_US);
	    dill_ldusi(c, dest, opnd1, offset);
	    dill_retus(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned short(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ldusi test, expected %u, got %u, for loading %u at offset %zx\n",
		       expected_result, result, source1_us, offset);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ldii */
    if (verbose) printf("test for dill_ldii");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec, void *p);
	    dill_exec_handle h;
            void *p;

	    offset &= ~3;
	    p = ((char*)&source1_i) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_I, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_I);
	    dill_ldii(c, dest, opnd1, offset);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ldii test, expected %d, got %d, for loading %d at offset %zx\n",
		       expected_result, result, source1_i, offset);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ldui */
    if (verbose) printf("test for dill_ldui");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec, void *p);
	    dill_exec_handle h;
            void *p;

	    offset &= ~3;
	    p = ((char*)&source1_u) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_U, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_U);
	    dill_ldui(c, dest, opnd1, offset);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ldui test, expected %u, got %u, for loading %u at offset %zx\n",
		       expected_result, result, source1_u, offset);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lduli */
    if (verbose) printf("test for dill_lduli");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec, void *p);
	    dill_exec_handle h;
            void *p;

	    offset &= ~3;
	    p = ((char*)&source1_ul) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_UL, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_UL);
	    dill_lduli(c, dest, opnd1, offset);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lduli test, expected %zu, got %zu, for loading %zu at offset %zx\n",
		       expected_result, result, source1_ul, offset);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ldli */
    if (verbose) printf("test for dill_ldli");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec, void *p);
	    dill_exec_handle h;
            void *p;

	    offset &= ~3;
	    p = ((char*)&source1_l) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_L, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_L);
	    dill_ldli(c, dest, opnd1, offset);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ldli test, expected %zx, got %zx, for loading %zx at offset %zx\n",
		       expected_result, result, source1_l, offset);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_ldfi */
    if (verbose) printf("test for dill_ldfi");
    for (i=0 ; i < sizeof(src1f_vals)/sizeof(src1f_vals[0]) ; i++) {
        float source1_f = src1f_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec, void *p);
	    dill_exec_handle h;
            void *p;

	    offset &= ~3;
	    p = ((char*)&source1_f) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_F, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_F);
	    dill_ldfi(c, dest, opnd1, offset);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_ldfi test, expected %g, got %g, for loading %g at offset %zx\n",
		       expected_result, result, source1_f, offset);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_lddi */
    if (verbose) printf("test for dill_lddi");
    for (i=0 ; i < sizeof(src1d_vals)/sizeof(src1d_vals[0]) ; i++) {
        double source1_d = src1d_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec, void *p);
	    dill_exec_handle h;
            void *p;

	    offset &= ~3;
	    p = ((char*)&source1_d) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_D, "%EC%p");
	    opnd1 = dill_param_reg(c, 1);
	    dest = dill_getreg(c, DILL_D);
	    dill_lddi(c, dest, opnd1, offset);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx, void*)) dill_get_fp(h);
	    expected_result = source1_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec, p);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_lddi test, expected %g, got %g, for loading %g at offset %zx\n",
		       expected_result, result, source1_d, offset);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");
    return failed;
}

static int
test_store(dill_stream c)
{
    int failed = 0;
    size_t i, j;
    (void)i; (void)j;  /* suppress unused warnings */

    /* test for dill_stc */
    if (verbose) printf("test for dill_stc");
    for (i=0 ; i < sizeof(src1c_vals)/sizeof(src1c_vals[0]) ; i++) {
        signed char source1_c = src1c_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, source;
	    signed char result = 0;
	    signed char expected_result;
	    void (*proc)(dill_exec_ctx ec, void *p, signed char a);
            void *p = ((char*)&result) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%i");
	    opnd1 = dill_param_reg(c, 1);
	    source = dill_param_reg(c, 2);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_stc(c, source, opnd1, opnd2);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, signed char)) dill_get_fp(h);
	    expected_result = source1_c;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stc test, expected %d, got %d, for storing %d\n",
		       expected_result, result, source1_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stuc */
    if (verbose) printf("test for dill_stuc");
    for (i=0 ; i < sizeof(src1uc_vals)/sizeof(src1uc_vals[0]) ; i++) {
        unsigned char source1_uc = src1uc_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, source;
	    unsigned char result = 0;
	    unsigned char expected_result;
	    void (*proc)(dill_exec_ctx ec, void *p, unsigned char a);
            void *p = ((char*)&result) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%u");
	    opnd1 = dill_param_reg(c, 1);
	    source = dill_param_reg(c, 2);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_stuc(c, source, opnd1, opnd2);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, unsigned char)) dill_get_fp(h);
	    expected_result = source1_uc;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stuc test, expected %u, got %u, for storing %u\n",
		       expected_result, result, source1_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_sts */
    if (verbose) printf("test for dill_sts");
    for (i=0 ; i < sizeof(src1s_vals)/sizeof(src1s_vals[0]) ; i++) {
        short source1_s = src1s_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, source;
	    short result = 0;
	    short expected_result;
	    void (*proc)(dill_exec_ctx ec, void *p, short a);
            void *p = ((char*)&result) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%i");
	    opnd1 = dill_param_reg(c, 1);
	    source = dill_param_reg(c, 2);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_sts(c, source, opnd1, opnd2);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, short)) dill_get_fp(h);
	    expected_result = source1_s;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_sts test, expected %d, got %d, for storing %d\n",
		       expected_result, result, source1_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stus */
    if (verbose) printf("test for dill_stus");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, source;
	    unsigned short result = 0;
	    unsigned short expected_result;
	    void (*proc)(dill_exec_ctx ec, void *p, unsigned short a);
            void *p = ((char*)&result) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%u");
	    opnd1 = dill_param_reg(c, 1);
	    source = dill_param_reg(c, 2);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_stus(c, source, opnd1, opnd2);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, unsigned short)) dill_get_fp(h);
	    expected_result = source1_us;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stus test, expected %u, got %u, for storing %u\n",
		       expected_result, result, source1_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_sti */
    if (verbose) printf("test for dill_sti");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, source;
	    int result = 0;
	    int expected_result;
	    void (*proc)(dill_exec_ctx ec, void *p, int a);
            void *p = ((char*)&result) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%i");
	    opnd1 = dill_param_reg(c, 1);
	    source = dill_param_reg(c, 2);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_sti(c, source, opnd1, opnd2);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, int)) dill_get_fp(h);
	    expected_result = source1_i;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_sti test, expected %d, got %d, for storing %d\n",
		       expected_result, result, source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stu */
    if (verbose) printf("test for dill_stu");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, source;
	    unsigned int result = 0;
	    unsigned int expected_result;
	    void (*proc)(dill_exec_ctx ec, void *p, unsigned int a);
            void *p = ((char*)&result) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%u");
	    opnd1 = dill_param_reg(c, 1);
	    source = dill_param_reg(c, 2);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_stu(c, source, opnd1, opnd2);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stu test, expected %u, got %u, for storing %u\n",
		       expected_result, result, source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stul */
    if (verbose) printf("test for dill_stul");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, source;
	    UIMM_TYPE result = 0;
	    UIMM_TYPE expected_result;
	    void (*proc)(dill_exec_ctx ec, void *p, UIMM_TYPE a);
            void *p = ((char*)&result) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%ul");
	    opnd1 = dill_param_reg(c, 1);
	    source = dill_param_reg(c, 2);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_stul(c, source, opnd1, opnd2);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stul test, expected %zu, got %zu, for storing %zu\n",
		       expected_result, result, source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stl */
    if (verbose) printf("test for dill_stl");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, source;
	    IMM_TYPE result = 0;
	    IMM_TYPE expected_result;
	    void (*proc)(dill_exec_ctx ec, void *p, IMM_TYPE a);
            void *p = ((char*)&result) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%l");
	    opnd1 = dill_param_reg(c, 1);
	    source = dill_param_reg(c, 2);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_stl(c, source, opnd1, opnd2);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stl test, expected %zx, got %zx, for storing %zx\n",
		       expected_result, result, source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stf */
    if (verbose) printf("test for dill_stf");
    for (i=0 ; i < sizeof(src1f_vals)/sizeof(src1f_vals[0]) ; i++) {
        float source1_f = src1f_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, source;
	    float result = 0;
	    float expected_result;
	    void (*proc)(dill_exec_ctx ec, void *p, float a);
            void *p = ((char*)&result) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%f");
	    opnd1 = dill_param_reg(c, 1);
	    source = dill_param_reg(c, 2);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_stf(c, source, opnd1, opnd2);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, float)) dill_get_fp(h);
	    expected_result = source1_f;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stf test, expected %g, got %g, for storing %g\n",
		       expected_result, result, source1_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_std */
    if (verbose) printf("test for dill_std");
    for (i=0 ; i < sizeof(src1d_vals)/sizeof(src1d_vals[0]) ; i++) {
        double source1_d = src1d_vals[i];
        for (j=0 ; j < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; j++) {
            IMM_TYPE offset = src1l_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2, source;
	    double result = 0;
	    double expected_result;
	    void (*proc)(dill_exec_ctx ec, void *p, double a);
            void *p = ((char*)&result) - offset;
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%d");
	    opnd1 = dill_param_reg(c, 1);
	    source = dill_param_reg(c, 2);
	    opnd2 = dill_getreg(c, DILL_P);
	    dill_setl(c, opnd2, offset);
	    dill_std(c, source, opnd1, opnd2);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, double)) dill_get_fp(h);
	    expected_result = source1_d;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_std test, expected %g, got %g, for storing %g\n",
		       expected_result, result, source1_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stci */
    if (verbose) printf("test for dill_stci");
    for (i=0 ; i < sizeof(src1c_vals)/sizeof(src1c_vals[0]) ; i++) {
        signed char source1_c = src1c_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    signed char result = 0;
	    signed char expected_result;
	    void (*proc)(dill_exec_ctx ec, void*p, signed char a);
            void *p;
	    dill_exec_handle h;

	    offset &= ~3;
	    p = ((char*)&result) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dill_stci(c, opnd2, opnd1, offset);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, signed char)) dill_get_fp(h);
	    expected_result = source1_c;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_c);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stci test, expected %d, got %d, for storing %d\n",
		       expected_result, result, source1_c);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stuci */
    if (verbose) printf("test for dill_stuci");
    for (i=0 ; i < sizeof(src1uc_vals)/sizeof(src1uc_vals[0]) ; i++) {
        unsigned char source1_uc = src1uc_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    unsigned char result = 0;
	    unsigned char expected_result;
	    void (*proc)(dill_exec_ctx ec, void*p, unsigned char a);
            void *p;
	    dill_exec_handle h;

	    offset &= ~3;
	    p = ((char*)&result) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dill_stuci(c, opnd2, opnd1, offset);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, unsigned char)) dill_get_fp(h);
	    expected_result = source1_uc;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_uc);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stuci test, expected %u, got %u, for storing %u\n",
		       expected_result, result, source1_uc);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stsi */
    if (verbose) printf("test for dill_stsi");
    for (i=0 ; i < sizeof(src1s_vals)/sizeof(src1s_vals[0]) ; i++) {
        short source1_s = src1s_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    short result = 0;
	    short expected_result;
	    void (*proc)(dill_exec_ctx ec, void*p, short a);
            void *p;
	    dill_exec_handle h;

	    offset &= ~3;
	    p = ((char*)&result) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dill_stsi(c, opnd2, opnd1, offset);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, short)) dill_get_fp(h);
	    expected_result = source1_s;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_s);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stsi test, expected %d, got %d, for storing %d\n",
		       expected_result, result, source1_s);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stusi */
    if (verbose) printf("test for dill_stusi");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    unsigned short result = 0;
	    unsigned short expected_result;
	    void (*proc)(dill_exec_ctx ec, void*p, unsigned short a);
            void *p;
	    dill_exec_handle h;

	    offset &= ~3;
	    p = ((char*)&result) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dill_stusi(c, opnd2, opnd1, offset);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, unsigned short)) dill_get_fp(h);
	    expected_result = source1_us;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_us);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stusi test, expected %u, got %u, for storing %u\n",
		       expected_result, result, source1_us);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stii */
    if (verbose) printf("test for dill_stii");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    int result = 0;
	    int expected_result;
	    void (*proc)(dill_exec_ctx ec, void*p, int a);
            void *p;
	    dill_exec_handle h;

	    offset &= ~3;
	    p = ((char*)&result) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%i");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dill_stii(c, opnd2, opnd1, offset);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, int)) dill_get_fp(h);
	    expected_result = source1_i;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_i);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stii test, expected %d, got %d, for storing %d\n",
		       expected_result, result, source1_i);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stui */
    if (verbose) printf("test for dill_stui");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    unsigned int result = 0;
	    unsigned int expected_result;
	    void (*proc)(dill_exec_ctx ec, void*p, unsigned int a);
            void *p;
	    dill_exec_handle h;

	    offset &= ~3;
	    p = ((char*)&result) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%u");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dill_stui(c, opnd2, opnd1, offset);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, unsigned int)) dill_get_fp(h);
	    expected_result = source1_u;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_u);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stui test, expected %u, got %u, for storing %u\n",
		       expected_result, result, source1_u);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stuli */
    if (verbose) printf("test for dill_stuli");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    UIMM_TYPE result = 0;
	    UIMM_TYPE expected_result;
	    void (*proc)(dill_exec_ctx ec, void*p, UIMM_TYPE a);
            void *p;
	    dill_exec_handle h;

	    offset &= ~3;
	    p = ((char*)&result) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%ul");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dill_stuli(c, opnd2, opnd1, offset);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, UIMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_ul;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_ul);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stuli test, expected %zu, got %zu, for storing %zu\n",
		       expected_result, result, source1_ul);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stli */
    if (verbose) printf("test for dill_stli");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    IMM_TYPE result = 0;
	    IMM_TYPE expected_result;
	    void (*proc)(dill_exec_ctx ec, void*p, IMM_TYPE a);
            void *p;
	    dill_exec_handle h;

	    offset &= ~3;
	    p = ((char*)&result) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%l");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dill_stli(c, opnd2, opnd1, offset);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, IMM_TYPE)) dill_get_fp(h);
	    expected_result = source1_l;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_l);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stli test, expected %zx, got %zx, for storing %zx\n",
		       expected_result, result, source1_l);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stfi */
    if (verbose) printf("test for dill_stfi");
    for (i=0 ; i < sizeof(src1f_vals)/sizeof(src1f_vals[0]) ; i++) {
        float source1_f = src1f_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    float result = 0;
	    float expected_result;
	    void (*proc)(dill_exec_ctx ec, void*p, float a);
            void *p;
	    dill_exec_handle h;

	    offset &= ~3;
	    p = ((char*)&result) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%f");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dill_stfi(c, opnd2, opnd1, offset);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, float)) dill_get_fp(h);
	    expected_result = source1_f;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_f);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stfi test, expected %g, got %g, for storing %g\n",
		       expected_result, result, source1_f);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for dill_stdi */
    if (verbose) printf("test for dill_stdi");
    for (i=0 ; i < sizeof(src1d_vals)/sizeof(src1d_vals[0]) ; i++) {
        double source1_d = src1d_vals[i];
        for (j=0 ; j < sizeof(bit_pattern_vals)/sizeof(bit_pattern_vals[0]) ; j++) {
            IMM_TYPE offset = bit_pattern_vals[j];

	    dill_exec_ctx ec;
	    dill_reg opnd1, opnd2;
	    double result = 0;
	    double expected_result;
	    void (*proc)(dill_exec_ctx ec, void*p, double a);
            void *p;
	    dill_exec_handle h;

	    offset &= ~3;
	    p = ((char*)&result) - offset;
	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_V, "%EC%p%d");
	    opnd1 = dill_param_reg(c, 1);
	    opnd2 = dill_param_reg(c, 2);
	    dill_stdi(c, opnd2, opnd1, offset);
	    dill_retii(c, 0);
	    
	    h = dill_finalize(c);
	    proc = (void (*)(dill_exec_ctx, void *, double)) dill_get_fp(h);
	    expected_result = source1_d;

	    ec = dill_get_exec_context(c);
	    proc(ec, p, source1_d);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed dill_stdi test, expected %g, got %g, for storing %g\n",
		       expected_result, result, source1_d);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");
    return failed;
}

static int
test_setmovret(dill_stream c)
{
    int failed = 0;
    size_t i, j;
    (void)i; (void)j;  /* suppress unused warnings */

    /* test for set/mov/ret c */
    if (verbose) printf("test for set/mov/ret c");
    for (i=0 ; i < sizeof(src1c_vals)/sizeof(src1c_vals[0]) ; i++) {
        signed char source1_c = src1c_vals[i];
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    signed char result;
	    signed char expected_result;
	    signed char (*proc)(dill_exec_ctx ec);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_C, "%EC");
	    opnd1 = dill_getreg(c, DILL_C);
	    dest = dill_getreg(c, DILL_C);
	    dill_setc(c, opnd1, source1_c);
	    dill_movc(c, dest, opnd1);
	    dill_retc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (signed char(*)(dill_exec_ctx)) dill_get_fp(h);
	    expected_result = source1_c;

	    ec = dill_get_exec_context(c);
	    result = proc(ec);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed set/mov/ret c test, expected %d, got %d\n",
		       expected_result, result);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for set/mov/ret uc */
    if (verbose) printf("test for set/mov/ret uc");
    for (i=0 ; i < sizeof(src1uc_vals)/sizeof(src1uc_vals[0]) ; i++) {
        unsigned char source1_uc = src1uc_vals[i];
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned char result;
	    unsigned char expected_result;
	    unsigned char (*proc)(dill_exec_ctx ec);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_UC, "%EC");
	    opnd1 = dill_getreg(c, DILL_UC);
	    dest = dill_getreg(c, DILL_UC);
	    dill_setuc(c, opnd1, source1_uc);
	    dill_movuc(c, dest, opnd1);
	    dill_retuc(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned char(*)(dill_exec_ctx)) dill_get_fp(h);
	    expected_result = source1_uc;

	    ec = dill_get_exec_context(c);
	    result = proc(ec);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed set/mov/ret uc test, expected %u, got %u\n",
		       expected_result, result);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for set/mov/ret s */
    if (verbose) printf("test for set/mov/ret s");
    for (i=0 ; i < sizeof(src1s_vals)/sizeof(src1s_vals[0]) ; i++) {
        short source1_s = src1s_vals[i];
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    short result;
	    short expected_result;
	    short (*proc)(dill_exec_ctx ec);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_S, "%EC");
	    opnd1 = dill_getreg(c, DILL_S);
	    dest = dill_getreg(c, DILL_S);
	    dill_sets(c, opnd1, source1_s);
	    dill_movs(c, dest, opnd1);
	    dill_rets(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (short(*)(dill_exec_ctx)) dill_get_fp(h);
	    expected_result = source1_s;

	    ec = dill_get_exec_context(c);
	    result = proc(ec);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed set/mov/ret s test, expected %d, got %d\n",
		       expected_result, result);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for set/mov/ret us */
    if (verbose) printf("test for set/mov/ret us");
    for (i=0 ; i < sizeof(src1us_vals)/sizeof(src1us_vals[0]) ; i++) {
        unsigned short source1_us = src1us_vals[i];
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned short result;
	    unsigned short expected_result;
	    unsigned short (*proc)(dill_exec_ctx ec);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_US, "%EC");
	    opnd1 = dill_getreg(c, DILL_US);
	    dest = dill_getreg(c, DILL_US);
	    dill_setus(c, opnd1, source1_us);
	    dill_movus(c, dest, opnd1);
	    dill_retus(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned short(*)(dill_exec_ctx)) dill_get_fp(h);
	    expected_result = source1_us;

	    ec = dill_get_exec_context(c);
	    result = proc(ec);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed set/mov/ret us test, expected %u, got %u\n",
		       expected_result, result);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for set/mov/ret i */
    if (verbose) printf("test for set/mov/ret i");
    for (i=0 ; i < sizeof(src1i_vals)/sizeof(src1i_vals[0]) ; i++) {
        int source1_i = src1i_vals[i];
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    int result;
	    int expected_result;
	    int (*proc)(dill_exec_ctx ec);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_I, "%EC");
	    opnd1 = dill_getreg(c, DILL_I);
	    dest = dill_getreg(c, DILL_I);
	    dill_seti(c, opnd1, source1_i);
	    dill_movi(c, dest, opnd1);
	    dill_reti(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (int(*)(dill_exec_ctx)) dill_get_fp(h);
	    expected_result = source1_i;

	    ec = dill_get_exec_context(c);
	    result = proc(ec);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed set/mov/ret i test, expected %d, got %d\n",
		       expected_result, result);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for set/mov/ret u */
    if (verbose) printf("test for set/mov/ret u");
    for (i=0 ; i < sizeof(src1u_vals)/sizeof(src1u_vals[0]) ; i++) {
        unsigned int source1_u = src1u_vals[i];
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    unsigned int result;
	    unsigned int expected_result;
	    unsigned int (*proc)(dill_exec_ctx ec);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_U, "%EC");
	    opnd1 = dill_getreg(c, DILL_U);
	    dest = dill_getreg(c, DILL_U);
	    dill_setu(c, opnd1, source1_u);
	    dill_movu(c, dest, opnd1);
	    dill_retu(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (unsigned int(*)(dill_exec_ctx)) dill_get_fp(h);
	    expected_result = source1_u;

	    ec = dill_get_exec_context(c);
	    result = proc(ec);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed set/mov/ret u test, expected %u, got %u\n",
		       expected_result, result);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for set/mov/ret ul */
    if (verbose) printf("test for set/mov/ret ul");
    for (i=0 ; i < sizeof(src1ul_vals)/sizeof(src1ul_vals[0]) ; i++) {
        UIMM_TYPE source1_ul = src1ul_vals[i];
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    UIMM_TYPE result;
	    UIMM_TYPE expected_result;
	    UIMM_TYPE (*proc)(dill_exec_ctx ec);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_UL, "%EC");
	    opnd1 = dill_getreg(c, DILL_UL);
	    dest = dill_getreg(c, DILL_UL);
	    dill_setul(c, opnd1, source1_ul);
	    dill_movul(c, dest, opnd1);
	    dill_retul(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (UIMM_TYPE(*)(dill_exec_ctx)) dill_get_fp(h);
	    expected_result = source1_ul;

	    ec = dill_get_exec_context(c);
	    result = proc(ec);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed set/mov/ret ul test, expected %zu, got %zu\n",
		       expected_result, result);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for set/mov/ret l */
    if (verbose) printf("test for set/mov/ret l");
    for (i=0 ; i < sizeof(src1l_vals)/sizeof(src1l_vals[0]) ; i++) {
        IMM_TYPE source1_l = src1l_vals[i];
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    IMM_TYPE result;
	    IMM_TYPE expected_result;
	    IMM_TYPE (*proc)(dill_exec_ctx ec);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_L, "%EC");
	    opnd1 = dill_getreg(c, DILL_L);
	    dest = dill_getreg(c, DILL_L);
	    dill_setl(c, opnd1, source1_l);
	    dill_movl(c, dest, opnd1);
	    dill_retl(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (IMM_TYPE(*)(dill_exec_ctx)) dill_get_fp(h);
	    expected_result = source1_l;

	    ec = dill_get_exec_context(c);
	    result = proc(ec);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed set/mov/ret l test, expected %zx, got %zx\n",
		       expected_result, result);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for set/mov/ret f */
    if (verbose) printf("test for set/mov/ret f");
    for (i=0 ; i < sizeof(src1f_vals)/sizeof(src1f_vals[0]) ; i++) {
        float source1_f = src1f_vals[i];
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    float result;
	    float expected_result;
	    float (*proc)(dill_exec_ctx ec);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_F, "%EC");
	    opnd1 = dill_getreg(c, DILL_F);
	    dest = dill_getreg(c, DILL_F);
	    dill_setf(c, opnd1, source1_f);
	    dill_movf(c, dest, opnd1);
	    dill_retf(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (float(*)(dill_exec_ctx)) dill_get_fp(h);
	    expected_result = source1_f;

	    ec = dill_get_exec_context(c);
	    result = proc(ec);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed set/mov/ret f test, expected %g, got %g\n",
		       expected_result, result);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");

    /* test for set/mov/ret d */
    if (verbose) printf("test for set/mov/ret d");
    for (i=0 ; i < sizeof(src1d_vals)/sizeof(src1d_vals[0]) ; i++) {
        double source1_d = src1d_vals[i];
        {

	    dill_exec_ctx ec;
	    dill_reg opnd1, dest;
	    double result;
	    double expected_result;
	    double (*proc)(dill_exec_ctx ec);
	    dill_exec_handle h;

	    if (verbose) {printf(".");fflush(stdout);}
	    dill_start_proc(c, "no name", DILL_D, "%EC");
	    opnd1 = dill_getreg(c, DILL_D);
	    dest = dill_getreg(c, DILL_D);
	    dill_setd(c, opnd1, source1_d);
	    dill_movd(c, dest, opnd1);
	    dill_retd(c, dest);
	    
	    h = dill_finalize(c);
	    proc = (double(*)(dill_exec_ctx)) dill_get_fp(h);
	    expected_result = source1_d;

	    ec = dill_get_exec_context(c);
	    result = proc(ec);
	    dill_free_handle(h);
	    dill_free_exec_context(ec);
	    if (expected_result != result) {
		printf("Failed set/mov/ret d test, expected %g, got %g\n",
		       expected_result, result);
		dill_dump(c);
		failed++;
	    }
	}
    }
    if (verbose) printf(" done\n");
    return failed;
}

int
main(int argc, char **argv)
{
    dill_stream c = dill_create_stream();
    int failed = 0;

    if (argc > 1) verbose++;

    /* Initialize random values */
    rand1_f = (float)drand48();
    rand2_f = (float)drand48();
    rand1_s = (short)lrand48();
    rand2_s = (short)lrand48();
    rand1_l = (IMM_TYPE)lrand48();
    rand2_l = (IMM_TYPE)lrand48();
    rand1_uc = (unsigned char)lrand48();
    rand2_uc = (unsigned char)lrand48();
    rand1_us = (unsigned short)lrand48();
    rand2_us = (unsigned short)lrand48();
    rand1_ul = (UIMM_TYPE)lrand48();
    rand2_ul = (UIMM_TYPE)lrand48();
    rand1_c = (signed char)lrand48();
    rand2_c = (signed char)lrand48();
    rand1_d = (double)drand48();
    rand2_d = (double)drand48();
    rand1_u = (unsigned int)lrand48();
    rand2_u = (unsigned int)lrand48();
    rand1_i = (int)lrand48();
    rand2_i = (int)lrand48();
    rand1_p = (char*)(char *)lrand48();
    rand2_p = (char*)(char *)lrand48();
    src1f_vals[0] = rand1_f;
    src1f_vals[1] = -rand1_f;
    src2f_vals[0] = rand2_f;
    src2f_vals[1] = -rand2_f;
    src1d_vals[0] = rand1_d;
    src1d_vals[1] = -rand1_d;
    src2d_vals[0] = rand2_d;
    src2d_vals[1] = -rand2_d;
    src1c_vals[0] = rand1_c;
    src1c_vals[1] = -rand1_c;
    src2c_vals[0] = rand2_c;
    src2c_vals[1] = -rand2_c;
    src1s_vals[0] = rand1_s;
    src1s_vals[1] = -rand1_s;
    src2s_vals[0] = rand2_s;
    src2s_vals[1] = -rand2_s;
    src1i_vals[0] = rand1_i;
    src1i_vals[1] = -rand1_i;
    src2i_vals[0] = rand2_i;
    src2i_vals[1] = -rand2_i;
    src1l_vals[0] = rand1_l;
    src1l_vals[1] = -rand1_l;
    src2l_vals[0] = rand2_l;
    src2l_vals[1] = -rand2_l;
    src1uc_vals[0] = (unsigned char) rand1_uc;
    src1uc_vals[1] = (unsigned char) - (char) rand1_uc;
    src2uc_vals[0] = (unsigned char) rand2_uc;
    src2uc_vals[1] = (unsigned char) - (char) rand2_uc;
    src1us_vals[0] = (unsigned short) rand1_us;
    src1us_vals[1] = (unsigned short) - (short) rand1_us;
    src2us_vals[0] = (unsigned short) rand2_us;
    src2us_vals[1] = (unsigned short) - (short) rand2_us;
    src1u_vals[0] = (unsigned int) rand1_u;
    src1u_vals[1] = (unsigned int) - (int) rand1_u;
    src2u_vals[0] = (unsigned int) rand2_u;
    src2u_vals[1] = (unsigned int) - (int) rand2_u;
    src1ul_vals[0] = (UIMM_TYPE) rand1_ul;
    src1ul_vals[1] = (UIMM_TYPE) - (intptr_t) rand1_ul;
    src2ul_vals[0] = (UIMM_TYPE) rand2_ul;
    src2ul_vals[1] = (UIMM_TYPE) - (intptr_t) rand2_ul;
    br_srcuc_vals[0] = (unsigned char) rand1_uc;
    br_srcuc_vals[1] = (unsigned char) - (char) rand1_uc;
    br_srcuc_vals[2] = (unsigned char) rand1_uc + 1;
    br_srcuc_vals[3] = (unsigned char) -((char) rand1_uc) + 1;
    br_srcuc_vals[4] = (unsigned char) rand1_uc - 1;
    br_srcuc_vals[5] = (unsigned char) -((char) rand1_uc) - 1;
    br_srcc_vals[0] = (signed char) rand1_c;
    br_srcc_vals[1] = (signed char) -  rand1_c;
    br_srcc_vals[2] = (signed char) rand1_c + 1;
    br_srcc_vals[3] = (signed char) -( rand1_c) + 1;
    br_srcc_vals[4] = (signed char) rand1_c - 1;
    br_srcc_vals[5] = (signed char) -( rand1_c) - 1;
    br_srcus_vals[0] = (unsigned short) rand1_us;
    br_srcus_vals[1] = (unsigned short) - (short) rand1_us;
    br_srcus_vals[2] = (unsigned short) rand1_us + 1;
    br_srcus_vals[3] = (unsigned short) -((short) rand1_us) + 1;
    br_srcus_vals[4] = (unsigned short) rand1_us - 1;
    br_srcus_vals[5] = (unsigned short) -((short) rand1_us) - 1;
    br_srcs_vals[0] = (short) rand1_s;
    br_srcs_vals[1] = (short) -  rand1_s;
    br_srcs_vals[2] = (short) rand1_s + 1;
    br_srcs_vals[3] = (short) -( rand1_s) + 1;
    br_srcs_vals[4] = (short) rand1_s - 1;
    br_srcs_vals[5] = (short) -( rand1_s) - 1;
    br_srci_vals[0] = (int) rand1_i;
    br_srci_vals[1] = (int) -  rand1_i;
    br_srci_vals[2] = (int) rand1_i + 1;
    br_srci_vals[3] = (int) -( rand1_i) + 1;
    br_srci_vals[4] = (int) rand1_i - 1;
    br_srci_vals[5] = (int) -( rand1_i) - 1;
    br_srcu_vals[0] = (unsigned int) rand1_u;
    br_srcu_vals[1] = (unsigned int) - (int) rand1_u;
    br_srcu_vals[2] = (unsigned int) rand1_u + 1;
    br_srcu_vals[3] = (unsigned int) -((int) rand1_u) + 1;
    br_srcu_vals[4] = (unsigned int) rand1_u - 1;
    br_srcu_vals[5] = (unsigned int) -((int) rand1_u) - 1;
    br_srcl_vals[0] = (IMM_TYPE) rand1_l;
    br_srcl_vals[1] = (IMM_TYPE) -  rand1_l;
    br_srcl_vals[2] = (IMM_TYPE) rand1_l + 1;
    br_srcl_vals[3] = (IMM_TYPE) -( rand1_l) + 1;
    br_srcl_vals[4] = (IMM_TYPE) rand1_l - 1;
    br_srcl_vals[5] = (IMM_TYPE) -( rand1_l) - 1;
    br_srcul_vals[0] = (UIMM_TYPE) rand1_ul;
    br_srcul_vals[1] = (UIMM_TYPE) - (intptr_t) rand1_ul;
    br_srcul_vals[2] = (UIMM_TYPE) rand1_ul + 1;
    br_srcul_vals[3] = (UIMM_TYPE) -((intptr_t) rand1_ul) + 1;
    br_srcul_vals[4] = (UIMM_TYPE) rand1_ul - 1;
    br_srcul_vals[5] = (UIMM_TYPE) -((intptr_t) rand1_ul) - 1;
    br_srcd_vals[0] = (double) rand1_d;
    br_srcd_vals[1] = (double) -  rand1_d;
    br_srcd_vals[2] = (double) rand1_d + 1;
    br_srcd_vals[3] = (double) -( rand1_d) + 1;
    br_srcd_vals[4] = (double) rand1_d - 1;
    br_srcd_vals[5] = (double) -( rand1_d) - 1;
    br_srcf_vals[0] = (float) rand1_f;
    br_srcf_vals[1] = (float) -  rand1_f;
    br_srcf_vals[2] = (float) rand1_f + 1;
    br_srcf_vals[3] = (float) -( rand1_f) + 1;
    br_srcf_vals[4] = (float) rand1_f - 1;
    br_srcf_vals[5] = (float) -( rand1_f) - 1;
    br_srcp_vals[0] = (char*) rand1_p;
    br_srcp_vals[1] = (char*) rand1_p;
    br_srcp_vals[2] = (char*) rand1_p + 1;
    br_srcp_vals[3] = (char*) rand1_p + 1;
    br_srcp_vals[4] = (char*) rand1_p - 1;
    br_srcp_vals[5] = (char*) rand1_p - 1;
    /* reference these values since they currently aren't done elsewhere */
    src2p_vals[0] = src1p_vals[0] = rand1_p; src2p_vals[1] = src1p_vals[1] = rand2_p;
    (void)src2d_vals[0];
    (void)src2s_vals[0];
    (void)src2p_vals[0];
    (void)src2c_vals[0];
    (void)src2uc_vals[0];
    (void)src2us_vals[0];
    (void)src2f_vals[0];

    /* Call test functions */
    failed += test_arith_int(c);
    failed += test_arith_ptr(c);
    failed += test_arith_float(c);
    failed += test_arith2_int(c);
    failed += test_arith2_float(c);
    failed += test_arithi(c);
    failed += test_branch(c);
    failed += test_branchi(c);
    failed += test_compare(c);
    failed += test_convert(c);
    failed += test_load(c);
    failed += test_store(c);
    failed += test_setmovret(c);

    dill_free_stream(c);
    return failed;
}
