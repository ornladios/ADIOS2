/*
 *   cod - T1
 *     
 *       This test is relatively simple.  Local ints, int parameters,
 *     simple struct parameters, structs with internal static arrays.
 */
#include "config.h"
#include "data_funcs.h"
#include "cod.h"
#include "assert.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int verbose = 0;
#ifdef NO_EMULATION
#define GEN_PARSE_CONTEXT(x) \
x = new_cod_parse_context();
#define EC_param0
#define EC_param1
#else
#define GEN_PARSE_CONTEXT(x) \
x = new_cod_parse_context();\
cod_add_param("ec", "cod_exec_context", 0, x);
#define EC_param0 ec
#define EC_param1 ec,
#endif

extern void
write_buffer(char *filename, FMStructDescList desc, void *data, 
             int test_num);
extern char *read_buffer(FMContext c, char *read_file, int test_num);

static int *
dummy(int *p)
{
  return p;
}

int
main(int argc, char**argv)
{
    int test_num = 0;
    int run_only = -1;
    char *read_file = NULL;
    char *write_file = NULL;

    struct fool {
	int x;
    } foo;
    foo.x = 1;
    struct fool y;
    y.x = 2;

    while (argc > 1) {
	if (strcmp(argv[1], "-v") == 0) {
	    verbose++;
	} else if (strcmp(argv[1], "-w") == 0) {
	    if (argc <= 1) {
		printf("Need argument to \"-w\"\n");
	    } else {
		write_file = strdup(argv[2]);
	    }
	    argc--; argv++;
	} else if (strcmp(argv[1], "-r") == 0) {
	    if (argc <= 1) {
		printf("Need argument to \"-r\"\n");
	    } else {
		read_file = strdup(argv[2]);
	    }
	    argc--; argv++;
	} else if (strcmp(argv[1], "-o") == 0) {
	    sscanf(argv[2], "%d", &run_only);
	    argc--; argv++;
	}
	argc--; argv++;
    }
    if ((run_only == -1) || (run_only == test_num)) {
	/* test the basics */
	char code_string[] = "\
{\n\
    struct foo {\n\
	int x;\n\
    } fool ;\n\
    fool.x = 4;\n\
\n\
    return fool.x;\n\
}";

	cod_parse_context context;
	cod_exec_context ec;
	cod_code gen_code;
	long (*func)();
	long result;

	GEN_PARSE_CONTEXT(context);
	gen_code = cod_code_gen(code_string, context);
	ec = cod_create_exec_context(gen_code);
	func = (long(*)()) (long) gen_code->func;
	if (verbose) cod_dump(gen_code);
	result = func(EC_param0);
	assert(result == 4);
	cod_code_free(gen_code);
	cod_exec_context_free(ec);
	cod_free_parse_context(context);
    }
    test_num++; /* 1 */
    if ((run_only == -1) || (run_only == test_num)) {
	/* test the basics */
	char code_string[] = "\
{\n\
    typedef struct foo {\n				\
	int x;\n\
    } foo;\n\
    foo v;\n					\
    v.x = 4;\n\
\n\
    return v.x;\n\
}";

	cod_parse_context context;
	cod_exec_context ec;
	cod_code gen_code;
	long (*func)();
	long result;

	GEN_PARSE_CONTEXT(context);
	gen_code = cod_code_gen(code_string, context);
	ec = cod_create_exec_context(gen_code);
	func = (long(*)()) (long) gen_code->func;
	if (verbose) cod_dump(gen_code);
	result = func(EC_param0);
	assert(result == 4);
	cod_code_free(gen_code);
	cod_exec_context_free(ec);
	cod_free_parse_context(context);
    }
    test_num++; /* 2 */
    if ((run_only == -1) || (run_only == test_num)) {
	/* test the basics */
	char code_string[] = "\
{\n\
    typedef short blort;\n\
    struct foo {\n\
	blort x;\n\
	blort y[0];\n\
    } foo;\n\
    struct foo var;\n					\
    var.x = 4;\n\
\n\
    return var.x;\n\
}";

	cod_parse_context context;
	cod_exec_context ec;
	cod_code gen_code;
	long (*func)();
	long result;

	GEN_PARSE_CONTEXT(context);
	gen_code = cod_code_gen(code_string, context);
	ec = cod_create_exec_context(gen_code);
	func = (long(*)()) (long) gen_code->func;
	if (verbose) cod_dump(gen_code);
	result = func(EC_param0);
	assert(result == 4);
	cod_code_free(gen_code);
	cod_exec_context_free(ec);
	cod_free_parse_context(context);
    }
    test_num++; /* 3 */

    return 0;
}
