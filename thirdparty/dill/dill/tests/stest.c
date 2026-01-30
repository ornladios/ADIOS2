/* Test that save/restore and locals do not overlap in the activation record. */
#include "config.h"
#include <assert.h>
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "../config.h"
#include "dill.h"
#ifdef USE_MMAP_CODE_SEG
#include "sys/mman.h"
#endif
#ifdef USE_MACOS_MAP_JIT
#include <sys/mman.h>
#include <pthread.h>
#endif
#ifdef USE_VIRTUAL_PROTECT
#include <windows.h>
#include <memoryapi.h>
#endif

#ifdef USE_MACOS_MAP_JIT
#define WRITE_PROTECT(x)    pthread_jit_write_protect_np(x)
#else
#define WRITE_PROTECT(x)
#endif

dill_exec_handle mk_test(dill_stream s) {
	int i, regs, fregs;
	int l[100];
	int abortl1;
	int abortl2;
	int abortl3;

	dill_reg r[32];
	dill_reg fr[32];

	dill_start_proc(s, "foo", DILL_I, "%i%d");

	abortl1 = dill_alloc_label(s, NULL);
	abortl2 = dill_alloc_label(s, NULL);
	abortl3 = dill_alloc_label(s, NULL);

	for(i = 0; i < 5 && dill_raw_getreg(s, &r[i], DILL_I, DILL_TEMP); i++) {
/*		(void)("allocated register %d\n", i);*/
		dill_seti(s, r[i], i);
		dill_savei(s, r[i]); 
	}
	regs = i;

	for(i = 0; dill_raw_getreg(s, &fr[i], DILL_D, DILL_TEMP); i++) {
/*		(void)("allocated register %d\n", i);*/
		dill_setd(s, fr[i], (double)i);
		dill_saved(s, fr[i]); 
	}
	fregs = i;

	for(i = 0; i < 10; i++) {
		l[i] = dill_local(s, DILL_I);
			
		dill_seti(s, dill_param_reg(s,0), 100 + i);
		dill_stii(s, dill_param_reg(s,0), dill_lp(s), l[i]);
	}

	/* call a procedure, then verify that everything is in place. */
	dill_scallv(s, (void*)printf, "printf", "%P", "hello world!\n");


	for(i = 0; i < regs; i++) {
		dill_restorei(s, r[i]); 
		dill_bneii(s, r[i], i, abortl1);
	}

	for(i = 0; i < fregs; i++) {
/*		(void)("allocated register %d\n", i);*/
		dill_restored(s, fr[i]); 
		dill_setd(s, dill_param_reg(s,1), (double)i);
		dill_bned(s, fr[i], dill_param_reg(s,1), abortl2);
	}

	for(i = 0; i < 10; i++) {
		dill_ldii(s, dill_param_reg(s, 0), dill_lp(s), l[i]);
		dill_bneii(s, dill_param_reg(s, 0), 100 + i, abortl3);
	}
	dill_retii(s, 0);

	dill_mark_label(s, abortl1);
	dill_retii(s, 1);		/* failure. */
	dill_mark_label(s, abortl2);
	dill_retii(s, 2);		/* failure. */
	dill_mark_label(s, abortl3);
	dill_retii(s, 3);		/* failure. */

	return dill_finalize(s);
}

int main() { 
	int (*ip)();
	dill_stream s = dill_create_raw_stream();
	dill_exec_handle h;
	int ret;
	char *target;

	h = mk_test(s);
	ip = (int (*)()) dill_get_fp(h);
	ret = ip();
	dill_free_handle(h);

	if(ret == 0) {
		printf("success!\n");
	} else {
		dill_dump(s);
		printf("failure at point %d!\n", ret);
	}
	h = mk_test(s);
	ip = (int (*)()) dill_get_fp(h);
	ret = ip();
	dill_free_handle(h);

	if(ret == 0)
		printf("success!\n");
	else {
		dill_dump(s);
		printf("failure at point %d (second)\n", ret);
	}
#if defined(USE_MMAP_CODE_SEG) || defined(USE_MACOS_MAP_JIT)
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#ifndef MAP_JIT
#define MAP_JIT 0
#endif
	{
	    int size = dill_code_size(s);
	    static long ps = -1;
	    if (ps == -1) {
	        ps = (getpagesize ());
	    }
	    if (ps > size) size = ps;
	    target = (void*)mmap(0, size,
				 PROT_EXEC | PROT_READ | PROT_WRITE,
				 MAP_ANONYMOUS|MAP_PRIVATE|MAP_JIT, -1, 0);
	}
	if (target == (void*)-1) perror("mmap");
#else
	target = (void*)malloc(dill_code_size(s));
	if (!target) {
	    printf("out of memory\n");
	    return 1;
	}
#ifdef USE_VIRTUAL_PROTECT
	{
	    DWORD dummy;
	    if (!VirtualProtect(target, dill_code_size(s), PAGE_EXECUTE_READWRITE, &dummy)) {
	        fprintf(stderr, "VirtualProtect failed\n");
	        return 1;
	    }
	}
#endif
#endif
	WRITE_PROTECT(0);
	ip = (int (*)()) dill_clone_code(s, target, dill_code_size(s));
	WRITE_PROTECT(1);
	dill_free_stream(s);
	ret = ip();
	if(ret == 0)
		printf("success!\n");
	else {
		dill_dump(s);
		printf("failure at point %d (third)\n", ret);
	}

	return 0;
}
