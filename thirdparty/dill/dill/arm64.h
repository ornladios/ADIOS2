#ifndef _ARM64_H
#define _ARM64_H

/*
 * AArch64 (ARM64) backend for DILL
 *
 * Register conventions (AAPCS64):
 *   x0-x7   : argument/result registers
 *   x8      : indirect result location register
 *   x9-x15  : temporary registers
 *   x16-x17 : intra-procedure-call scratch registers (IP0, IP1)
 *   x18     : platform register (reserved)
 *   x19-x28 : callee-saved registers
 *   x29     : frame pointer (FP)
 *   x30     : link register (LR)
 *   SP      : stack pointer (not a general register)
 *
 * Floating point:
 *   v0-v7   : argument/result registers
 *   v8-v15  : callee-saved (lower 64 bits only)
 *   v16-v31 : temporary registers
 */

/* Macro to emit a 32-bit instruction */
#define INSN_OUT(s, insn) \
    do { \
        if ((s)->p->cur_ip >= (s)->p->code_limit) { \
            extend_dill_stream(s); \
        } \
        *(unsigned int*)(s)->p->cur_ip = (unsigned int)(insn); \
        if ((s)->dill_debug) \
            printf("  %p: %08x\n", (s)->p->cur_ip, (unsigned int)(insn)); \
        (s)->p->cur_ip = ((char*)(s)->p->cur_ip) + 4; \
    } while (0)

/* General purpose registers (64-bit) */
enum {
    _x0 = 0, _x1, _x2, _x3, _x4, _x5, _x6, _x7,
    _x8, _x9, _x10, _x11, _x12, _x13, _x14, _x15,
    _x16, _x17, _x18, _x19, _x20, _x21, _x22, _x23,
    _x24, _x25, _x26, _x27, _x28, _x29, _x30,

    /* Aliases */
    _fp = _x29,     /* frame pointer */
    _lr = _x30,     /* link register */
    _sp = 31,       /* stack pointer (special encoding) */
    _xzr = 31,      /* zero register (same encoding as SP, context-dependent) */

    /* Argument registers */
    _a0 = _x0, _a1 = _x1, _a2 = _x2, _a3 = _x3,
    _a4 = _x4, _a5 = _x5, _a6 = _x6, _a7 = _x7,

    /* Temporary registers */
    _t0 = _x9, _t1 = _x10, _t2 = _x11, _t3 = _x12,
    _t4 = _x13, _t5 = _x14, _t6 = _x15,

    /* Callee-saved registers */
    _s0 = _x19, _s1 = _x20, _s2 = _x21, _s3 = _x22,
    _s4 = _x23, _s5 = _x24, _s6 = _x25, _s7 = _x26,
    _s8 = _x27, _s9 = _x28,

    /* Floating point / SIMD registers */
    _v0 = 0, _v1, _v2, _v3, _v4, _v5, _v6, _v7,
    _v8, _v9, _v10, _v11, _v12, _v13, _v14, _v15,
    _v16, _v17, _v18, _v19, _v20, _v21, _v22, _v23,
    _v24, _v25, _v26, _v27, _v28, _v29, _v30, _v31
};

/* Machine-specific information */
typedef struct arm64_mach_info {
    int act_rec_size;
    int stack_align;
    int stack_constant_offset;
    int gp_save_offset;
    int fp_save_offset;
    int conversion_word;
    int cur_arg_offset;
    int next_core_register;
    int next_float_register;
    int varidiac_call;
    int non_var_args;
    int pushed_args;
    int save_insn_offset;
    int max_arg_size;
} *arm64_mach_info;

/* Type size and alignment tables */
extern int arm64_type_align[];
extern int arm64_type_size[];

/* Function prototypes */
extern void *gen_arm64_mach_info(dill_stream s);
extern void arm64_proc_start(dill_stream s, char *subr_name, int arg_count,
                             arg_info_list args, dill_reg *arglist);
