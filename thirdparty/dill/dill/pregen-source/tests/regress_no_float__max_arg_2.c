int dill_errors;
#include <math.h>
#include "string.h"
#include "dill.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


#include <../config.h>
#undef BITSPERBYTE
#define BITSPERBYTE 8

float c_fabs(float x) { return (x) < 0.0 ? -x : x; }
double c_abs(double x) { return (x) < 0.0 ? -x : x; }
float c_fceil(float x) { return (float)(int)(x + .5); }
double c_ceil(double x) { return (double)(int)(x + .5);}
float c_ffloor(float x) { return (float)(int)(x); }
double c_floor(double x) { return (double)(int)(x);}
float c_fsqrt(float x) { extern double sqrt(double); return (float)sqrt((double)x); }
double c_sqrt(double x) { extern double sqrt(double); 	return sqrt(x);}

#define dill_fatal(str) do {fprintf(stderr, "%s\n", str); exit(0);} while (0)

static int
get_reg_pair(dill_stream s, int type1, int *reg1p, int type2, int *reg2p,
	     int regpairid);
int main(int argc, char *argv[])
{
    dill_stream s = dill_create_raw_stream();
    char		dc, s1c;
    unsigned char	duc, s1uc;
    short		ds, s1s;
    unsigned short	dus, s2us, s1us;
    int 	     	di, s1i, s2i;
    unsigned     	du, s1u, s2u;
    UIMM_TYPE   	dul, s1ul, s2ul;
    IMM_TYPE     		s1l, s2l;
    char		s2c;
    short		s2s;
    unsigned char	s2uc;
    float		df, s1f, s2f;
    double		dd, s1d, s2d;
    void		*dp, *s1p, *s2p;
    int		l, verbose = 0, i;
    int 	iters = 10, loop_count = 0;
    int 	aligned_offset, unaligned_offset;
    int 	shifti, shiftu, shiftl, shiftul;

    for (i=1; i < argc; i++) {
	if (strcmp(argv[i], "-v") == 0) {
	    verbose++;
	} else {
	    iters = atoi(argv[i]);
	}
    }
	
loop:
    s1p = (void *)(IMM_TYPE)rand();
    s2p = (void *)(IMM_TYPE)rand();

    s1c = rand() - rand();
    if(!(s1uc = rand() - rand()))
	s1uc = rand() + 1;

    s1s = rand() - rand();
    if(!(s1us = rand() - rand()))
	s1us = rand() + 1;

    s1i = rand() - rand(); 
    s2i = rand() - rand();
    if(!(s2i = rand() - rand()))
	s2i = rand() + 1;

    s1u = rand() - rand();
    if(!(s2u = rand() - rand()))
	s2u = rand() + 1;

    s1ul = rand() - rand();
    if(!(s2ul = rand() - rand()))
	s2ul = rand() + 1;

    s1l = rand() - rand();
    if(!(s2l = rand() - rand()))
	s2l = rand() + 1;

    s2us = rand() - rand();
    if(!(s2us = rand() - rand()))
	s2us = rand() + 1;

    s2c = rand() - rand();
    if(!(s2c = rand() - rand()))
	s2c = rand() + 1;
    s2uc = rand() - rand();
    if(!(s2uc = rand() - rand()))
	s2uc = rand() + 1;
    s2s = rand() - rand();
    if(!(s2s = rand() - rand()))
	s2s = rand() + 1;

    s1f = (float)rand() / (float)rand();
    s2f = (float)rand() / (float)((rand()+1) * ((rand()%1) ? 1. : -1.));

    s1d = (double)rand() / rand();
    s2d = (double)rand() / (rand()+1) * ((rand()%1) ? 1. : -1.);

    shifti = rand() % ((sizeof(int) * BITSPERBYTE) - 2) + 1;
    shiftu = rand() % ((sizeof(unsigned) * BITSPERBYTE)-2) + 1;
    shiftl = rand() % ((sizeof(IMM_TYPE) * BITSPERBYTE)-2) + 1;
    shiftul = rand() % ((sizeof(UIMM_TYPE) * BITSPERBYTE)-2) + 1;

    aligned_offset = (rand() - rand()) & ~7;
    unaligned_offset = (rand() - rand());

    switch (loop_count) {
    case 0:
	aligned_offset = unaligned_offset = 0;
	break;
    case 1:
	aligned_offset &= 0xf;
	unaligned_offset &= 0xf;
	break;
    case 2:
	aligned_offset &= 0xff;
	unaligned_offset &= 0xff;
	break;
    default:
	break;
    }
    {
	int expected_result;
	int result;
	int(*func)(int,int);
	
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg + reg) */
	if (verbose) printf(" - addi\n");
        dill_start_proc(s, "addi",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_addi(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, s2i);
	dill_free_handle(h);
        expected_result = (s1i + s2i);
	if (expected_result != result) {
	    printf("Failed test for %d addi %d, expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg + imm) */
	if (verbose) printf(" - addii\n");
        dill_start_proc(s, "addii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_addii(s, rdi, dill_param_reg(s, 0), s2i);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i + s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d addii (imm %d), expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg + reg) */
	if (verbose) printf(" - addu\n");
        dill_start_proc(s, "addu",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_addu(s, rdu, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, s2u);
	dill_free_handle(h);
        expected_result = (s1u + s2u);
	if (expected_result != result) {
	    printf("Failed test for %u addu %u, expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg + imm) */
	if (verbose) printf(" - addui\n");
        dill_start_proc(s, "addui", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_addui(s, rdu, dill_param_reg(s, 0), s2u);
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u + s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u addui (imm %u), expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg + reg) */
	if (verbose) printf(" - addul\n");
        dill_start_proc(s, "addul",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_addul(s, rdul, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
        expected_result = (s1ul + s2ul);
	if (expected_result != result) {
	    printf("Failed test for %zx addul %zx, expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg + imm) */
	if (verbose) printf(" - adduli\n");
        dill_start_proc(s, "adduli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_adduli(s, rdul, dill_param_reg(s, 0), s2ul);
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul + s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx adduli (imm %zx), expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg + reg) */
	if (verbose) printf(" - addl\n");
        dill_start_proc(s, "addl",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_addl(s, rdl, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, s2l);
	dill_free_handle(h);
        expected_result = (s1l + s2l);
	if (expected_result != result) {
	    printf("Failed test for %zx addl %zx, expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg + imm) */
	if (verbose) printf(" - addli\n");
        dill_start_proc(s, "addli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_addli(s, rdl, dill_param_reg(s, 0), s2l);
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l + s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx addli (imm %zx), expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg - reg) */
	if (verbose) printf(" - subi\n");
        dill_start_proc(s, "subi",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_subi(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, s2i);
	dill_free_handle(h);
        expected_result = (s1i - s2i);
	if (expected_result != result) {
	    printf("Failed test for %d subi %d, expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg - imm) */
	if (verbose) printf(" - subii\n");
        dill_start_proc(s, "subii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_subii(s, rdi, dill_param_reg(s, 0), s2i);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i - s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d subii (imm %d), expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg - reg) */
	if (verbose) printf(" - subu\n");
        dill_start_proc(s, "subu",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_subu(s, rdu, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, s2u);
	dill_free_handle(h);
        expected_result = (s1u - s2u);
	if (expected_result != result) {
	    printf("Failed test for %u subu %u, expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg - imm) */
	if (verbose) printf(" - subui\n");
        dill_start_proc(s, "subui", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_subui(s, rdu, dill_param_reg(s, 0), s2u);
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u - s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u subui (imm %u), expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg - reg) */
	if (verbose) printf(" - subul\n");
        dill_start_proc(s, "subul",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_subul(s, rdul, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
        expected_result = (s1ul - s2ul);
	if (expected_result != result) {
	    printf("Failed test for %zx subul %zx, expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg - imm) */
	if (verbose) printf(" - subuli\n");
        dill_start_proc(s, "subuli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_subuli(s, rdul, dill_param_reg(s, 0), s2ul);
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul - s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx subuli (imm %zx), expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg - reg) */
	if (verbose) printf(" - subl\n");
        dill_start_proc(s, "subl",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_subl(s, rdl, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, s2l);
	dill_free_handle(h);
        expected_result = (s1l - s2l);
	if (expected_result != result) {
	    printf("Failed test for %zx subl %zx, expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg - imm) */
	if (verbose) printf(" - subli\n");
        dill_start_proc(s, "subli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_subli(s, rdl, dill_param_reg(s, 0), s2l);
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l - s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx subli (imm %zx), expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg * reg) */
	if (verbose) printf(" - muli\n");
        dill_start_proc(s, "muli",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_muli(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, s2i);
	dill_free_handle(h);
        expected_result = (s1i * s2i);
	if (expected_result != result) {
	    printf("Failed test for %d muli %d, expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg * imm) */
	if (verbose) printf(" - mulii\n");
        dill_start_proc(s, "mulii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_mulii(s, rdi, dill_param_reg(s, 0), s2i);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i * s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d mulii (imm %d), expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg * reg) */
	if (verbose) printf(" - mulu\n");
        dill_start_proc(s, "mulu",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_mulu(s, rdu, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, s2u);
	dill_free_handle(h);
        expected_result = (s1u * s2u);
	if (expected_result != result) {
	    printf("Failed test for %u mulu %u, expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg * imm) */
	if (verbose) printf(" - mului\n");
        dill_start_proc(s, "mului", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_mului(s, rdu, dill_param_reg(s, 0), s2u);
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u * s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u mului (imm %u), expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg * reg) */
	if (verbose) printf(" - mulul\n");
        dill_start_proc(s, "mulul",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_mulul(s, rdul, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
        expected_result = (s1ul * s2ul);
	if (expected_result != result) {
	    printf("Failed test for %zx mulul %zx, expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg * imm) */
	if (verbose) printf(" - mululi\n");
        dill_start_proc(s, "mululi", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_mululi(s, rdul, dill_param_reg(s, 0), s2ul);
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul * s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx mululi (imm %zx), expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg * reg) */
	if (verbose) printf(" - mull\n");
        dill_start_proc(s, "mull",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_mull(s, rdl, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, s2l);
	dill_free_handle(h);
        expected_result = (s1l * s2l);
	if (expected_result != result) {
	    printf("Failed test for %zx mull %zx, expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg * imm) */
	if (verbose) printf(" - mulli\n");
        dill_start_proc(s, "mulli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_mulli(s, rdl, dill_param_reg(s, 0), s2l);
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l * s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx mulli (imm %zx), expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg / reg) */
	if (verbose) printf(" - divi\n");
        dill_start_proc(s, "divi",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_divi(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, s2i);
	dill_free_handle(h);
        expected_result = (s1i / s2i);
	if (expected_result != result) {
	    printf("Failed test for %d divi %d, expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg / imm) */
	if (verbose) printf(" - divii\n");
        dill_start_proc(s, "divii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_divii(s, rdi, dill_param_reg(s, 0), s2i);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i / s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d divii (imm %d), expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg / reg) */
	if (verbose) printf(" - divu\n");
        dill_start_proc(s, "divu",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_divu(s, rdu, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, s2u);
	dill_free_handle(h);
        expected_result = (s1u / s2u);
	if (expected_result != result) {
	    printf("Failed test for %u divu %u, expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg / imm) */
	if (verbose) printf(" - divui\n");
        dill_start_proc(s, "divui", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_divui(s, rdu, dill_param_reg(s, 0), s2u);
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u / s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u divui (imm %u), expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg / reg) */
	if (verbose) printf(" - divul\n");
        dill_start_proc(s, "divul",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_divul(s, rdul, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
        expected_result = (s1ul / s2ul);
	if (expected_result != result) {
	    printf("Failed test for %zx divul %zx, expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg / imm) */
	if (verbose) printf(" - divuli\n");
        dill_start_proc(s, "divuli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_divuli(s, rdul, dill_param_reg(s, 0), s2ul);
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul / s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx divuli (imm %zx), expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg / reg) */
	if (verbose) printf(" - divl\n");
        dill_start_proc(s, "divl",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_divl(s, rdl, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, s2l);
	dill_free_handle(h);
        expected_result = (s1l / s2l);
	if (expected_result != result) {
	    printf("Failed test for %zx divl %zx, expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg / imm) */
	if (verbose) printf(" - divli\n");
        dill_start_proc(s, "divli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_divli(s, rdl, dill_param_reg(s, 0), s2l);
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l / s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx divli (imm %zx), expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg % reg) */
	if (verbose) printf(" - modi\n");
        dill_start_proc(s, "modi",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_modi(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, s2i);
	dill_free_handle(h);
        expected_result = (s1i % s2i);
	if (expected_result != result) {
	    printf("Failed test for %d modi %d, expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg % imm) */
	if (verbose) printf(" - modii\n");
        dill_start_proc(s, "modii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_modii(s, rdi, dill_param_reg(s, 0), s2i);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i % s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d modii (imm %d), expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg % reg) */
	if (verbose) printf(" - modu\n");
        dill_start_proc(s, "modu",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_modu(s, rdu, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, s2u);
	dill_free_handle(h);
        expected_result = (s1u % s2u);
	if (expected_result != result) {
	    printf("Failed test for %u modu %u, expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg % imm) */
	if (verbose) printf(" - modui\n");
        dill_start_proc(s, "modui", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_modui(s, rdu, dill_param_reg(s, 0), s2u);
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u % s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u modui (imm %u), expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg % reg) */
	if (verbose) printf(" - modul\n");
        dill_start_proc(s, "modul",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_modul(s, rdul, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
        expected_result = (s1ul % s2ul);
	if (expected_result != result) {
	    printf("Failed test for %zx modul %zx, expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg % imm) */
	if (verbose) printf(" - moduli\n");
        dill_start_proc(s, "moduli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_moduli(s, rdul, dill_param_reg(s, 0), s2ul);
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul % s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx moduli (imm %zx), expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg % reg) */
	if (verbose) printf(" - modl\n");
        dill_start_proc(s, "modl",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_modl(s, rdl, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, s2l);
	dill_free_handle(h);
        expected_result = (s1l % s2l);
	if (expected_result != result) {
	    printf("Failed test for %zx modl %zx, expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg % imm) */
	if (verbose) printf(" - modli\n");
        dill_start_proc(s, "modli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_modli(s, rdl, dill_param_reg(s, 0), s2l);
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l % s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx modli (imm %zx), expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg ^ reg) */
	if (verbose) printf(" - xori\n");
        dill_start_proc(s, "xori",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_xori(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, s2i);
	dill_free_handle(h);
        expected_result = (s1i ^ s2i);
	if (expected_result != result) {
	    printf("Failed test for %d xori %d, expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg ^ imm) */
	if (verbose) printf(" - xorii\n");
        dill_start_proc(s, "xorii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_xorii(s, rdi, dill_param_reg(s, 0), s2i);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i ^ s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d xorii (imm %d), expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg ^ reg) */
	if (verbose) printf(" - xoru\n");
        dill_start_proc(s, "xoru",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_xoru(s, rdu, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, s2u);
	dill_free_handle(h);
        expected_result = (s1u ^ s2u);
	if (expected_result != result) {
	    printf("Failed test for %u xoru %u, expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg ^ imm) */
	if (verbose) printf(" - xorui\n");
        dill_start_proc(s, "xorui", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_xorui(s, rdu, dill_param_reg(s, 0), s2u);
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u ^ s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u xorui (imm %u), expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg ^ reg) */
	if (verbose) printf(" - xorul\n");
        dill_start_proc(s, "xorul",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_xorul(s, rdul, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
        expected_result = (s1ul ^ s2ul);
	if (expected_result != result) {
	    printf("Failed test for %zx xorul %zx, expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg ^ imm) */
	if (verbose) printf(" - xoruli\n");
        dill_start_proc(s, "xoruli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_xoruli(s, rdul, dill_param_reg(s, 0), s2ul);
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul ^ s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx xoruli (imm %zx), expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg ^ reg) */
	if (verbose) printf(" - xorl\n");
        dill_start_proc(s, "xorl",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_xorl(s, rdl, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, s2l);
	dill_free_handle(h);
        expected_result = (s1l ^ s2l);
	if (expected_result != result) {
	    printf("Failed test for %zx xorl %zx, expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg ^ imm) */
	if (verbose) printf(" - xorli\n");
        dill_start_proc(s, "xorli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_xorli(s, rdl, dill_param_reg(s, 0), s2l);
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l ^ s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx xorli (imm %zx), expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg & reg) */
	if (verbose) printf(" - andi\n");
        dill_start_proc(s, "andi",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_andi(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, s2i);
	dill_free_handle(h);
        expected_result = (s1i & s2i);
	if (expected_result != result) {
	    printf("Failed test for %d andi %d, expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg & imm) */
	if (verbose) printf(" - andii\n");
        dill_start_proc(s, "andii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_andii(s, rdi, dill_param_reg(s, 0), s2i);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i & s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d andii (imm %d), expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg & reg) */
	if (verbose) printf(" - andu\n");
        dill_start_proc(s, "andu",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_andu(s, rdu, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, s2u);
	dill_free_handle(h);
        expected_result = (s1u & s2u);
	if (expected_result != result) {
	    printf("Failed test for %u andu %u, expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg & imm) */
	if (verbose) printf(" - andui\n");
        dill_start_proc(s, "andui", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_andui(s, rdu, dill_param_reg(s, 0), s2u);
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u & s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u andui (imm %u), expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg & reg) */
	if (verbose) printf(" - andul\n");
        dill_start_proc(s, "andul",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_andul(s, rdul, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
        expected_result = (s1ul & s2ul);
	if (expected_result != result) {
	    printf("Failed test for %zx andul %zx, expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg & imm) */
	if (verbose) printf(" - anduli\n");
        dill_start_proc(s, "anduli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_anduli(s, rdul, dill_param_reg(s, 0), s2ul);
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul & s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx anduli (imm %zx), expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg & reg) */
	if (verbose) printf(" - andl\n");
        dill_start_proc(s, "andl",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_andl(s, rdl, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, s2l);
	dill_free_handle(h);
        expected_result = (s1l & s2l);
	if (expected_result != result) {
	    printf("Failed test for %zx andl %zx, expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg & imm) */
	if (verbose) printf(" - andli\n");
        dill_start_proc(s, "andli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_andli(s, rdl, dill_param_reg(s, 0), s2l);
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l & s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx andli (imm %zx), expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg | reg) */
	if (verbose) printf(" - ori\n");
        dill_start_proc(s, "ori",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ori(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, s2i);
	dill_free_handle(h);
        expected_result = (s1i | s2i);
	if (expected_result != result) {
	    printf("Failed test for %d ori %d, expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg | imm) */
	if (verbose) printf(" - orii\n");
        dill_start_proc(s, "orii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_orii(s, rdi, dill_param_reg(s, 0), s2i);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i | s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d orii (imm %d), expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg | reg) */
	if (verbose) printf(" - oru\n");
        dill_start_proc(s, "oru",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_oru(s, rdu, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, s2u);
	dill_free_handle(h);
        expected_result = (s1u | s2u);
	if (expected_result != result) {
	    printf("Failed test for %u oru %u, expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg | imm) */
	if (verbose) printf(" - orui\n");
        dill_start_proc(s, "orui", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_orui(s, rdu, dill_param_reg(s, 0), s2u);
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u | s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u orui (imm %u), expected %u, got %u\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg | reg) */
	if (verbose) printf(" - orul\n");
        dill_start_proc(s, "orul",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_orul(s, rdul, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
        expected_result = (s1ul | s2ul);
	if (expected_result != result) {
	    printf("Failed test for %zx orul %zx, expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg | imm) */
	if (verbose) printf(" - oruli\n");
        dill_start_proc(s, "oruli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_oruli(s, rdul, dill_param_reg(s, 0), s2ul);
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul | s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx oruli (imm %zx), expected %zx, got %zx\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg | reg) */
	if (verbose) printf(" - orl\n");
        dill_start_proc(s, "orl",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_orl(s, rdl, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, s2l);
	dill_free_handle(h);
        expected_result = (s1l | s2l);
	if (expected_result != result) {
	    printf("Failed test for %zx orl %zx, expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg | imm) */
	if (verbose) printf(" - orli\n");
        dill_start_proc(s, "orli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_orli(s, rdl, dill_param_reg(s, 0), s2l);
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l | s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx orli (imm %zx), expected %zx, got %zx\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg << reg) */
	if (verbose) printf(" - lshi\n");
        dill_start_proc(s, "lshi",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lshi(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, shifti);
	dill_free_handle(h);
        expected_result = (s1i << shifti);
	if (expected_result != result) {
	    printf("Failed test for %d lshi %d, expected %x, got %x\n", s1i, shifti, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg << imm) */
	if (verbose) printf(" - lshii\n");
        dill_start_proc(s, "lshii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lshii(s, rdi, dill_param_reg(s, 0), shifti);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i << shifti);
	result = func(s1i, shifti);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d lshii (imm %d), expected %d, got %d\n", s1i, shifti, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg << reg) */
	if (verbose) printf(" - lshu\n");
        dill_start_proc(s, "lshu",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lshu(s, rdu, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, shiftu);
	dill_free_handle(h);
        expected_result = (s1u << shiftu);
	if (expected_result != result) {
	    printf("Failed test for %u lshu %d, expected %x, got %x\n", s1u, shiftu, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg << imm) */
	if (verbose) printf(" - lshui\n");
        dill_start_proc(s, "lshui", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lshui(s, rdu, dill_param_reg(s, 0), shiftu);
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u << shiftu);
	result = func(s1u, shiftu);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u lshui (imm %d), expected %u, got %u\n", s1u, shiftu, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg << reg) */
	if (verbose) printf(" - lshul\n");
        dill_start_proc(s, "lshul",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lshul(s, rdul, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, shiftul);
	dill_free_handle(h);
        expected_result = (s1ul << shiftul);
	if (expected_result != result) {
	    printf("Failed test for %zx lshul %d, expected %zu, got %zu\n", s1ul, shiftul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg << imm) */
	if (verbose) printf(" - lshuli\n");
        dill_start_proc(s, "lshuli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lshuli(s, rdul, dill_param_reg(s, 0), shiftul);
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul << shiftul);
	result = func(s1ul, shiftul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx lshuli (imm %d), expected %zx, got %zx\n", s1ul, shiftul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg << reg) */
	if (verbose) printf(" - lshl\n");
        dill_start_proc(s, "lshl",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lshl(s, rdl, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, shiftl);
	dill_free_handle(h);
        expected_result = (s1l << shiftl);
	if (expected_result != result) {
	    printf("Failed test for %zx lshl %d, expected %zx, got %zx\n", s1l, shiftl, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg << imm) */
	if (verbose) printf(" - lshli\n");
        dill_start_proc(s, "lshli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lshli(s, rdl, dill_param_reg(s, 0), shiftl);
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l << shiftl);
	result = func(s1l, shiftl);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx lshli (imm %d), expected %zx, got %zx\n", s1l, shiftl, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >> reg) */
	if (verbose) printf(" - rshi\n");
        dill_start_proc(s, "rshi",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_rshi(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, shifti);
	dill_free_handle(h);
        expected_result = (s1i >> shifti);
	if (expected_result != result) {
	    printf("Failed test for %d rshi %d, expected %x, got %x\n", s1i, shifti, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int(*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >> imm) */
	if (verbose) printf(" - rshii\n");
        dill_start_proc(s, "rshii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_rshii(s, rdi, dill_param_reg(s, 0), shifti);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i >> shifti);
	result = func(s1i, shifti);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d rshii (imm %d), expected %d, got %d\n", s1i, shifti, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg >> reg) */
	if (verbose) printf(" - rshu\n");
        dill_start_proc(s, "rshu",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_rshu(s, rdu, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, shiftu);
	dill_free_handle(h);
        expected_result = (s1u >> shiftu);
	if (expected_result != result) {
	    printf("Failed test for %u rshu %d, expected %x, got %x\n", s1u, shiftu, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned,unsigned);
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (reg >> imm) */
	if (verbose) printf(" - rshui\n");
        dill_start_proc(s, "rshui", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_rshui(s, rdu, dill_param_reg(s, 0), shiftu);
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u >> shiftu);
	result = func(s1u, shiftu);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u rshui (imm %d), expected %u, got %u\n", s1u, shiftu, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg >> reg) */
	if (verbose) printf(" - rshul\n");
        dill_start_proc(s, "rshul",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_rshul(s, rdul, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, shiftul);
	dill_free_handle(h);
        expected_result = (s1ul >> shiftul);
	if (expected_result != result) {
	    printf("Failed test for %zx rshul %d, expected %zu, got %zu\n", s1ul, shiftul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (reg >> imm) */
	if (verbose) printf(" - rshuli\n");
        dill_start_proc(s, "rshuli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_rshuli(s, rdul, dill_param_reg(s, 0), shiftul);
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul >> shiftul);
	result = func(s1ul, shiftul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx rshuli (imm %d), expected %zx, got %zx\n", s1ul, shiftul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg >> reg) */
	if (verbose) printf(" - rshl\n");
        dill_start_proc(s, "rshl",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_rshl(s, rdl, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, shiftl);
	dill_free_handle(h);
        expected_result = (s1l >> shiftl);
	if (expected_result != result) {
	    printf("Failed test for %zx rshl %d, expected %zx, got %zx\n", s1l, shiftl, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (reg >> imm) */
	if (verbose) printf(" - rshli\n");
        dill_start_proc(s, "rshli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_rshli(s, rdl, dill_param_reg(s, 0), shiftl);
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l >> shiftl);
	result = func(s1l, shiftl);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx rshli (imm %d), expected %zx, got %zx\n", s1l, shiftl, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int(*func)(unsigned);
	dill_reg	rdi1, rdu2;
	dill_exec_handle h;
	int reg_set = 0;

	/* reg <- (int) reg */
	if (verbose) printf(" - cvu2i\n");
	while (get_reg_pair(s, DILL_I, &rdi1, DILL_U, &rdu2, reg_set) != 0) {
	    dill_start_proc(s, "cvu2i",  DILL_I, "%u");
	    /* once more, with feeling (after dill_start_proc) */
	    get_reg_pair(s, DILL_I, &rdi1, DILL_U, &rdu2, reg_set++);
	    if (rdu2 == -1) {
	        rdu2 = dill_param_reg(s, 0);
	    } else {
	        dill_movu(s, rdu2, dill_param_reg(s, 0));
	    }

	    dill_cvu2i(s, rdi1, rdu2);
	    dill_reti(s, rdi1);
	    h = dill_finalize(s);
            func = (int(*)(unsigned)) dill_get_fp(h);
            expected_result = (int) s1u;
	    result = func(s1u);
	    dill_free_handle(h);
	    if (expected_result != result) {
	        printf("Failed test for cvu2i, reg pair %d, %d, got %d, expected %d for (int) %u\n", rdi1, rdu2,  result,  expected_result,  s1u);
	        dill_errors++;
	        dill_dump(s);
	    }
	}
    }
    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(unsigned);
	dill_reg	rdul1, rdu2;
	dill_exec_handle h;
	int reg_set = 0;

	/* reg <- (UIMM_TYPE) reg */
	if (verbose) printf(" - cvu2ul\n");
	while (get_reg_pair(s, DILL_UL, &rdul1, DILL_U, &rdu2, reg_set) != 0) {
	    dill_start_proc(s, "cvu2ul",  DILL_UL, "%u");
	    /* once more, with feeling (after dill_start_proc) */
	    get_reg_pair(s, DILL_UL, &rdul1, DILL_U, &rdu2, reg_set++);
	    if (rdu2 == -1) {
	        rdu2 = dill_param_reg(s, 0);
	    } else {
	        dill_movu(s, rdu2, dill_param_reg(s, 0));
	    }

	    dill_cvu2ul(s, rdul1, rdu2);
	    dill_retul(s, rdul1);
	    h = dill_finalize(s);
            func = (UIMM_TYPE(*)(unsigned)) dill_get_fp(h);
            expected_result = (UIMM_TYPE) s1u;
	    result = func(s1u);
	    dill_free_handle(h);
	    if (expected_result != result) {
	        printf("Failed test for cvu2ul, reg pair %d, %d, got %zx, expected %zx for (UIMM_TYPE) %u\n", rdul1, rdu2,  result,  expected_result,  s1u);
	        dill_errors++;
	        dill_dump(s);
	    }
	}
    }
    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(unsigned);
	dill_reg	rdl1, rdu2;
	dill_exec_handle h;
	int reg_set = 0;

	/* reg <- (IMM_TYPE) reg */
	if (verbose) printf(" - cvu2l\n");
	while (get_reg_pair(s, DILL_L, &rdl1, DILL_U, &rdu2, reg_set) != 0) {
	    dill_start_proc(s, "cvu2l",  DILL_L, "%u");
	    /* once more, with feeling (after dill_start_proc) */
	    get_reg_pair(s, DILL_L, &rdl1, DILL_U, &rdu2, reg_set++);
	    if (rdu2 == -1) {
	        rdu2 = dill_param_reg(s, 0);
	    } else {
	        dill_movu(s, rdu2, dill_param_reg(s, 0));
	    }

	    dill_cvu2l(s, rdl1, rdu2);
	    dill_retl(s, rdl1);
	    h = dill_finalize(s);
            func = (IMM_TYPE(*)(unsigned)) dill_get_fp(h);
            expected_result = (IMM_TYPE) s1u;
	    result = func(s1u);
	    dill_free_handle(h);
	    if (expected_result != result) {
	        printf("Failed test for cvu2l, reg pair %d, %d, got %zx, expected %zx for (IMM_TYPE) %u\n", rdl1, rdu2,  result,  expected_result,  s1u);
	        dill_errors++;
	        dill_dump(s);
	    }
	}
    }
    {
	int expected_result;
	int result;
	int(*func)(IMM_TYPE);
	dill_reg	rdi1, rdl2;
	dill_exec_handle h;
	int reg_set = 0;

	/* reg <- (int) reg */
	if (verbose) printf(" - cvl2i\n");
	while (get_reg_pair(s, DILL_I, &rdi1, DILL_L, &rdl2, reg_set) != 0) {
	    dill_start_proc(s, "cvl2i",  DILL_I, "%l");
	    /* once more, with feeling (after dill_start_proc) */
	    get_reg_pair(s, DILL_I, &rdi1, DILL_L, &rdl2, reg_set++);
	    if (rdl2 == -1) {
	        rdl2 = dill_param_reg(s, 0);
	    } else {
	        dill_movl(s, rdl2, dill_param_reg(s, 0));
	    }

	    dill_cvl2i(s, rdi1, rdl2);
	    dill_reti(s, rdi1);
	    h = dill_finalize(s);
            func = (int(*)(IMM_TYPE)) dill_get_fp(h);
            expected_result = (int) s1l;
	    result = func(s1l);
	    dill_free_handle(h);
	    if (expected_result != result) {
	        printf("Failed test for cvl2i, reg pair %d, %d, got %d, expected %d for (int) %zx\n", rdi1, rdl2,  result,  expected_result,  s1l);
	        dill_errors++;
	        dill_dump(s);
	    }
	}
    }
    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(IMM_TYPE);
	dill_reg	rdu1, rdl2;
	dill_exec_handle h;
	int reg_set = 0;

	/* reg <- (unsigned) reg */
	if (verbose) printf(" - cvl2u\n");
	while (get_reg_pair(s, DILL_U, &rdu1, DILL_L, &rdl2, reg_set) != 0) {
	    dill_start_proc(s, "cvl2u",  DILL_U, "%l");
	    /* once more, with feeling (after dill_start_proc) */
	    get_reg_pair(s, DILL_U, &rdu1, DILL_L, &rdl2, reg_set++);
	    if (rdl2 == -1) {
	        rdl2 = dill_param_reg(s, 0);
	    } else {
	        dill_movl(s, rdl2, dill_param_reg(s, 0));
	    }

	    dill_cvl2u(s, rdu1, rdl2);
	    dill_retu(s, rdu1);
	    h = dill_finalize(s);
            func = (unsigned(*)(IMM_TYPE)) dill_get_fp(h);
            expected_result = (unsigned) s1l;
	    result = func(s1l);
	    dill_free_handle(h);
	    if (expected_result != result) {
	        printf("Failed test for cvl2u, reg pair %d, %d, got %u, expected %u for (unsigned) %zx\n", rdu1, rdl2,  result,  expected_result,  s1l);
	        dill_errors++;
	        dill_dump(s);
	    }
	}
    }
    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(IMM_TYPE);
	dill_reg	rdul1, rdl2;
	dill_exec_handle h;
	int reg_set = 0;

	/* reg <- (UIMM_TYPE) reg */
	if (verbose) printf(" - cvl2ul\n");
	while (get_reg_pair(s, DILL_UL, &rdul1, DILL_L, &rdl2, reg_set) != 0) {
	    dill_start_proc(s, "cvl2ul",  DILL_UL, "%l");
	    /* once more, with feeling (after dill_start_proc) */
	    get_reg_pair(s, DILL_UL, &rdul1, DILL_L, &rdl2, reg_set++);
	    if (rdl2 == -1) {
	        rdl2 = dill_param_reg(s, 0);
	    } else {
	        dill_movl(s, rdl2, dill_param_reg(s, 0));
	    }

	    dill_cvl2ul(s, rdul1, rdl2);
	    dill_retul(s, rdul1);
	    h = dill_finalize(s);
            func = (UIMM_TYPE(*)(IMM_TYPE)) dill_get_fp(h);
            expected_result = (UIMM_TYPE) s1l;
	    result = func(s1l);
	    dill_free_handle(h);
	    if (expected_result != result) {
	        printf("Failed test for cvl2ul, reg pair %d, %d, got %zx, expected %zx for (UIMM_TYPE) %zx\n", rdul1, rdl2,  result,  expected_result,  s1l);
	        dill_errors++;
	        dill_dump(s);
	    }
	}
    }
    {
	int expected_result;
	int result;
	int(*func)(UIMM_TYPE);
	dill_reg	rdi1, rdul2;
	dill_exec_handle h;
	int reg_set = 0;

	/* reg <- (int) reg */
	if (verbose) printf(" - cvul2i\n");
	while (get_reg_pair(s, DILL_I, &rdi1, DILL_UL, &rdul2, reg_set) != 0) {
	    dill_start_proc(s, "cvul2i",  DILL_I, "%ul");
	    /* once more, with feeling (after dill_start_proc) */
	    get_reg_pair(s, DILL_I, &rdi1, DILL_UL, &rdul2, reg_set++);
	    if (rdul2 == -1) {
	        rdul2 = dill_param_reg(s, 0);
	    } else {
	        dill_movul(s, rdul2, dill_param_reg(s, 0));
	    }

	    dill_cvul2i(s, rdi1, rdul2);
	    dill_reti(s, rdi1);
	    h = dill_finalize(s);
            func = (int(*)(UIMM_TYPE)) dill_get_fp(h);
            expected_result = (int) s1ul;
	    result = func(s1ul);
	    dill_free_handle(h);
	    if (expected_result != result) {
	        printf("Failed test for cvul2i, reg pair %d, %d, got %d, expected %d for (int) %zx\n", rdi1, rdul2,  result,  expected_result,  s1ul);
	        dill_errors++;
	        dill_dump(s);
	    }
	}
    }
    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(UIMM_TYPE);
	dill_reg	rdu1, rdul2;
	dill_exec_handle h;
	int reg_set = 0;

	/* reg <- (unsigned) reg */
	if (verbose) printf(" - cvul2u\n");
	while (get_reg_pair(s, DILL_U, &rdu1, DILL_UL, &rdul2, reg_set) != 0) {
	    dill_start_proc(s, "cvul2u",  DILL_U, "%ul");
	    /* once more, with feeling (after dill_start_proc) */
	    get_reg_pair(s, DILL_U, &rdu1, DILL_UL, &rdul2, reg_set++);
	    if (rdul2 == -1) {
	        rdul2 = dill_param_reg(s, 0);
	    } else {
	        dill_movul(s, rdul2, dill_param_reg(s, 0));
	    }

	    dill_cvul2u(s, rdu1, rdul2);
	    dill_retu(s, rdu1);
	    h = dill_finalize(s);
            func = (unsigned(*)(UIMM_TYPE)) dill_get_fp(h);
            expected_result = (unsigned) s1ul;
	    result = func(s1ul);
	    dill_free_handle(h);
	    if (expected_result != result) {
	        printf("Failed test for cvul2u, reg pair %d, %d, got %u, expected %u for (unsigned) %zx\n", rdu1, rdul2,  result,  expected_result,  s1ul);
	        dill_errors++;
	        dill_dump(s);
	    }
	}
    }
    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(UIMM_TYPE);
	dill_reg	rdl1, rdul2;
	dill_exec_handle h;
	int reg_set = 0;

	/* reg <- (IMM_TYPE) reg */
	if (verbose) printf(" - cvul2l\n");
	while (get_reg_pair(s, DILL_L, &rdl1, DILL_UL, &rdul2, reg_set) != 0) {
	    dill_start_proc(s, "cvul2l",  DILL_L, "%ul");
	    /* once more, with feeling (after dill_start_proc) */
	    get_reg_pair(s, DILL_L, &rdl1, DILL_UL, &rdul2, reg_set++);
	    if (rdul2 == -1) {
	        rdul2 = dill_param_reg(s, 0);
	    } else {
	        dill_movul(s, rdul2, dill_param_reg(s, 0));
	    }

	    dill_cvul2l(s, rdl1, rdul2);
	    dill_retl(s, rdl1);
	    h = dill_finalize(s);
            func = (IMM_TYPE(*)(UIMM_TYPE)) dill_get_fp(h);
            expected_result = (IMM_TYPE) s1ul;
	    result = func(s1ul);
	    dill_free_handle(h);
	    if (expected_result != result) {
	        printf("Failed test for cvul2l, reg pair %d, %d, got %zx, expected %zx for (IMM_TYPE) %zx\n", rdl1, rdul2,  result,  expected_result,  s1ul);
	        dill_errors++;
	        dill_dump(s);
	    }
	}
    }
    {
	void * expected_result;
	void * result;
	void *(*func)(UIMM_TYPE);
	dill_reg	rdp1, rdul2;
	dill_exec_handle h;
	int reg_set = 0;

	/* reg <- (void *) reg */
	if (verbose) printf(" - cvul2p\n");
	while (get_reg_pair(s, DILL_P, &rdp1, DILL_UL, &rdul2, reg_set) != 0) {
	    dill_start_proc(s, "cvul2p",  DILL_P, "%ul");
	    /* once more, with feeling (after dill_start_proc) */
	    get_reg_pair(s, DILL_P, &rdp1, DILL_UL, &rdul2, reg_set++);
	    if (rdul2 == -1) {
	        rdul2 = dill_param_reg(s, 0);
	    } else {
	        dill_movul(s, rdul2, dill_param_reg(s, 0));
	    }

	    dill_cvul2p(s, rdp1, rdul2);
	    dill_retp(s, rdp1);
	    h = dill_finalize(s);
            func = (void *(*)(UIMM_TYPE)) dill_get_fp(h);
            expected_result = (void *) s1ul;
	    result = func(s1ul);
	    dill_free_handle(h);
	    if (expected_result != result) {
	        printf("Failed test for cvul2p, reg pair %d, %d, got %p, expected %p for (void *) %zx\n", rdp1, rdul2,  result,  expected_result,  s1ul);
	        dill_errors++;
	        dill_dump(s);
	    }
	}
    }
    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(void *);
	dill_reg	rdul1, rdp2;
	dill_exec_handle h;
	int reg_set = 0;

	/* reg <- (UIMM_TYPE) reg */
	if (verbose) printf(" - cvp2ul\n");
	while (get_reg_pair(s, DILL_UL, &rdul1, DILL_P, &rdp2, reg_set) != 0) {
	    dill_start_proc(s, "cvp2ul",  DILL_UL, "%p");
	    /* once more, with feeling (after dill_start_proc) */
	    get_reg_pair(s, DILL_UL, &rdul1, DILL_P, &rdp2, reg_set++);
	    if (rdp2 == -1) {
	        rdp2 = dill_param_reg(s, 0);
	    } else {
	        dill_movp(s, rdp2, dill_param_reg(s, 0));
	    }

	    dill_cvp2ul(s, rdul1, rdp2);
	    dill_retul(s, rdul1);
	    h = dill_finalize(s);
            func = (UIMM_TYPE(*)(void *)) dill_get_fp(h);
            expected_result = (UIMM_TYPE) s1p;
	    result = func(s1p);
	    dill_free_handle(h);
	    if (expected_result != result) {
	        printf("Failed test for cvp2ul, reg pair %d, %d, got %zx, expected %zx for (UIMM_TYPE) %p\n", rdul1, rdp2,  result,  expected_result,  s1p);
	        dill_errors++;
	        dill_dump(s);
	    }
	}
    }
    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(int);
	dill_reg	rdu1, rdi2;
	dill_exec_handle h;
	int reg_set = 0;

	/* reg <- (unsigned) reg */
	if (verbose) printf(" - cvi2u\n");
	while (get_reg_pair(s, DILL_U, &rdu1, DILL_I, &rdi2, reg_set) != 0) {
	    dill_start_proc(s, "cvi2u",  DILL_U, "%i");
	    /* once more, with feeling (after dill_start_proc) */
	    get_reg_pair(s, DILL_U, &rdu1, DILL_I, &rdi2, reg_set++);
	    if (rdi2 == -1) {
	        rdi2 = dill_param_reg(s, 0);
	    } else {
	        dill_movi(s, rdi2, dill_param_reg(s, 0));
	    }

	    dill_cvi2u(s, rdu1, rdi2);
	    dill_retu(s, rdu1);
	    h = dill_finalize(s);
            func = (unsigned(*)(int)) dill_get_fp(h);
            expected_result = (unsigned) s1i;
	    result = func(s1i);
	    dill_free_handle(h);
	    if (expected_result != result) {
	        printf("Failed test for cvi2u, reg pair %d, %d, got %u, expected %u for (unsigned) %d\n", rdu1, rdi2,  result,  expected_result,  s1i);
	        dill_errors++;
	        dill_dump(s);
	    }
	}
    }
    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(int);
	dill_reg	rdul1, rdi2;
	dill_exec_handle h;
	int reg_set = 0;

	/* reg <- (UIMM_TYPE) reg */
	if (verbose) printf(" - cvi2ul\n");
	while (get_reg_pair(s, DILL_UL, &rdul1, DILL_I, &rdi2, reg_set) != 0) {
	    dill_start_proc(s, "cvi2ul",  DILL_UL, "%i");
	    /* once more, with feeling (after dill_start_proc) */
	    get_reg_pair(s, DILL_UL, &rdul1, DILL_I, &rdi2, reg_set++);
	    if (rdi2 == -1) {
	        rdi2 = dill_param_reg(s, 0);
	    } else {
	        dill_movi(s, rdi2, dill_param_reg(s, 0));
	    }

	    dill_cvi2ul(s, rdul1, rdi2);
	    dill_retul(s, rdul1);
	    h = dill_finalize(s);
            func = (UIMM_TYPE(*)(int)) dill_get_fp(h);
            expected_result = (UIMM_TYPE) s1i;
	    result = func(s1i);
	    dill_free_handle(h);
	    if (expected_result != result) {
	        printf("Failed test for cvi2ul, reg pair %d, %d, got %zx, expected %zx for (UIMM_TYPE) %d\n", rdul1, rdi2,  result,  expected_result,  s1i);
	        dill_errors++;
	        dill_dump(s);
	    }
	}
    }
    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(int);
	dill_reg	rdl1, rdi2;
	dill_exec_handle h;
	int reg_set = 0;

	/* reg <- (IMM_TYPE) reg */
	if (verbose) printf(" - cvi2l\n");
	while (get_reg_pair(s, DILL_L, &rdl1, DILL_I, &rdi2, reg_set) != 0) {
	    dill_start_proc(s, "cvi2l",  DILL_L, "%i");
	    /* once more, with feeling (after dill_start_proc) */
	    get_reg_pair(s, DILL_L, &rdl1, DILL_I, &rdi2, reg_set++);
	    if (rdi2 == -1) {
	        rdi2 = dill_param_reg(s, 0);
	    } else {
	        dill_movi(s, rdi2, dill_param_reg(s, 0));
	    }

	    dill_cvi2l(s, rdl1, rdi2);
	    dill_retl(s, rdl1);
	    h = dill_finalize(s);
            func = (IMM_TYPE(*)(int)) dill_get_fp(h);
            expected_result = (IMM_TYPE) s1i;
	    result = func(s1i);
	    dill_free_handle(h);
	    if (expected_result != result) {
	        printf("Failed test for cvi2l, reg pair %d, %d, got %zx, expected %zx for (IMM_TYPE) %d\n", rdl1, rdi2,  result,  expected_result,  s1i);
	        dill_errors++;
	        dill_dump(s);
	    }
	}
    }
    {
	int expected_result;
	int result;
	int(*func)(int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- ~ reg */
	if (verbose) printf(" - comi\n");
        dill_start_proc(s, "comi", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_comi(s, rdi, dill_param_reg(s, 0));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int)) dill_get_fp(h);
        expected_result = ~ s1i;
	result = func(s1i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for comi %d , expected %d, got %d\n",  s1i,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned);
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- ~ reg */
	if (verbose) printf(" - comu\n");
        dill_start_proc(s, "comu", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_comu(s, rdu, dill_param_reg(s, 0));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned)) dill_get_fp(h);
        expected_result = ~ s1u;
	result = func(s1u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for comu %u , expected %u, got %u\n",  s1u,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- ~ reg */
	if (verbose) printf(" - comul\n");
        dill_start_proc(s, "comul", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_comul(s, rdul, dill_param_reg(s, 0));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE)) dill_get_fp(h);
        expected_result = ~ s1ul;
	result = func(s1ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for comul %zx , expected %zx, got %zx\n",  s1ul,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE);
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- ~ reg */
	if (verbose) printf(" - coml\n");
        dill_start_proc(s, "coml", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_coml(s, rdl, dill_param_reg(s, 0));
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE)) dill_get_fp(h);
        expected_result = ~ s1l;
	result = func(s1l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for coml %zx , expected %zx, got %zx\n",  s1l,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int(*func)(int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- ! reg */
	if (verbose) printf(" - noti\n");
        dill_start_proc(s, "noti", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_noti(s, rdi, dill_param_reg(s, 0));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int)) dill_get_fp(h);
        expected_result = ! s1i;
	result = func(s1i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for noti %d , expected %d, got %d\n",  s1i,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned);
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- ! reg */
	if (verbose) printf(" - notu\n");
        dill_start_proc(s, "notu", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_notu(s, rdu, dill_param_reg(s, 0));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned)) dill_get_fp(h);
        expected_result = ! s1u;
	result = func(s1u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for notu %u , expected %u, got %u\n",  s1u,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- ! reg */
	if (verbose) printf(" - notul\n");
        dill_start_proc(s, "notul", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_notul(s, rdul, dill_param_reg(s, 0));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE)) dill_get_fp(h);
        expected_result = ! s1ul;
	result = func(s1ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for notul %zx , expected %zx, got %zx\n",  s1ul,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE);
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- ! reg */
	if (verbose) printf(" - notl\n");
        dill_start_proc(s, "notl", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_notl(s, rdl, dill_param_reg(s, 0));
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE)) dill_get_fp(h);
        expected_result = ! s1l;
	result = func(s1l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for notl %zx , expected %zx, got %zx\n",  s1l,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int(*func)(int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <-   reg */
	if (verbose) printf(" - movi\n");
        dill_start_proc(s, "movi", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_movi(s, rdi, dill_param_reg(s, 0));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int)) dill_get_fp(h);
        expected_result =   s1i;
	result = func(s1i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for movi %d , expected %d, got %d\n",  s1i,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned);
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <-   reg */
	if (verbose) printf(" - movu\n");
        dill_start_proc(s, "movu", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_movu(s, rdu, dill_param_reg(s, 0));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned)) dill_get_fp(h);
        expected_result =   s1u;
	result = func(s1u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for movu %u , expected %u, got %u\n",  s1u,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <-   reg */
	if (verbose) printf(" - movul\n");
        dill_start_proc(s, "movul", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_movul(s, rdul, dill_param_reg(s, 0));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE)) dill_get_fp(h);
        expected_result =   s1ul;
	result = func(s1ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for movul %zx , expected %zx, got %zx\n",  s1ul,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE);
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <-   reg */
	if (verbose) printf(" - movl\n");
        dill_start_proc(s, "movl", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_movl(s, rdl, dill_param_reg(s, 0));
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE)) dill_get_fp(h);
        expected_result =   s1l;
	result = func(s1l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for movl %zx , expected %zx, got %zx\n",  s1l,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	void * expected_result;
	void * result;
	void *(*func)(void *);
	dill_reg	rdp;
	dill_exec_handle h;

	/* reg <-   reg */
	if (verbose) printf(" - movp\n");
        dill_start_proc(s, "movp", DILL_P, "%p");
		if(!dill_raw_getreg(s, &rdp, DILL_P, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_movp(s, rdp, dill_param_reg(s, 0));
        	dill_retp(s, rdp);
	h = dill_finalize(s);
        func = (void *(*)(void *)) dill_get_fp(h);
        expected_result =   s1p;
	result = func(s1p);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for movp %p , expected %p, got %p\n",  s1p,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int(*func)(int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- - reg */
	if (verbose) printf(" - negi\n");
        dill_start_proc(s, "negi", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_negi(s, rdi, dill_param_reg(s, 0));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int)) dill_get_fp(h);
        expected_result = - s1i;
	result = func(s1i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for negi %d , expected %d, got %d\n",  s1i,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned expected_result;
	unsigned result;
	unsigned(*func)(unsigned);
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (unsigned) - (int) reg */
	if (verbose) printf(" - negu\n");
        dill_start_proc(s, "negu", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_negu(s, rdu, dill_param_reg(s, 0));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned)) dill_get_fp(h);
        expected_result = (unsigned) - (int) s1u;
	result = func(s1u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for negu %u , expected %u, got %u\n",  s1u,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
	UIMM_TYPE(*func)(UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (uintptr_t) - (intptr_t) reg */
	if (verbose) printf(" - negul\n");
        dill_start_proc(s, "negul", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_negul(s, rdul, dill_param_reg(s, 0));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE)) dill_get_fp(h);
        expected_result = (uintptr_t) - (intptr_t) s1ul;
	result = func(s1ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for negul %zx , expected %zx, got %zx\n",  s1ul,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	IMM_TYPE expected_result;
	IMM_TYPE result;
	IMM_TYPE(*func)(IMM_TYPE);
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- - reg */
	if (verbose) printf(" - negl\n");
        dill_start_proc(s, "negl", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_negl(s, rdl, dill_param_reg(s, 0));
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE)) dill_get_fp(h);
        expected_result = - s1l;
	result = func(s1l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for negl %zx , expected %zx, got %zx\n",  s1l,  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {  /* bswap */
        char tmp;
	short expected_result;
	short result;
        union {
	   short val;
	   char c[8];
	   int  i[2];
        }u;
	short(*func)(short);
	dill_reg	rds;
	dill_exec_handle h;

	/* reg <- (BSWAP) reg */
	if (verbose) printf(" - bswaps\n");
        dill_start_proc(s, "bswap",  DILL_S, "%s");
		if(!dill_raw_getreg(s, &rds, DILL_S, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_bswaps(s, rds, dill_param_reg(s, 0));
        	dill_rets(s, rds);
	h = dill_finalize(s);
        func = (short(*)(short)) dill_get_fp(h);
	u.val = s1s;
	switch(sizeof(short)) {
	case 2:
	tmp = u.c[0];
	u.c[0] = u.c[1];
	u.c[1] = tmp;
	break;
	case 4:
	tmp = u.c[0];
	u.c[0] = u.c[3];
	u.c[3] = tmp;
	tmp = u.c[1];
	u.c[1] = u.c[2];
	u.c[2] = tmp;
	break;
	case 8:
	tmp = u.c[0];
	u.c[0] = u.c[7];
	u.c[7] = tmp;
	tmp = u.c[1];
	u.c[1] = u.c[6];
	u.c[6] = tmp;
	tmp = u.c[2];
	u.c[2] = u.c[5];
	u.c[5] = tmp;
	tmp = u.c[3];
	u.c[3] = u.c[4];
	u.c[4] = tmp;
	break;
	}
        expected_result = u.val;
	result = func(s1s);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for bswaps, got %d, expected %d for (short) %d\n",  result,  expected_result,  s1s);
	    u.val = expected_result;
	    printf("expected %x %x ", u.i[0], u.i[1]);
	    u.val = result;
	    printf("got  %x %x \n", u.i[0], u.i[1]);
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {  /* bswap */
        char tmp;
	unsigned short expected_result;
	unsigned short result;
        union {
	   unsigned short val;
	   char c[8];
	   int  i[2];
        }u;
	unsigned short(*func)(unsigned short);
	dill_reg	rdus;
	dill_exec_handle h;

	/* reg <- (BSWAP) reg */
	if (verbose) printf(" - bswapus\n");
        dill_start_proc(s, "bswap",  DILL_US, "%us");
		if(!dill_raw_getreg(s, &rdus, DILL_US, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_bswapus(s, rdus, dill_param_reg(s, 0));
        	dill_retus(s, rdus);
	h = dill_finalize(s);
        func = (unsigned short(*)(unsigned short)) dill_get_fp(h);
	u.val = s1us;
	switch(sizeof(unsigned short)) {
	case 2:
	tmp = u.c[0];
	u.c[0] = u.c[1];
	u.c[1] = tmp;
	break;
	case 4:
	tmp = u.c[0];
	u.c[0] = u.c[3];
	u.c[3] = tmp;
	tmp = u.c[1];
	u.c[1] = u.c[2];
	u.c[2] = tmp;
	break;
	case 8:
	tmp = u.c[0];
	u.c[0] = u.c[7];
	u.c[7] = tmp;
	tmp = u.c[1];
	u.c[1] = u.c[6];
	u.c[6] = tmp;
	tmp = u.c[2];
	u.c[2] = u.c[5];
	u.c[5] = tmp;
	tmp = u.c[3];
	u.c[3] = u.c[4];
	u.c[4] = tmp;
	break;
	}
        expected_result = u.val;
	result = func(s1us);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for bswapus, got %u, expected %u for (unsigned short) %u\n",  result,  expected_result,  s1us);
	    u.val = expected_result;
	    printf("expected %x %x ", u.i[0], u.i[1]);
	    u.val = result;
	    printf("got  %x %x \n", u.i[0], u.i[1]);
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {  /* bswap */
        char tmp;
	int expected_result;
	int result;
        union {
	   int val;
	   char c[8];
	   int  i[2];
        }u;
	int(*func)(int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (BSWAP) reg */
	if (verbose) printf(" - bswapi\n");
        dill_start_proc(s, "bswap",  DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_bswapi(s, rdi, dill_param_reg(s, 0));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int)) dill_get_fp(h);
	u.val = s1i;
	switch(sizeof(int)) {
	case 2:
	tmp = u.c[0];
	u.c[0] = u.c[1];
	u.c[1] = tmp;
	break;
	case 4:
	tmp = u.c[0];
	u.c[0] = u.c[3];
	u.c[3] = tmp;
	tmp = u.c[1];
	u.c[1] = u.c[2];
	u.c[2] = tmp;
	break;
	case 8:
	tmp = u.c[0];
	u.c[0] = u.c[7];
	u.c[7] = tmp;
	tmp = u.c[1];
	u.c[1] = u.c[6];
	u.c[6] = tmp;
	tmp = u.c[2];
	u.c[2] = u.c[5];
	u.c[5] = tmp;
	tmp = u.c[3];
	u.c[3] = u.c[4];
	u.c[4] = tmp;
	break;
	}
        expected_result = u.val;
	result = func(s1i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for bswapi, got %d, expected %d for (int) %d\n",  result,  expected_result,  s1i);
	    u.val = expected_result;
	    printf("expected %x %x ", u.i[0], u.i[1]);
	    u.val = result;
	    printf("got  %x %x \n", u.i[0], u.i[1]);
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {  /* bswap */
        char tmp;
	unsigned expected_result;
	unsigned result;
        union {
	   unsigned val;
	   char c[8];
	   int  i[2];
        }u;
	unsigned(*func)(unsigned);
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- (BSWAP) reg */
	if (verbose) printf(" - bswapu\n");
        dill_start_proc(s, "bswap",  DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_bswapu(s, rdu, dill_param_reg(s, 0));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned)) dill_get_fp(h);
	u.val = s1u;
	switch(sizeof(unsigned)) {
	case 2:
	tmp = u.c[0];
	u.c[0] = u.c[1];
	u.c[1] = tmp;
	break;
	case 4:
	tmp = u.c[0];
	u.c[0] = u.c[3];
	u.c[3] = tmp;
	tmp = u.c[1];
	u.c[1] = u.c[2];
	u.c[2] = tmp;
	break;
	case 8:
	tmp = u.c[0];
	u.c[0] = u.c[7];
	u.c[7] = tmp;
	tmp = u.c[1];
	u.c[1] = u.c[6];
	u.c[6] = tmp;
	tmp = u.c[2];
	u.c[2] = u.c[5];
	u.c[5] = tmp;
	tmp = u.c[3];
	u.c[3] = u.c[4];
	u.c[4] = tmp;
	break;
	}
        expected_result = u.val;
	result = func(s1u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for bswapu, got %u, expected %u for (unsigned) %u\n",  result,  expected_result,  s1u);
	    u.val = expected_result;
	    printf("expected %x %x ", u.i[0], u.i[1]);
	    u.val = result;
	    printf("got  %x %x \n", u.i[0], u.i[1]);
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {  /* bswap */
        char tmp;
	UIMM_TYPE expected_result;
	UIMM_TYPE result;
        union {
	   UIMM_TYPE val;
	   char c[8];
	   int  i[2];
        }u;
	UIMM_TYPE(*func)(UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- (BSWAP) reg */
	if (verbose) printf(" - bswapul\n");
        dill_start_proc(s, "bswap",  DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_bswapul(s, rdul, dill_param_reg(s, 0));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE)) dill_get_fp(h);
	u.val = s1ul;
	switch(sizeof(UIMM_TYPE)) {
	case 2:
	tmp = u.c[0];
	u.c[0] = u.c[1];
	u.c[1] = tmp;
	break;
	case 4:
	tmp = u.c[0];
	u.c[0] = u.c[3];
	u.c[3] = tmp;
	tmp = u.c[1];
	u.c[1] = u.c[2];
	u.c[2] = tmp;
	break;
	case 8:
	tmp = u.c[0];
	u.c[0] = u.c[7];
	u.c[7] = tmp;
	tmp = u.c[1];
	u.c[1] = u.c[6];
	u.c[6] = tmp;
	tmp = u.c[2];
	u.c[2] = u.c[5];
	u.c[5] = tmp;
	tmp = u.c[3];
	u.c[3] = u.c[4];
	u.c[4] = tmp;
	break;
	}
        expected_result = u.val;
	result = func(s1ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for bswapul, got %zx, expected %zx for (UIMM_TYPE) %zx\n",  result,  expected_result,  s1ul);
	    u.val = expected_result;
	    printf("expected %x %x ", u.i[0], u.i[1]);
	    u.val = result;
	    printf("got  %x %x \n", u.i[0], u.i[1]);
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {  /* bswap */
        char tmp;
	IMM_TYPE expected_result;
	IMM_TYPE result;
        union {
	   IMM_TYPE val;
	   char c[8];
	   int  i[2];
        }u;
	IMM_TYPE(*func)(IMM_TYPE);
	dill_reg	rdl;
	dill_exec_handle h;

	/* reg <- (BSWAP) reg */
	if (verbose) printf(" - bswapl\n");
        dill_start_proc(s, "bswap",  DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdl, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_bswapl(s, rdl, dill_param_reg(s, 0));
        	dill_retl(s, rdl);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE)) dill_get_fp(h);
	u.val = s1l;
	switch(sizeof(IMM_TYPE)) {
	case 2:
	tmp = u.c[0];
	u.c[0] = u.c[1];
	u.c[1] = tmp;
	break;
	case 4:
	tmp = u.c[0];
	u.c[0] = u.c[3];
	u.c[3] = tmp;
	tmp = u.c[1];
	u.c[1] = u.c[2];
	u.c[2] = tmp;
	break;
	case 8:
	tmp = u.c[0];
	u.c[0] = u.c[7];
	u.c[7] = tmp;
	tmp = u.c[1];
	u.c[1] = u.c[6];
	u.c[6] = tmp;
	tmp = u.c[2];
	u.c[2] = u.c[5];
	u.c[5] = tmp;
	tmp = u.c[3];
	u.c[3] = u.c[4];
	u.c[4] = tmp;
	break;
	}
        expected_result = u.val;
	result = func(s1l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for bswapl, got %zx, expected %zx for (IMM_TYPE) %zx\n",  result,  expected_result,  s1l);
	    u.val = expected_result;
	    printf("expected %x %x ", u.i[0], u.i[1]);
	    u.val = result;
	    printf("got  %x %x \n", u.i[0], u.i[1]);
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	/* ret reg */
	int(*func)(int);
	int result;
	int expected_result;
	dill_exec_handle h;

	if (verbose) printf(" - reti\n");
        dill_start_proc(s, "reti",  DILL_I, "%i");
        	dill_reti(s, dill_param_reg(s, 0));
	h = dill_finalize(s);
        func = (int(*)(int)) dill_get_fp(h);
	result = func(s1i);
	dill_free_handle(h);
	expected_result = s1i;
	if (expected_result != result) {
	    printf("Failed test for reti, expected %d, got %d\n",  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	/* ret imm */
	int(*func)();
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - retii\n");
        dill_start_simple_proc(s, "retii", DILL_I);
        	dill_retii(s, s1i);
	h = dill_finalize(s);
        func = (int(*)()) dill_get_fp(h);
	result = func();
	dill_free_handle(h);

	if (s1i != result) {
	    printf("Failed test for retii\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	/* ret reg */
	unsigned(*func)(unsigned);
	unsigned result;
	unsigned expected_result;
	dill_exec_handle h;

	if (verbose) printf(" - retu\n");
        dill_start_proc(s, "retu",  DILL_U, "%u");
        	dill_retu(s, dill_param_reg(s, 0));
	h = dill_finalize(s);
        func = (unsigned(*)(unsigned)) dill_get_fp(h);
	result = func(s1u);
	dill_free_handle(h);
	expected_result = s1u;
	if (expected_result != result) {
	    printf("Failed test for retu, expected %u, got %u\n",  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	/* ret imm */
	unsigned(*func)();
	unsigned result;
	dill_exec_handle h;

	if (verbose) printf(" - retui\n");
        dill_start_simple_proc(s, "retui", DILL_U);
        	dill_retui(s, s1u);
	h = dill_finalize(s);
        func = (unsigned(*)()) dill_get_fp(h);
	result = func();
	dill_free_handle(h);

	if (s1u != result) {
	    printf("Failed test for retui\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	/* ret reg */
	UIMM_TYPE(*func)(UIMM_TYPE);
	UIMM_TYPE result;
	UIMM_TYPE expected_result;
	dill_exec_handle h;

	if (verbose) printf(" - retul\n");
        dill_start_proc(s, "retul",  DILL_UL, "%ul");
        	dill_retul(s, dill_param_reg(s, 0));
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul);
	dill_free_handle(h);
	expected_result = s1ul;
	if (expected_result != result) {
	    printf("Failed test for retul, expected %zx, got %zx\n",  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	/* ret imm */
	UIMM_TYPE(*func)();
	UIMM_TYPE result;
	dill_exec_handle h;

	if (verbose) printf(" - retuli\n");
        dill_start_simple_proc(s, "retuli", DILL_UL);
        	dill_retuli(s, s1ul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)()) dill_get_fp(h);
	result = func();
	dill_free_handle(h);

	if (s1ul != result) {
	    printf("Failed test for retuli\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	/* ret reg */
	IMM_TYPE(*func)(IMM_TYPE);
	IMM_TYPE result;
	IMM_TYPE expected_result;
	dill_exec_handle h;

	if (verbose) printf(" - retl\n");
        dill_start_proc(s, "retl",  DILL_L, "%l");
        	dill_retl(s, dill_param_reg(s, 0));
	h = dill_finalize(s);
        func = (IMM_TYPE(*)(IMM_TYPE)) dill_get_fp(h);
	result = func(s1l);
	dill_free_handle(h);
	expected_result = s1l;
	if (expected_result != result) {
	    printf("Failed test for retl, expected %zx, got %zx\n",  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	/* ret imm */
	IMM_TYPE(*func)();
	IMM_TYPE result;
	dill_exec_handle h;

	if (verbose) printf(" - retli\n");
        dill_start_simple_proc(s, "retli", DILL_L);
        	dill_retli(s, s1l);
	h = dill_finalize(s);
        func = (IMM_TYPE(*)()) dill_get_fp(h);
	result = func();
	dill_free_handle(h);

	if (s1l != result) {
	    printf("Failed test for retli\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	/* ret reg */
	void *(*func)(void *);
	void * result;
	void * expected_result;
	dill_exec_handle h;

	if (verbose) printf(" - retp\n");
        dill_start_proc(s, "retp",  DILL_P, "%p");
        	dill_retp(s, dill_param_reg(s, 0));
	h = dill_finalize(s);
        func = (void *(*)(void *)) dill_get_fp(h);
	result = func(s1p);
	dill_free_handle(h);
	expected_result = s1p;
	if (expected_result != result) {
	    printf("Failed test for retp, expected %p, got %p\n",  expected_result,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	/* ret imm */
	void *(*func)();
	void * result;
	dill_exec_handle h;

	if (verbose) printf(" - retpi\n");
        dill_start_simple_proc(s, "retpi", DILL_P);
        	dill_retpi(s, s1p);
	h = dill_finalize(s);
        func = (void *(*)()) dill_get_fp(h);
	result = func();
	dill_free_handle(h);

	if (s1p != result) {
	    printf("Failed test for retpi\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
        void (*func)(UIMM_TYPE, UIMM_TYPE);
	dill_reg	rdc;
	dill_exec_handle h;

	if (verbose) printf(" - stc\n");
	s2ul = (UIMM_TYPE)&dc - aligned_offset;

	/* mem [ reg + reg ] <- reg */
        dill_start_proc(s, "stc", DILL_V, "%ul%ul");
		if(!dill_raw_getreg(s, &rdc, DILL_C, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_setc(s, rdc, s1c);
        	dill_stc(s, rdc, dill_param_reg(s, 0), dill_param_reg(s, 1));
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
        ((void(*)(UIMM_TYPE, UIMM_TYPE))func)(s2ul, aligned_offset);
	dill_free_handle(h);
	if (dc != (char)s1c) {
	    printf("Failed test for stc\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	/* mem [ reg + reg ] <- reg */
        void (*func)(UIMM_TYPE);
	dill_reg	rdc;
	dill_exec_handle h;

	dc = 0;
	if (verbose) printf(" - stci\n");
        dill_start_proc(s, "stci", DILL_V, "%ul");
		if(!dill_raw_getreg(s, &rdc, DILL_C, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_setc(s, rdc, s1c);
        	dill_stci(s, rdc, dill_param_reg(s, 0), aligned_offset);
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE))dill_get_fp(h);
        ((void(*)(UIMM_TYPE))func)(s2ul);
	dill_free_handle(h);

	if (dc != (char)s1c) {
	    printf("Failed test for stci\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
        void (*func)(UIMM_TYPE, UIMM_TYPE);
	dill_reg	rduc;
	dill_exec_handle h;

	if (verbose) printf(" - stuc\n");
	s2ul = (UIMM_TYPE)&duc - aligned_offset;

	/* mem [ reg + reg ] <- reg */
        dill_start_proc(s, "stuc", DILL_V, "%ul%ul");
		if(!dill_raw_getreg(s, &rduc, DILL_UC, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_setuc(s, rduc, s1uc);
        	dill_stuc(s, rduc, dill_param_reg(s, 0), dill_param_reg(s, 1));
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
        ((void(*)(UIMM_TYPE, UIMM_TYPE))func)(s2ul, aligned_offset);
	dill_free_handle(h);
	if (duc != (unsigned char)s1uc) {
	    printf("Failed test for stuc\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	/* mem [ reg + reg ] <- reg */
        void (*func)(UIMM_TYPE);
	dill_reg	rduc;
	dill_exec_handle h;

	duc = 0;
	if (verbose) printf(" - stuci\n");
        dill_start_proc(s, "stuci", DILL_V, "%ul");
		if(!dill_raw_getreg(s, &rduc, DILL_UC, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_setuc(s, rduc, s1uc);
        	dill_stuci(s, rduc, dill_param_reg(s, 0), aligned_offset);
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE))dill_get_fp(h);
        ((void(*)(UIMM_TYPE))func)(s2ul);
	dill_free_handle(h);

	if (duc != (unsigned char)s1uc) {
	    printf("Failed test for stuci\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
        void (*func)(UIMM_TYPE, UIMM_TYPE);
	dill_reg	rds;
	dill_exec_handle h;

	if (verbose) printf(" - sts\n");
	s2ul = (UIMM_TYPE)&ds - aligned_offset;

	/* mem [ reg + reg ] <- reg */
        dill_start_proc(s, "sts", DILL_V, "%ul%ul");
		if(!dill_raw_getreg(s, &rds, DILL_S, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_sets(s, rds, s1s);
        	dill_sts(s, rds, dill_param_reg(s, 0), dill_param_reg(s, 1));
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
        ((void(*)(UIMM_TYPE, UIMM_TYPE))func)(s2ul, aligned_offset);
	dill_free_handle(h);
	if (ds != (short)s1s) {
	    printf("Failed test for sts\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	/* mem [ reg + reg ] <- reg */
        void (*func)(UIMM_TYPE);
	dill_reg	rds;
	dill_exec_handle h;

	ds = 0;
	if (verbose) printf(" - stsi\n");
        dill_start_proc(s, "stsi", DILL_V, "%ul");
		if(!dill_raw_getreg(s, &rds, DILL_S, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_sets(s, rds, s1s);
        	dill_stsi(s, rds, dill_param_reg(s, 0), aligned_offset);
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE))dill_get_fp(h);
        ((void(*)(UIMM_TYPE))func)(s2ul);
	dill_free_handle(h);

	if (ds != (short)s1s) {
	    printf("Failed test for stsi\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
        void (*func)(UIMM_TYPE, UIMM_TYPE);
	dill_reg	rdus;
	dill_exec_handle h;

	if (verbose) printf(" - stus\n");
	s2ul = (UIMM_TYPE)&dus - aligned_offset;

	/* mem [ reg + reg ] <- reg */
        dill_start_proc(s, "stus", DILL_V, "%ul%ul");
		if(!dill_raw_getreg(s, &rdus, DILL_US, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_setus(s, rdus, s1us);
        	dill_stus(s, rdus, dill_param_reg(s, 0), dill_param_reg(s, 1));
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
        ((void(*)(UIMM_TYPE, UIMM_TYPE))func)(s2ul, aligned_offset);
	dill_free_handle(h);
	if (dus != (unsigned short)s1us) {
	    printf("Failed test for stus\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	/* mem [ reg + reg ] <- reg */
        void (*func)(UIMM_TYPE);
	dill_reg	rdus;
	dill_exec_handle h;

	dus = 0;
	if (verbose) printf(" - stusi\n");
        dill_start_proc(s, "stusi", DILL_V, "%ul");
		if(!dill_raw_getreg(s, &rdus, DILL_US, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_setus(s, rdus, s1us);
        	dill_stusi(s, rdus, dill_param_reg(s, 0), aligned_offset);
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE))dill_get_fp(h);
        ((void(*)(UIMM_TYPE))func)(s2ul);
	dill_free_handle(h);

	if (dus != (unsigned short)s1us) {
	    printf("Failed test for stusi\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
        void (*func)(UIMM_TYPE, UIMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	if (verbose) printf(" - sti\n");
	s2ul = (UIMM_TYPE)&di - aligned_offset;

	/* mem [ reg + reg ] <- reg */
        dill_start_proc(s, "sti", DILL_V, "%ul%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_seti(s, rdi, s1i);
        	dill_sti(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
        ((void(*)(UIMM_TYPE, UIMM_TYPE))func)(s2ul, aligned_offset);
	dill_free_handle(h);
	if (di != (int)s1i) {
	    printf("Failed test for sti\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	/* mem [ reg + reg ] <- reg */
        void (*func)(UIMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	di = 0;
	if (verbose) printf(" - stii\n");
        dill_start_proc(s, "stii", DILL_V, "%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_seti(s, rdi, s1i);
        	dill_stii(s, rdi, dill_param_reg(s, 0), aligned_offset);
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE))dill_get_fp(h);
        ((void(*)(UIMM_TYPE))func)(s2ul);
	dill_free_handle(h);

	if (di != (int)s1i) {
	    printf("Failed test for stii\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
        void (*func)(UIMM_TYPE, UIMM_TYPE);
	dill_reg	rdu;
	dill_exec_handle h;

	if (verbose) printf(" - stu\n");
	s2ul = (UIMM_TYPE)&du - aligned_offset;

	/* mem [ reg + reg ] <- reg */
        dill_start_proc(s, "stu", DILL_V, "%ul%ul");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_setu(s, rdu, s1u);
        	dill_stu(s, rdu, dill_param_reg(s, 0), dill_param_reg(s, 1));
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
        ((void(*)(UIMM_TYPE, UIMM_TYPE))func)(s2ul, aligned_offset);
	dill_free_handle(h);
	if (du != (unsigned)s1u) {
	    printf("Failed test for stu\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	/* mem [ reg + reg ] <- reg */
        void (*func)(UIMM_TYPE);
	dill_reg	rdu;
	dill_exec_handle h;

	du = 0;
	if (verbose) printf(" - stui\n");
        dill_start_proc(s, "stui", DILL_V, "%ul");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_setu(s, rdu, s1u);
        	dill_stui(s, rdu, dill_param_reg(s, 0), aligned_offset);
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE))dill_get_fp(h);
        ((void(*)(UIMM_TYPE))func)(s2ul);
	dill_free_handle(h);

	if (du != (unsigned)s1u) {
	    printf("Failed test for stui\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
        void (*func)(UIMM_TYPE, UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	if (verbose) printf(" - stul\n");
	s2ul = (UIMM_TYPE)&dul - aligned_offset;

	/* mem [ reg + reg ] <- reg */
        dill_start_proc(s, "stul", DILL_V, "%ul%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_setul(s, rdul, s1ul);
        	dill_stul(s, rdul, dill_param_reg(s, 0), dill_param_reg(s, 1));
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
        ((void(*)(UIMM_TYPE, UIMM_TYPE))func)(s2ul, aligned_offset);
	dill_free_handle(h);
	if (dul != (UIMM_TYPE)s1ul) {
	    printf("Failed test for stul\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	/* mem [ reg + reg ] <- reg */
        void (*func)(UIMM_TYPE);
	dill_reg	rdul;
	dill_exec_handle h;

	dul = 0;
	if (verbose) printf(" - stuli\n");
        dill_start_proc(s, "stuli", DILL_V, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_setul(s, rdul, s1ul);
        	dill_stuli(s, rdul, dill_param_reg(s, 0), aligned_offset);
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE))dill_get_fp(h);
        ((void(*)(UIMM_TYPE))func)(s2ul);
	dill_free_handle(h);

	if (dul != (UIMM_TYPE)s1ul) {
	    printf("Failed test for stuli\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
        void (*func)(UIMM_TYPE, UIMM_TYPE);
	dill_reg	rdp;
	dill_exec_handle h;

	if (verbose) printf(" - stp\n");
	s2ul = (UIMM_TYPE)&dp - aligned_offset;

	/* mem [ reg + reg ] <- reg */
        dill_start_proc(s, "stp", DILL_V, "%ul%ul");
		if(!dill_raw_getreg(s, &rdp, DILL_P, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_setp(s, rdp, s1p);
        	dill_stp(s, rdp, dill_param_reg(s, 0), dill_param_reg(s, 1));
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
        ((void(*)(UIMM_TYPE, UIMM_TYPE))func)(s2ul, aligned_offset);
	dill_free_handle(h);
	if (dp != (void *)s1p) {
	    printf("Failed test for stp\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	/* mem [ reg + reg ] <- reg */
        void (*func)(UIMM_TYPE);
	dill_reg	rdp;
	dill_exec_handle h;

	dp = 0;
	if (verbose) printf(" - stpi\n");
        dill_start_proc(s, "stpi", DILL_V, "%ul");
		if(!dill_raw_getreg(s, &rdp, DILL_P, DILL_TEMP))
			dill_fatal("out of registers!");

		dill_setp(s, rdp, s1p);
        	dill_stpi(s, rdp, dill_param_reg(s, 0), aligned_offset);
	h = dill_finalize(s);
        func = (void(*)(UIMM_TYPE))dill_get_fp(h);
        ((void(*)(UIMM_TYPE))func)(s2ul);
	dill_free_handle(h);

	if (dp != (void *)s1p) {
	    printf("Failed test for stpi\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	char (*func)(UIMM_TYPE, UIMM_TYPE);
	char result;
	dill_exec_handle h;
	dill_reg	rdc;

	/* reg <- mem[reg + reg]  */
	if (verbose) printf(" - ldc\n");
        dill_start_proc(s, "ldc", DILL_C, "%ul%ul");
		if(!dill_raw_getreg(s, &rdc, DILL_C, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ldc(s, rdc, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdc);
	h = dill_finalize(s);
        func = (char(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	result = func((UIMM_TYPE)&dc - aligned_offset, aligned_offset);
	dill_free_handle(h);
	if (dc != result) {
	    printf("Failed test for ldc , expected %d, got %d\n",  dc,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }	
    {
	char (*func)(UIMM_TYPE);
	char result;
	dill_reg	rdc;
	dill_exec_handle h;

	/* reg <- mem[reg + imm] */
	if (verbose) printf(" - ldci\n");
        dill_start_proc(s, "ldci", DILL_C, "%ul");
		if(!dill_raw_getreg(s, &rdc, DILL_C, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ldci(s, rdc, dill_param_reg(s, 0), aligned_offset);
        	dill_reti(s, rdc);
	h = dill_finalize(s);
        func = (char(*)(UIMM_TYPE))dill_get_fp(h);
	result = func((UIMM_TYPE)&dc - aligned_offset);
	dill_free_handle(h);
	if (dc != result) {
	    printf("Failed test for ldc , expected %d, got %d\n",  dc,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned char (*func)(UIMM_TYPE, UIMM_TYPE);
	unsigned char result;
	dill_exec_handle h;
	dill_reg	rduc;

	/* reg <- mem[reg + reg]  */
	if (verbose) printf(" - lduc\n");
        dill_start_proc(s, "lduc", DILL_UC, "%ul%ul");
		if(!dill_raw_getreg(s, &rduc, DILL_UC, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lduc(s, rduc, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rduc);
	h = dill_finalize(s);
        func = (unsigned char(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	result = func((UIMM_TYPE)&duc - aligned_offset, aligned_offset);
	dill_free_handle(h);
	if (duc != result) {
	    printf("Failed test for lduc , expected %u, got %u\n",  duc,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }	
    {
	unsigned char (*func)(UIMM_TYPE);
	unsigned char result;
	dill_reg	rduc;
	dill_exec_handle h;

	/* reg <- mem[reg + imm] */
	if (verbose) printf(" - lduci\n");
        dill_start_proc(s, "lduci", DILL_UC, "%ul");
		if(!dill_raw_getreg(s, &rduc, DILL_UC, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lduci(s, rduc, dill_param_reg(s, 0), aligned_offset);
        	dill_reti(s, rduc);
	h = dill_finalize(s);
        func = (unsigned char(*)(UIMM_TYPE))dill_get_fp(h);
	result = func((UIMM_TYPE)&duc - aligned_offset);
	dill_free_handle(h);
	if (duc != result) {
	    printf("Failed test for lduc , expected %u, got %u\n",  duc,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	short (*func)(UIMM_TYPE, UIMM_TYPE);
	short result;
	dill_exec_handle h;
	dill_reg	rds;

	/* reg <- mem[reg + reg]  */
	if (verbose) printf(" - lds\n");
        dill_start_proc(s, "lds", DILL_S, "%ul%ul");
		if(!dill_raw_getreg(s, &rds, DILL_S, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lds(s, rds, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rds);
	h = dill_finalize(s);
        func = (short(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	result = func((UIMM_TYPE)&ds - aligned_offset, aligned_offset);
	dill_free_handle(h);
	if (ds != result) {
	    printf("Failed test for lds , expected %d, got %d\n",  ds,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }	
    {
	short (*func)(UIMM_TYPE);
	short result;
	dill_reg	rds;
	dill_exec_handle h;

	/* reg <- mem[reg + imm] */
	if (verbose) printf(" - ldsi\n");
        dill_start_proc(s, "ldsi", DILL_S, "%ul");
		if(!dill_raw_getreg(s, &rds, DILL_S, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ldsi(s, rds, dill_param_reg(s, 0), aligned_offset);
        	dill_reti(s, rds);
	h = dill_finalize(s);
        func = (short(*)(UIMM_TYPE))dill_get_fp(h);
	result = func((UIMM_TYPE)&ds - aligned_offset);
	dill_free_handle(h);
	if (ds != result) {
	    printf("Failed test for lds , expected %d, got %d\n",  ds,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned short (*func)(UIMM_TYPE, UIMM_TYPE);
	unsigned short result;
	dill_exec_handle h;
	dill_reg	rdus;

	/* reg <- mem[reg + reg]  */
	if (verbose) printf(" - ldus\n");
        dill_start_proc(s, "ldus", DILL_US, "%ul%ul");
		if(!dill_raw_getreg(s, &rdus, DILL_US, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ldus(s, rdus, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdus);
	h = dill_finalize(s);
        func = (unsigned short(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	result = func((UIMM_TYPE)&dus - aligned_offset, aligned_offset);
	dill_free_handle(h);
	if (dus != result) {
	    printf("Failed test for ldus , expected %u, got %u\n",  dus,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }	
    {
	unsigned short (*func)(UIMM_TYPE);
	unsigned short result;
	dill_reg	rdus;
	dill_exec_handle h;

	/* reg <- mem[reg + imm] */
	if (verbose) printf(" - ldusi\n");
        dill_start_proc(s, "ldusi", DILL_US, "%ul");
		if(!dill_raw_getreg(s, &rdus, DILL_US, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ldusi(s, rdus, dill_param_reg(s, 0), aligned_offset);
        	dill_reti(s, rdus);
	h = dill_finalize(s);
        func = (unsigned short(*)(UIMM_TYPE))dill_get_fp(h);
	result = func((UIMM_TYPE)&dus - aligned_offset);
	dill_free_handle(h);
	if (dus != result) {
	    printf("Failed test for ldus , expected %u, got %u\n",  dus,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int (*func)(UIMM_TYPE, UIMM_TYPE);
	int result;
	dill_exec_handle h;
	dill_reg	rdi;

	/* reg <- mem[reg + reg]  */
	if (verbose) printf(" - ldi\n");
        dill_start_proc(s, "ldi", DILL_I, "%ul%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ldi(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	result = func((UIMM_TYPE)&di - aligned_offset, aligned_offset);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for ldi , expected %d, got %d\n",  di,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }	
    {
	int (*func)(UIMM_TYPE);
	int result;
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- mem[reg + imm] */
	if (verbose) printf(" - ldii\n");
        dill_start_proc(s, "ldii", DILL_I, "%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ldii(s, rdi, dill_param_reg(s, 0), aligned_offset);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(UIMM_TYPE))dill_get_fp(h);
	result = func((UIMM_TYPE)&di - aligned_offset);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for ldi , expected %d, got %d\n",  di,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	unsigned (*func)(UIMM_TYPE, UIMM_TYPE);
	unsigned result;
	dill_exec_handle h;
	dill_reg	rdu;

	/* reg <- mem[reg + reg]  */
	if (verbose) printf(" - ldu\n");
        dill_start_proc(s, "ldu", DILL_U, "%ul%ul");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ldu(s, rdu, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	result = func((UIMM_TYPE)&du - aligned_offset, aligned_offset);
	dill_free_handle(h);
	if (du != result) {
	    printf("Failed test for ldu , expected %u, got %u\n",  du,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }	
    {
	unsigned (*func)(UIMM_TYPE);
	unsigned result;
	dill_reg	rdu;
	dill_exec_handle h;

	/* reg <- mem[reg + imm] */
	if (verbose) printf(" - ldui\n");
        dill_start_proc(s, "ldui", DILL_U, "%ul");
		if(!dill_raw_getreg(s, &rdu, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ldui(s, rdu, dill_param_reg(s, 0), aligned_offset);
        	dill_retu(s, rdu);
	h = dill_finalize(s);
        func = (unsigned(*)(UIMM_TYPE))dill_get_fp(h);
	result = func((UIMM_TYPE)&du - aligned_offset);
	dill_free_handle(h);
	if (du != result) {
	    printf("Failed test for ldu , expected %u, got %u\n",  du,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	UIMM_TYPE (*func)(UIMM_TYPE, UIMM_TYPE);
	UIMM_TYPE result;
	dill_exec_handle h;
	dill_reg	rdul;

	/* reg <- mem[reg + reg]  */
	if (verbose) printf(" - ldul\n");
        dill_start_proc(s, "ldul", DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ldul(s, rdul, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	result = func((UIMM_TYPE)&dul - aligned_offset, aligned_offset);
	dill_free_handle(h);
	if (dul != result) {
	    printf("Failed test for ldul , expected %zx, got %zx\n",  dul,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }	
    {
	UIMM_TYPE (*func)(UIMM_TYPE);
	UIMM_TYPE result;
	dill_reg	rdul;
	dill_exec_handle h;

	/* reg <- mem[reg + imm] */
	if (verbose) printf(" - lduli\n");
        dill_start_proc(s, "lduli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdul, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lduli(s, rdul, dill_param_reg(s, 0), aligned_offset);
        	dill_retul(s, rdul);
	h = dill_finalize(s);
        func = (UIMM_TYPE(*)(UIMM_TYPE))dill_get_fp(h);
	result = func((UIMM_TYPE)&dul - aligned_offset);
	dill_free_handle(h);
	if (dul != result) {
	    printf("Failed test for ldul , expected %zx, got %zx\n",  dul,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	void * (*func)(UIMM_TYPE, UIMM_TYPE);
	void * result;
	dill_exec_handle h;
	dill_reg	rdp;

	/* reg <- mem[reg + reg]  */
	if (verbose) printf(" - ldp\n");
        dill_start_proc(s, "ldp", DILL_P, "%ul%ul");
		if(!dill_raw_getreg(s, &rdp, DILL_P, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ldp(s, rdp, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_retp(s, rdp);
	h = dill_finalize(s);
        func = (void *(*)(UIMM_TYPE, UIMM_TYPE)) dill_get_fp(h);
	result = func((UIMM_TYPE)&dp - aligned_offset, aligned_offset);
	dill_free_handle(h);
	if (dp != result) {
	    printf("Failed test for ldp , expected %p, got %p\n",  dp,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }	
    {
	void * (*func)(UIMM_TYPE);
	void * result;
	dill_reg	rdp;
	dill_exec_handle h;

	/* reg <- mem[reg + imm] */
	if (verbose) printf(" - ldpi\n");
        dill_start_proc(s, "ldpi", DILL_P, "%ul");
		if(!dill_raw_getreg(s, &rdp, DILL_P, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ldpi(s, rdp, dill_param_reg(s, 0), aligned_offset);
        	dill_retp(s, rdp);
	h = dill_finalize(s);
        func = (void *(*)(UIMM_TYPE))dill_get_fp(h);
	result = func((UIMM_TYPE)&dp - aligned_offset);
	dill_free_handle(h);
	if (dp != result) {
	    printf("Failed test for ldp , expected %p, got %p\n",  dp,  result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int (*func)(char,char);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - beqc\n");
	/* reg <- (reg == reg) */
        dill_start_proc(s, "beqc", DILL_C, "%c%c");
		l = dill_alloc_label(s, NULL);
        	dill_beqc(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(char,char)) dill_get_fp(h);
        di = (s1c == s2c);
	result = func(s1c, s2c);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for beqc\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(char);
	int result;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - beqci\n");
        dill_start_proc(s, "beqci", DILL_C, "%c");
		l = dill_alloc_label(s, NULL);
        	dill_beqci(s, dill_param_reg(s, 0),  s2c, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(char)) dill_get_fp(h);
	result = func(s1c);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for beqci\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned char,unsigned char);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bequc\n");
	/* reg <- (reg == reg) */
        dill_start_proc(s, "bequc", DILL_UC, "%uc%uc");
		l = dill_alloc_label(s, NULL);
        	dill_bequc(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned char,unsigned char)) dill_get_fp(h);
        di = (s1uc == s2uc);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bequc\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned char);
	int result;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - bequci\n");
        dill_start_proc(s, "bequci", DILL_UC, "%uc");
		l = dill_alloc_label(s, NULL);
        	dill_bequci(s, dill_param_reg(s, 0),  s2uc, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned char)) dill_get_fp(h);
	result = func(s1uc);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bequci\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(short,short);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - beqs\n");
	/* reg <- (reg == reg) */
        dill_start_proc(s, "beqs", DILL_S, "%s%s");
		l = dill_alloc_label(s, NULL);
        	dill_beqs(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(short,short)) dill_get_fp(h);
        di = (s1s == s2s);
	result = func(s1s, s2s);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for beqs\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(short);
	int result;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - beqsi\n");
        dill_start_proc(s, "beqsi", DILL_S, "%s");
		l = dill_alloc_label(s, NULL);
        	dill_beqsi(s, dill_param_reg(s, 0),  s2s, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(short)) dill_get_fp(h);
	result = func(s1s);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for beqsi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned short,unsigned short);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bequs\n");
	/* reg <- (reg == reg) */
        dill_start_proc(s, "bequs", DILL_US, "%us%us");
		l = dill_alloc_label(s, NULL);
        	dill_bequs(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned short,unsigned short)) dill_get_fp(h);
        di = (s1us == s2us);
	result = func(s1us, s2us);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bequs\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned short);
	int result;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - bequsi\n");
        dill_start_proc(s, "bequsi", DILL_US, "%us");
		l = dill_alloc_label(s, NULL);
        	dill_bequsi(s, dill_param_reg(s, 0),  s2us, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned short)) dill_get_fp(h);
	result = func(s1us);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bequsi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(int,int);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - beqi\n");
	/* reg <- (reg == reg) */
        dill_start_proc(s, "beqi", DILL_I, "%i%i");
		l = dill_alloc_label(s, NULL);
        	dill_beqi(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(int,int)) dill_get_fp(h);
        di = (s1i == s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for beqi\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(int);
	int result;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - beqii\n");
        dill_start_proc(s, "beqii", DILL_I, "%i");
		l = dill_alloc_label(s, NULL);
        	dill_beqii(s, dill_param_reg(s, 0),  s2i, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(int)) dill_get_fp(h);
	result = func(s1i);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for beqii\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned,unsigned);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bequ\n");
	/* reg <- (reg == reg) */
        dill_start_proc(s, "bequ", DILL_U, "%u%u");
		l = dill_alloc_label(s, NULL);
        	dill_bequ(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned,unsigned)) dill_get_fp(h);
        di = (s1u == s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bequ\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned);
	int result;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - bequi\n");
        dill_start_proc(s, "bequi", DILL_U, "%u");
		l = dill_alloc_label(s, NULL);
        	dill_bequi(s, dill_param_reg(s, 0),  s2u, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned)) dill_get_fp(h);
	result = func(s1u);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bequi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bequl\n");
	/* reg <- (reg == reg) */
        dill_start_proc(s, "bequl", DILL_UL, "%ul%ul");
		l = dill_alloc_label(s, NULL);
        	dill_bequl(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        di = (s1ul == s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bequl\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(UIMM_TYPE);
	int result;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - bequli\n");
        dill_start_proc(s, "bequli", DILL_UL, "%ul");
		l = dill_alloc_label(s, NULL);
        	dill_bequli(s, dill_param_reg(s, 0),  s2ul, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bequli\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(IMM_TYPE,IMM_TYPE);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - beql\n");
	/* reg <- (reg == reg) */
        dill_start_proc(s, "beql", DILL_L, "%l%l");
		l = dill_alloc_label(s, NULL);
        	dill_beql(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        di = (s1l == s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for beql\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(IMM_TYPE);
	int result;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - beqli\n");
        dill_start_proc(s, "beqli", DILL_L, "%l");
		l = dill_alloc_label(s, NULL);
        	dill_beqli(s, dill_param_reg(s, 0),  s2l, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(IMM_TYPE)) dill_get_fp(h);
	result = func(s1l);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for beqli\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(void *,void *);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - beqp\n");
	/* reg <- (reg == reg) */
        dill_start_proc(s, "beqp", DILL_P, "%p%p");
		l = dill_alloc_label(s, NULL);
        	dill_beqp(s, dill_param_reg(s, 0), (IMM_TYPE) dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(void *,void *)) dill_get_fp(h);
        di = (s1p == s2p);
	result = func(s1p, s2p);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for beqp\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(void *);
	int result;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - beqpi\n");
        dill_start_proc(s, "beqpi", DILL_P, "%p");
		l = dill_alloc_label(s, NULL);
        	dill_beqpi(s, dill_param_reg(s, 0), (IMM_TYPE) s2p, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(void *)) dill_get_fp(h);
	result = func(s1p);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for beqpi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(char,char);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bnec\n");
	/* reg <- (reg != reg) */
        dill_start_proc(s, "bnec", DILL_C, "%c%c");
		l = dill_alloc_label(s, NULL);
        	dill_bnec(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(char,char)) dill_get_fp(h);
        di = (s1c != s2c);
	result = func(s1c, s2c);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bnec\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(char);
	int result;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - bneci\n");
        dill_start_proc(s, "bneci", DILL_C, "%c");
		l = dill_alloc_label(s, NULL);
        	dill_bneci(s, dill_param_reg(s, 0),  s2c, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(char)) dill_get_fp(h);
	result = func(s1c);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bneci\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned char,unsigned char);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bneuc\n");
	/* reg <- (reg != reg) */
        dill_start_proc(s, "bneuc", DILL_UC, "%uc%uc");
		l = dill_alloc_label(s, NULL);
        	dill_bneuc(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned char,unsigned char)) dill_get_fp(h);
        di = (s1uc != s2uc);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bneuc\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned char);
	int result;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - bneuci\n");
        dill_start_proc(s, "bneuci", DILL_UC, "%uc");
		l = dill_alloc_label(s, NULL);
        	dill_bneuci(s, dill_param_reg(s, 0),  s2uc, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned char)) dill_get_fp(h);
	result = func(s1uc);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bneuci\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(short,short);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bnes\n");
	/* reg <- (reg != reg) */
        dill_start_proc(s, "bnes", DILL_S, "%s%s");
		l = dill_alloc_label(s, NULL);
        	dill_bnes(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(short,short)) dill_get_fp(h);
        di = (s1s != s2s);
	result = func(s1s, s2s);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bnes\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(short);
	int result;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - bnesi\n");
        dill_start_proc(s, "bnesi", DILL_S, "%s");
		l = dill_alloc_label(s, NULL);
        	dill_bnesi(s, dill_param_reg(s, 0),  s2s, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(short)) dill_get_fp(h);
	result = func(s1s);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bnesi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned short,unsigned short);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bneus\n");
	/* reg <- (reg != reg) */
        dill_start_proc(s, "bneus", DILL_US, "%us%us");
		l = dill_alloc_label(s, NULL);
        	dill_bneus(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned short,unsigned short)) dill_get_fp(h);
        di = (s1us != s2us);
	result = func(s1us, s2us);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bneus\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned short);
	int result;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - bneusi\n");
        dill_start_proc(s, "bneusi", DILL_US, "%us");
		l = dill_alloc_label(s, NULL);
        	dill_bneusi(s, dill_param_reg(s, 0),  s2us, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned short)) dill_get_fp(h);
	result = func(s1us);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bneusi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(int,int);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bnei\n");
	/* reg <- (reg != reg) */
        dill_start_proc(s, "bnei", DILL_I, "%i%i");
		l = dill_alloc_label(s, NULL);
        	dill_bnei(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(int,int)) dill_get_fp(h);
        di = (s1i != s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bnei\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(int);
	int result;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - bneii\n");
        dill_start_proc(s, "bneii", DILL_I, "%i");
		l = dill_alloc_label(s, NULL);
        	dill_bneii(s, dill_param_reg(s, 0),  s2i, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(int)) dill_get_fp(h);
	result = func(s1i);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bneii\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned,unsigned);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bneu\n");
	/* reg <- (reg != reg) */
        dill_start_proc(s, "bneu", DILL_U, "%u%u");
		l = dill_alloc_label(s, NULL);
        	dill_bneu(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned,unsigned)) dill_get_fp(h);
        di = (s1u != s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bneu\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned);
	int result;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - bneui\n");
        dill_start_proc(s, "bneui", DILL_U, "%u");
		l = dill_alloc_label(s, NULL);
        	dill_bneui(s, dill_param_reg(s, 0),  s2u, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned)) dill_get_fp(h);
	result = func(s1u);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bneui\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bneul\n");
	/* reg <- (reg != reg) */
        dill_start_proc(s, "bneul", DILL_UL, "%ul%ul");
		l = dill_alloc_label(s, NULL);
        	dill_bneul(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        di = (s1ul != s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bneul\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(UIMM_TYPE);
	int result;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - bneuli\n");
        dill_start_proc(s, "bneuli", DILL_UL, "%ul");
		l = dill_alloc_label(s, NULL);
        	dill_bneuli(s, dill_param_reg(s, 0),  s2ul, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bneuli\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(IMM_TYPE,IMM_TYPE);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bnel\n");
	/* reg <- (reg != reg) */
        dill_start_proc(s, "bnel", DILL_L, "%l%l");
		l = dill_alloc_label(s, NULL);
        	dill_bnel(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        di = (s1l != s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bnel\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(IMM_TYPE);
	int result;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - bneli\n");
        dill_start_proc(s, "bneli", DILL_L, "%l");
		l = dill_alloc_label(s, NULL);
        	dill_bneli(s, dill_param_reg(s, 0),  s2l, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(IMM_TYPE)) dill_get_fp(h);
	result = func(s1l);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bneli\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(void *,void *);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bnep\n");
	/* reg <- (reg != reg) */
        dill_start_proc(s, "bnep", DILL_P, "%p%p");
		l = dill_alloc_label(s, NULL);
        	dill_bnep(s, dill_param_reg(s, 0), (IMM_TYPE) dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(void *,void *)) dill_get_fp(h);
        di = (s1p != s2p);
	result = func(s1p, s2p);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bnep\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(void *);
	int result;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - bnepi\n");
        dill_start_proc(s, "bnepi", DILL_P, "%p");
		l = dill_alloc_label(s, NULL);
        	dill_bnepi(s, dill_param_reg(s, 0), (IMM_TYPE) s2p, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(void *)) dill_get_fp(h);
	result = func(s1p);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bnepi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(char,char);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bltc\n");
	/* reg <- (reg < reg) */
        dill_start_proc(s, "bltc", DILL_C, "%c%c");
		l = dill_alloc_label(s, NULL);
        	dill_bltc(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(char,char)) dill_get_fp(h);
        di = (s1c < s2c);
	result = func(s1c, s2c);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bltc\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(char);
	int result;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - bltci\n");
        dill_start_proc(s, "bltci", DILL_C, "%c");
		l = dill_alloc_label(s, NULL);
        	dill_bltci(s, dill_param_reg(s, 0),  s2c, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(char)) dill_get_fp(h);
	result = func(s1c);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bltci\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned char,unsigned char);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bltuc\n");
	/* reg <- (reg < reg) */
        dill_start_proc(s, "bltuc", DILL_UC, "%uc%uc");
		l = dill_alloc_label(s, NULL);
        	dill_bltuc(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned char,unsigned char)) dill_get_fp(h);
        di = (s1uc < s2uc);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bltuc\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned char);
	int result;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - bltuci\n");
        dill_start_proc(s, "bltuci", DILL_UC, "%uc");
		l = dill_alloc_label(s, NULL);
        	dill_bltuci(s, dill_param_reg(s, 0),  s2uc, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned char)) dill_get_fp(h);
	result = func(s1uc);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bltuci\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(short,short);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - blts\n");
	/* reg <- (reg < reg) */
        dill_start_proc(s, "blts", DILL_S, "%s%s");
		l = dill_alloc_label(s, NULL);
        	dill_blts(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(short,short)) dill_get_fp(h);
        di = (s1s < s2s);
	result = func(s1s, s2s);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for blts\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(short);
	int result;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - bltsi\n");
        dill_start_proc(s, "bltsi", DILL_S, "%s");
		l = dill_alloc_label(s, NULL);
        	dill_bltsi(s, dill_param_reg(s, 0),  s2s, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(short)) dill_get_fp(h);
	result = func(s1s);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bltsi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned short,unsigned short);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bltus\n");
	/* reg <- (reg < reg) */
        dill_start_proc(s, "bltus", DILL_US, "%us%us");
		l = dill_alloc_label(s, NULL);
        	dill_bltus(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned short,unsigned short)) dill_get_fp(h);
        di = (s1us < s2us);
	result = func(s1us, s2us);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bltus\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned short);
	int result;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - bltusi\n");
        dill_start_proc(s, "bltusi", DILL_US, "%us");
		l = dill_alloc_label(s, NULL);
        	dill_bltusi(s, dill_param_reg(s, 0),  s2us, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned short)) dill_get_fp(h);
	result = func(s1us);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bltusi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(int,int);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - blti\n");
	/* reg <- (reg < reg) */
        dill_start_proc(s, "blti", DILL_I, "%i%i");
		l = dill_alloc_label(s, NULL);
        	dill_blti(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(int,int)) dill_get_fp(h);
        di = (s1i < s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for blti\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(int);
	int result;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - bltii\n");
        dill_start_proc(s, "bltii", DILL_I, "%i");
		l = dill_alloc_label(s, NULL);
        	dill_bltii(s, dill_param_reg(s, 0),  s2i, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(int)) dill_get_fp(h);
	result = func(s1i);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bltii\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned,unsigned);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bltu\n");
	/* reg <- (reg < reg) */
        dill_start_proc(s, "bltu", DILL_U, "%u%u");
		l = dill_alloc_label(s, NULL);
        	dill_bltu(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned,unsigned)) dill_get_fp(h);
        di = (s1u < s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bltu\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned);
	int result;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - bltui\n");
        dill_start_proc(s, "bltui", DILL_U, "%u");
		l = dill_alloc_label(s, NULL);
        	dill_bltui(s, dill_param_reg(s, 0),  s2u, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned)) dill_get_fp(h);
	result = func(s1u);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bltui\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bltul\n");
	/* reg <- (reg < reg) */
        dill_start_proc(s, "bltul", DILL_UL, "%ul%ul");
		l = dill_alloc_label(s, NULL);
        	dill_bltul(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        di = (s1ul < s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bltul\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(UIMM_TYPE);
	int result;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - bltuli\n");
        dill_start_proc(s, "bltuli", DILL_UL, "%ul");
		l = dill_alloc_label(s, NULL);
        	dill_bltuli(s, dill_param_reg(s, 0),  s2ul, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bltuli\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(IMM_TYPE,IMM_TYPE);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bltl\n");
	/* reg <- (reg < reg) */
        dill_start_proc(s, "bltl", DILL_L, "%l%l");
		l = dill_alloc_label(s, NULL);
        	dill_bltl(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        di = (s1l < s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bltl\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(IMM_TYPE);
	int result;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - bltli\n");
        dill_start_proc(s, "bltli", DILL_L, "%l");
		l = dill_alloc_label(s, NULL);
        	dill_bltli(s, dill_param_reg(s, 0),  s2l, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(IMM_TYPE)) dill_get_fp(h);
	result = func(s1l);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bltli\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(void *,void *);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bltp\n");
	/* reg <- (reg < reg) */
        dill_start_proc(s, "bltp", DILL_P, "%p%p");
		l = dill_alloc_label(s, NULL);
        	dill_bltp(s, dill_param_reg(s, 0), (IMM_TYPE) dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(void *,void *)) dill_get_fp(h);
        di = (s1p < s2p);
	result = func(s1p, s2p);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bltp\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(void *);
	int result;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - bltpi\n");
        dill_start_proc(s, "bltpi", DILL_P, "%p");
		l = dill_alloc_label(s, NULL);
        	dill_bltpi(s, dill_param_reg(s, 0), (IMM_TYPE) s2p, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(void *)) dill_get_fp(h);
	result = func(s1p);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bltpi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(char,char);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - blec\n");
	/* reg <- (reg <= reg) */
        dill_start_proc(s, "blec", DILL_C, "%c%c");
		l = dill_alloc_label(s, NULL);
        	dill_blec(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(char,char)) dill_get_fp(h);
        di = (s1c <= s2c);
	result = func(s1c, s2c);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for blec\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(char);
	int result;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - bleci\n");
        dill_start_proc(s, "bleci", DILL_C, "%c");
		l = dill_alloc_label(s, NULL);
        	dill_bleci(s, dill_param_reg(s, 0),  s2c, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(char)) dill_get_fp(h);
	result = func(s1c);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bleci\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned char,unsigned char);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bleuc\n");
	/* reg <- (reg <= reg) */
        dill_start_proc(s, "bleuc", DILL_UC, "%uc%uc");
		l = dill_alloc_label(s, NULL);
        	dill_bleuc(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned char,unsigned char)) dill_get_fp(h);
        di = (s1uc <= s2uc);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bleuc\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned char);
	int result;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - bleuci\n");
        dill_start_proc(s, "bleuci", DILL_UC, "%uc");
		l = dill_alloc_label(s, NULL);
        	dill_bleuci(s, dill_param_reg(s, 0),  s2uc, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned char)) dill_get_fp(h);
	result = func(s1uc);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bleuci\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(short,short);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bles\n");
	/* reg <- (reg <= reg) */
        dill_start_proc(s, "bles", DILL_S, "%s%s");
		l = dill_alloc_label(s, NULL);
        	dill_bles(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(short,short)) dill_get_fp(h);
        di = (s1s <= s2s);
	result = func(s1s, s2s);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bles\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(short);
	int result;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - blesi\n");
        dill_start_proc(s, "blesi", DILL_S, "%s");
		l = dill_alloc_label(s, NULL);
        	dill_blesi(s, dill_param_reg(s, 0),  s2s, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(short)) dill_get_fp(h);
	result = func(s1s);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for blesi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned short,unsigned short);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bleus\n");
	/* reg <- (reg <= reg) */
        dill_start_proc(s, "bleus", DILL_US, "%us%us");
		l = dill_alloc_label(s, NULL);
        	dill_bleus(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned short,unsigned short)) dill_get_fp(h);
        di = (s1us <= s2us);
	result = func(s1us, s2us);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bleus\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned short);
	int result;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - bleusi\n");
        dill_start_proc(s, "bleusi", DILL_US, "%us");
		l = dill_alloc_label(s, NULL);
        	dill_bleusi(s, dill_param_reg(s, 0),  s2us, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned short)) dill_get_fp(h);
	result = func(s1us);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bleusi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(int,int);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - blei\n");
	/* reg <- (reg <= reg) */
        dill_start_proc(s, "blei", DILL_I, "%i%i");
		l = dill_alloc_label(s, NULL);
        	dill_blei(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(int,int)) dill_get_fp(h);
        di = (s1i <= s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for blei\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(int);
	int result;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - bleii\n");
        dill_start_proc(s, "bleii", DILL_I, "%i");
		l = dill_alloc_label(s, NULL);
        	dill_bleii(s, dill_param_reg(s, 0),  s2i, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(int)) dill_get_fp(h);
	result = func(s1i);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bleii\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned,unsigned);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bleu\n");
	/* reg <- (reg <= reg) */
        dill_start_proc(s, "bleu", DILL_U, "%u%u");
		l = dill_alloc_label(s, NULL);
        	dill_bleu(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned,unsigned)) dill_get_fp(h);
        di = (s1u <= s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bleu\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned);
	int result;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - bleui\n");
        dill_start_proc(s, "bleui", DILL_U, "%u");
		l = dill_alloc_label(s, NULL);
        	dill_bleui(s, dill_param_reg(s, 0),  s2u, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned)) dill_get_fp(h);
	result = func(s1u);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bleui\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bleul\n");
	/* reg <- (reg <= reg) */
        dill_start_proc(s, "bleul", DILL_UL, "%ul%ul");
		l = dill_alloc_label(s, NULL);
        	dill_bleul(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        di = (s1ul <= s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bleul\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(UIMM_TYPE);
	int result;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - bleuli\n");
        dill_start_proc(s, "bleuli", DILL_UL, "%ul");
		l = dill_alloc_label(s, NULL);
        	dill_bleuli(s, dill_param_reg(s, 0),  s2ul, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bleuli\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(IMM_TYPE,IMM_TYPE);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - blel\n");
	/* reg <- (reg <= reg) */
        dill_start_proc(s, "blel", DILL_L, "%l%l");
		l = dill_alloc_label(s, NULL);
        	dill_blel(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        di = (s1l <= s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for blel\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(IMM_TYPE);
	int result;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - bleli\n");
        dill_start_proc(s, "bleli", DILL_L, "%l");
		l = dill_alloc_label(s, NULL);
        	dill_bleli(s, dill_param_reg(s, 0),  s2l, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(IMM_TYPE)) dill_get_fp(h);
	result = func(s1l);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bleli\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(void *,void *);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - blep\n");
	/* reg <- (reg <= reg) */
        dill_start_proc(s, "blep", DILL_P, "%p%p");
		l = dill_alloc_label(s, NULL);
        	dill_blep(s, dill_param_reg(s, 0), (IMM_TYPE) dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(void *,void *)) dill_get_fp(h);
        di = (s1p <= s2p);
	result = func(s1p, s2p);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for blep\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(void *);
	int result;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - blepi\n");
        dill_start_proc(s, "blepi", DILL_P, "%p");
		l = dill_alloc_label(s, NULL);
        	dill_blepi(s, dill_param_reg(s, 0), (IMM_TYPE) s2p, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(void *)) dill_get_fp(h);
	result = func(s1p);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for blepi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(char,char);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgtc\n");
	/* reg <- (reg > reg) */
        dill_start_proc(s, "bgtc", DILL_C, "%c%c");
		l = dill_alloc_label(s, NULL);
        	dill_bgtc(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(char,char)) dill_get_fp(h);
        di = (s1c > s2c);
	result = func(s1c, s2c);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgtc\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(char);
	int result;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - bgtci\n");
        dill_start_proc(s, "bgtci", DILL_C, "%c");
		l = dill_alloc_label(s, NULL);
        	dill_bgtci(s, dill_param_reg(s, 0),  s2c, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(char)) dill_get_fp(h);
	result = func(s1c);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgtci\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned char,unsigned char);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgtuc\n");
	/* reg <- (reg > reg) */
        dill_start_proc(s, "bgtuc", DILL_UC, "%uc%uc");
		l = dill_alloc_label(s, NULL);
        	dill_bgtuc(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned char,unsigned char)) dill_get_fp(h);
        di = (s1uc > s2uc);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgtuc\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned char);
	int result;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - bgtuci\n");
        dill_start_proc(s, "bgtuci", DILL_UC, "%uc");
		l = dill_alloc_label(s, NULL);
        	dill_bgtuci(s, dill_param_reg(s, 0),  s2uc, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned char)) dill_get_fp(h);
	result = func(s1uc);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgtuci\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(short,short);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgts\n");
	/* reg <- (reg > reg) */
        dill_start_proc(s, "bgts", DILL_S, "%s%s");
		l = dill_alloc_label(s, NULL);
        	dill_bgts(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(short,short)) dill_get_fp(h);
        di = (s1s > s2s);
	result = func(s1s, s2s);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgts\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(short);
	int result;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - bgtsi\n");
        dill_start_proc(s, "bgtsi", DILL_S, "%s");
		l = dill_alloc_label(s, NULL);
        	dill_bgtsi(s, dill_param_reg(s, 0),  s2s, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(short)) dill_get_fp(h);
	result = func(s1s);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgtsi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned short,unsigned short);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgtus\n");
	/* reg <- (reg > reg) */
        dill_start_proc(s, "bgtus", DILL_US, "%us%us");
		l = dill_alloc_label(s, NULL);
        	dill_bgtus(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned short,unsigned short)) dill_get_fp(h);
        di = (s1us > s2us);
	result = func(s1us, s2us);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgtus\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned short);
	int result;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - bgtusi\n");
        dill_start_proc(s, "bgtusi", DILL_US, "%us");
		l = dill_alloc_label(s, NULL);
        	dill_bgtusi(s, dill_param_reg(s, 0),  s2us, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned short)) dill_get_fp(h);
	result = func(s1us);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgtusi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(int,int);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgti\n");
	/* reg <- (reg > reg) */
        dill_start_proc(s, "bgti", DILL_I, "%i%i");
		l = dill_alloc_label(s, NULL);
        	dill_bgti(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(int,int)) dill_get_fp(h);
        di = (s1i > s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgti\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(int);
	int result;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - bgtii\n");
        dill_start_proc(s, "bgtii", DILL_I, "%i");
		l = dill_alloc_label(s, NULL);
        	dill_bgtii(s, dill_param_reg(s, 0),  s2i, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(int)) dill_get_fp(h);
	result = func(s1i);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgtii\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned,unsigned);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgtu\n");
	/* reg <- (reg > reg) */
        dill_start_proc(s, "bgtu", DILL_U, "%u%u");
		l = dill_alloc_label(s, NULL);
        	dill_bgtu(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned,unsigned)) dill_get_fp(h);
        di = (s1u > s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgtu\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned);
	int result;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - bgtui\n");
        dill_start_proc(s, "bgtui", DILL_U, "%u");
		l = dill_alloc_label(s, NULL);
        	dill_bgtui(s, dill_param_reg(s, 0),  s2u, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned)) dill_get_fp(h);
	result = func(s1u);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgtui\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgtul\n");
	/* reg <- (reg > reg) */
        dill_start_proc(s, "bgtul", DILL_UL, "%ul%ul");
		l = dill_alloc_label(s, NULL);
        	dill_bgtul(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        di = (s1ul > s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgtul\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(UIMM_TYPE);
	int result;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - bgtuli\n");
        dill_start_proc(s, "bgtuli", DILL_UL, "%ul");
		l = dill_alloc_label(s, NULL);
        	dill_bgtuli(s, dill_param_reg(s, 0),  s2ul, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgtuli\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(IMM_TYPE,IMM_TYPE);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgtl\n");
	/* reg <- (reg > reg) */
        dill_start_proc(s, "bgtl", DILL_L, "%l%l");
		l = dill_alloc_label(s, NULL);
        	dill_bgtl(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        di = (s1l > s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgtl\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(IMM_TYPE);
	int result;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - bgtli\n");
        dill_start_proc(s, "bgtli", DILL_L, "%l");
		l = dill_alloc_label(s, NULL);
        	dill_bgtli(s, dill_param_reg(s, 0),  s2l, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(IMM_TYPE)) dill_get_fp(h);
	result = func(s1l);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgtli\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(void *,void *);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgtp\n");
	/* reg <- (reg > reg) */
        dill_start_proc(s, "bgtp", DILL_P, "%p%p");
		l = dill_alloc_label(s, NULL);
        	dill_bgtp(s, dill_param_reg(s, 0), (IMM_TYPE) dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(void *,void *)) dill_get_fp(h);
        di = (s1p > s2p);
	result = func(s1p, s2p);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgtp\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(void *);
	int result;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - bgtpi\n");
        dill_start_proc(s, "bgtpi", DILL_P, "%p");
		l = dill_alloc_label(s, NULL);
        	dill_bgtpi(s, dill_param_reg(s, 0), (IMM_TYPE) s2p, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(void *)) dill_get_fp(h);
	result = func(s1p);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgtpi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(char,char);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgec\n");
	/* reg <- (reg >= reg) */
        dill_start_proc(s, "bgec", DILL_C, "%c%c");
		l = dill_alloc_label(s, NULL);
        	dill_bgec(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(char,char)) dill_get_fp(h);
        di = (s1c >= s2c);
	result = func(s1c, s2c);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgec\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(char);
	int result;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - bgeci\n");
        dill_start_proc(s, "bgeci", DILL_C, "%c");
		l = dill_alloc_label(s, NULL);
        	dill_bgeci(s, dill_param_reg(s, 0),  s2c, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(char)) dill_get_fp(h);
	result = func(s1c);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgeci\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned char,unsigned char);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgeuc\n");
	/* reg <- (reg >= reg) */
        dill_start_proc(s, "bgeuc", DILL_UC, "%uc%uc");
		l = dill_alloc_label(s, NULL);
        	dill_bgeuc(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned char,unsigned char)) dill_get_fp(h);
        di = (s1uc >= s2uc);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgeuc\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned char);
	int result;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - bgeuci\n");
        dill_start_proc(s, "bgeuci", DILL_UC, "%uc");
		l = dill_alloc_label(s, NULL);
        	dill_bgeuci(s, dill_param_reg(s, 0),  s2uc, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned char)) dill_get_fp(h);
	result = func(s1uc);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgeuci\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(short,short);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bges\n");
	/* reg <- (reg >= reg) */
        dill_start_proc(s, "bges", DILL_S, "%s%s");
		l = dill_alloc_label(s, NULL);
        	dill_bges(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(short,short)) dill_get_fp(h);
        di = (s1s >= s2s);
	result = func(s1s, s2s);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bges\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(short);
	int result;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - bgesi\n");
        dill_start_proc(s, "bgesi", DILL_S, "%s");
		l = dill_alloc_label(s, NULL);
        	dill_bgesi(s, dill_param_reg(s, 0),  s2s, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(short)) dill_get_fp(h);
	result = func(s1s);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgesi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned short,unsigned short);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgeus\n");
	/* reg <- (reg >= reg) */
        dill_start_proc(s, "bgeus", DILL_US, "%us%us");
		l = dill_alloc_label(s, NULL);
        	dill_bgeus(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned short,unsigned short)) dill_get_fp(h);
        di = (s1us >= s2us);
	result = func(s1us, s2us);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgeus\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned short);
	int result;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - bgeusi\n");
        dill_start_proc(s, "bgeusi", DILL_US, "%us");
		l = dill_alloc_label(s, NULL);
        	dill_bgeusi(s, dill_param_reg(s, 0),  s2us, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned short)) dill_get_fp(h);
	result = func(s1us);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgeusi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(int,int);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgei\n");
	/* reg <- (reg >= reg) */
        dill_start_proc(s, "bgei", DILL_I, "%i%i");
		l = dill_alloc_label(s, NULL);
        	dill_bgei(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(int,int)) dill_get_fp(h);
        di = (s1i >= s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgei\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(int);
	int result;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - bgeii\n");
        dill_start_proc(s, "bgeii", DILL_I, "%i");
		l = dill_alloc_label(s, NULL);
        	dill_bgeii(s, dill_param_reg(s, 0),  s2i, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(int)) dill_get_fp(h);
	result = func(s1i);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgeii\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(unsigned,unsigned);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgeu\n");
	/* reg <- (reg >= reg) */
        dill_start_proc(s, "bgeu", DILL_U, "%u%u");
		l = dill_alloc_label(s, NULL);
        	dill_bgeu(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned,unsigned)) dill_get_fp(h);
        di = (s1u >= s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgeu\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(unsigned);
	int result;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - bgeui\n");
        dill_start_proc(s, "bgeui", DILL_U, "%u");
		l = dill_alloc_label(s, NULL);
        	dill_bgeui(s, dill_param_reg(s, 0),  s2u, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(unsigned)) dill_get_fp(h);
	result = func(s1u);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgeui\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgeul\n");
	/* reg <- (reg >= reg) */
        dill_start_proc(s, "bgeul", DILL_UL, "%ul%ul");
		l = dill_alloc_label(s, NULL);
        	dill_bgeul(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        di = (s1ul >= s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgeul\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(UIMM_TYPE);
	int result;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - bgeuli\n");
        dill_start_proc(s, "bgeuli", DILL_UL, "%ul");
		l = dill_alloc_label(s, NULL);
        	dill_bgeuli(s, dill_param_reg(s, 0),  s2ul, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgeuli\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(IMM_TYPE,IMM_TYPE);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgel\n");
	/* reg <- (reg >= reg) */
        dill_start_proc(s, "bgel", DILL_L, "%l%l");
		l = dill_alloc_label(s, NULL);
        	dill_bgel(s, dill_param_reg(s, 0),  dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        di = (s1l >= s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgel\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(IMM_TYPE);
	int result;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - bgeli\n");
        dill_start_proc(s, "bgeli", DILL_L, "%l");
		l = dill_alloc_label(s, NULL);
        	dill_bgeli(s, dill_param_reg(s, 0),  s2l, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(IMM_TYPE)) dill_get_fp(h);
	result = func(s1l);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgeli\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)(void *,void *);
	int result;
	dill_exec_handle h;

	if (verbose) printf(" - bgep\n");
	/* reg <- (reg >= reg) */
        dill_start_proc(s, "bgep", DILL_P, "%p%p");
		l = dill_alloc_label(s, NULL);
        	dill_bgep(s, dill_param_reg(s, 0), (IMM_TYPE) dill_param_reg(s, 1), l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(void *,void *)) dill_get_fp(h);
        di = (s1p >= s2p);
	result = func(s1p, s2p);
	dill_free_handle(h);
	if (di != result) {
	    printf("Failed test for bgep\n");
	    dill_errors++;
	    dill_dump(s);
	}
    }
    {
	int (*func)(void *);
	int result;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - bgepi\n");
        dill_start_proc(s, "bgepi", DILL_P, "%p");
		l = dill_alloc_label(s, NULL);
        	dill_bgepi(s, dill_param_reg(s, 0), (IMM_TYPE) s2p, l);
        		dill_retii(s, 0);
		dill_mark_label(s, l);
			dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int (*)(void *)) dill_get_fp(h);
	result = func(s1p);
	dill_free_handle(h);
	if (di  != result) {
	    printf("Failed test for bgepi\n");
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int expected_result;
	int result;
	int (*func)(char,char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == reg) */
	if (verbose) printf(" - eqc\n");
        dill_start_proc(s, "eqc",  DILL_C, "%c%c");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_eqc(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(char,char)) dill_get_fp(h);
	result = func(s1c, s2c);
	dill_free_handle(h);
        expected_result = (s1c == s2c);
	if (expected_result != result) {
	    printf("Failed test for %d eqc %d, expected %d, got %d\n", s1c, s2c, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(char,char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - eqci\n");
        dill_start_proc(s, "eqci", DILL_C, "%c");
		if(!dill_raw_getreg(s, &rdi, DILL_C, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_eqci(s, rdi, dill_param_reg(s, 0), s2c);
        	dill_retc(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(char,char)) dill_get_fp(h);
        expected_result = (s1c == s2c);
	result = func(s1c, s2c);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d eqci (imm %d), expected %d, got %d\n", s1c, s2c, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned char,unsigned char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == reg) */
	if (verbose) printf(" - equc\n");
        dill_start_proc(s, "equc",  DILL_UC, "%uc%uc");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_equc(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned char,unsigned char)) dill_get_fp(h);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
        expected_result = (s1uc == s2uc);
	if (expected_result != result) {
	    printf("Failed test for %u equc %u, expected %d, got %d\n", s1uc, s2uc, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned char,unsigned char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - equci\n");
        dill_start_proc(s, "equci", DILL_UC, "%uc");
		if(!dill_raw_getreg(s, &rdi, DILL_UC, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_equci(s, rdi, dill_param_reg(s, 0), s2uc);
        	dill_retuc(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned char,unsigned char)) dill_get_fp(h);
        expected_result = (s1uc == s2uc);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u equci (imm %u), expected %d, got %d\n", s1uc, s2uc, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(short,short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == reg) */
	if (verbose) printf(" - eqs\n");
        dill_start_proc(s, "eqs",  DILL_S, "%s%s");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_eqs(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(short,short)) dill_get_fp(h);
	result = func(s1s, s2s);
	dill_free_handle(h);
        expected_result = (s1s == s2s);
	if (expected_result != result) {
	    printf("Failed test for %d eqs %d, expected %d, got %d\n", s1s, s2s, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(short,short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - eqsi\n");
        dill_start_proc(s, "eqsi", DILL_S, "%s");
		if(!dill_raw_getreg(s, &rdi, DILL_S, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_eqsi(s, rdi, dill_param_reg(s, 0), s2s);
        	dill_rets(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(short,short)) dill_get_fp(h);
        expected_result = (s1s == s2s);
	result = func(s1s, s2s);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d eqsi (imm %d), expected %d, got %d\n", s1s, s2s, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned short,unsigned short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == reg) */
	if (verbose) printf(" - equs\n");
        dill_start_proc(s, "equs",  DILL_US, "%us%us");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_equs(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned short,unsigned short)) dill_get_fp(h);
	result = func(s1us, s2us);
	dill_free_handle(h);
        expected_result = (s1us == s2us);
	if (expected_result != result) {
	    printf("Failed test for %u equs %u, expected %d, got %d\n", s1us, s2us, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned short,unsigned short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - equsi\n");
        dill_start_proc(s, "equsi", DILL_US, "%us");
		if(!dill_raw_getreg(s, &rdi, DILL_US, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_equsi(s, rdi, dill_param_reg(s, 0), s2us);
        	dill_retus(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned short,unsigned short)) dill_get_fp(h);
        expected_result = (s1us == s2us);
	result = func(s1us, s2us);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u equsi (imm %u), expected %d, got %d\n", s1us, s2us, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == reg) */
	if (verbose) printf(" - eqi\n");
        dill_start_proc(s, "eqi",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_eqi(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, s2i);
	dill_free_handle(h);
        expected_result = (s1i == s2i);
	if (expected_result != result) {
	    printf("Failed test for %d eqi %d, expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - eqii\n");
        dill_start_proc(s, "eqii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_eqii(s, rdi, dill_param_reg(s, 0), s2i);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i == s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d eqii (imm %d), expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned,unsigned);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == reg) */
	if (verbose) printf(" - equ\n");
        dill_start_proc(s, "equ",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_equ(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, s2u);
	dill_free_handle(h);
        expected_result = (s1u == s2u);
	if (expected_result != result) {
	    printf("Failed test for %u equ %u, expected %d, got %d\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned,unsigned);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - equi\n");
        dill_start_proc(s, "equi", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdi, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_equi(s, rdi, dill_param_reg(s, 0), s2u);
        	dill_retu(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u == s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u equi (imm %u), expected %d, got %d\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == reg) */
	if (verbose) printf(" - equl\n");
        dill_start_proc(s, "equl",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_equl(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
        expected_result = (s1ul == s2ul);
	if (expected_result != result) {
	    printf("Failed test for %zx equl %zx, expected %d, got %d\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - equli\n");
        dill_start_proc(s, "equli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_equli(s, rdi, dill_param_reg(s, 0), s2ul);
        	dill_retul(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul == s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx equli (imm %zx), expected %d, got %d\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == reg) */
	if (verbose) printf(" - eql\n");
        dill_start_proc(s, "eql",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_eql(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, s2l);
	dill_free_handle(h);
        expected_result = (s1l == s2l);
	if (expected_result != result) {
	    printf("Failed test for %zx eql %zx, expected %d, got %d\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == imm) */
	if (verbose) printf(" - eqli\n");
        dill_start_proc(s, "eqli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdi, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_eqli(s, rdi, dill_param_reg(s, 0), s2l);
        	dill_retl(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l == s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx eqli (imm %zx), expected %d, got %d\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(void *,void *);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg == reg) */
	if (verbose) printf(" - eqp\n");
        dill_start_proc(s, "eqp",  DILL_P, "%p%p");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_eqp(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(void *,void *)) dill_get_fp(h);
	result = func(s1p, s2p);
	dill_free_handle(h);
        expected_result = (s1p == s2p);
	if (expected_result != result) {
	    printf("Failed test for %p eqp %p, expected %d, got %d\n", s1p, s2p, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int expected_result;
	int result;
	int (*func)(char,char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != reg) */
	if (verbose) printf(" - nec\n");
        dill_start_proc(s, "nec",  DILL_C, "%c%c");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_nec(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(char,char)) dill_get_fp(h);
	result = func(s1c, s2c);
	dill_free_handle(h);
        expected_result = (s1c != s2c);
	if (expected_result != result) {
	    printf("Failed test for %d nec %d, expected %d, got %d\n", s1c, s2c, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(char,char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - neci\n");
        dill_start_proc(s, "neci", DILL_C, "%c");
		if(!dill_raw_getreg(s, &rdi, DILL_C, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_neci(s, rdi, dill_param_reg(s, 0), s2c);
        	dill_retc(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(char,char)) dill_get_fp(h);
        expected_result = (s1c != s2c);
	result = func(s1c, s2c);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d neci (imm %d), expected %d, got %d\n", s1c, s2c, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned char,unsigned char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != reg) */
	if (verbose) printf(" - neuc\n");
        dill_start_proc(s, "neuc",  DILL_UC, "%uc%uc");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_neuc(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned char,unsigned char)) dill_get_fp(h);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
        expected_result = (s1uc != s2uc);
	if (expected_result != result) {
	    printf("Failed test for %u neuc %u, expected %d, got %d\n", s1uc, s2uc, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned char,unsigned char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - neuci\n");
        dill_start_proc(s, "neuci", DILL_UC, "%uc");
		if(!dill_raw_getreg(s, &rdi, DILL_UC, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_neuci(s, rdi, dill_param_reg(s, 0), s2uc);
        	dill_retuc(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned char,unsigned char)) dill_get_fp(h);
        expected_result = (s1uc != s2uc);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u neuci (imm %u), expected %d, got %d\n", s1uc, s2uc, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(short,short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != reg) */
	if (verbose) printf(" - nes\n");
        dill_start_proc(s, "nes",  DILL_S, "%s%s");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_nes(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(short,short)) dill_get_fp(h);
	result = func(s1s, s2s);
	dill_free_handle(h);
        expected_result = (s1s != s2s);
	if (expected_result != result) {
	    printf("Failed test for %d nes %d, expected %d, got %d\n", s1s, s2s, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(short,short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - nesi\n");
        dill_start_proc(s, "nesi", DILL_S, "%s");
		if(!dill_raw_getreg(s, &rdi, DILL_S, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_nesi(s, rdi, dill_param_reg(s, 0), s2s);
        	dill_rets(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(short,short)) dill_get_fp(h);
        expected_result = (s1s != s2s);
	result = func(s1s, s2s);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d nesi (imm %d), expected %d, got %d\n", s1s, s2s, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned short,unsigned short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != reg) */
	if (verbose) printf(" - neus\n");
        dill_start_proc(s, "neus",  DILL_US, "%us%us");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_neus(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned short,unsigned short)) dill_get_fp(h);
	result = func(s1us, s2us);
	dill_free_handle(h);
        expected_result = (s1us != s2us);
	if (expected_result != result) {
	    printf("Failed test for %u neus %u, expected %d, got %d\n", s1us, s2us, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned short,unsigned short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - neusi\n");
        dill_start_proc(s, "neusi", DILL_US, "%us");
		if(!dill_raw_getreg(s, &rdi, DILL_US, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_neusi(s, rdi, dill_param_reg(s, 0), s2us);
        	dill_retus(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned short,unsigned short)) dill_get_fp(h);
        expected_result = (s1us != s2us);
	result = func(s1us, s2us);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u neusi (imm %u), expected %d, got %d\n", s1us, s2us, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != reg) */
	if (verbose) printf(" - nei\n");
        dill_start_proc(s, "nei",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_nei(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, s2i);
	dill_free_handle(h);
        expected_result = (s1i != s2i);
	if (expected_result != result) {
	    printf("Failed test for %d nei %d, expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - neii\n");
        dill_start_proc(s, "neii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_neii(s, rdi, dill_param_reg(s, 0), s2i);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i != s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d neii (imm %d), expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned,unsigned);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != reg) */
	if (verbose) printf(" - neu\n");
        dill_start_proc(s, "neu",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_neu(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, s2u);
	dill_free_handle(h);
        expected_result = (s1u != s2u);
	if (expected_result != result) {
	    printf("Failed test for %u neu %u, expected %d, got %d\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned,unsigned);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - neui\n");
        dill_start_proc(s, "neui", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdi, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_neui(s, rdi, dill_param_reg(s, 0), s2u);
        	dill_retu(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u != s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u neui (imm %u), expected %d, got %d\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != reg) */
	if (verbose) printf(" - neul\n");
        dill_start_proc(s, "neul",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_neul(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
        expected_result = (s1ul != s2ul);
	if (expected_result != result) {
	    printf("Failed test for %zx neul %zx, expected %d, got %d\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - neuli\n");
        dill_start_proc(s, "neuli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_neuli(s, rdi, dill_param_reg(s, 0), s2ul);
        	dill_retul(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul != s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx neuli (imm %zx), expected %d, got %d\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != reg) */
	if (verbose) printf(" - nel\n");
        dill_start_proc(s, "nel",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_nel(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, s2l);
	dill_free_handle(h);
        expected_result = (s1l != s2l);
	if (expected_result != result) {
	    printf("Failed test for %zx nel %zx, expected %d, got %d\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != imm) */
	if (verbose) printf(" - neli\n");
        dill_start_proc(s, "neli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdi, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_neli(s, rdi, dill_param_reg(s, 0), s2l);
        	dill_retl(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l != s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx neli (imm %zx), expected %d, got %d\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(void *,void *);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg != reg) */
	if (verbose) printf(" - nep\n");
        dill_start_proc(s, "nep",  DILL_P, "%p%p");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_nep(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(void *,void *)) dill_get_fp(h);
	result = func(s1p, s2p);
	dill_free_handle(h);
        expected_result = (s1p != s2p);
	if (expected_result != result) {
	    printf("Failed test for %p nep %p, expected %d, got %d\n", s1p, s2p, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int expected_result;
	int result;
	int (*func)(char,char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < reg) */
	if (verbose) printf(" - ltc\n");
        dill_start_proc(s, "ltc",  DILL_C, "%c%c");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ltc(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(char,char)) dill_get_fp(h);
	result = func(s1c, s2c);
	dill_free_handle(h);
        expected_result = (s1c < s2c);
	if (expected_result != result) {
	    printf("Failed test for %d ltc %d, expected %d, got %d\n", s1c, s2c, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(char,char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - ltci\n");
        dill_start_proc(s, "ltci", DILL_C, "%c");
		if(!dill_raw_getreg(s, &rdi, DILL_C, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ltci(s, rdi, dill_param_reg(s, 0), s2c);
        	dill_retc(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(char,char)) dill_get_fp(h);
        expected_result = (s1c < s2c);
	result = func(s1c, s2c);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d ltci (imm %d), expected %d, got %d\n", s1c, s2c, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned char,unsigned char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < reg) */
	if (verbose) printf(" - ltuc\n");
        dill_start_proc(s, "ltuc",  DILL_UC, "%uc%uc");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ltuc(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned char,unsigned char)) dill_get_fp(h);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
        expected_result = (s1uc < s2uc);
	if (expected_result != result) {
	    printf("Failed test for %u ltuc %u, expected %d, got %d\n", s1uc, s2uc, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned char,unsigned char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - ltuci\n");
        dill_start_proc(s, "ltuci", DILL_UC, "%uc");
		if(!dill_raw_getreg(s, &rdi, DILL_UC, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ltuci(s, rdi, dill_param_reg(s, 0), s2uc);
        	dill_retuc(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned char,unsigned char)) dill_get_fp(h);
        expected_result = (s1uc < s2uc);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u ltuci (imm %u), expected %d, got %d\n", s1uc, s2uc, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(short,short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < reg) */
	if (verbose) printf(" - lts\n");
        dill_start_proc(s, "lts",  DILL_S, "%s%s");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lts(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(short,short)) dill_get_fp(h);
	result = func(s1s, s2s);
	dill_free_handle(h);
        expected_result = (s1s < s2s);
	if (expected_result != result) {
	    printf("Failed test for %d lts %d, expected %d, got %d\n", s1s, s2s, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(short,short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - ltsi\n");
        dill_start_proc(s, "ltsi", DILL_S, "%s");
		if(!dill_raw_getreg(s, &rdi, DILL_S, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ltsi(s, rdi, dill_param_reg(s, 0), s2s);
        	dill_rets(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(short,short)) dill_get_fp(h);
        expected_result = (s1s < s2s);
	result = func(s1s, s2s);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d ltsi (imm %d), expected %d, got %d\n", s1s, s2s, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned short,unsigned short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < reg) */
	if (verbose) printf(" - ltus\n");
        dill_start_proc(s, "ltus",  DILL_US, "%us%us");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ltus(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned short,unsigned short)) dill_get_fp(h);
	result = func(s1us, s2us);
	dill_free_handle(h);
        expected_result = (s1us < s2us);
	if (expected_result != result) {
	    printf("Failed test for %u ltus %u, expected %d, got %d\n", s1us, s2us, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned short,unsigned short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - ltusi\n");
        dill_start_proc(s, "ltusi", DILL_US, "%us");
		if(!dill_raw_getreg(s, &rdi, DILL_US, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ltusi(s, rdi, dill_param_reg(s, 0), s2us);
        	dill_retus(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned short,unsigned short)) dill_get_fp(h);
        expected_result = (s1us < s2us);
	result = func(s1us, s2us);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u ltusi (imm %u), expected %d, got %d\n", s1us, s2us, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < reg) */
	if (verbose) printf(" - lti\n");
        dill_start_proc(s, "lti",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lti(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, s2i);
	dill_free_handle(h);
        expected_result = (s1i < s2i);
	if (expected_result != result) {
	    printf("Failed test for %d lti %d, expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - ltii\n");
        dill_start_proc(s, "ltii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ltii(s, rdi, dill_param_reg(s, 0), s2i);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i < s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d ltii (imm %d), expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned,unsigned);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < reg) */
	if (verbose) printf(" - ltu\n");
        dill_start_proc(s, "ltu",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ltu(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, s2u);
	dill_free_handle(h);
        expected_result = (s1u < s2u);
	if (expected_result != result) {
	    printf("Failed test for %u ltu %u, expected %d, got %d\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned,unsigned);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - ltui\n");
        dill_start_proc(s, "ltui", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdi, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ltui(s, rdi, dill_param_reg(s, 0), s2u);
        	dill_retu(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u < s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u ltui (imm %u), expected %d, got %d\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < reg) */
	if (verbose) printf(" - ltul\n");
        dill_start_proc(s, "ltul",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ltul(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
        expected_result = (s1ul < s2ul);
	if (expected_result != result) {
	    printf("Failed test for %zx ltul %zx, expected %d, got %d\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - ltuli\n");
        dill_start_proc(s, "ltuli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ltuli(s, rdi, dill_param_reg(s, 0), s2ul);
        	dill_retul(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul < s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx ltuli (imm %zx), expected %d, got %d\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < reg) */
	if (verbose) printf(" - ltl\n");
        dill_start_proc(s, "ltl",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ltl(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, s2l);
	dill_free_handle(h);
        expected_result = (s1l < s2l);
	if (expected_result != result) {
	    printf("Failed test for %zx ltl %zx, expected %d, got %d\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < imm) */
	if (verbose) printf(" - ltli\n");
        dill_start_proc(s, "ltli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdi, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ltli(s, rdi, dill_param_reg(s, 0), s2l);
        	dill_retl(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l < s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx ltli (imm %zx), expected %d, got %d\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(void *,void *);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg < reg) */
	if (verbose) printf(" - ltp\n");
        dill_start_proc(s, "ltp",  DILL_P, "%p%p");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ltp(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(void *,void *)) dill_get_fp(h);
	result = func(s1p, s2p);
	dill_free_handle(h);
        expected_result = (s1p < s2p);
	if (expected_result != result) {
	    printf("Failed test for %p ltp %p, expected %d, got %d\n", s1p, s2p, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int expected_result;
	int result;
	int (*func)(char,char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= reg) */
	if (verbose) printf(" - lec\n");
        dill_start_proc(s, "lec",  DILL_C, "%c%c");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lec(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(char,char)) dill_get_fp(h);
	result = func(s1c, s2c);
	dill_free_handle(h);
        expected_result = (s1c <= s2c);
	if (expected_result != result) {
	    printf("Failed test for %d lec %d, expected %d, got %d\n", s1c, s2c, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(char,char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - leci\n");
        dill_start_proc(s, "leci", DILL_C, "%c");
		if(!dill_raw_getreg(s, &rdi, DILL_C, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_leci(s, rdi, dill_param_reg(s, 0), s2c);
        	dill_retc(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(char,char)) dill_get_fp(h);
        expected_result = (s1c <= s2c);
	result = func(s1c, s2c);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d leci (imm %d), expected %d, got %d\n", s1c, s2c, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned char,unsigned char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= reg) */
	if (verbose) printf(" - leuc\n");
        dill_start_proc(s, "leuc",  DILL_UC, "%uc%uc");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_leuc(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned char,unsigned char)) dill_get_fp(h);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
        expected_result = (s1uc <= s2uc);
	if (expected_result != result) {
	    printf("Failed test for %u leuc %u, expected %d, got %d\n", s1uc, s2uc, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned char,unsigned char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - leuci\n");
        dill_start_proc(s, "leuci", DILL_UC, "%uc");
		if(!dill_raw_getreg(s, &rdi, DILL_UC, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_leuci(s, rdi, dill_param_reg(s, 0), s2uc);
        	dill_retuc(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned char,unsigned char)) dill_get_fp(h);
        expected_result = (s1uc <= s2uc);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u leuci (imm %u), expected %d, got %d\n", s1uc, s2uc, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(short,short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= reg) */
	if (verbose) printf(" - les\n");
        dill_start_proc(s, "les",  DILL_S, "%s%s");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_les(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(short,short)) dill_get_fp(h);
	result = func(s1s, s2s);
	dill_free_handle(h);
        expected_result = (s1s <= s2s);
	if (expected_result != result) {
	    printf("Failed test for %d les %d, expected %d, got %d\n", s1s, s2s, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(short,short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - lesi\n");
        dill_start_proc(s, "lesi", DILL_S, "%s");
		if(!dill_raw_getreg(s, &rdi, DILL_S, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lesi(s, rdi, dill_param_reg(s, 0), s2s);
        	dill_rets(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(short,short)) dill_get_fp(h);
        expected_result = (s1s <= s2s);
	result = func(s1s, s2s);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d lesi (imm %d), expected %d, got %d\n", s1s, s2s, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned short,unsigned short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= reg) */
	if (verbose) printf(" - leus\n");
        dill_start_proc(s, "leus",  DILL_US, "%us%us");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_leus(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned short,unsigned short)) dill_get_fp(h);
	result = func(s1us, s2us);
	dill_free_handle(h);
        expected_result = (s1us <= s2us);
	if (expected_result != result) {
	    printf("Failed test for %u leus %u, expected %d, got %d\n", s1us, s2us, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned short,unsigned short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - leusi\n");
        dill_start_proc(s, "leusi", DILL_US, "%us");
		if(!dill_raw_getreg(s, &rdi, DILL_US, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_leusi(s, rdi, dill_param_reg(s, 0), s2us);
        	dill_retus(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned short,unsigned short)) dill_get_fp(h);
        expected_result = (s1us <= s2us);
	result = func(s1us, s2us);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u leusi (imm %u), expected %d, got %d\n", s1us, s2us, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= reg) */
	if (verbose) printf(" - lei\n");
        dill_start_proc(s, "lei",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lei(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, s2i);
	dill_free_handle(h);
        expected_result = (s1i <= s2i);
	if (expected_result != result) {
	    printf("Failed test for %d lei %d, expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - leii\n");
        dill_start_proc(s, "leii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_leii(s, rdi, dill_param_reg(s, 0), s2i);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i <= s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d leii (imm %d), expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned,unsigned);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= reg) */
	if (verbose) printf(" - leu\n");
        dill_start_proc(s, "leu",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_leu(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, s2u);
	dill_free_handle(h);
        expected_result = (s1u <= s2u);
	if (expected_result != result) {
	    printf("Failed test for %u leu %u, expected %d, got %d\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned,unsigned);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - leui\n");
        dill_start_proc(s, "leui", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdi, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_leui(s, rdi, dill_param_reg(s, 0), s2u);
        	dill_retu(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u <= s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u leui (imm %u), expected %d, got %d\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= reg) */
	if (verbose) printf(" - leul\n");
        dill_start_proc(s, "leul",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_leul(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
        expected_result = (s1ul <= s2ul);
	if (expected_result != result) {
	    printf("Failed test for %zx leul %zx, expected %d, got %d\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - leuli\n");
        dill_start_proc(s, "leuli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_leuli(s, rdi, dill_param_reg(s, 0), s2ul);
        	dill_retul(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul <= s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx leuli (imm %zx), expected %d, got %d\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= reg) */
	if (verbose) printf(" - lel\n");
        dill_start_proc(s, "lel",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lel(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, s2l);
	dill_free_handle(h);
        expected_result = (s1l <= s2l);
	if (expected_result != result) {
	    printf("Failed test for %zx lel %zx, expected %d, got %d\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= imm) */
	if (verbose) printf(" - leli\n");
        dill_start_proc(s, "leli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdi, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_leli(s, rdi, dill_param_reg(s, 0), s2l);
        	dill_retl(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l <= s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx leli (imm %zx), expected %d, got %d\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(void *,void *);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg <= reg) */
	if (verbose) printf(" - lep\n");
        dill_start_proc(s, "lep",  DILL_P, "%p%p");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_lep(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(void *,void *)) dill_get_fp(h);
	result = func(s1p, s2p);
	dill_free_handle(h);
        expected_result = (s1p <= s2p);
	if (expected_result != result) {
	    printf("Failed test for %p lep %p, expected %d, got %d\n", s1p, s2p, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int expected_result;
	int result;
	int (*func)(char,char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > reg) */
	if (verbose) printf(" - gtc\n");
        dill_start_proc(s, "gtc",  DILL_C, "%c%c");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gtc(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(char,char)) dill_get_fp(h);
	result = func(s1c, s2c);
	dill_free_handle(h);
        expected_result = (s1c > s2c);
	if (expected_result != result) {
	    printf("Failed test for %d gtc %d, expected %d, got %d\n", s1c, s2c, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(char,char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - gtci\n");
        dill_start_proc(s, "gtci", DILL_C, "%c");
		if(!dill_raw_getreg(s, &rdi, DILL_C, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gtci(s, rdi, dill_param_reg(s, 0), s2c);
        	dill_retc(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(char,char)) dill_get_fp(h);
        expected_result = (s1c > s2c);
	result = func(s1c, s2c);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d gtci (imm %d), expected %d, got %d\n", s1c, s2c, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned char,unsigned char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > reg) */
	if (verbose) printf(" - gtuc\n");
        dill_start_proc(s, "gtuc",  DILL_UC, "%uc%uc");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gtuc(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned char,unsigned char)) dill_get_fp(h);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
        expected_result = (s1uc > s2uc);
	if (expected_result != result) {
	    printf("Failed test for %u gtuc %u, expected %d, got %d\n", s1uc, s2uc, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned char,unsigned char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - gtuci\n");
        dill_start_proc(s, "gtuci", DILL_UC, "%uc");
		if(!dill_raw_getreg(s, &rdi, DILL_UC, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gtuci(s, rdi, dill_param_reg(s, 0), s2uc);
        	dill_retuc(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned char,unsigned char)) dill_get_fp(h);
        expected_result = (s1uc > s2uc);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u gtuci (imm %u), expected %d, got %d\n", s1uc, s2uc, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(short,short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > reg) */
	if (verbose) printf(" - gts\n");
        dill_start_proc(s, "gts",  DILL_S, "%s%s");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gts(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(short,short)) dill_get_fp(h);
	result = func(s1s, s2s);
	dill_free_handle(h);
        expected_result = (s1s > s2s);
	if (expected_result != result) {
	    printf("Failed test for %d gts %d, expected %d, got %d\n", s1s, s2s, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(short,short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - gtsi\n");
        dill_start_proc(s, "gtsi", DILL_S, "%s");
		if(!dill_raw_getreg(s, &rdi, DILL_S, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gtsi(s, rdi, dill_param_reg(s, 0), s2s);
        	dill_rets(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(short,short)) dill_get_fp(h);
        expected_result = (s1s > s2s);
	result = func(s1s, s2s);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d gtsi (imm %d), expected %d, got %d\n", s1s, s2s, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned short,unsigned short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > reg) */
	if (verbose) printf(" - gtus\n");
        dill_start_proc(s, "gtus",  DILL_US, "%us%us");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gtus(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned short,unsigned short)) dill_get_fp(h);
	result = func(s1us, s2us);
	dill_free_handle(h);
        expected_result = (s1us > s2us);
	if (expected_result != result) {
	    printf("Failed test for %u gtus %u, expected %d, got %d\n", s1us, s2us, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned short,unsigned short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - gtusi\n");
        dill_start_proc(s, "gtusi", DILL_US, "%us");
		if(!dill_raw_getreg(s, &rdi, DILL_US, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gtusi(s, rdi, dill_param_reg(s, 0), s2us);
        	dill_retus(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned short,unsigned short)) dill_get_fp(h);
        expected_result = (s1us > s2us);
	result = func(s1us, s2us);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u gtusi (imm %u), expected %d, got %d\n", s1us, s2us, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > reg) */
	if (verbose) printf(" - gti\n");
        dill_start_proc(s, "gti",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gti(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, s2i);
	dill_free_handle(h);
        expected_result = (s1i > s2i);
	if (expected_result != result) {
	    printf("Failed test for %d gti %d, expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - gtii\n");
        dill_start_proc(s, "gtii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gtii(s, rdi, dill_param_reg(s, 0), s2i);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i > s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d gtii (imm %d), expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned,unsigned);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > reg) */
	if (verbose) printf(" - gtu\n");
        dill_start_proc(s, "gtu",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gtu(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, s2u);
	dill_free_handle(h);
        expected_result = (s1u > s2u);
	if (expected_result != result) {
	    printf("Failed test for %u gtu %u, expected %d, got %d\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned,unsigned);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - gtui\n");
        dill_start_proc(s, "gtui", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdi, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gtui(s, rdi, dill_param_reg(s, 0), s2u);
        	dill_retu(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u > s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u gtui (imm %u), expected %d, got %d\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > reg) */
	if (verbose) printf(" - gtul\n");
        dill_start_proc(s, "gtul",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gtul(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
        expected_result = (s1ul > s2ul);
	if (expected_result != result) {
	    printf("Failed test for %zx gtul %zx, expected %d, got %d\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - gtuli\n");
        dill_start_proc(s, "gtuli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gtuli(s, rdi, dill_param_reg(s, 0), s2ul);
        	dill_retul(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul > s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx gtuli (imm %zx), expected %d, got %d\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > reg) */
	if (verbose) printf(" - gtl\n");
        dill_start_proc(s, "gtl",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gtl(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, s2l);
	dill_free_handle(h);
        expected_result = (s1l > s2l);
	if (expected_result != result) {
	    printf("Failed test for %zx gtl %zx, expected %d, got %d\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > imm) */
	if (verbose) printf(" - gtli\n");
        dill_start_proc(s, "gtli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdi, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gtli(s, rdi, dill_param_reg(s, 0), s2l);
        	dill_retl(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l > s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx gtli (imm %zx), expected %d, got %d\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(void *,void *);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg > reg) */
	if (verbose) printf(" - gtp\n");
        dill_start_proc(s, "gtp",  DILL_P, "%p%p");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gtp(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(void *,void *)) dill_get_fp(h);
	result = func(s1p, s2p);
	dill_free_handle(h);
        expected_result = (s1p > s2p);
	if (expected_result != result) {
	    printf("Failed test for %p gtp %p, expected %d, got %d\n", s1p, s2p, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int expected_result;
	int result;
	int (*func)(char,char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= reg) */
	if (verbose) printf(" - gec\n");
        dill_start_proc(s, "gec",  DILL_C, "%c%c");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gec(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(char,char)) dill_get_fp(h);
	result = func(s1c, s2c);
	dill_free_handle(h);
        expected_result = (s1c >= s2c);
	if (expected_result != result) {
	    printf("Failed test for %d gec %d, expected %d, got %d\n", s1c, s2c, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(char,char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - geci\n");
        dill_start_proc(s, "geci", DILL_C, "%c");
		if(!dill_raw_getreg(s, &rdi, DILL_C, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_geci(s, rdi, dill_param_reg(s, 0), s2c);
        	dill_retc(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(char,char)) dill_get_fp(h);
        expected_result = (s1c >= s2c);
	result = func(s1c, s2c);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d geci (imm %d), expected %d, got %d\n", s1c, s2c, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned char,unsigned char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= reg) */
	if (verbose) printf(" - geuc\n");
        dill_start_proc(s, "geuc",  DILL_UC, "%uc%uc");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_geuc(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned char,unsigned char)) dill_get_fp(h);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
        expected_result = (s1uc >= s2uc);
	if (expected_result != result) {
	    printf("Failed test for %u geuc %u, expected %d, got %d\n", s1uc, s2uc, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned char,unsigned char);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - geuci\n");
        dill_start_proc(s, "geuci", DILL_UC, "%uc");
		if(!dill_raw_getreg(s, &rdi, DILL_UC, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_geuci(s, rdi, dill_param_reg(s, 0), s2uc);
        	dill_retuc(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned char,unsigned char)) dill_get_fp(h);
        expected_result = (s1uc >= s2uc);
	result = func(s1uc, s2uc);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u geuci (imm %u), expected %d, got %d\n", s1uc, s2uc, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(short,short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= reg) */
	if (verbose) printf(" - ges\n");
        dill_start_proc(s, "ges",  DILL_S, "%s%s");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_ges(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(short,short)) dill_get_fp(h);
	result = func(s1s, s2s);
	dill_free_handle(h);
        expected_result = (s1s >= s2s);
	if (expected_result != result) {
	    printf("Failed test for %d ges %d, expected %d, got %d\n", s1s, s2s, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(short,short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - gesi\n");
        dill_start_proc(s, "gesi", DILL_S, "%s");
		if(!dill_raw_getreg(s, &rdi, DILL_S, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gesi(s, rdi, dill_param_reg(s, 0), s2s);
        	dill_rets(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(short,short)) dill_get_fp(h);
        expected_result = (s1s >= s2s);
	result = func(s1s, s2s);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d gesi (imm %d), expected %d, got %d\n", s1s, s2s, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned short,unsigned short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= reg) */
	if (verbose) printf(" - geus\n");
        dill_start_proc(s, "geus",  DILL_US, "%us%us");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_geus(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned short,unsigned short)) dill_get_fp(h);
	result = func(s1us, s2us);
	dill_free_handle(h);
        expected_result = (s1us >= s2us);
	if (expected_result != result) {
	    printf("Failed test for %u geus %u, expected %d, got %d\n", s1us, s2us, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned short,unsigned short);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - geusi\n");
        dill_start_proc(s, "geusi", DILL_US, "%us");
		if(!dill_raw_getreg(s, &rdi, DILL_US, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_geusi(s, rdi, dill_param_reg(s, 0), s2us);
        	dill_retus(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned short,unsigned short)) dill_get_fp(h);
        expected_result = (s1us >= s2us);
	result = func(s1us, s2us);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u geusi (imm %u), expected %d, got %d\n", s1us, s2us, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= reg) */
	if (verbose) printf(" - gei\n");
        dill_start_proc(s, "gei",  DILL_I, "%i%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gei(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
	result = func(s1i, s2i);
	dill_free_handle(h);
        expected_result = (s1i >= s2i);
	if (expected_result != result) {
	    printf("Failed test for %d gei %d, expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(int,int);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - geii\n");
        dill_start_proc(s, "geii", DILL_I, "%i");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_geii(s, rdi, dill_param_reg(s, 0), s2i);
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(int,int)) dill_get_fp(h);
        expected_result = (s1i >= s2i);
	result = func(s1i, s2i);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %d geii (imm %d), expected %d, got %d\n", s1i, s2i, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned,unsigned);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= reg) */
	if (verbose) printf(" - geu\n");
        dill_start_proc(s, "geu",  DILL_U, "%u%u");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_geu(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned,unsigned)) dill_get_fp(h);
	result = func(s1u, s2u);
	dill_free_handle(h);
        expected_result = (s1u >= s2u);
	if (expected_result != result) {
	    printf("Failed test for %u geu %u, expected %d, got %d\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(unsigned,unsigned);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - geui\n");
        dill_start_proc(s, "geui", DILL_U, "%u");
		if(!dill_raw_getreg(s, &rdi, DILL_U, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_geui(s, rdi, dill_param_reg(s, 0), s2u);
        	dill_retu(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(unsigned,unsigned)) dill_get_fp(h);
        expected_result = (s1u >= s2u);
	result = func(s1u, s2u);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %u geui (imm %u), expected %d, got %d\n", s1u, s2u, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= reg) */
	if (verbose) printf(" - geul\n");
        dill_start_proc(s, "geul",  DILL_UL, "%ul%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_geul(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
        expected_result = (s1ul >= s2ul);
	if (expected_result != result) {
	    printf("Failed test for %zx geul %zx, expected %d, got %d\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(UIMM_TYPE,UIMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - geuli\n");
        dill_start_proc(s, "geuli", DILL_UL, "%ul");
		if(!dill_raw_getreg(s, &rdi, DILL_UL, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_geuli(s, rdi, dill_param_reg(s, 0), s2ul);
        	dill_retul(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(UIMM_TYPE,UIMM_TYPE)) dill_get_fp(h);
        expected_result = (s1ul >= s2ul);
	result = func(s1ul, s2ul);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx geuli (imm %zx), expected %d, got %d\n", s1ul, s2ul, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= reg) */
	if (verbose) printf(" - gel\n");
        dill_start_proc(s, "gel",  DILL_L, "%l%l");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gel(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
	result = func(s1l, s2l);
	dill_free_handle(h);
        expected_result = (s1l >= s2l);
	if (expected_result != result) {
	    printf("Failed test for %zx gel %zx, expected %d, got %d\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }

    {
	int expected_result;
	int result;
	int (*func)(IMM_TYPE,IMM_TYPE);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= imm) */
	if (verbose) printf(" - geli\n");
        dill_start_proc(s, "geli", DILL_L, "%l");
		if(!dill_raw_getreg(s, &rdi, DILL_L, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_geli(s, rdi, dill_param_reg(s, 0), s2l);
        	dill_retl(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(IMM_TYPE,IMM_TYPE)) dill_get_fp(h);
        expected_result = (s1l >= s2l);
	result = func(s1l, s2l);
	dill_free_handle(h);
	if (expected_result != result) {
	    printf("Failed test for %zx geli (imm %zx), expected %d, got %d\n", s1l, s2l, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}
    }

    {
	int expected_result;
	int result;
	int (*func)(void *,void *);
	dill_reg	rdi;
	dill_exec_handle h;

	/* reg <- (reg >= reg) */
	if (verbose) printf(" - gep\n");
        dill_start_proc(s, "gep",  DILL_P, "%p%p");
		if(!dill_raw_getreg(s, &rdi, DILL_I, DILL_TEMP))
			dill_fatal("out of registers!");

        	dill_gep(s, rdi, dill_param_reg(s, 0), dill_param_reg(s, 1));
        	dill_reti(s, rdi);
	h = dill_finalize(s);
        func = (int(*)(void *,void *)) dill_get_fp(h);
	result = func(s1p, s2p);
	dill_free_handle(h);
        expected_result = (s1p >= s2p);
	if (expected_result != result) {
	    printf("Failed test for %p gep %p, expected %d, got %d\n", s1p, s2p, expected_result, result);
	    dill_errors++;
	    dill_dump(s);
	}

    }
    {
	int (*func)();
	dill_reg rdp;
	dill_exec_handle h;

	/* ret reg */
	if (verbose) printf(" - dill_jv\n");
        dill_start_simple_proc(s, "dill_jv", DILL_I);
		l = dill_alloc_label(s, NULL);
		dill_jv(s, l);
			dill_retii(s, 0);
		dill_mark_label(s, l);
        		dill_retii(s, 1);
	h = dill_finalize(s);
        func = (int(*)()) dill_get_fp(h);
	if (func() != 1) {
	    printf("Failed test for dill_jv\n");
	    dill_errors++;
	    dill_dump(s);
	}
	dill_free_handle(h);

	/* ret imm */
	if (verbose) printf(" - dill_jp\n");
        dill_start_simple_proc(s, "dill_jp", DILL_I);
	{
		dill_reg zero;
		if(!dill_raw_getreg(s, &rdp, DILL_P, DILL_TEMP))
			dill_fatal("out of registers!");
		if(!dill_raw_getreg(s, &zero, DILL_P, DILL_TEMP))
			dill_fatal("out of registers!");

        		dill_retii(s, 1);
	}
	h = dill_finalize(s);
        func = (int(*)()) dill_get_fp(h);
	if (func() != 1) {
	    printf("Failed test for dill_jp\n");
	    dill_errors++;
	    dill_dump(s);
	}
	dill_free_handle(h);
    }


    loop_count++;
    if(!dill_errors && (loop_count < iters)) goto loop;

    if(!dill_errors) {
	printf("No errors!\n");
	return 0;
    }
    dill_free_stream(s);

    printf("*** %d Errors! on loop %d ****\n", dill_errors, loop_count -1);
    printf("s1i %d, s2i %d, s1u %x, s2u %x\n", s1i,s2i,s1u,s2u);
    printf("s1ul %zu, s2ul %zu, s1l %zx, s2l %zx\n", 
	   s1ul,s2ul,s1l,s2l);
    printf("s1f = %f, s2f = %f, s1d = %f, s2d = %f\n",
	   s1f,s2f,s1d,s2d);
    printf(" aligned offset = %d, unaligned offset %d\n", 
	   aligned_offset, unaligned_offset);
    printf("shifti = %d, shiftu = %d, shiftl = %d, shiftul = %d\n",
	   shifti, shiftu, shiftl, shiftul);	
    return 1;
}

struct regset {
    int reg1;
    int reg2;
};

static int
get_reg_pair(dill_stream s, int type1, int *reg1p, int type2, int *reg2p,
	     int regpairid)
{
#ifdef HOST_X86_64
typedef void *arg_info_list;  /* just to get us through the #include below */
#include "../x86_64.h"
    struct regset intregs[] = {
	{-1, EBX},
	{EBX, EAX},
	{EAX, EBX},
	{EBX, R12},
	{R13, EBX}};
    struct regset floatregs[] = {
	{-1, XMM1},
	{XMM1, XMM5},
	{XMM2, XMM5},
	{XMM1, XMM10},
	{XMM10, XMM1}};

    x86_64_mach_info *tmp = NULL;
    (void)tmp;

    if (regpairid >= (sizeof(intregs) / sizeof(struct regset)))  return 0;
    if ((type1 == DILL_F) || (type1 == DILL_D)) {
    	*reg1p = floatregs[regpairid].reg1;
    } else {
    	*reg1p = intregs[regpairid].reg1;
    }
    if (*reg1p != -1) dill_markused(s, type1, *reg1p);
    if ((type2 == DILL_F) || (type2 == DILL_D)) {
    	*reg2p = floatregs[regpairid].reg2;
    } else {
    	*reg2p = intregs[regpairid].reg2;
    }
    if (*reg2p != -1) dill_markused(s, type2, *reg2p);
    return 1;
#else
    if (regpairid != 0) return 0;
    *reg2p = -1;	/* use param reg */
    if(!dill_raw_getreg(s, reg1p, type1, DILL_TEMP))
	dill_fatal("out of registers!");
    return 1;
#endif
}

