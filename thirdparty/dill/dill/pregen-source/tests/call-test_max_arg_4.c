int dill_errors;
#include <math.h>
#include "dill.h"
#include "stdio.h"
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[]) {
    dill_reg	reg;
    int 	     	s1i, s2i;
    unsigned     	s1u, s2u;
    unsigned long   s1ul, s2ul;
    long     	s1l, s2l;
    float		s1f, s2f;
    double		s1d, s2d;
    int		i;
    int verbose = 0;

    for (i=1; i < argc; i++) {
	if (strcmp(argv[i], "-v") == 0) {
	    verbose++;
	}
    }

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

    s1f = (float)rand() / rand();
    s2f = (float)rand() / (float) ((rand()+1) * ((rand()%1) ? 1. : -1.));

    s1d = (double)rand() / rand();
    s2d = (double)rand() / (rand()+1) * ((rand()%1) ? 1. : -1.);

    {	/* test for 1 of type i */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param0;
	dill_exec_handle h, h2;
	int (*proc)(int);
	int (*proc2)(int);
	int result;
	if (verbose) {
	    printf("test for 1 of type i ...");
	}
	dill_start_proc(s, "param1_i", DILL_I, "%i");
	param0 = dill_param_reg(s, 0);
	dill_reti(s, param0);
	h = dill_finalize(s);
	proc = (int (*)(int)) dill_get_fp(h);
	result = proc(s2i);
	if (result != s2i) {
	    printf("test for 1 arguments of type \"i\" failed, expected %d, got %d\n",
		   s2i, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call1_i", DILL_I, "%i");
	reg = dill_scalli(s2, (void*)proc, "proc_1_i", "%i", dill_param_reg(s2, 0));
	dill_reti(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (int (*) (int)) dill_get_fp(h2);
	result = proc2(s2i);
	if (result != s2i) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 1,
		   "i");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 2 of type i */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param1;
	dill_exec_handle h, h2;
	int (*proc)(int, int);
	int (*proc2)(int, int);
	int result;
	if (verbose) {
	    printf("test for 2 of type i ...");
	}
	dill_start_proc(s, "param2_i", DILL_I, "%i%i");
	param1 = dill_param_reg(s, 1);
	dill_reti(s, param1);
	h = dill_finalize(s);
	proc = (int (*)(int, int)) dill_get_fp(h);
	result = proc(s1i, s2i);
	if (result != s2i) {
	    printf("test for 2 arguments of type \"i\" failed, expected %d, got %d\n",
		   s2i, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call2_i", DILL_I, "%i%i");
	reg = dill_scalli(s2, (void*)proc, "proc_2_i", "%i%i", dill_param_reg(s2, 0), dill_param_reg(s2, 1));
	dill_reti(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (int (*) (int, int)) dill_get_fp(h2);
	result = proc2(s1i, s2i);
	if (result != s2i) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 2,
		   "i");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 3 of type i */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param2;
	dill_exec_handle h, h2;
	int (*proc)(int, int, int);
	int (*proc2)(int, int, int);
	int result;
	if (verbose) {
	    printf("test for 3 of type i ...");
	}
	dill_start_proc(s, "param3_i", DILL_I, "%i%i%i");
	param2 = dill_param_reg(s, 2);
	dill_reti(s, param2);
	h = dill_finalize(s);
	proc = (int (*)(int, int, int)) dill_get_fp(h);
	result = proc(s1i, s1i, s2i);
	if (result != s2i) {
	    printf("test for 3 arguments of type \"i\" failed, expected %d, got %d\n",
		   s2i, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call3_i", DILL_I, "%i%i%i");
	reg = dill_scalli(s2, (void*)proc, "proc_3_i", "%i%i%i", dill_param_reg(s2, 0), dill_param_reg(s2, 1), dill_param_reg(s2, 2));
	dill_reti(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (int (*) (int, int, int)) dill_get_fp(h2);
	result = proc2(s1i, s1i, s2i);
	if (result != s2i) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 3,
		   "i");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 4 of type i */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param3;
	dill_exec_handle h, h2;
	int (*proc)(int, int, int, int);
	int (*proc2)(int, int, int, int);
	int result;
	if (verbose) {
	    printf("test for 4 of type i ...");
	}
	dill_start_proc(s, "param4_i", DILL_I, "%i%i%i%i");
	param3 = dill_param_reg(s, 3);
	dill_reti(s, param3);
	h = dill_finalize(s);
	proc = (int (*)(int, int, int, int)) dill_get_fp(h);
	result = proc(s1i, s1i, s1i, s2i);
	if (result != s2i) {
	    printf("test for 4 arguments of type \"i\" failed, expected %d, got %d\n",
		   s2i, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call4_i", DILL_I, "%i%i%i%i");
	reg = dill_scalli(s2, (void*)proc, "proc_4_i", "%i%i%i%i", dill_param_reg(s2, 0), dill_param_reg(s2, 1), dill_param_reg(s2, 2), dill_param_reg(s2, 3));
	dill_reti(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (int (*) (int, int, int, int)) dill_get_fp(h2);
	result = proc2(s1i, s1i, s1i, s2i);
	if (result != s2i) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 4,
		   "i");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 1 of type u */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param0;
	dill_exec_handle h, h2;
	unsigned (*proc)(unsigned);
	unsigned (*proc2)(unsigned);
	unsigned result;
	if (verbose) {
	    printf("test for 1 of type u ...");
	}
	dill_start_proc(s, "param1_u", DILL_U, "%u");
	param0 = dill_param_reg(s, 0);
	dill_retu(s, param0);
	h = dill_finalize(s);
	proc = (unsigned (*)(unsigned)) dill_get_fp(h);
	result = proc(s2u);
	if (result != s2u) {
	    printf("test for 1 arguments of type \"u\" failed, expected %u, got %u\n",
		   s2u, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call1_u", DILL_U, "%u");
	reg = dill_scallu(s2, (void*)proc, "proc_1_u", "%u", dill_param_reg(s2, 0));
	dill_retu(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (unsigned (*) (unsigned)) dill_get_fp(h2);
	result = proc2(s2u);
	if (result != s2u) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 1,
		   "u");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 2 of type u */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param1;
	dill_exec_handle h, h2;
	unsigned (*proc)(unsigned, unsigned);
	unsigned (*proc2)(unsigned, unsigned);
	unsigned result;
	if (verbose) {
	    printf("test for 2 of type u ...");
	}
	dill_start_proc(s, "param2_u", DILL_U, "%u%u");
	param1 = dill_param_reg(s, 1);
	dill_retu(s, param1);
	h = dill_finalize(s);
	proc = (unsigned (*)(unsigned, unsigned)) dill_get_fp(h);
	result = proc(s1u, s2u);
	if (result != s2u) {
	    printf("test for 2 arguments of type \"u\" failed, expected %u, got %u\n",
		   s2u, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call2_u", DILL_U, "%u%u");
	reg = dill_scallu(s2, (void*)proc, "proc_2_u", "%u%u", dill_param_reg(s2, 0), dill_param_reg(s2, 1));
	dill_retu(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (unsigned (*) (unsigned, unsigned)) dill_get_fp(h2);
	result = proc2(s1u, s2u);
	if (result != s2u) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 2,
		   "u");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 3 of type u */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param2;
	dill_exec_handle h, h2;
	unsigned (*proc)(unsigned, unsigned, unsigned);
	unsigned (*proc2)(unsigned, unsigned, unsigned);
	unsigned result;
	if (verbose) {
	    printf("test for 3 of type u ...");
	}
	dill_start_proc(s, "param3_u", DILL_U, "%u%u%u");
	param2 = dill_param_reg(s, 2);
	dill_retu(s, param2);
	h = dill_finalize(s);
	proc = (unsigned (*)(unsigned, unsigned, unsigned)) dill_get_fp(h);
	result = proc(s1u, s1u, s2u);
	if (result != s2u) {
	    printf("test for 3 arguments of type \"u\" failed, expected %u, got %u\n",
		   s2u, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call3_u", DILL_U, "%u%u%u");
	reg = dill_scallu(s2, (void*)proc, "proc_3_u", "%u%u%u", dill_param_reg(s2, 0), dill_param_reg(s2, 1), dill_param_reg(s2, 2));
	dill_retu(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (unsigned (*) (unsigned, unsigned, unsigned)) dill_get_fp(h2);
	result = proc2(s1u, s1u, s2u);
	if (result != s2u) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 3,
		   "u");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 4 of type u */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param3;
	dill_exec_handle h, h2;
	unsigned (*proc)(unsigned, unsigned, unsigned, unsigned);
	unsigned (*proc2)(unsigned, unsigned, unsigned, unsigned);
	unsigned result;
	if (verbose) {
	    printf("test for 4 of type u ...");
	}
	dill_start_proc(s, "param4_u", DILL_U, "%u%u%u%u");
	param3 = dill_param_reg(s, 3);
	dill_retu(s, param3);
	h = dill_finalize(s);
	proc = (unsigned (*)(unsigned, unsigned, unsigned, unsigned)) dill_get_fp(h);
	result = proc(s1u, s1u, s1u, s2u);
	if (result != s2u) {
	    printf("test for 4 arguments of type \"u\" failed, expected %u, got %u\n",
		   s2u, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call4_u", DILL_U, "%u%u%u%u");
	reg = dill_scallu(s2, (void*)proc, "proc_4_u", "%u%u%u%u", dill_param_reg(s2, 0), dill_param_reg(s2, 1), dill_param_reg(s2, 2), dill_param_reg(s2, 3));
	dill_retu(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (unsigned (*) (unsigned, unsigned, unsigned, unsigned)) dill_get_fp(h2);
	result = proc2(s1u, s1u, s1u, s2u);
	if (result != s2u) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 4,
		   "u");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 1 of type l */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param0;
	dill_exec_handle h, h2;
	long (*proc)(long);
	long (*proc2)(long);
	long result;
	if (verbose) {
	    printf("test for 1 of type l ...");
	}
	dill_start_proc(s, "param1_l", DILL_L, "%l");
	param0 = dill_param_reg(s, 0);
	dill_retl(s, param0);
	h = dill_finalize(s);
	proc = (long (*)(long)) dill_get_fp(h);
	result = proc(s2l);
	if (result != s2l) {
	    printf("test for 1 arguments of type \"l\" failed, expected %ld, got %ld\n",
		   s2l, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call1_l", DILL_L, "%l");
	reg = dill_scalll(s2, (void*)proc, "proc_1_l", "%l", dill_param_reg(s2, 0));
	dill_retl(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (long (*) (long)) dill_get_fp(h2);
	result = proc2(s2l);
	if (result != s2l) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 1,
		   "l");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 2 of type l */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param1;
	dill_exec_handle h, h2;
	long (*proc)(long, long);
	long (*proc2)(long, long);
	long result;
	if (verbose) {
	    printf("test for 2 of type l ...");
	}
	dill_start_proc(s, "param2_l", DILL_L, "%l%l");
	param1 = dill_param_reg(s, 1);
	dill_retl(s, param1);
	h = dill_finalize(s);
	proc = (long (*)(long, long)) dill_get_fp(h);
	result = proc(s1l, s2l);
	if (result != s2l) {
	    printf("test for 2 arguments of type \"l\" failed, expected %ld, got %ld\n",
		   s2l, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call2_l", DILL_L, "%l%l");
	reg = dill_scalll(s2, (void*)proc, "proc_2_l", "%l%l", dill_param_reg(s2, 0), dill_param_reg(s2, 1));
	dill_retl(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (long (*) (long, long)) dill_get_fp(h2);
	result = proc2(s1l, s2l);
	if (result != s2l) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 2,
		   "l");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 3 of type l */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param2;
	dill_exec_handle h, h2;
	long (*proc)(long, long, long);
	long (*proc2)(long, long, long);
	long result;
	if (verbose) {
	    printf("test for 3 of type l ...");
	}
	dill_start_proc(s, "param3_l", DILL_L, "%l%l%l");
	param2 = dill_param_reg(s, 2);
	dill_retl(s, param2);
	h = dill_finalize(s);
	proc = (long (*)(long, long, long)) dill_get_fp(h);
	result = proc(s1l, s1l, s2l);
	if (result != s2l) {
	    printf("test for 3 arguments of type \"l\" failed, expected %ld, got %ld\n",
		   s2l, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call3_l", DILL_L, "%l%l%l");
	reg = dill_scalll(s2, (void*)proc, "proc_3_l", "%l%l%l", dill_param_reg(s2, 0), dill_param_reg(s2, 1), dill_param_reg(s2, 2));
	dill_retl(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (long (*) (long, long, long)) dill_get_fp(h2);
	result = proc2(s1l, s1l, s2l);
	if (result != s2l) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 3,
		   "l");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 4 of type l */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param3;
	dill_exec_handle h, h2;
	long (*proc)(long, long, long, long);
	long (*proc2)(long, long, long, long);
	long result;
	if (verbose) {
	    printf("test for 4 of type l ...");
	}
	dill_start_proc(s, "param4_l", DILL_L, "%l%l%l%l");
	param3 = dill_param_reg(s, 3);
	dill_retl(s, param3);
	h = dill_finalize(s);
	proc = (long (*)(long, long, long, long)) dill_get_fp(h);
	result = proc(s1l, s1l, s1l, s2l);
	if (result != s2l) {
	    printf("test for 4 arguments of type \"l\" failed, expected %ld, got %ld\n",
		   s2l, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call4_l", DILL_L, "%l%l%l%l");
	reg = dill_scalll(s2, (void*)proc, "proc_4_l", "%l%l%l%l", dill_param_reg(s2, 0), dill_param_reg(s2, 1), dill_param_reg(s2, 2), dill_param_reg(s2, 3));
	dill_retl(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (long (*) (long, long, long, long)) dill_get_fp(h2);
	result = proc2(s1l, s1l, s1l, s2l);
	if (result != s2l) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 4,
		   "l");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 1 of type ul */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param0;
	dill_exec_handle h, h2;
	unsigned long (*proc)(unsigned long);
	unsigned long (*proc2)(unsigned long);
	unsigned long result;
	if (verbose) {
	    printf("test for 1 of type ul ...");
	}
	dill_start_proc(s, "param1_ul", DILL_UL, "%ul");
	param0 = dill_param_reg(s, 0);
	dill_retul(s, param0);
	h = dill_finalize(s);
	proc = (unsigned long (*)(unsigned long)) dill_get_fp(h);
	result = proc(s2ul);
	if (result != s2ul) {
	    printf("test for 1 arguments of type \"ul\" failed, expected %lu, got %lu\n",
		   s2ul, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call1_ul", DILL_UL, "%ul");
	reg = dill_scallul(s2, (void*)proc, "proc_1_ul", "%ul", dill_param_reg(s2, 0));
	dill_retul(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (unsigned long (*) (unsigned long)) dill_get_fp(h2);
	result = proc2(s2ul);
	if (result != s2ul) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 1,
		   "ul");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 2 of type ul */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param1;
	dill_exec_handle h, h2;
	unsigned long (*proc)(unsigned long, unsigned long);
	unsigned long (*proc2)(unsigned long, unsigned long);
	unsigned long result;
	if (verbose) {
	    printf("test for 2 of type ul ...");
	}
	dill_start_proc(s, "param2_ul", DILL_UL, "%ul%ul");
	param1 = dill_param_reg(s, 1);
	dill_retul(s, param1);
	h = dill_finalize(s);
	proc = (unsigned long (*)(unsigned long, unsigned long)) dill_get_fp(h);
	result = proc(s1ul, s2ul);
	if (result != s2ul) {
	    printf("test for 2 arguments of type \"ul\" failed, expected %lu, got %lu\n",
		   s2ul, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call2_ul", DILL_UL, "%ul%ul");
	reg = dill_scallul(s2, (void*)proc, "proc_2_ul", "%ul%ul", dill_param_reg(s2, 0), dill_param_reg(s2, 1));
	dill_retul(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (unsigned long (*) (unsigned long, unsigned long)) dill_get_fp(h2);
	result = proc2(s1ul, s2ul);
	if (result != s2ul) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 2,
		   "ul");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 3 of type ul */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param2;
	dill_exec_handle h, h2;
	unsigned long (*proc)(unsigned long, unsigned long, unsigned long);
	unsigned long (*proc2)(unsigned long, unsigned long, unsigned long);
	unsigned long result;
	if (verbose) {
	    printf("test for 3 of type ul ...");
	}
	dill_start_proc(s, "param3_ul", DILL_UL, "%ul%ul%ul");
	param2 = dill_param_reg(s, 2);
	dill_retul(s, param2);
	h = dill_finalize(s);
	proc = (unsigned long (*)(unsigned long, unsigned long, unsigned long)) dill_get_fp(h);
	result = proc(s1ul, s1ul, s2ul);
	if (result != s2ul) {
	    printf("test for 3 arguments of type \"ul\" failed, expected %lu, got %lu\n",
		   s2ul, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call3_ul", DILL_UL, "%ul%ul%ul");
	reg = dill_scallul(s2, (void*)proc, "proc_3_ul", "%ul%ul%ul", dill_param_reg(s2, 0), dill_param_reg(s2, 1), dill_param_reg(s2, 2));
	dill_retul(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (unsigned long (*) (unsigned long, unsigned long, unsigned long)) dill_get_fp(h2);
	result = proc2(s1ul, s1ul, s2ul);
	if (result != s2ul) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 3,
		   "ul");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 4 of type ul */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param3;
	dill_exec_handle h, h2;
	unsigned long (*proc)(unsigned long, unsigned long, unsigned long, unsigned long);
	unsigned long (*proc2)(unsigned long, unsigned long, unsigned long, unsigned long);
	unsigned long result;
	if (verbose) {
	    printf("test for 4 of type ul ...");
	}
	dill_start_proc(s, "param4_ul", DILL_UL, "%ul%ul%ul%ul");
	param3 = dill_param_reg(s, 3);
	dill_retul(s, param3);
	h = dill_finalize(s);
	proc = (unsigned long (*)(unsigned long, unsigned long, unsigned long, unsigned long)) dill_get_fp(h);
	result = proc(s1ul, s1ul, s1ul, s2ul);
	if (result != s2ul) {
	    printf("test for 4 arguments of type \"ul\" failed, expected %lu, got %lu\n",
		   s2ul, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call4_ul", DILL_UL, "%ul%ul%ul%ul");
	reg = dill_scallul(s2, (void*)proc, "proc_4_ul", "%ul%ul%ul%ul", dill_param_reg(s2, 0), dill_param_reg(s2, 1), dill_param_reg(s2, 2), dill_param_reg(s2, 3));
	dill_retul(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (unsigned long (*) (unsigned long, unsigned long, unsigned long, unsigned long)) dill_get_fp(h2);
	result = proc2(s1ul, s1ul, s1ul, s2ul);
	if (result != s2ul) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 4,
		   "ul");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 1 of type f */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param0;
	dill_exec_handle h, h2;
	float (*proc)(float);
	float (*proc2)(float);
	float result;
	if (verbose) {
	    printf("test for 1 of type f ...");
	}
	dill_start_proc(s, "param1_f", DILL_F, "%f");
	param0 = dill_param_reg(s, 0);
	dill_retf(s, param0);
	h = dill_finalize(s);
	proc = (float (*)(float)) dill_get_fp(h);
	result = proc(s2f);
	if (result != s2f) {
	    printf("test for 1 arguments of type \"f\" failed, expected %g, got %g\n",
		   s2f, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call1_f", DILL_F, "%f");
	reg = dill_scallf(s2, (void*)proc, "proc_1_f", "%f", dill_param_reg(s2, 0));
	dill_retf(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (float (*) (float)) dill_get_fp(h2);
	result = proc2(s2f);
	if (result != s2f) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 1,
		   "f");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 2 of type f */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param1;
	dill_exec_handle h, h2;
	float (*proc)(float, float);
	float (*proc2)(float, float);
	float result;
	if (verbose) {
	    printf("test for 2 of type f ...");
	}
	dill_start_proc(s, "param2_f", DILL_F, "%f%f");
	param1 = dill_param_reg(s, 1);
	dill_retf(s, param1);
	h = dill_finalize(s);
	proc = (float (*)(float, float)) dill_get_fp(h);
	result = proc(s1f, s2f);
	if (result != s2f) {
	    printf("test for 2 arguments of type \"f\" failed, expected %g, got %g\n",
		   s2f, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call2_f", DILL_F, "%f%f");
	reg = dill_scallf(s2, (void*)proc, "proc_2_f", "%f%f", dill_param_reg(s2, 0), dill_param_reg(s2, 1));
	dill_retf(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (float (*) (float, float)) dill_get_fp(h2);
	result = proc2(s1f, s2f);
	if (result != s2f) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 2,
		   "f");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 3 of type f */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param2;
	dill_exec_handle h, h2;
	float (*proc)(float, float, float);
	float (*proc2)(float, float, float);
	float result;
	if (verbose) {
	    printf("test for 3 of type f ...");
	}
	dill_start_proc(s, "param3_f", DILL_F, "%f%f%f");
	param2 = dill_param_reg(s, 2);
	dill_retf(s, param2);
	h = dill_finalize(s);
	proc = (float (*)(float, float, float)) dill_get_fp(h);
	result = proc(s1f, s1f, s2f);
	if (result != s2f) {
	    printf("test for 3 arguments of type \"f\" failed, expected %g, got %g\n",
		   s2f, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call3_f", DILL_F, "%f%f%f");
	reg = dill_scallf(s2, (void*)proc, "proc_3_f", "%f%f%f", dill_param_reg(s2, 0), dill_param_reg(s2, 1), dill_param_reg(s2, 2));
	dill_retf(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (float (*) (float, float, float)) dill_get_fp(h2);
	result = proc2(s1f, s1f, s2f);
	if (result != s2f) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 3,
		   "f");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 4 of type f */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param3;
	dill_exec_handle h, h2;
	float (*proc)(float, float, float, float);
	float (*proc2)(float, float, float, float);
	float result;
	if (verbose) {
	    printf("test for 4 of type f ...");
	}
	dill_start_proc(s, "param4_f", DILL_F, "%f%f%f%f");
	param3 = dill_param_reg(s, 3);
	dill_retf(s, param3);
	h = dill_finalize(s);
	proc = (float (*)(float, float, float, float)) dill_get_fp(h);
	result = proc(s1f, s1f, s1f, s2f);
	if (result != s2f) {
	    printf("test for 4 arguments of type \"f\" failed, expected %g, got %g\n",
		   s2f, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call4_f", DILL_F, "%f%f%f%f");
	reg = dill_scallf(s2, (void*)proc, "proc_4_f", "%f%f%f%f", dill_param_reg(s2, 0), dill_param_reg(s2, 1), dill_param_reg(s2, 2), dill_param_reg(s2, 3));
	dill_retf(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (float (*) (float, float, float, float)) dill_get_fp(h2);
	result = proc2(s1f, s1f, s1f, s2f);
	if (result != s2f) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 4,
		   "f");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 1 of type d */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param0;
	dill_exec_handle h, h2;
	double (*proc)(double);
	double (*proc2)(double);
	double result;
	if (verbose) {
	    printf("test for 1 of type d ...");
	}
	dill_start_proc(s, "param1_d", DILL_D, "%d");
	param0 = dill_param_reg(s, 0);
	dill_retd(s, param0);
	h = dill_finalize(s);
	proc = (double (*)(double)) dill_get_fp(h);
	result = proc(s2d);
	if (result != s2d) {
	    printf("test for 1 arguments of type \"d\" failed, expected %g, got %g\n",
		   s2d, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call1_d", DILL_D, "%d");
	reg = dill_scalld(s2, (void*)proc, "proc_1_d", "%d", dill_param_reg(s2, 0));
	dill_retd(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (double (*) (double)) dill_get_fp(h2);
	result = proc2(s2d);
	if (result != s2d) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 1,
		   "d");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 2 of type d */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param1;
	dill_exec_handle h, h2;
	double (*proc)(double, double);
	double (*proc2)(double, double);
	double result;
	if (verbose) {
	    printf("test for 2 of type d ...");
	}
	dill_start_proc(s, "param2_d", DILL_D, "%d%d");
	param1 = dill_param_reg(s, 1);
	dill_retd(s, param1);
	h = dill_finalize(s);
	proc = (double (*)(double, double)) dill_get_fp(h);
	result = proc(s1d, s2d);
	if (result != s2d) {
	    printf("test for 2 arguments of type \"d\" failed, expected %g, got %g\n",
		   s2d, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call2_d", DILL_D, "%d%d");
	reg = dill_scalld(s2, (void*)proc, "proc_2_d", "%d%d", dill_param_reg(s2, 0), dill_param_reg(s2, 1));
	dill_retd(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (double (*) (double, double)) dill_get_fp(h2);
	result = proc2(s1d, s2d);
	if (result != s2d) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 2,
		   "d");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 3 of type d */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param2;
	dill_exec_handle h, h2;
	double (*proc)(double, double, double);
	double (*proc2)(double, double, double);
	double result;
	if (verbose) {
	    printf("test for 3 of type d ...");
	}
	dill_start_proc(s, "param3_d", DILL_D, "%d%d%d");
	param2 = dill_param_reg(s, 2);
	dill_retd(s, param2);
	h = dill_finalize(s);
	proc = (double (*)(double, double, double)) dill_get_fp(h);
	result = proc(s1d, s1d, s2d);
	if (result != s2d) {
	    printf("test for 3 arguments of type \"d\" failed, expected %g, got %g\n",
		   s2d, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call3_d", DILL_D, "%d%d%d");
	reg = dill_scalld(s2, (void*)proc, "proc_3_d", "%d%d%d", dill_param_reg(s2, 0), dill_param_reg(s2, 1), dill_param_reg(s2, 2));
	dill_retd(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (double (*) (double, double, double)) dill_get_fp(h2);
	result = proc2(s1d, s1d, s2d);
	if (result != s2d) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 3,
		   "d");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    {	/* test for 4 of type d */
	dill_stream s = dill_create_raw_stream();
	dill_stream s2 = dill_create_raw_stream();
	dill_reg param3;
	dill_exec_handle h, h2;
	double (*proc)(double, double, double, double);
	double (*proc2)(double, double, double, double);
	double result;
	if (verbose) {
	    printf("test for 4 of type d ...");
	}
	dill_start_proc(s, "param4_d", DILL_D, "%d%d%d%d");
	param3 = dill_param_reg(s, 3);
	dill_retd(s, param3);
	h = dill_finalize(s);
	proc = (double (*)(double, double, double, double)) dill_get_fp(h);
	result = proc(s1d, s1d, s1d, s2d);
	if (result != s2d) {
	    printf("test for 4 arguments of type \"d\" failed, expected %g, got %g\n",
		   s2d, result);
	    dill_dump(s);
	    printf("\n*************\n\n");
	    dill_errors++;
	}

	dill_start_proc(s2, "call4_d", DILL_D, "%d%d%d%d");
	reg = dill_scalld(s2, (void*)proc, "proc_4_d", "%d%d%d%d", dill_param_reg(s2, 0), dill_param_reg(s2, 1), dill_param_reg(s2, 2), dill_param_reg(s2, 3));
	dill_retd(s2, reg);
	h2 = dill_finalize(s2);
	proc2 = (double (*) (double, double, double, double)) dill_get_fp(h2);
	result = proc2(s1d, s1d, s1d, s2d);
	if (result != s2d) {
	    printf("test for calling procedure with %d arguments of type %s failed\n", 4,
		   "d");
	    dill_dump(s2);
	    printf("\n*************\n\n");
	    dill_errors++;
	} else if (verbose) {
	    printf(" passed\n");
	}
	dill_free_handle(h);
	dill_free_handle(h2);
	dill_free_stream(s);
	dill_free_stream(s2);
    }
    if (dill_errors != 0) {
	printf("*** %d Errors ! ****\n", dill_errors);
    } else {
	printf("No errors!\n");
    }
    return dill_errors;
}

