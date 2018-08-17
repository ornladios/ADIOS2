#include "config.h"
#include "cod.h"
#include "assert.h"
#include <stdio.h>
char code_string[] = "{return sizeof(int);}";
char code_string2[] = "{return sizeof(int*);}";
char code_string3[] = "{   int j;      return sizeof j;}";

int
main()
{
    char *code_blocks[] = {code_string, code_string2, code_string3, NULL};
    int results[] = {sizeof(int), sizeof(int*), sizeof(int)};
    int test = 0;
	
    while (code_blocks[test] != NULL) {
	int ret;
	cod_parse_context context = new_cod_parse_context();
	cod_exec_context ec;

	cod_code gen_code;
	int (*func)();

	cod_add_param("ec", "cod_exec_context", 0, context);
	gen_code = cod_code_gen(code_blocks[test], context);
	ec = cod_create_exec_context(gen_code);
	func = (int(*)())gen_code->func;
	ret = (func)(ec);
	if (ret != results[test]) {
	    printf("bad test %d, ret was %d\n", test, ret);
	}
	cod_exec_context_free(ec);
	cod_code_free(gen_code);
	cod_free_parse_context(context);
	test++;
    }
    return 0;
}