extern void arm64_end(dill_stream s);
extern void arm64_package_end(dill_stream s);
extern void *arm64_clone_code(dill_stream s, void *base, int size);
extern void arm64_ret(dill_stream s, int data1, int data2, int src);
extern void arm64_reti(dill_stream s, int data1, int data2, IMM_TYPE imm);
extern void arm64_retf(dill_stream s, int data1, int data2, double imm);
extern int arm64_getreg(dill_stream s, dill_reg *reg_p, int type, int class);
extern int arm64_putreg(dill_stream s, dill_reg reg, int type);
extern void arm64_ploadi(dill_stream s, int type, int junk, int dest, int src, IMM_TYPE offset);
extern void arm64_pload(dill_stream s, int type, int junk, int dest, int src1, int src2);
extern void arm64_pbsloadi(dill_stream s, int type, int junk, int dest, int src, IMM_TYPE offset);
extern void arm64_pbsload(dill_stream s, int type, int junk, int dest, int src1, int src2);
extern void arm64_pstorei(dill_stream s, int type, int junk, int dest, int src, IMM_TYPE offset);
extern void arm64_pstore(dill_stream s, int type, int junk, int dest, int src1, int src2);
extern void arm64_mov(dill_stream s, int type, int junk, int dest, int src);
extern void arm64_pset(dill_stream s, int type, int junk, int dest, IMM_TYPE imm);
extern void arm64_setf(dill_stream s, int type, int junk, int dest, double imm);
extern void arm64_setp(dill_stream s, int type, int junk, int dest, void *imm);
extern int arm64_local(dill_stream s, int type);
extern int arm64_localb(dill_stream s, int size);
extern int arm64_local_op(dill_stream s, int flag, int val);
extern void arm64_save_restore_op(dill_stream s, int save_restore, int type, int reg);
extern int arm64_init_disassembly_info(dill_stream s, void *ptr);
extern int arm64_print_insn(dill_stream s, void *info_ptr, void *insn);
extern int arm64_count_insn(dill_stream s, int start, int end);
extern void arm64_print_reg(dill_stream s, int typ, int reg);

/* Arithmetic operations */
extern void arm64_arith3(dill_stream s, int data1, int data2, int dest, int src1, int src2);
extern void arm64_arith3i(dill_stream s, int data1, int data2, int dest, int src, IMM_TYPE imm);
extern void arm64_arith2(dill_stream s, int data1, int data2, int dest, int src);
extern void arm64_mul(dill_stream s, int data1, int data2, int dest, int src1, int src2);
extern void arm64_muli(dill_stream s, int data1, int data2, int dest, int src, IMM_TYPE imm);
extern void arm64_div(dill_stream s, int data1, int data2, int dest, int src1, int src2);
extern void arm64_divi(dill_stream s, int data1, int data2, int dest, int src, IMM_TYPE imm);
extern void arm64_mod(dill_stream s, int data1, int data2, int dest, int src1, int src2);
extern void arm64_modi(dill_stream s, int data1, int data2, int dest, int src, IMM_TYPE imm);
extern void arm64_farith(dill_stream s, int data1, int data2, int dest, int src1, int src2);
extern void arm64_farith2(dill_stream s, int data1, int data2, int dest, int src);

/* Branch and compare operations */
extern void arm64_branch(dill_stream s, int op, int type, int src1, int src2, int label);
extern void arm64_branchi(dill_stream s, int op, int type, int src, IMM_TYPE imm, int label);
extern void arm64_compare(dill_stream s, int op, int type, int dest, int src1, int src2);
extern void arm64_comparei(dill_stream s, int op, int type, int dest, int src, IMM_TYPE imm);

/* Jump operations */
extern void arm64_jump_to_label(dill_stream s, unsigned long label);
extern void arm64_jump_to_reg(dill_stream s, unsigned long reg);
extern void arm64_jump_to_imm(dill_stream s, void *imm);
extern void arm64_jal(dill_stream s, int return_addr_reg, int target);

/* Call operations */
extern int arm64_calli(dill_stream s, int type, void *xfer_address, const char *name);
extern int arm64_callr(dill_stream s, int type, int src);

/* Push operations */
extern void arm64_push(dill_stream s, int type, int reg);
extern void arm64_pushi(dill_stream s, int type, IMM_TYPE value);
extern void arm64_pushpi(dill_stream s, int type, void *value);
extern void arm64_pushfi(dill_stream s, int type, double value);

/* Misc operations */
extern void arm64_convert(dill_stream s, int from_type, int to_type, int dest, int src);
extern void arm64_bswap(dill_stream s, int data1, int data2, int dest, int src);

#endif /* _ARM64_H */
