#include <assert.h>
#include <stdio.h>
#include "dill.h"
#include <stdlib.h>

int main(int argc, char **argv) 
{ 
    dill_stream s = dill_create_raw_stream();
    int (*func)();
    int verbose = 0;

    if (argc > 1) verbose++;

    {
	int result;
	dill_exec_handle h;
	dill_start_simple_proc(s, "foo", DILL_I);
	dill_retii(s, 5);
	h = dill_finalize(s);
	func = (int (*)())dill_get_fp(h);
	if (verbose) dill_dump(s);
	result = func();
	dill_free_handle(h);

	dill_free_stream(s);
	if (result != 5) {
	    printf("Test 1 failed, got %d instead of 5\n", result);
	    exit(1);
	}
    }

    return 0;
}
