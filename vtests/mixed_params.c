#include <stdio.h>
#include <math.h>
#include "dill.h"
#include "config.h"

/* Dump hex bytes of generated code */
void dump_hex(unsigned char* p, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02x ", p[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}

int main() {
    dill_stream s;
    dill_reg a, b, tmp;
    dill_exec_handle h;

    printf("Generating ul+f test (like FFS)...\n");

    s = dill_create_stream();
    dill_start_proc(s, "no name", DILL_UL, "%ul%f");
    a = dill_param_reg(s, 0);
    b = dill_param_reg(s, 1);

    tmp = dill_getreg(s, DILL_F);
    dill_cvul2f(s, tmp, a);
    dill_addf(s, tmp, tmp, b);
    dill_cvf2ul(s, a, tmp);
    dill_retul(s, a);

    h = dill_finalize(s);

    unsigned char* code = (unsigned char*)dill_get_fp(h);

    /* Now run the test */
    {
        size_t (*proc)(size_t, float) = (size_t (*)(size_t, float))code;
        size_t source1_ul = 233;
        float source2_f = -9.0f;
        size_t expected_result = (size_t)(source1_ul + source2_f);
        size_t result = proc(source1_ul, source2_f);

        printf("Result: %zu (expected: %zu)\n", result, expected_result);
        if (result == expected_result) {
            printf("PASSED!\n");
        } else {
            printf("FAILED!\n");
            printf("\nVirtual instruction dump:\n");
            dill_dump(s);
            printf("\nFirst 128 bytes of native code at %p:\n", (void*)code);
            dump_hex(code, 128);
        }
    }

    dill_free_handle(h);
    dill_free_stream(s);

    return 0;
}
