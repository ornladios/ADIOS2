/*
 * AArch64 (ARM64) code generation for DILL
 * Minimal implementation to get t1 test working
 */

#include "config.h"
#include "dill.h"
#include "dill_internal.h"
#include "arm64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <libkern/OSCacheControl.h>
#endif

#ifndef CLEAR_CACHE_DEFINED
extern void __clear_cache(void *, void *);
#endif

/* Type sizes for AArch64 (LP64 model) */
int arm64_type_size[] = {
    1, /* DILL_C */
    1, /* DILL_UC */
    2, /* DILL_S */
    2, /* DILL_US */
    4, /* DILL_I */
    4, /* DILL_U */
    8, /* DILL_L */
    8, /* DILL_UL */
    8, /* DILL_P */
    4, /* DILL_F */
    8, /* DILL_D */
    0, /* DILL_V */
    0, /* DILL_B */
    8, /* DILL_EC */
};

/* Type alignments for AArch64 */
int arm64_type_align[] = {
    1, /* DILL_C */
    1, /* DILL_UC */
    2, /* DILL_S */
    2, /* DILL_US */
    4, /* DILL_I */
    4, /* DILL_U */
    8, /* DILL_L */
    8, /* DILL_UL */
    8, /* DILL_P */
    4, /* DILL_F */
    8, /* DILL_D */
    0, /* DILL_V */
    0, /* DILL_B */
    8, /* DILL_EC */
};

/* Forward declaration */
static void arm64_set64(dill_stream s, int rd, unsigned long imm);

/*
 * AArch64 Instruction Encoding Helpers
 */

/* MOV (wide immediate) - MOVZ variant
 * Encoding: sf=1 (64-bit), opc=10, hw=0, imm16, Rd
 * 1 10 100101 00 imm16[15:0] Rd[4:0]
 * 0xd2800000 | (imm16 << 5) | rd
 */
static void
arm64_movz(dill_stream s, int rd, unsigned long imm16)
{
    unsigned int insn = 0xd2800000 | ((imm16 & 0xffff) << 5) | (rd & 0x1f);
    INSN_OUT(s, insn);
}

/* MOV (wide immediate) - MOVZ with shift
 * hw = shift / 16 (0, 1, 2, or 3 for 64-bit)
 */
static void
arm64_movz_shift(dill_stream s, int rd, unsigned long imm16, int shift)
{
    int hw = shift / 16;
    unsigned int insn = 0xd2800000 | (hw << 21) | ((imm16 & 0xffff) << 5) | (rd & 0x1f);
    INSN_OUT(s, insn);
}

/* MOVK - move with keep
 * 1 11 100101 hw imm16 Rd
 * 0xf2800000 | (hw << 21) | (imm16 << 5) | rd
 */
static void
arm64_movk(dill_stream s, int rd, unsigned long imm16, int shift)
{
    int hw = shift / 16;
    unsigned int insn = 0xf2800000 | (hw << 21) | ((imm16 & 0xffff) << 5) | (rd & 0x1f);
    INSN_OUT(s, insn);
}

/* RET - return from subroutine
 * Encoding: 1101011 0010 11111 000000 Rn[4:0] 00000
 * Default Rn = x30 (link register)
 * 0xd65f0000 | (rn << 5)
 */
static void
arm64_ret_insn(dill_stream s)
{
    /* RET x30 = 0xd65f03c0 */
    unsigned int insn = 0xd65f0000 | (_lr << 5);
    INSN_OUT(s, insn);
}

/* NOP instruction */
static void
arm64_nop(dill_stream s)
{
    INSN_OUT(s, 0xd503201f);
}

/* Simple return - emits return with nops for potential patching */
static void
arm64_simple_ret(dill_stream s)
{
    dill_mark_ret_location(s);
    /* Emit frame teardown epilogue:
     * mov sp, x29               ; Restore SP to frame pointer (deallocate save area)
     * ldp x29, x30, [sp], #16   ; Restore FP and LR
     * ldp d14, d15, [sp], #16   ; Restore float callee-saved regs
     * ldp d12, d13, [sp], #16
     * ldp d10, d11, [sp], #16
     * ldp d8, d9, [sp], #16
     * ldp x27, x28, [sp], #16   ; Restore integer callee-saved regs
     * ldp x25, x26, [sp], #16
     * ldp x23, x24, [sp], #16
     * ldp x21, x22, [sp], #16
     * ldp x19, x20, [sp], #16
     * ret
     */
    /* MOV sp, x29 (ADD sp, x29, #0) = 0x910003BF */
    INSN_OUT(s, 0x910003BF);
    /* LDP x29, x30, [sp], #16 = 0xA8C17BFD */
    INSN_OUT(s, 0xA8C17BFD);
    /* Restore float callee-saved registers d8-d15 */
    /* LDP d14, d15, [sp], #16 = 0x6CC13FEE */
    INSN_OUT(s, 0x6CC13FEE);
    /* LDP d12, d13, [sp], #16 = 0x6CC137EC */
    INSN_OUT(s, 0x6CC137EC);
    /* LDP d10, d11, [sp], #16 = 0x6CC12FEA */
    INSN_OUT(s, 0x6CC12FEA);
    /* LDP d8, d9, [sp], #16 = 0x6CC127E8 */
    INSN_OUT(s, 0x6CC127E8);
    /* Restore integer callee-saved registers x19-x28 */
    /* LDP x27, x28, [sp], #16 = 0xA8C173FB */
    INSN_OUT(s, 0xA8C173FB);
    /* LDP x25, x26, [sp], #16 = 0xA8C16BF9 */
    INSN_OUT(s, 0xA8C16BF9);
    /* LDP x23, x24, [sp], #16 = 0xA8C163F7 */
    INSN_OUT(s, 0xA8C163F7);
    /* LDP x21, x22, [sp], #16 = 0xA8C15BF5 */
    INSN_OUT(s, 0xA8C15BF5);
    /* LDP x19, x20, [sp], #16 = 0xA8C153F3 */
    INSN_OUT(s, 0xA8C153F3);
    arm64_ret_insn(s);
    arm64_nop(s);  /* nops for potential epilogue patching */
    arm64_nop(s);
}

/* Flush instruction cache */
static void
arm64_flush(void *base, void *limit)
{
#if defined(__APPLE__) && defined(HOST_ARM64)
    /* On macOS ARM64, use sys_icache_invalidate */
    sys_icache_invalidate(base, (size_t)((char*)limit - (char*)base));
#elif defined(HOST_ARM64)
    __clear_cache(base, limit);
#endif
}

/* Link data labels - currently unused */
static void
arm64_data_link(dill_stream s)
{
    /* TODO: Implement if needed for data label support */
}

/* Link branch targets */
static void
arm64_branch_link(dill_stream s)
{
    struct branch_table *t = &s->p->branch_table;
    int i;

    for (i = 0; i < t->branch_count; i++) {
	int label = t->branch_locs[i].label;
	int label_offset = t->label_locs[label] - t->branch_locs[i].loc;
	int *branch_addr = (int*)((char *)s->p->code_base +
				  t->branch_locs[i].loc);
	/* ARM64 branch offset is in instructions (divide by 4) */
	label_offset = label_offset >> 2;

	/* Check instruction type by top byte */
	int insn = *branch_addr;
	if ((insn & 0xff000000) == 0x54000000) {
	    /* B.cond: imm19 in bits 5-23, cond in bits 0-3 */
	    *branch_addr &= 0xff00001f;  /* Keep opcode and condition */
	    *branch_addr |= ((label_offset & 0x7ffff) << 5);
	} else {
	    /* B/BL: imm26 in bits 0-25 */
	    *branch_addr &= 0xfc000000;
	    *branch_addr |= (label_offset & 0x03ffffff);
	}
    }
}

extern void arm64_rt_call_link(char *code, call_t *t);

/* Link call targets */
static void
arm64_call_link(dill_stream s)
{
    arm64_rt_call_link(s->p->code_base, &s->p->call_table);
}

/*
 * On ARM64, we need a procedure linkage table to manage
 * calls to addresses that are more than 26 bits away (128MB).
 * We emit a PLT that loads the address into a register and branches.
 */
extern void
arm64_PLT_emit(dill_stream s, int package)
{
    call_t *t = &s->p->call_table;
    int i;

    for (i = 0; i < t->call_count; i++) {
	int *call_addr = (int*)((unsigned long)s->p->code_base +
				t->call_locs[i].loc);
	long call_offset = (unsigned long)t->call_locs[i].xfer_addr -
	    (unsigned long)call_addr;
	/* Check if offset exceeds 26-bit signed range (128MB) */
	call_offset = call_offset >> 2;
	long high_bits = call_offset >> 25;
#ifdef HOST_ARM64
	if (((high_bits != 0) && (high_bits != -1)) || package) {
	    t->call_locs[i].mach_info = (void*)
		((long)s->p->cur_ip - (long)s->p->code_base);
	    /* Load address into x16 (IP0) and branch */
	    arm64_set64(s, _x16, (unsigned long)t->call_locs[i].xfer_addr);
	    /* BR x16 = 0xd61f0200 */
	    INSN_OUT(s, 0xd61f0000 | (_x16 << 5));
	}
#endif
    }
}

/* MOVN - move wide with NOT
 * 1 00 100101 hw imm16 Rd
 * 0x92800000 | (hw << 21) | (imm16 << 5) | rd
 */
static void
arm64_movn(dill_stream s, int rd, unsigned long imm16, int shift)
{
    int hw = shift / 16;
    unsigned int insn = 0x92800000 | (hw << 21) | ((imm16 & 0xffff) << 5) | (rd & 0x1f);
    INSN_OUT(s, insn);
}

/* Load a 64-bit immediate value into a register
 * Uses MOVZ/MOVK sequence, or MOVN for values near -1
 */
static void
arm64_set64(dill_stream s, int rd, unsigned long imm)
{
    unsigned long inv = ~imm;
    int i;
    int first_chunk = -1;
    int num_zero_chunks = 0;
    int num_ffff_chunks = 0;

    /* Count zero and 0xffff chunks to decide between MOVZ and MOVN */
    for (i = 0; i < 4; i++) {
	unsigned long chunk = (imm >> (i * 16)) & 0xffff;
	if (chunk == 0) num_zero_chunks++;
	if (chunk == 0xffff) num_ffff_chunks++;
    }

    if (num_ffff_chunks > num_zero_chunks) {
	/* Use MOVN - more 0xffff chunks means fewer instructions with NOT */
	/* Find first chunk that isn't 0xffff */
	for (i = 0; i < 4; i++) {
	    unsigned long chunk = (inv >> (i * 16)) & 0xffff;
	    if (chunk != 0) {
		first_chunk = i;
		break;
	    }
	}
	if (first_chunk == -1) {
	    /* All 0xffff - value is -1 */
	    arm64_movn(s, rd, 0, 0);
	    return;
	}
	/* MOVN with first non-zero chunk of inverted value */
	arm64_movn(s, rd, (inv >> (first_chunk * 16)) & 0xffff, first_chunk * 16);
	/* MOVK for remaining chunks that aren't 0xffff in original */
	for (i = first_chunk + 1; i < 4; i++) {
	    unsigned long chunk = (imm >> (i * 16)) & 0xffff;
	    if (chunk != 0xffff) {
		arm64_movk(s, rd, chunk, i * 16);
	    }
	}
    } else {
	/* Use MOVZ - find first non-zero chunk */
	for (i = 0; i < 4; i++) {
	    unsigned long chunk = (imm >> (i * 16)) & 0xffff;
	    if (chunk != 0) {
		first_chunk = i;
		break;
	    }
	}
	if (first_chunk == -1) {
	    /* All zero */
	    arm64_movz(s, rd, 0);
	    return;
	}
	/* MOVZ with first non-zero chunk */
	arm64_movz_shift(s, rd, (imm >> (first_chunk * 16)) & 0xffff, first_chunk * 16);
	/* MOVK for remaining non-zero chunks */
	for (i = first_chunk + 1; i < 4; i++) {
	    unsigned long chunk = (imm >> (i * 16)) & 0xffff;
	    if (chunk != 0) {
		arm64_movk(s, rd, chunk, i * 16);
	    }
	}
    }
}

/*
 * Register initialization
 * Sets up available registers for allocation
 */
#define bit_R(x) ((unsigned long)1 << (x))

static void
arm64_reg_init(dill_stream s)
{
    /* Variable registers (callee-saved) - x19-x28 */
    s->p->var_i.init_avail[0] = (bit_R(_s0) | bit_R(_s1) | bit_R(_s2) | bit_R(_s3) |
				bit_R(_s4) | bit_R(_s5) | bit_R(_s6) | bit_R(_s7) |
				bit_R(_s8) | bit_R(_s9));
    s->p->var_i.members[0] = s->p->var_i.init_avail[0];

    /* Temporary registers (caller-saved) - x9-x15 */
    s->p->tmp_i.init_avail[0] = (bit_R(_t0) | bit_R(_t1) | bit_R(_t2) | bit_R(_t3) |
				bit_R(_t4) | bit_R(_t5) | bit_R(_t6));
    s->p->tmp_i.members[0] = s->p->tmp_i.init_avail[0] |
	(bit_R(_a0) | bit_R(_a1) | bit_R(_a2) | bit_R(_a3) |
	 bit_R(_a4) | bit_R(_a5) | bit_R(_a6) | bit_R(_a7));

    /* Float variable registers (callee-saved lower 64 bits) - v8-v15 */
    s->p->var_f.init_avail[0] = (bit_R(_v8) | bit_R(_v9) | bit_R(_v10) |
				bit_R(_v11) | bit_R(_v12) | bit_R(_v13) |
				bit_R(_v14) | bit_R(_v15));
    s->p->var_f.members[0] = s->p->var_f.init_avail[0];

    /* Float temporary registers - v16-v31 */
    s->p->tmp_f.init_avail[0] = (bit_R(_v16) | bit_R(_v17) | bit_R(_v18) |
				bit_R(_v19) | bit_R(_v20) | bit_R(_v21) |
				bit_R(_v22) | bit_R(_v23) | bit_R(_v24) |
				bit_R(_v25) | bit_R(_v26) | bit_R(_v27) |
				bit_R(_v28) | bit_R(_v29) | bit_R(_v30) |
				bit_R(_v31));
    s->p->tmp_f.members[0] = s->p->tmp_f.init_avail[0] |
	(bit_R(_v0) | bit_R(_v1) | bit_R(_v2) | bit_R(_v3) |
	 bit_R(_v4) | bit_R(_v5) | bit_R(_v6) | bit_R(_v7));
}

/*
 * Create machine info structure
 */
void *
gen_arm64_mach_info(dill_stream s)
{
    arm64_mach_info ami = malloc(sizeof(struct arm64_mach_info));
    if (s->p->mach_info != NULL) {
	free(s->p->mach_info);
	s->p->mach_info = NULL;
	s->p->native.mach_info = NULL;
    }
    arm64_reg_init(s);
    memset(ami, 0, sizeof(struct arm64_mach_info));
    ami->stack_align = 16;  /* AArch64 requires 16-byte stack alignment */
    ami->act_rec_size = 0;
    ami->cur_arg_offset = 0;
    ami->next_core_register = _x0;
    ami->next_float_register = _v0;
    ami->gp_save_offset = 0;
    ami->fp_save_offset = 0;
    ami->max_arg_size = 0;
    return ami;
}

/*
 * Procedure start
 * Sets up parameter handling and function prologue
 */
void
arm64_proc_start(dill_stream s, char *subr_name, int arg_count,
                 arg_info_list args, dill_reg *arglist)
{
    arm64_mach_info ami = (arm64_mach_info)s->p->mach_info;
    int i;
    int next_core_reg = _x0;
    int next_float_reg = _v0;
    int cur_arg_offset = 0;

    /* Reset machine info */
    /* Start act_rec_size at 224 to account for the save area allocated in prologue.
     * Save area layout (FP-16 to FP-224):
     * - Integer arg regs x0-x7: FP-16 to FP-80 (64 bytes)
     * - Integer tmp regs x9-x15: FP-96 to FP-152 (56 bytes)
     * - Float regs v0-v7: FP-160 to FP-224 (64 bytes)
     * Local variables will be allocated below FP-224.
     */
    ami->act_rec_size = 224;
    ami->cur_arg_offset = 0;
    ami->max_arg_size = 0;

    /* Mark the function entry point */
    s->p->fp = s->p->cur_ip;

    /* Emit frame setup prologue:
     * First save callee-saved registers (x19-x28, d8-d15), then FP/LR:
     * stp x19, x20, [sp, #-16]!  ; Save integer callee-saved regs (80 bytes)
     * stp x21, x22, [sp, #-16]!
     * stp x23, x24, [sp, #-16]!
     * stp x25, x26, [sp, #-16]!
     * stp x27, x28, [sp, #-16]!
     * stp d8, d9, [sp, #-16]!    ; Save float callee-saved regs (64 bytes)
     * stp d10, d11, [sp, #-16]!
     * stp d12, d13, [sp, #-16]!
     * stp d14, d15, [sp, #-16]!
     * stp x29, x30, [sp, #-16]!  ; Save FP and LR
     * mov x29, sp                 ; Set up frame pointer
     * sub sp, sp, #224           ; Allocate save area
     *
     * Stack layout after prologue (offsets from FP):
     * [FP + 160] = caller's stack (incoming stack args start here)
     * [FP + 144] = saved x19/x20
     * [FP + 128] = saved x21/x22
     * [FP + 112] = saved x23/x24
     * [FP + 96]  = saved x25/x26
     * [FP + 80]  = saved x27/x28
     * [FP + 64]  = saved d8/d9
     * [FP + 48]  = saved d10/d11
     * [FP + 32]  = saved d12/d13
     * [FP + 16]  = saved d14/d15
     * [FP + 8]   = saved x30 (LR)
     * [FP + 0]   = saved x29 (old FP)
     * [FP - 16] to [FP - 80]   = integer arg reg save area (x0-x7)
     * [FP - 96] to [FP - 152]  = integer tmp reg save area (x9-x15)
     * [FP - 160] to [FP - 224] = float reg save area (v0-v7)
     * [SP] = bottom of save area (FP - 224)
     */
    /* Save callee-saved integer registers x19-x28 */
    /* STP x19, x20, [sp, #-16]! = 0xA9BF53F3 */
    INSN_OUT(s, 0xA9BF53F3);
    /* STP x21, x22, [sp, #-16]! = 0xA9BF5BF5 */
    INSN_OUT(s, 0xA9BF5BF5);
    /* STP x23, x24, [sp, #-16]! = 0xA9BF63F7 */
    INSN_OUT(s, 0xA9BF63F7);
    /* STP x25, x26, [sp, #-16]! = 0xA9BF6BF9 */
    INSN_OUT(s, 0xA9BF6BF9);
    /* STP x27, x28, [sp, #-16]! = 0xA9BF73FB */
    INSN_OUT(s, 0xA9BF73FB);
    /* Save callee-saved float registers d8-d15 (lower 64 bits of v8-v15) */
    /* STP d8, d9, [sp, #-16]! = 0x6DBF27E8 */
    INSN_OUT(s, 0x6DBF27E8);
    /* STP d10, d11, [sp, #-16]! = 0x6DBF2FEA */
    INSN_OUT(s, 0x6DBF2FEA);
    /* STP d12, d13, [sp, #-16]! = 0x6DBF37EC */
    INSN_OUT(s, 0x6DBF37EC);
    /* STP d14, d15, [sp, #-16]! = 0x6DBF3FEE */
    INSN_OUT(s, 0x6DBF3FEE);
    /* STP x29, x30, [sp, #-16]! = 0xA9BF7BFD */
    INSN_OUT(s, 0xA9BF7BFD);
    /* MOV x29, sp (ADD x29, sp, #0) = 0x910003FD */
    INSN_OUT(s, 0x910003FD);
    /* SUB sp, sp, #224 = 0xD10383FF (imm12 = 224 = 0x0E0) */
    INSN_OUT(s, 0xD10383FF);

    /* Process arguments - determine register/stack location for each */
    for (i = 0; i < arg_count; i++) {
	int type = args[i].type;
	int slot_size;

	switch (type) {
	case DILL_F:
	    /* Float arguments go in v0-v7 */
	    if (next_float_reg <= _v7) {
		args[i].is_register = 1;
		args[i].in_reg = next_float_reg;
		args[i].out_reg = next_float_reg;
		next_float_reg++;
	    } else {
		args[i].is_register = 0;
		args[i].offset = cur_arg_offset;
		cur_arg_offset += 4;  /* 32-bit float takes 4 bytes on stack */
	    }
	    break;
	case DILL_D:
	    /* Double arguments go in v0-v7 */
	    if (next_float_reg <= _v7) {
		args[i].is_register = 1;
		args[i].in_reg = next_float_reg;
		args[i].out_reg = next_float_reg;
		next_float_reg++;
	    } else {
		/* 64-bit double needs 8-byte alignment */
		if (cur_arg_offset & 7) cur_arg_offset = (cur_arg_offset + 7) & ~7;
		args[i].is_register = 0;
		args[i].offset = cur_arg_offset;
		cur_arg_offset += 8;
	    }
	    break;
	case DILL_C:
	case DILL_UC:
	case DILL_S:
	case DILL_US:
	case DILL_I:
	case DILL_U:
	    /* Small integer arguments go in x0-x7 */
	    if (next_core_reg <= _x7) {
		args[i].is_register = 1;
		args[i].in_reg = next_core_reg;
		args[i].out_reg = next_core_reg;
		next_core_reg++;
	    } else {
		args[i].is_register = 0;
		args[i].offset = cur_arg_offset;
		cur_arg_offset += 4;  /* 32-bit int takes 4 bytes on stack */
	    }
	    break;
	default:
	    /* 64-bit types (L, UL, P) go in x0-x7 */
	    if (next_core_reg <= _x7) {
		args[i].is_register = 1;
		args[i].in_reg = next_core_reg;
		args[i].out_reg = next_core_reg;
		next_core_reg++;
	    } else {
		/* 64-bit needs 8-byte alignment */
		if (cur_arg_offset & 7) cur_arg_offset = (cur_arg_offset + 7) & ~7;
		args[i].is_register = 0;
		args[i].offset = cur_arg_offset;
		cur_arg_offset += 8;
	    }
	    break;
	}
    }

    /* Move parameters from argument registers to variable registers if needed */
    for (i = 0; i < arg_count; i++) {
	int tmp_reg;
	if (args[i].is_register) {
	    /* Try to get a variable register to hold this parameter */
	    if (!dill_raw_getreg(s, &tmp_reg, args[i].type, DILL_VAR)) {
		/* No variable register available, leave in argument register */
		if (arglist != NULL) arglist[i] = args[i].in_reg;
		continue;
	    }
	    /* Move from argument register to variable register */
	    arm64_mov(s, args[i].type, 0, tmp_reg, args[i].in_reg);
	    args[i].in_reg = tmp_reg;
	    args[i].out_reg = tmp_reg;
	    if (arglist != NULL) arglist[i] = tmp_reg;
	} else {
	    /* Parameter is on stack - load it into a register */
	    int type = args[i].type;
	    if (!dill_raw_getreg(s, &tmp_reg, type, DILL_VAR)) {
		/* No variable register available */
		if (arglist != NULL) arglist[i] = -1;
		continue;
	    }
	    /* Load from stack: incoming stack args are at [FP + 160 + offset]
	     * (+16 for FP/LR, +64 for d8-d15, +80 for x19-x28)
	     */
	    arm64_ploadi(s, type, 0, tmp_reg, _fp, args[i].offset + 160);
	    args[i].in_reg = tmp_reg;
	    args[i].out_reg = tmp_reg;
	    args[i].is_register = 1;
	    if (arglist != NULL) arglist[i] = tmp_reg;
	}
    }
}

void
arm64_emit_save(dill_stream s)
{
    void *save_ip = s->p->cur_ip;
    s->p->fp = (char*)s->p->code_base;
    s->p->cur_ip = save_ip;
    if (s->dill_debug) {
        int num_insns = ((char*)s->p->cur_ip - (char*)s->p->fp) / 4;
        printf("arm64_emit_save: fp=%p, code_base=%p, cur_ip=%p, %d instructions\n",
               s->p->fp, s->p->code_base, s->p->cur_ip, num_insns);
        printf("  All instructions at fp:\n");
        unsigned int *code = (unsigned int*)s->p->fp;
        for (int i = 0; i < num_insns && i < 32; i++) {
            printf("    %p: %08x\n", &code[i], code[i]);
        }
    }
}
    
/*
 * Procedure end
 */
void
arm64_end(dill_stream s)
{
    arm64_nop(s);
    arm64_simple_ret(s);
    arm64_PLT_emit(s, 0);   /* must be done before linking */
    arm64_branch_link(s);
    arm64_call_link(s);
    arm64_data_link(s);
    arm64_emit_save(s);
    arm64_flush(s->p->code_base, s->p->code_limit);
}

/*
 * Package end (for serializable code)
 */
void
arm64_package_end(dill_stream s)
{
    arm64_nop(s);
    arm64_simple_ret(s);
    arm64_PLT_emit(s, 1);   /* package=1 means emit PLT for all calls */
    arm64_branch_link(s);
    arm64_emit_save(s);
}

/*
 * Clone code to new location
 */
void *
arm64_clone_code(dill_stream s, void *new_base, int available_size)
{
    int size = dill_code_size(s);
    void *old_base = s->p->code_base;
    void *native_base = s->p->code_base;
    if (available_size < size) {
        return NULL;
    }
    if (native_base == NULL)
        native_base = s->p->native.code_base;
    memcpy(new_base, native_base, size);
    s->p->code_base = new_base;
    s->p->cur_ip = (char*)new_base + size;
    s->p->fp = new_base;
    arm64_branch_link(s);
    arm64_call_link(s);
    arm64_data_link(s);
    s->p->code_base = old_base;
    s->p->cur_ip = (char*)old_base + size;
    s->p->fp = old_base;
    arm64_flush(new_base, (char*)new_base + size);
    return new_base;
}

/*
 * Return a value in a register
 */
void
arm64_ret(dill_stream s, int data1, int data2, int src)
{
    switch (data1) {
    case DILL_C:
    case DILL_UC:
    case DILL_S:
    case DILL_US:
    case DILL_I:
    case DILL_U:
    case DILL_L:
    case DILL_UL:
    case DILL_P:
	/* Move src to x0 (return register) if needed */
	if (src != _x0) {
	    /* MOV x0, src using ORR */
	    /* ORR Xd, XZR, Xm : 1 01 01010 00 0 Xm 000000 11111 Xd */
	    unsigned int insn = 0xaa0003e0 | (src << 16) | _x0;
	    INSN_OUT(s, insn);
	}
	break;
    case DILL_F:
	/* Move to v0/s0 if needed */
	if (src != _v0) {
	    /* FMOV Sd, Sn: 0001 1110 0010 0000 0100 00 Sn Sd */
	    unsigned int insn = 0x1e204000 | (src << 5) | _v0;
	    INSN_OUT(s, insn);
	}
	break;
    case DILL_D:
	/* Move to v0/d0 if needed */
	if (src != _v0) {
	    /* FMOV Dd, Dn: 0001 1110 0110 0000 0100 00 Dn Dd */
	    unsigned int insn = 0x1e604000 | (src << 5) | _v0;
	    INSN_OUT(s, insn);
	}
	break;
    }
    arm64_simple_ret(s);
}

/*
 * Return an immediate value - THIS IS THE KEY FUNCTION FOR T1
 */
void
arm64_reti(dill_stream s, int data1, int data2, IMM_TYPE imm)
{
    switch (data1) {
    case DILL_C:
    case DILL_UC:
    case DILL_S:
    case DILL_US:
    case DILL_I:
    case DILL_U:
    case DILL_L:
    case DILL_UL:
    case DILL_P:
	/* Load immediate into x0 (return register) */
	arm64_set64(s, _x0, (unsigned long)imm);
	break;
    case DILL_F:
    case DILL_D:
	break; /* no return immediate of floats */
    }
    arm64_simple_ret(s);
}

/*
 * Return a floating point immediate
 */
void
arm64_retf(dill_stream s, int data1, int data2, double imm)
{
    union {
	float f;
	unsigned int i;
    } a;
    union {
	double d;
	unsigned long l;
    } b;

    switch (data1) {
    case DILL_F:
	a.f = (float)imm;
	/* Load float bits into w0, then move to s0 */
	arm64_set64(s, _x0, a.i);
	/* FMOV Sd, Wn: 0001 1110 0010 0111 0000 00 Wn Sd */
	INSN_OUT(s, 0x1e270000 | (_x0 << 5) | _v0);
	break;
    case DILL_D:
	b.d = imm;
	/* Load double bits into x0, then move to d0 */
	arm64_set64(s, _x0, b.l);
	/* FMOV Dd, Xn: 1001 1110 0110 0111 0000 00 Xn Dd */
	INSN_OUT(s, 0x9e670000 | (_x0 << 5) | _v0);
	break;
    }
    arm64_simple_ret(s);
}

/*
 * Stub implementations for other required functions
 */

void
arm64_ploadi(dill_stream s, int type, int junk, int dest, int src, IMM_TYPE offset)
{
    unsigned int insn;

    switch (type) {
    case DILL_C:
	/* LDRSB (signed byte, sign-extend to 64-bit): load and sign extend */
	if (offset >= 0 && offset < 4096) {
	    /* LDRSB Xt (unsigned offset): 0x39800000 */
	    insn = 0x39800000 | ((offset & 0xfff) << 10) | (src << 5) | dest;
	} else if (offset >= -256 && offset < 256) {
	    /* LDURSB Xt (unscaled): 0x38800000 */
	    insn = 0x38800000 | ((offset & 0x1ff) << 12) | (src << 5) | dest;
	} else {
	    arm64_set64(s, _x16, offset);
	    arm64_pload(s, type, junk, dest, src, _x16);
	    return;
	}
	break;

    case DILL_UC:
	/* LDRB (unsigned byte, zero-extend) */
	if (offset >= 0 && offset < 4096) {
	    /* LDRB Wt (unsigned offset): 0x39400000 */
	    insn = 0x39400000 | ((offset & 0xfff) << 10) | (src << 5) | dest;
	} else if (offset >= -256 && offset < 256) {
	    /* LDURB (unscaled): 0x38400000 */
	    insn = 0x38400000 | ((offset & 0x1ff) << 12) | (src << 5) | dest;
	} else {
	    arm64_set64(s, _x16, offset);
	    arm64_pload(s, type, junk, dest, src, _x16);
	    return;
	}
	break;

    case DILL_S:
	/* LDRSH (signed halfword, sign-extend to 64-bit) */
	if (offset >= 0 && (offset & 1) == 0 && offset < 8192) {
	    /* LDRSH Xt (unsigned offset, scaled): 0x79800000 */
	    insn = 0x79800000 | (((offset >> 1) & 0xfff) << 10) | (src << 5) | dest;
	} else if (offset >= -256 && offset < 256) {
	    /* LDURSH Xt (unscaled): 0x78800000 */
	    insn = 0x78800000 | ((offset & 0x1ff) << 12) | (src << 5) | dest;
	} else {
	    arm64_set64(s, _x16, offset);
	    arm64_pload(s, type, junk, dest, src, _x16);
	    return;
	}
	break;

    case DILL_US:
	/* LDRH (unsigned halfword, zero-extend) */
	if (offset >= 0 && (offset & 1) == 0 && offset < 8192) {
	    /* LDRH Wt (unsigned offset, scaled): 0x79400000 */
	    insn = 0x79400000 | (((offset >> 1) & 0xfff) << 10) | (src << 5) | dest;
	} else if (offset >= -256 && offset < 256) {
	    /* LDURH (unscaled): 0x78400000 */
	    insn = 0x78400000 | ((offset & 0x1ff) << 12) | (src << 5) | dest;
	} else {
	    arm64_set64(s, _x16, offset);
	    arm64_pload(s, type, junk, dest, src, _x16);
	    return;
	}
	break;

    case DILL_I:
	/* LDRSW (sign-extend 32-bit to 64-bit) for signed int */
	if (offset >= 0 && (offset & 3) == 0 && offset < 16384) {
	    /* LDRSW Xt (unsigned offset, scaled by 4): 0xB9800000 */
	    insn = 0xB9800000 | (((offset >> 2) & 0xfff) << 10) | (src << 5) | dest;
	} else if (offset >= -256 && offset < 256) {
	    /* LDURSW Xt (unscaled): 0xB8800000 */
	    insn = 0xB8800000 | ((offset & 0x1ff) << 12) | (src << 5) | dest;
	} else {
	    arm64_set64(s, _x16, offset);
	    arm64_pload(s, type, junk, dest, src, _x16);
	    return;
	}
	break;

    case DILL_U:
	/* LDR Wt (zero-extend 32-bit to 64-bit) for unsigned int */
	if (offset >= 0 && (offset & 3) == 0 && offset < 16384) {
	    /* LDR Wt (unsigned offset, scaled by 4): 0xB9400000 */
	    insn = 0xB9400000 | (((offset >> 2) & 0xfff) << 10) | (src << 5) | dest;
	} else if (offset >= -256 && offset < 256) {
	    /* LDUR Wt (unscaled): 0xB8400000 */
	    insn = 0xB8400000 | ((offset & 0x1ff) << 12) | (src << 5) | dest;
	} else {
	    arm64_set64(s, _x16, offset);
	    arm64_pload(s, type, junk, dest, src, _x16);
	    return;
	}
	break;

    case DILL_L:
    case DILL_UL:
    case DILL_P:
	/* LDR Xt (64-bit) */
	if (offset >= 0 && (offset & 7) == 0 && offset < 32768) {
	    /* LDR Xt (unsigned offset, scaled): 0xF9400000 */
	    insn = 0xF9400000 | (((offset >> 3) & 0xfff) << 10) | (src << 5) | dest;
	} else if (offset >= -256 && offset < 256) {
	    /* LDUR Xt (unscaled): 0xF8400000 */
	    insn = 0xF8400000 | ((offset & 0x1ff) << 12) | (src << 5) | dest;
	} else {
	    arm64_set64(s, _x16, offset);
	    arm64_pload(s, type, junk, dest, src, _x16);
	    return;
	}
	break;

    case DILL_F:
	/* LDR St (32-bit float) */
	if (offset >= 0 && (offset & 3) == 0 && offset < 16384) {
	    /* LDR St (unsigned offset, scaled): 0xBD400000 */
	    insn = 0xBD400000 | (((offset >> 2) & 0xfff) << 10) | (src << 5) | dest;
	} else if (offset >= -256 && offset < 256) {
	    /* LDUR St (unscaled): 0xBC400000 */
	    insn = 0xBC400000 | ((offset & 0x1ff) << 12) | (src << 5) | dest;
	} else {
	    arm64_set64(s, _x16, offset);
	    arm64_pload(s, type, junk, dest, src, _x16);
	    return;
	}
	break;

    case DILL_D:
	/* LDR Dt (64-bit double) */
	if (offset >= 0 && (offset & 7) == 0 && offset < 32768) {
	    /* LDR Dt (unsigned offset, scaled): 0xFD400000 */
	    insn = 0xFD400000 | (((offset >> 3) & 0xfff) << 10) | (src << 5) | dest;
	} else if (offset >= -256 && offset < 256) {
	    /* LDUR Dt (unscaled): 0xFC400000 */
	    insn = 0xFC400000 | ((offset & 0x1ff) << 12) | (src << 5) | dest;
	} else {
	    arm64_set64(s, _x16, offset);
	    arm64_pload(s, type, junk, dest, src, _x16);
	    return;
	}
	break;

    default:
	return;
    }
    INSN_OUT(s, insn);
}

void
arm64_pload(dill_stream s, int type, int junk, int dest, int src1, int src2)
{
    /* Load with register offset: LDR Rt, [Xn, Xm] */
    unsigned int insn;

    switch (type) {
    case DILL_C:
	/* LDRSB Xt, [Xn, Xm] (sign extend byte to 64-bit): 0x38A06800 */
	insn = 0x38A06800 | (src2 << 16) | (src1 << 5) | dest;
	break;

    case DILL_UC:
	/* LDRB Wt, [Xn, Xm] (zero extend byte): 0x38606800 */
	insn = 0x38606800 | (src2 << 16) | (src1 << 5) | dest;
	break;

    case DILL_S:
	/* LDRSH Xt, [Xn, Xm] (sign extend halfword to 64-bit): 0x78A06800 */
	insn = 0x78A06800 | (src2 << 16) | (src1 << 5) | dest;
	break;

    case DILL_US:
	/* LDRH Wt, [Xn, Xm] (zero extend halfword): 0x78606800 */
	insn = 0x78606800 | (src2 << 16) | (src1 << 5) | dest;
	break;

    case DILL_I:
	/* LDRSW Xt, [Xn, Xm] (sign extend 32-bit to 64-bit): 0xB8A06800 */
	insn = 0xB8A06800 | (src2 << 16) | (src1 << 5) | dest;
	break;

    case DILL_U:
	/* LDR Wt, [Xn, Xm] (zero extend 32-bit): 0xB8606800 */
	insn = 0xB8606800 | (src2 << 16) | (src1 << 5) | dest;
	break;

    case DILL_L:
    case DILL_UL:
    case DILL_P:
	/* LDR Xt, [Xn, Xm] (64-bit): 0xF8606800 */
	insn = 0xF8606800 | (src2 << 16) | (src1 << 5) | dest;
	break;

    case DILL_F:
	/* LDR St, [Xn, Xm] (32-bit float): 0xBC606800 */
	insn = 0xBC606800 | (src2 << 16) | (src1 << 5) | dest;
	break;

    case DILL_D:
	/* LDR Dt, [Xn, Xm] (64-bit double): 0xFC606800 */
	insn = 0xFC606800 | (src2 << 16) | (src1 << 5) | dest;
	break;

    default:
	return;
    }
    INSN_OUT(s, insn);
}

void
arm64_pbsloadi(dill_stream s, int type, int junk, int dest, int src, IMM_TYPE offset)
{
    /* Load the value first */
    arm64_ploadi(s, type, junk, dest, src, offset);

    /* Byte swap (no-op for single-byte types) */
    if (type != DILL_C && type != DILL_UC) {
	arm64_bswap(s, type, 0, dest, dest);
    }
}

void
arm64_pbsload(dill_stream s, int type, int junk, int dest, int src1, int src2)
{
    /* Load the value first */
    arm64_pload(s, type, junk, dest, src1, src2);

    /* Byte swap (no-op for single-byte types) */
    if (type != DILL_C && type != DILL_UC) {
	arm64_bswap(s, type, 0, dest, dest);
    }
}

void
arm64_pstorei(dill_stream s, int type, int junk, int dest, int src, IMM_TYPE offset)
{
    unsigned int insn;

    /* Use unscaled store (STUR) for signed offsets in range -256 to 255 */
    /* Use scaled store (STR) for larger positive offsets that are properly aligned */

    switch (type) {
    case DILL_C:
    case DILL_UC:
	/* STRB: byte store */
	if (offset >= 0 && offset < 4096) {
	    /* STR (unsigned offset): 0x39000000 | (imm12 << 10) | (Rn << 5) | Rt */
	    insn = 0x39000000 | ((offset & 0xfff) << 10) | (src << 5) | dest;
	} else if (offset >= -256 && offset < 256) {
	    /* STURB (unscaled): 0x38000000 | (imm9 << 12) | (Rn << 5) | Rt */
	    insn = 0x38000000 | ((offset & 0x1ff) << 12) | (src << 5) | dest;
	} else {
	    arm64_set64(s, _x16, offset);
	    arm64_pstore(s, type, junk, dest, src, _x16);
	    return;
	}
	break;

    case DILL_S:
    case DILL_US:
	/* STRH: halfword store */
	if (offset >= 0 && (offset & 1) == 0 && offset < 8192) {
	    /* STR (unsigned offset, scaled by 2): 0x79000000 */
	    insn = 0x79000000 | (((offset >> 1) & 0xfff) << 10) | (src << 5) | dest;
	} else if (offset >= -256 && offset < 256) {
	    /* STURH (unscaled): 0x78000000 */
	    insn = 0x78000000 | ((offset & 0x1ff) << 12) | (src << 5) | dest;
	} else {
	    arm64_set64(s, _x16, offset);
	    arm64_pstore(s, type, junk, dest, src, _x16);
	    return;
	}
	break;

    case DILL_I:
    case DILL_U:
	/* STR Wt: 32-bit store */
	if (offset >= 0 && (offset & 3) == 0 && offset < 16384) {
	    /* STR (unsigned offset, scaled by 4): 0xB9000000 */
	    insn = 0xB9000000 | (((offset >> 2) & 0xfff) << 10) | (src << 5) | dest;
	} else if (offset >= -256 && offset < 256) {
	    /* STUR (unscaled): 0xB8000000 */
	    insn = 0xB8000000 | ((offset & 0x1ff) << 12) | (src << 5) | dest;
	} else {
	    arm64_set64(s, _x16, offset);
	    arm64_pstore(s, type, junk, dest, src, _x16);
	    return;
	}
	break;

    case DILL_L:
    case DILL_UL:
    case DILL_P:
	/* STR Xt: 64-bit store */
	if (offset >= 0 && (offset & 7) == 0 && offset < 32768) {
	    /* STR (unsigned offset, scaled by 8): 0xF9000000 */
	    insn = 0xF9000000 | (((offset >> 3) & 0xfff) << 10) | (src << 5) | dest;
	} else if (offset >= -256 && offset < 256) {
	    /* STUR (unscaled): 0xF8000000 */
	    insn = 0xF8000000 | ((offset & 0x1ff) << 12) | (src << 5) | dest;
	} else {
	    arm64_set64(s, _x16, offset);
	    arm64_pstore(s, type, junk, dest, src, _x16);
	    return;
	}
	break;

    case DILL_F:
	/* STR St: 32-bit float store */
	if (offset >= 0 && (offset & 3) == 0 && offset < 16384) {
	    /* STR (unsigned offset, scaled by 4): 0xBD000000 */
	    insn = 0xBD000000 | (((offset >> 2) & 0xfff) << 10) | (src << 5) | dest;
	} else if (offset >= -256 && offset < 256) {
	    /* STUR (unscaled): 0xBC000000 */
	    insn = 0xBC000000 | ((offset & 0x1ff) << 12) | (src << 5) | dest;
	} else {
	    arm64_set64(s, _x16, offset);
	    arm64_pstore(s, type, junk, dest, src, _x16);
	    return;
	}
	break;

    case DILL_D:
	/* STR Dt: 64-bit double store */
	if (offset >= 0 && (offset & 7) == 0 && offset < 32768) {
	    /* STR (unsigned offset, scaled by 8): 0xFD000000 */
	    insn = 0xFD000000 | (((offset >> 3) & 0xfff) << 10) | (src << 5) | dest;
	} else if (offset >= -256 && offset < 256) {
	    /* STUR (unscaled): 0xFC000000 */
	    insn = 0xFC000000 | ((offset & 0x1ff) << 12) | (src << 5) | dest;
	} else {
	    arm64_set64(s, _x16, offset);
	    arm64_pstore(s, type, junk, dest, src, _x16);
	    return;
	}
	break;

    default:
	return;
    }
    INSN_OUT(s, insn);
}

void
arm64_pstore(dill_stream s, int type, int junk, int dest, int src1, int src2)
{
    /* Store with register offset: STR Rt, [Xn, Xm] */
    unsigned int insn;

    switch (type) {
    case DILL_C:
    case DILL_UC:
	/* STRB Wt, [Xn, Xm]: 0x38206800 */
	insn = 0x38206800 | (src2 << 16) | (src1 << 5) | dest;
	break;

    case DILL_S:
    case DILL_US:
	/* STRH Wt, [Xn, Xm]: 0x78206800 */
	insn = 0x78206800 | (src2 << 16) | (src1 << 5) | dest;
	break;

    case DILL_I:
    case DILL_U:
	/* STR Wt, [Xn, Xm]: 0xB8206800 */
	insn = 0xB8206800 | (src2 << 16) | (src1 << 5) | dest;
	break;

    case DILL_L:
    case DILL_UL:
    case DILL_P:
	/* STR Xt, [Xn, Xm]: 0xF8206800 */
	insn = 0xF8206800 | (src2 << 16) | (src1 << 5) | dest;
	break;

    case DILL_F:
	/* STR St, [Xn, Xm]: 0xBC206800 */
	insn = 0xBC206800 | (src2 << 16) | (src1 << 5) | dest;
	break;

    case DILL_D:
	/* STR Dt, [Xn, Xm]: 0xFC206800 */
	insn = 0xFC206800 | (src2 << 16) | (src1 << 5) | dest;
	break;

    default:
	return;
    }
    INSN_OUT(s, insn);
}

void
arm64_mov(dill_stream s, int type, int junk, int dest, int src)
{
    unsigned int insn;
    if (dest == src) return;

    switch (type) {
    case DILL_F:
	/* FMOV Sd, Sn = 0x1E204000 | (Sn << 5) | Sd */
	insn = 0x1E204000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;
    case DILL_D:
	/* FMOV Dd, Dn = 0x1E604000 | (Dn << 5) | Dd */
	insn = 0x1E604000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;
    default:
	/* MOV Xd, Xm using ORR Xd, XZR, Xm = 0xAA0003E0 */
	insn = 0xAA0003E0 | (src << 16) | dest;
	INSN_OUT(s, insn);
	break;
    }
}

void
arm64_pset(dill_stream s, int type, int junk, int dest, IMM_TYPE imm)
{
    arm64_set64(s, dest, (unsigned long)imm);
}

void
arm64_setf(dill_stream s, int type, int junk, int dest, double imm)
{
    unsigned int insn;

    if (type == DILL_F) {
	/* Convert double to float, then load into float register */
	union {
	    float f;
	    unsigned int i;
	} u;
	u.f = (float)imm;

	/* Load 32-bit pattern into x16 */
	arm64_set64(s, _x16, u.i);

	/* FMOV Sd, Wn = 0x1E270000 | (Wn << 5) | Sd */
	insn = 0x1E270000 | (_x16 << 5) | dest;
	INSN_OUT(s, insn);
    } else {
	/* Double: load 64-bit pattern into x16, then move to D register */
	union {
	    double d;
	    unsigned long l;
	} u;
	u.d = imm;

	/* Load 64-bit pattern into x16 */
	arm64_set64(s, _x16, u.l);

	/* FMOV Dd, Xn = 0x9E670000 | (Xn << 5) | Dd */
	insn = 0x9E670000 | (_x16 << 5) | dest;
	INSN_OUT(s, insn);
    }
}

void
arm64_setp(dill_stream s, int type, int junk, int dest, void *imm)
{
    arm64_set64(s, dest, (unsigned long)imm);
}

int
arm64_local(dill_stream s, int type)
{
    arm64_mach_info ami = (arm64_mach_info)s->p->mach_info;
    int size = arm64_type_size[type];
    int align = arm64_type_align[type];

    /* Align the allocation */
    ami->act_rec_size = (ami->act_rec_size + align - 1) & ~(align - 1);
    ami->act_rec_size += size;

    /* Return negative offset from frame pointer */
    return -ami->act_rec_size;
}

int
arm64_localb(dill_stream s, int size)
{
    arm64_mach_info ami = (arm64_mach_info)s->p->mach_info;
    if (size < 0) size = 0;

    /* Align to size (power of 2 up to 16) */
    int align = (size > 16) ? 16 : size;
    if (align > 0) {
	ami->act_rec_size = (ami->act_rec_size + align - 1) & ~(align - 1);
    }
    ami->act_rec_size += size;

    /* Return negative offset from frame pointer */
    return -ami->act_rec_size;
}

int
arm64_local_op(dill_stream s, int flag, int val)
{
    int size = val;
    if (flag == 0) {
	/* val is a type, get its size */
	size = arm64_type_size[val];
    }
    if (size < 0) size = 0;
    return arm64_localb(s, size);
}

/*
 * Save area layout (negative offsets from FP):
 * This area is used to save caller-saved registers around function calls.
 * We save argument registers (x0-x7, v0-v7) and temp registers (x9-x15)
 * since they may hold values that need to survive across calls in virtual mode.
 *
 * - Integer arg regs x0-x7: FP - (16 + reg*8) = FP-16 to FP-80 (64 bytes)
 * - Integer tmp regs x9-x15: FP - (96 + (reg-9)*8) = FP-96 to FP-152 (56 bytes)
 * - Float regs v0-v7: FP - (160 + reg*8) = FP-160 to FP-224 (64 bytes)
 *
 * STUR/LDUR use 9-bit signed immediate (-256 to +255), so all offsets fit.
 * Total save area needed: 224 bytes (16-byte aligned)
 */
#define GP_ARG_SAVE_OFFSET 16      /* x0-x7 save area starts at FP-16 */
#define GP_TMP_SAVE_OFFSET 96      /* x9-x15 save area starts at FP-96 */
#define FP_SAVE_AREA_OFFSET 160    /* v0-v7 save area starts at FP-160 */
#define CALLEE_SAVE_OFFSET 80      /* x19-x28 saved above FP (5 pairs * 16 bytes) */

void
arm64_save_restore_op(dill_stream s, int save_restore, int type, int reg)
{
    int offset;
    unsigned int insn;

    /* Callee-saved registers don't need saving by caller */
    switch (type) {
    case DILL_D:
    case DILL_F:
        /* v8-v15 are callee-saved (lower 64 bits) */
        if (reg >= _v8 && reg <= _v15) {
            return;
        }
        /* Only save v0-v7 (argument registers) */
        if (reg > _v7) {
            return;
        }
        offset = -(FP_SAVE_AREA_OFFSET + reg * 8);
        break;
    default:
        /* x19-x28 are callee-saved */
        if (reg >= _x19 && reg <= _x28) {
            return;
        }
        /* x29 (FP) and x30 (LR) are handled by prologue/epilogue */
        if (reg == _fp || reg == _lr) {
            return;
        }
        /* Skip x8, x16, x17, x18 - these are special or platform registers */
        if (reg == _x8 || reg >= _x16) {
            return;
        }
        /* Save x0-x7 (argument registers) at FP-16 to FP-80 */
        if (reg <= _x7) {
            offset = -(GP_ARG_SAVE_OFFSET + reg * 8);
        } else {
            /* Save x9-x15 (temp registers) at FP-96 to FP-152 */
            offset = -(GP_TMP_SAVE_OFFSET + (reg - _x9) * 8);
        }
        break;
    }

    if (save_restore == 0) {
        /* Save register to stack */
        switch (type) {
        case DILL_F:
            /* STR Sn, [FP, #offset] - unscaled offset */
            /* STUR Sn, [Xn, #simm9] = 0xBC000000 | (simm9 << 12) | (Xn << 5) | Sn */
            insn = 0xBC000000 | ((offset & 0x1FF) << 12) | (_fp << 5) | reg;
            INSN_OUT(s, insn);
            break;
        case DILL_D:
            /* STR Dn, [FP, #offset] - unscaled offset */
            /* STUR Dn, [Xn, #simm9] = 0xFC000000 | (simm9 << 12) | (Xn << 5) | Dn */
            insn = 0xFC000000 | ((offset & 0x1FF) << 12) | (_fp << 5) | reg;
            INSN_OUT(s, insn);
            break;
        default:
            /* STR Xn, [FP, #offset] - unscaled offset */
            /* STUR Xn, [Xn, #simm9] = 0xF8000000 | (simm9 << 12) | (Xn << 5) | Xn */
            insn = 0xF8000000 | ((offset & 0x1FF) << 12) | (_fp << 5) | reg;
            INSN_OUT(s, insn);
            break;
        }
    } else {
        /* Restore register from stack */
        switch (type) {
        case DILL_F:
            /* LDR Sn, [FP, #offset] - unscaled offset */
            /* LDUR Sn, [Xn, #simm9] = 0xBC400000 | (simm9 << 12) | (Xn << 5) | Sn */
            insn = 0xBC400000 | ((offset & 0x1FF) << 12) | (_fp << 5) | reg;
            INSN_OUT(s, insn);
            break;
        case DILL_D:
            /* LDR Dn, [FP, #offset] - unscaled offset */
            /* LDUR Dn, [Xn, #simm9] = 0xFC400000 | (simm9 << 12) | (Xn << 5) | Dn */
            insn = 0xFC400000 | ((offset & 0x1FF) << 12) | (_fp << 5) | reg;
            INSN_OUT(s, insn);
            break;
        default:
            /* LDR Xn, [FP, #offset] - unscaled offset */
            /* LDUR Xn, [Xn, #simm9] = 0xF8400000 | (simm9 << 12) | (Xn << 5) | Xn */
            insn = 0xF8400000 | ((offset & 0x1FF) << 12) | (_fp << 5) | reg;
            INSN_OUT(s, insn);
            break;
        }
    }
}

int
arm64_init_disassembly_info(dill_stream s, void *ptr)
{
    /* No disassembler yet */
    return 0;
}

int
arm64_print_insn(dill_stream s, void *info_ptr, void *insn)
{
    unsigned int *ip = (unsigned int *)insn;
    printf("%08x", *ip);
    return 4;
}

int
arm64_count_insn(dill_stream s, int start, int end)
{
    return (end - start) / 4;
}

void
arm64_print_reg(dill_stream s, int typ, int reg)
{
    if (reg == _sp) {
        printf("sp");
    } else if (reg == _xzr) {
        printf("xzr");
    } else if (reg < 32) {
        printf("x%d", reg);
    } else {
        printf("v%d", reg);
    }
}

/* Stub functions that may be called */
#define CONV(x,y) ((x*100)+y)

void arm64_convert(dill_stream s, int from_type, int to_type, int dest, int src)
{
    unsigned int insn;
    from_type &= 0xf;
    to_type &= 0xf;

    switch(CONV(from_type, to_type)) {
    /* Integer to integer - same size or simple truncation/widening */
    case CONV(DILL_I, DILL_L):   /* sign extend 32->64 */
    case CONV(DILL_I, DILL_UL):
    case CONV(DILL_I, DILL_P):
	/* SXTW Xd, Wn = SBFM Xd, Xn, #0, #31 = 0x93407C00 */
	insn = 0x93407C00 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    case CONV(DILL_I, DILL_U):   /* just move, same size */
    case CONV(DILL_U, DILL_I):
    case CONV(DILL_L, DILL_UL):
    case CONV(DILL_UL, DILL_L):
    case CONV(DILL_UL, DILL_P):
    case CONV(DILL_P, DILL_UL):
    case CONV(DILL_P, DILL_L):
    case CONV(DILL_L, DILL_P):
	if (src != dest) {
	    /* MOV Xd, Xn = ORR Xd, XZR, Xn = 0xAA0003E0 */
	    insn = 0xAA0003E0 | (src << 16) | dest;
	    INSN_OUT(s, insn);
	}
	break;

    case CONV(DILL_U, DILL_L):   /* zero extend 32->64 */
    case CONV(DILL_U, DILL_UL):
    case CONV(DILL_U, DILL_P):
	/* Use 32-bit mov which zero-extends to 64-bit */
	/* MOV Wd, Wn = ORR Wd, WZR, Wn = 0x2A0003E0 */
	insn = 0x2A0003E0 | (src << 16) | dest;
	INSN_OUT(s, insn);
	break;

    case CONV(DILL_L, DILL_I):   /* truncate 64->32 */
    case CONV(DILL_L, DILL_U):
    case CONV(DILL_UL, DILL_I):
    case CONV(DILL_UL, DILL_U):
    case CONV(DILL_P, DILL_I):
    case CONV(DILL_P, DILL_U):
	/* Use 32-bit mov which takes lower 32 bits */
	if (src != dest) {
	    insn = 0x2A0003E0 | (src << 16) | dest;
	    INSN_OUT(s, insn);
	}
	break;

    /* Sign extension from smaller integer types */
    case CONV(DILL_C, DILL_S):
    case CONV(DILL_C, DILL_US):
    case CONV(DILL_C, DILL_I):
    case CONV(DILL_C, DILL_U):
    case CONV(DILL_C, DILL_L):
    case CONV(DILL_C, DILL_UL):
    case CONV(DILL_C, DILL_P):
	/* SXTB Xd, Wn = SBFM Xd, Xn, #0, #7 = 0x93401C00 */
	insn = 0x93401C00 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    case CONV(DILL_S, DILL_I):
    case CONV(DILL_S, DILL_U):
    case CONV(DILL_S, DILL_L):
    case CONV(DILL_S, DILL_UL):
    case CONV(DILL_S, DILL_P):
	/* SXTH Xd, Wn = SBFM Xd, Xn, #0, #15 = 0x93403C00 */
	insn = 0x93403C00 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    /* Zero extension / truncation to unsigned char */
    case CONV(DILL_UC, DILL_C):
    case CONV(DILL_UC, DILL_S):
    case CONV(DILL_UC, DILL_US):
    case CONV(DILL_UC, DILL_I):
    case CONV(DILL_UC, DILL_U):
    case CONV(DILL_UC, DILL_L):
    case CONV(DILL_UC, DILL_UL):
    case CONV(DILL_UC, DILL_P):
    case CONV(DILL_S, DILL_UC):
    case CONV(DILL_US, DILL_UC):
    case CONV(DILL_I, DILL_UC):
    case CONV(DILL_U, DILL_UC):
    case CONV(DILL_L, DILL_UC):
    case CONV(DILL_UL, DILL_UC):
    case CONV(DILL_C, DILL_UC):
	/* AND with 0xFF: UXTB Wd, Wn = UBFM Wd, Wn, #0, #7 = 0x53001C00 */
	insn = 0x53001C00 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    /* Sign extension / truncation to signed char */
    case CONV(DILL_I, DILL_C):
    case CONV(DILL_U, DILL_C):
    case CONV(DILL_L, DILL_C):
    case CONV(DILL_UL, DILL_C):
    case CONV(DILL_S, DILL_C):
    case CONV(DILL_US, DILL_C):
	/* SXTB Xd, Wn = SBFM Xd, Xn, #0, #7 = 0x93401C00 */
	insn = 0x93401C00 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    /* Zero extension / truncation to unsigned short */
    case CONV(DILL_US, DILL_S):
    case CONV(DILL_US, DILL_I):
    case CONV(DILL_US, DILL_U):
    case CONV(DILL_US, DILL_L):
    case CONV(DILL_US, DILL_UL):
    case CONV(DILL_US, DILL_P):
    case CONV(DILL_I, DILL_US):
    case CONV(DILL_U, DILL_US):
    case CONV(DILL_L, DILL_US):
    case CONV(DILL_UL, DILL_US):
    case CONV(DILL_S, DILL_US):
	/* AND with 0xFFFF: UXTH Wd, Wn = UBFM Wd, Wn, #0, #15 = 0x53003C00 */
	insn = 0x53003C00 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    /* Sign extension / truncation to signed short */
    case CONV(DILL_I, DILL_S):
    case CONV(DILL_U, DILL_S):
    case CONV(DILL_L, DILL_S):
    case CONV(DILL_UL, DILL_S):
	/* SXTH Xd, Wn = SBFM Xd, Xn, #0, #15 = 0x93403C00 */
	insn = 0x93403C00 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    /* Float to Double */
    case CONV(DILL_F, DILL_D):
	/* FCVT Dd, Sn = 0x1E22C000 */
	insn = 0x1E22C000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    /* Double to Float */
    case CONV(DILL_D, DILL_F):
	/* FCVT Sd, Dn = 0x1E624000 */
	insn = 0x1E624000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    /* Signed integer to Float */
    case CONV(DILL_C, DILL_F):
    case CONV(DILL_S, DILL_F):
    case CONV(DILL_I, DILL_F):
	/* SCVTF Sd, Wn = 0x1E220000 */
	insn = 0x1E220000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    case CONV(DILL_L, DILL_F):
	/* SCVTF Sd, Xn = 0x9E220000 */
	insn = 0x9E220000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    /* Unsigned integer to Float */
    case CONV(DILL_UC, DILL_F):
    case CONV(DILL_US, DILL_F):
    case CONV(DILL_U, DILL_F):
	/* UCVTF Sd, Wn = 0x1E230000 */
	insn = 0x1E230000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    case CONV(DILL_UL, DILL_F):
    case CONV(DILL_P, DILL_F):
	/* UCVTF Sd, Xn = 0x9E230000 */
	insn = 0x9E230000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    /* Signed integer to Double */
    case CONV(DILL_C, DILL_D):
    case CONV(DILL_S, DILL_D):
    case CONV(DILL_I, DILL_D):
	/* SCVTF Dd, Wn = 0x1E620000 */
	insn = 0x1E620000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    case CONV(DILL_L, DILL_D):
	/* SCVTF Dd, Xn = 0x9E620000 */
	insn = 0x9E620000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    /* Unsigned integer to Double */
    case CONV(DILL_UC, DILL_D):
    case CONV(DILL_US, DILL_D):
    case CONV(DILL_U, DILL_D):
	/* UCVTF Dd, Wn = 0x1E630000 */
	insn = 0x1E630000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    case CONV(DILL_UL, DILL_D):
    case CONV(DILL_P, DILL_D):
	/* UCVTF Dd, Xn = 0x9E630000 */
	insn = 0x9E630000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    /* Float to signed integer */
    case CONV(DILL_F, DILL_C):
    case CONV(DILL_F, DILL_S):
    case CONV(DILL_F, DILL_I):
	/* FCVTZS Wd, Sn = 0x1E380000 */
	insn = 0x1E380000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    case CONV(DILL_F, DILL_L):
	/* FCVTZS Xd, Sn = 0x9E380000 */
	insn = 0x9E380000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    /* Float to unsigned integer */
    case CONV(DILL_F, DILL_UC):
    case CONV(DILL_F, DILL_US):
    case CONV(DILL_F, DILL_U):
	/* FCVTZU Wd, Sn = 0x1E390000 */
	insn = 0x1E390000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    case CONV(DILL_F, DILL_UL):
    case CONV(DILL_F, DILL_P):
	/* FCVTZU Xd, Sn = 0x9E390000 */
	insn = 0x9E390000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    /* Double to signed integer */
    case CONV(DILL_D, DILL_C):
    case CONV(DILL_D, DILL_S):
    case CONV(DILL_D, DILL_I):
	/* FCVTZS Wd, Dn = 0x1E780000 */
	insn = 0x1E780000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    case CONV(DILL_D, DILL_L):
	/* FCVTZS Xd, Dn = 0x9E780000 */
	insn = 0x9E780000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    /* Double to unsigned integer */
    case CONV(DILL_D, DILL_UC):
    case CONV(DILL_D, DILL_US):
    case CONV(DILL_D, DILL_U):
	/* FCVTZU Wd, Dn = 0x1E790000 */
	insn = 0x1E790000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    case CONV(DILL_D, DILL_UL):
    case CONV(DILL_D, DILL_P):
	/* FCVTZU Xd, Dn = 0x9E790000 */
	insn = 0x9E790000 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    default:
	/* No conversion needed or not implemented */
	if (src != dest) {
	    /* MOV Xd, Xn */
	    insn = 0xAA0003E0 | (src << 16) | dest;
	    INSN_OUT(s, insn);
	}
	break;
    }
}
/*
 * ARM64 condition codes for B.cond:
 * EQ=0x0, NE=0x1, CS/HS=0x2, CC/LO=0x3, MI=0x4, PL=0x5,
 * VS=0x6, VC=0x7, HI=0x8, LS=0x9, GE=0xA, LT=0xB, GT=0xC, LE=0xD
 *
 * dill comparison codes:
 * dill_eq_code=0, dill_ge_code=1, dill_gt_code=2, dill_le_code=3, dill_lt_code=4, dill_ne_code=5
 */
static int arm64_conds_signed[] = {
    0x0,  /* dill_eq_code -> EQ */
    0xA,  /* dill_ge_code -> GE (signed) */
    0xC,  /* dill_gt_code -> GT (signed) */
    0xD,  /* dill_le_code -> LE (signed) */
    0xB,  /* dill_lt_code -> LT (signed) */
    0x1,  /* dill_ne_code -> NE */
};

static int arm64_conds_unsigned[] = {
    0x0,  /* dill_eq_code -> EQ */
    0x2,  /* dill_ge_code -> CS/HS (unsigned >=) */
    0x8,  /* dill_gt_code -> HI (unsigned >) */
    0x9,  /* dill_le_code -> LS (unsigned <=) */
    0x3,  /* dill_lt_code -> CC/LO (unsigned <) */
    0x1,  /* dill_ne_code -> NE */
};

/* Float comparisons use signed condition codes */
static int arm64_conds_float[] = {
    0x0,  /* dill_eq_code -> EQ */
    0xA,  /* dill_ge_code -> GE */
    0xC,  /* dill_gt_code -> GT */
    0xD,  /* dill_le_code -> LE */
    0xB,  /* dill_lt_code -> LT */
    0x1,  /* dill_ne_code -> NE */
};

void arm64_branch(dill_stream s, int op, int type, int src1, int src2, int label)
{
    unsigned int insn;
    int cond;
    int is_float = (type == DILL_F || type == DILL_D);
    int is_unsigned = (type == DILL_UC || type == DILL_US || type == DILL_U || type == DILL_UL || type == DILL_P);

    if (is_float) {
	/* FCMP */
	if (type == DILL_D) {
	    /* FCMP Dn, Dm = 0x1E602000 */
	    insn = 0x1E602000 | (src2 << 16) | (src1 << 5);
	} else {
	    /* FCMP Sn, Sm = 0x1E202000 */
	    insn = 0x1E202000 | (src2 << 16) | (src1 << 5);
	}
	INSN_OUT(s, insn);
	cond = arm64_conds_float[op];
    } else {
	/* Integer CMP */
	int is_64bit = (type == DILL_L || type == DILL_UL || type == DILL_P);
	if (is_64bit) {
	    /* CMP Xn, Xm = SUBS XZR, Xn, Xm = 0xEB00001F */
	    insn = 0xEB00001F | (src2 << 16) | (src1 << 5);
	} else {
	    /* CMP Wn, Wm = SUBS WZR, Wn, Wm = 0x6B00001F */
	    insn = 0x6B00001F | (src2 << 16) | (src1 << 5);
	}
	INSN_OUT(s, insn);
	cond = is_unsigned ? arm64_conds_unsigned[op] : arm64_conds_signed[op];
    }

    /* Mark branch location for later fixup */
    dill_mark_branch_location(s, label);

    /* B.cond with zero offset (will be patched) = 0x54000000 | cond */
    insn = 0x54000000 | cond;
    INSN_OUT(s, insn);
}

void arm64_branchi(dill_stream s, int op, int type, int src, IMM_TYPE imm, int label)
{
    unsigned int insn;
    int cond;
    int is_unsigned = (type == DILL_UC || type == DILL_US || type == DILL_U || type == DILL_UL || type == DILL_P);
    int is_64bit = (type == DILL_L || type == DILL_UL || type == DILL_P);

    /* Load immediate into temp register and compare */
    if (imm >= 0 && imm < 4096) {
	/* CMP with immediate: CMP Rn, #imm */
	if (is_64bit) {
	    /* CMP Xn, #imm = SUBS XZR, Xn, #imm = 0xF100001F */
	    insn = 0xF100001F | ((imm & 0xfff) << 10) | (src << 5);
	} else {
	    /* CMP Wn, #imm = SUBS WZR, Wn, #imm = 0x7100001F */
	    insn = 0x7100001F | ((imm & 0xfff) << 10) | (src << 5);
	}
	INSN_OUT(s, insn);
    } else {
	/* Load imm into x16 and compare */
	arm64_set64(s, _x16, imm);
	if (is_64bit) {
	    insn = 0xEB00001F | (_x16 << 16) | (src << 5);
	} else {
	    insn = 0x6B00001F | (_x16 << 16) | (src << 5);
	}
	INSN_OUT(s, insn);
    }

    cond = is_unsigned ? arm64_conds_unsigned[op] : arm64_conds_signed[op];

    /* Mark branch location for later fixup */
    dill_mark_branch_location(s, label);

    /* B.cond with zero offset (will be patched) = 0x54000000 | cond */
    insn = 0x54000000 | cond;
    INSN_OUT(s, insn);
}

void arm64_compare(dill_stream s, int op, int type, int dest, int src1, int src2)
{
    unsigned int insn;
    int cond;
    int is_float = (type == DILL_F || type == DILL_D);
    int is_unsigned = (type == DILL_UC || type == DILL_US || type == DILL_U || type == DILL_UL || type == DILL_P);

    /* Set dest to 0 first */
    /* MOV Xd, XZR = ORR Xd, XZR, XZR = 0xAA1F03E0 */
    insn = 0xAA1F03E0 | dest;
    INSN_OUT(s, insn);

    if (is_float) {
	/* FCMP */
	if (type == DILL_D) {
	    insn = 0x1E602000 | (src2 << 16) | (src1 << 5);
	} else {
	    insn = 0x1E202000 | (src2 << 16) | (src1 << 5);
	}
	INSN_OUT(s, insn);
	cond = arm64_conds_float[op];
    } else {
	/* Integer CMP */
	int is_64bit = (type == DILL_L || type == DILL_UL || type == DILL_P);
	if (is_64bit) {
	    insn = 0xEB00001F | (src2 << 16) | (src1 << 5);
	} else {
	    insn = 0x6B00001F | (src2 << 16) | (src1 << 5);
	}
	INSN_OUT(s, insn);
	cond = is_unsigned ? arm64_conds_unsigned[op] : arm64_conds_signed[op];
    }

    /* CSET Xd, cond = CSINC Xd, XZR, XZR, invert(cond) = 0x9A9F07E0 | (invert_cond << 12) | dest */
    /* invert_cond is cond XOR 1 for most conditions */
    int invert_cond = cond ^ 1;
    insn = 0x9A9F07E0 | (invert_cond << 12) | dest;
    INSN_OUT(s, insn);
}

void arm64_comparei(dill_stream s, int op, int type, int dest, int src, IMM_TYPE imm)
{
    unsigned int insn;
    int cond;
    int is_unsigned = (type == DILL_UC || type == DILL_US || type == DILL_U || type == DILL_UL || type == DILL_P);
    int is_64bit = (type == DILL_L || type == DILL_UL || type == DILL_P);

    /* Set dest to 0 first */
    insn = 0xAA1F03E0 | dest;
    INSN_OUT(s, insn);

    /* Compare */
    if (imm >= 0 && imm < 4096) {
	if (is_64bit) {
	    insn = 0xF100001F | ((imm & 0xfff) << 10) | (src << 5);
	} else {
	    insn = 0x7100001F | ((imm & 0xfff) << 10) | (src << 5);
	}
	INSN_OUT(s, insn);
    } else {
	arm64_set64(s, _x16, imm);
	if (is_64bit) {
	    insn = 0xEB00001F | (_x16 << 16) | (src << 5);
	} else {
	    insn = 0x6B00001F | (_x16 << 16) | (src << 5);
	}
	INSN_OUT(s, insn);
    }

    cond = is_unsigned ? arm64_conds_unsigned[op] : arm64_conds_signed[op];

    /* CSET */
    int invert_cond = cond ^ 1;
    insn = 0x9A9F07E0 | (invert_cond << 12) | dest;
    INSN_OUT(s, insn);
}
void arm64_jump_to_label(dill_stream s, unsigned long label)
{
    /* Mark branch location for later fixup */
    dill_mark_branch_location(s, label);

    /* B (unconditional branch) with zero offset = 0x14000000 */
    INSN_OUT(s, 0x14000000);
}
void arm64_jump_to_reg(dill_stream s, unsigned long reg)
{
    /* BR Xn = 0xD61F0000 | (Xn << 5) */
    INSN_OUT(s, 0xD61F0000 | (reg << 5));
}

void arm64_jump_to_imm(dill_stream s, void *imm)
{
    /* Load address into x16, then branch */
    arm64_set64(s, _x16, (unsigned long)imm);
    /* BR x16 */
    INSN_OUT(s, 0xD61F0000 | (_x16 << 5));
}

void arm64_jal(dill_stream s, int return_addr_reg, int target)
{
    /* BLR Xn = 0xD63F0000 | (Xn << 5) */
    INSN_OUT(s, 0xD63F0000 | (target << 5));
}

/*
 * Push/Call implementation for ARM64 calling convention (AAPCS64)
 * - x0-x7: first 8 integer/pointer arguments
 * - v0-v7: first 8 floating-point arguments
 * - Stack for overflow arguments
 */

/* Stack layout for calls:
 * [sp + 128]: saved LR
 * [sp + 0] to [sp + 120]: overflow arguments (up to 16)
 * Total: 144 bytes (must be 16-byte aligned, so 144 rounds to 144)
 */
#define CALL_STACK_SIZE 144

static void push_init(dill_stream s)
{
    arm64_mach_info ami = (arm64_mach_info)s->p->mach_info;
    ami->cur_arg_offset = 0;
    ami->next_core_register = _x0;
    ami->next_float_register = _v0;
    ami->varidiac_call = 0;
    ami->non_var_args = 0;
    ami->pushed_args = 0;
    ami->max_arg_size = 0;

    /* Pre-allocate stack space for arguments + LR save
     * SUB sp, sp, #144
     */
    INSN_OUT(s, 0xD10243FF);  /* SUB sp, sp, #144 */
}

static void internal_push(dill_stream s, int type, int immediate, void *value_ptr)
{
    arm64_mach_info ami = (arm64_mach_info)s->p->mach_info;
    struct arg_info arg;
    int stack_offset;
    int slot_size;
    int is_variadic_anon = 0;  /* True if this is an anonymous variadic argument */

    ami->pushed_args++;
    arg.is_immediate = immediate;
    arg.type = type;  /* Keep original type for proper stack slot sizing */

    if (s->dill_debug) {
        printf("internal_push: type=%d, imm=%d, pushed_args=%d, next_core=%d, next_float=%d\n",
               type, immediate, ami->pushed_args, ami->next_core_register, ami->next_float_register);
    }

    /* Check if this is an anonymous argument in a variadic call */
    if (ami->varidiac_call && ami->pushed_args > ami->non_var_args) {
        is_variadic_anon = 1;
    }

    /* Determine register or stack placement based on type */
    switch (type) {
    case DILL_C: case DILL_UC: case DILL_S: case DILL_US:
    case DILL_I: case DILL_U: case DILL_L: case DILL_UL: case DILL_P:
#ifdef __APPLE__
        /* On Apple ARM64, variadic anonymous args go on stack, not registers */
        if (is_variadic_anon) {
            arg.is_register = 0;
        } else
#endif
        if (ami->next_core_register <= _x7) {
            arg.is_register = 1;
            arg.in_reg = ami->next_core_register;
            arg.out_reg = ami->next_core_register;
            ami->next_core_register++;
        } else {
            arg.is_register = 0;
        }
        break;
    case DILL_F:
    case DILL_D:
#ifdef __APPLE__
        /* On Apple ARM64, variadic anonymous args go on stack, not registers */
        if (is_variadic_anon) {
            arg.is_register = 0;
        } else
#endif
        /* For variadic anonymous args on non-Apple, float/double go in integer registers */
        if (is_variadic_anon) {
            if (ami->next_core_register <= _x7) {
                arg.is_register = 1;
                arg.in_reg = ami->next_core_register;
                arg.out_reg = ami->next_core_register;
                ami->next_core_register++;
            } else {
                arg.is_register = 0;
            }
        } else if (ami->next_float_register <= _v7) {
            arg.is_register = 1;
            arg.in_reg = ami->next_float_register;
            arg.out_reg = ami->next_float_register;
            ami->next_float_register++;
        } else {
            arg.is_register = 0;
        }
        break;
    default:
        arg.is_register = 0;
        break;
    }

    /* Determine stack slot size based on type */
#ifdef __APPLE__
    /* On Apple ARM64, variadic args are all promoted to 64-bit on stack */
    if (is_variadic_anon) {
        slot_size = 8;
    } else
#endif
    switch (type) {
    case DILL_C: case DILL_UC: case DILL_S: case DILL_US:
    case DILL_I: case DILL_U: case DILL_F:
        slot_size = 4;  /* 32-bit types use 4-byte slots */
        break;
    case DILL_L: case DILL_UL: case DILL_P: case DILL_D:
    default:
        slot_size = 8;  /* 64-bit types use 8-byte slots */
        break;
    }

    /* Only track stack offset for stack arguments */
    if (arg.is_register == 0) {
        /* Align offset for 64-bit types */
        if (slot_size == 8 && (ami->cur_arg_offset & 7)) {
            ami->cur_arg_offset = (ami->cur_arg_offset + 7) & ~7;
        }
        stack_offset = ami->cur_arg_offset;
        ami->cur_arg_offset += slot_size;
        if (ami->cur_arg_offset > ami->max_arg_size) {
            ami->max_arg_size = ami->cur_arg_offset;
        }
    }

    if (arg.is_register == 0) {
        /* Store on stack - offset is relative to where SP will be after we
         * allocate space. We'll store at [SP + stack_offset] */
        if (immediate) {
            switch (arg.type) {
            case DILL_C: case DILL_UC: case DILL_S: case DILL_US:
            case DILL_I: case DILL_U:
                arm64_set64(s, _x16, *(unsigned long*)value_ptr);
#ifdef __APPLE__
                /* On Apple ARM64, variadic args are stored as 64-bit values */
                if (is_variadic_anon) {
                    /* STUR Xn, [sp, #offset] = 0xF8000000 */
                    if (stack_offset >= 0 && stack_offset < 256) {
                        unsigned int insn = 0xF8000000 | ((stack_offset & 0x1FF) << 12) |
                                           (_sp << 5) | _x16;
                        INSN_OUT(s, insn);
                    } else {
                        arm64_set64(s, _x17, stack_offset);
                        INSN_OUT(s, 0x8B110000 | (_sp << 5) | _x17 | (_x17 << 16));
                        INSN_OUT(s, 0xF9000000 | (_x17 << 5) | _x16);
                    }
                } else
#endif
                {
                    /* 32-bit integer immediate - store as 32-bit */
                    /* STUR Wn, [sp, #offset] = 0xB8000000 */
                    if (stack_offset >= 0 && stack_offset < 256) {
                        unsigned int insn = 0xB8000000 | ((stack_offset & 0x1FF) << 12) |
                                           (_sp << 5) | _x16;
                        INSN_OUT(s, insn);
                    } else {
                        arm64_set64(s, _x17, stack_offset);
                        INSN_OUT(s, 0x8B110000 | (_sp << 5) | _x17 | (_x17 << 16));
                        /* STR Wn, [x17] = 0xB9000000 */
                        INSN_OUT(s, 0xB9000000 | (_x17 << 5) | _x16);
                    }
                }
                break;
            case DILL_L:
            case DILL_UL:
            case DILL_P:
                arm64_set64(s, _x16, *(unsigned long*)value_ptr);
                /* STUR Xn, [sp, #offset] = 0xF8000000 */
                if (stack_offset >= 0 && stack_offset < 256) {
                    unsigned int insn = 0xF8000000 | ((stack_offset & 0x1FF) << 12) |
                                       (_sp << 5) | _x16;
                    INSN_OUT(s, insn);
                } else {
                    arm64_set64(s, _x17, stack_offset);
                    INSN_OUT(s, 0x8B110000 | (_sp << 5) | _x17 | (_x17 << 16));
                    INSN_OUT(s, 0xF9000000 | (_x17 << 5) | _x16);
                }
                break;
            case DILL_F:
                arm64_setf(s, arg.type, 0, _v16, *(double*)value_ptr);
                /* STUR Sn, [sp, #offset] = 0xBC000000 */
                {
                    unsigned int insn = 0xBC000000 | ((stack_offset & 0x1FF) << 12) |
                                       (_sp << 5) | _v16;
                    INSN_OUT(s, insn);
                }
                break;
            case DILL_D:
                arm64_setf(s, arg.type, 0, _v16, *(double*)value_ptr);
                /* STUR Dn, [sp, #offset] = 0xFC000000 */
                {
                    unsigned int insn = 0xFC000000 | ((stack_offset & 0x1FF) << 12) |
                                       (_sp << 5) | _v16;
                    INSN_OUT(s, insn);
                }
                break;
            }
        } else {
            int reg = *(int*)value_ptr;
            switch (arg.type) {
            case DILL_C: case DILL_UC: case DILL_S: case DILL_US:
            case DILL_I: case DILL_U:
#ifdef __APPLE__
                /* On Apple ARM64, variadic args are stored as 64-bit values */
                if (is_variadic_anon) {
                    /* STUR Xn, [sp, #offset] = 0xF8000000 */
                    if (stack_offset >= 0 && stack_offset < 256) {
                        unsigned int insn = 0xF8000000 | ((stack_offset & 0x1FF) << 12) |
                                           (_sp << 5) | reg;
                        INSN_OUT(s, insn);
                    } else {
                        arm64_set64(s, _x17, stack_offset);
                        INSN_OUT(s, 0x8B110000 | (_sp << 5) | _x17 | (_x17 << 16));
                        INSN_OUT(s, 0xF9000000 | (_x17 << 5) | reg);
                    }
                } else
#endif
                {
                    /* 32-bit integer register - store as 32-bit */
                    /* STUR Wn, [sp, #offset] = 0xB8000000 */
                    if (stack_offset >= 0 && stack_offset < 256) {
                        unsigned int insn = 0xB8000000 | ((stack_offset & 0x1FF) << 12) |
                                           (_sp << 5) | reg;
                        INSN_OUT(s, insn);
                    } else {
                        arm64_set64(s, _x17, stack_offset);
                        INSN_OUT(s, 0x8B110000 | (_sp << 5) | _x17 | (_x17 << 16));
                        INSN_OUT(s, 0xB9000000 | (_x17 << 5) | reg);
                    }
                }
                break;
            case DILL_L:
            case DILL_UL:
            case DILL_P:
                /* STUR Xn, [sp, #offset] = 0xF8000000 */
                if (stack_offset >= 0 && stack_offset < 256) {
                    unsigned int insn = 0xF8000000 | ((stack_offset & 0x1FF) << 12) |
                                       (_sp << 5) | reg;
                    INSN_OUT(s, insn);
                } else {
                    arm64_set64(s, _x17, stack_offset);
                    INSN_OUT(s, 0x8B110000 | (_sp << 5) | _x17 | (_x17 << 16));
                    INSN_OUT(s, 0xF9000000 | (_x17 << 5) | reg);
                }
                break;
            case DILL_F:
                /* STUR Sn, [sp, #offset] = 0xBC000000 */
                {
                    unsigned int insn = 0xBC000000 | ((stack_offset & 0x1FF) << 12) |
                                       (_sp << 5) | reg;
                    INSN_OUT(s, insn);
                }
                break;
            case DILL_D:
                /* STUR Dn, [sp, #offset] = 0xFC000000 */
                {
                    unsigned int insn = 0xFC000000 | ((stack_offset & 0x1FF) << 12) |
                                       (_sp << 5) | reg;
                    INSN_OUT(s, insn);
                }
                break;
            }
        }
    } else {
        /* Put in register */
        int target_reg = arg.out_reg;
        if (s->dill_debug) {
            printf("  -> register target_reg=%d, is_register=%d, immediate=%d\n",
                   target_reg, arg.is_register, immediate);
        }
        if (immediate) {
            switch (arg.type) {
            case DILL_C: case DILL_UC: case DILL_S: case DILL_US:
            case DILL_I: case DILL_U:
            case DILL_L: case DILL_UL: case DILL_P:
                /* All integer types can use set64 - value fits in register */
                if (s->dill_debug) {
                    printf("  -> set64 target=%d value=%lu\n", target_reg, *(unsigned long*)value_ptr);
                }
                arm64_set64(s, target_reg, *(unsigned long*)value_ptr);
                break;
            case DILL_F:
            case DILL_D:
                if (is_variadic_anon) {
                    /* Variadic anonymous float/double: load into FP temp, then FMOV to integer reg */
                    /* Float is promoted to double in variadic calls */
                    arm64_setf(s, DILL_D, 0, _v16, *(double*)value_ptr);
                    /* FMOV Xd, Dn = 0x9E660000 | (Rn << 5) | Rd */
                    INSN_OUT(s, 0x9E660000 | (_v16 << 5) | target_reg);
                } else {
                    arm64_setf(s, arg.type, 0, target_reg, *(double*)value_ptr);
                }
                break;
            }
        } else {
            int src_reg = *(int*)value_ptr;
            if (s->dill_debug) {
                printf("  -> mov from src_reg=%d to target_reg=%d\n", src_reg, target_reg);
            }
            switch (arg.type) {
            case DILL_C: case DILL_UC: case DILL_S: case DILL_US:
            case DILL_I: case DILL_U:
            case DILL_L: case DILL_UL: case DILL_P:
                if (src_reg != target_reg) {
                    arm64_mov(s, DILL_L, 0, target_reg, src_reg);
                }
                break;
            case DILL_F:
                if (is_variadic_anon) {
                    /* Variadic anonymous float: convert to double, then FMOV to integer reg */
                    /* FCVT Dd, Sn = 0x1E22C000 | (Rn << 5) | Rd */
                    INSN_OUT(s, 0x1E22C000 | (src_reg << 5) | _v16);
                    /* FMOV Xd, Dn = 0x9E660000 | (Rn << 5) | Rd */
                    INSN_OUT(s, 0x9E660000 | (_v16 << 5) | target_reg);
                } else if (src_reg != target_reg) {
                    arm64_mov(s, DILL_F, 0, target_reg, src_reg);
                }
                break;
            case DILL_D:
                if (is_variadic_anon) {
                    /* Variadic anonymous double: FMOV to integer reg */
                    /* FMOV Xd, Dn = 0x9E660000 | (Rn << 5) | Rd */
                    INSN_OUT(s, 0x9E660000 | (src_reg << 5) | target_reg);
                } else if (src_reg != target_reg) {
                    arm64_mov(s, DILL_D, 0, target_reg, src_reg);
                }
                break;
            }
        }
    }
}

void arm64_push(dill_stream s, int type, int reg)
{
    arm64_mach_info ami = (arm64_mach_info)s->p->mach_info;
    if ((type == DILL_V) && (reg <= -1)) {
        push_init(s);
        if (reg <= -2) {
            ami->varidiac_call = 1;
            /* Extract non_var_args count: push_varidiac_init calls push(s, DILL_V, -2-count) */
            ami->non_var_args = -(reg + 2);
        }
    } else {
        internal_push(s, type, 0, &reg);
    }
}

void arm64_pushi(dill_stream s, int type, IMM_TYPE value)
{
    internal_push(s, type, 1, &value);
}

void arm64_pushpi(dill_stream s, int type, void *value)
{
    internal_push(s, type, 1, &value);
}

void arm64_pushfi(dill_stream s, int type, double value)
{
    internal_push(s, type, 1, &value);
}

/*
 * Save/restore caller-saved temporary registers around function calls.
 * This is needed because virtual mode may allocate live virtual registers
 * to temp registers (x9-x15), which would otherwise be clobbered by calls.
 */
static void
arm64_saverestore_temps(dill_stream s, int saverestore)
{
    int i;
    /* Save/restore integer temp registers x9-x15 */
    for (i = _t0; i <= _t6; i++) {
        if (dill_mustsave(&s->p->tmp_i, i)) {
            arm64_save_restore_op(s, saverestore, DILL_L, i);
        }
    }
    /* Save/restore float temp registers v0-v7 (if used) */
    for (i = _v0; i <= _v7; i++) {
        if (dill_mustsave(&s->p->tmp_f, i)) {
            arm64_save_restore_op(s, saverestore, DILL_D, i);
        }
    }
}

int arm64_calli(dill_stream s, int type, void *xfer_address, const char *name)
{
    int caller_side_ret_reg = _x0;

    /* Save caller-saved temp registers before call */
    arm64_saverestore_temps(s, 0);

    /* Note: LR is already saved in prologue at FP+8. We don't need to save it
     * again here. The BLR will set LR to our return address, and the callee
     * will return to us. Our original LR (return to our caller) is safe.
     */

    /* Mark the call location for potential relocation */
    dill_mark_call_location(s, name, xfer_address);

    /* Load address into x16 (IP0, a scratch register for calls) */
    arm64_set64(s, _x16, (unsigned long)xfer_address);

    /* BLR x16 = 0xD63F0000 | (x16 << 5) */
    INSN_OUT(s, 0xD63F0000 | (_x16 << 5));

    /* Deallocate stack space: ADD sp, sp, #144 */
    INSN_OUT(s, 0x910243FF);

    /* Restore caller-saved temp registers after call */
    arm64_saverestore_temps(s, 1);

    /* Return value is in x0 (integer) or v0 (float/double) */
    if ((type == DILL_D) || (type == DILL_F)) {
        caller_side_ret_reg = _v0;
    }

    /* Reset push state (but don't re-allocate stack space) */
    {
        arm64_mach_info ami = (arm64_mach_info)s->p->mach_info;
        ami->cur_arg_offset = 0;
        ami->next_core_register = _x0;
        ami->next_float_register = _v0;
        ami->varidiac_call = 0;
        ami->max_arg_size = 0;
    }

    return caller_side_ret_reg;
}

int arm64_callr(dill_stream s, int type, int src)
{
    int caller_side_ret_reg = _x0;

    /* Save caller-saved temp registers before call */
    arm64_saverestore_temps(s, 0);

    /* Note: LR is already saved in prologue at FP+8 - no need to save again */

    /* BLR Xn = 0xD63F0000 | (Xn << 5) */
    INSN_OUT(s, 0xD63F0000 | (src << 5));

    /* Deallocate stack space: ADD sp, sp, #144 */
    INSN_OUT(s, 0x910243FF);

    /* Restore caller-saved temp registers after call */
    arm64_saverestore_temps(s, 1);

    /* Return value is in x0 (integer) or v0 (float/double) */
    if ((type == DILL_D) || (type == DILL_F)) {
        caller_side_ret_reg = _v0;
    }

    /* Reset push state (but don't re-allocate stack space) */
    {
        arm64_mach_info ami = (arm64_mach_info)s->p->mach_info;
        ami->cur_arg_offset = 0;
        ami->next_core_register = _x0;
        ami->next_float_register = _v0;
        ami->varidiac_call = 0;
        ami->max_arg_size = 0;
    }

    return caller_side_ret_reg;
}
void arm64_bswap(dill_stream s, int data1, int data2, int dest, int src)
{
    /* data1 = type (from arm64.ops "T" parameter) */
    unsigned int insn;

    switch (data1) {
    case DILL_S:
    case DILL_US:
	/* 16-bit byte swap: REV16 Wd, Wn = 0x5AC00400 */
	insn = 0x5AC00400 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    case DILL_I:
    case DILL_U:
	/* 32-bit byte swap: REV Wd, Wn = 0x5AC00800 */
	insn = 0x5AC00800 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    case DILL_L:
    case DILL_UL:
	/* 64-bit byte swap: REV Xd, Xn = 0xDAC00C00 */
	insn = 0xDAC00C00 | (src << 5) | dest;
	INSN_OUT(s, insn);
	break;

    case DILL_F:
	/* Float: move to int reg, swap, move back */
	/* FMOV Wd, Sn = 0x1E260000 | (Sn << 5) | Wd */
	insn = 0x1E260000 | (src << 5) | _x16;
	INSN_OUT(s, insn);
	/* REV Wd, Wn = 0x5AC00800 */
	insn = 0x5AC00800 | (_x16 << 5) | _x16;
	INSN_OUT(s, insn);
	/* FMOV Sd, Wn = 0x1E270000 | (Wn << 5) | Sd */
	insn = 0x1E270000 | (_x16 << 5) | dest;
	INSN_OUT(s, insn);
	break;

    case DILL_D:
	/* Double: move to int reg, swap, move back */
	/* FMOV Xd, Dn = 0x9E660000 | (Dn << 5) | Xd */
	insn = 0x9E660000 | (src << 5) | _x16;
	INSN_OUT(s, insn);
	/* REV Xd, Xn = 0xDAC00C00 */
	insn = 0xDAC00C00 | (_x16 << 5) | _x16;
	INSN_OUT(s, insn);
	/* FMOV Dd, Xn = 0x9E670000 | (Xn << 5) | Dd */
	insn = 0x9E670000 | (_x16 << 5) | dest;
	INSN_OUT(s, insn);
	break;
    }
}

/* Arithmetic functions - signature: (dill_stream s, int data1, int data2, int dest, int src1, int src2) */
/* data1 values: 0=add, 1=sub, 2=and, 3=or, 4=xor, 5=lsh, 6=rsh(signed), 7=rsh(unsigned) */
/* data2 = type (DILL_I, DILL_U, DILL_L, etc.) - determines 32-bit vs 64-bit ops */
void arm64_arith3(dill_stream s, int data1, int data2, int dest, int src1, int src2)
{
    unsigned int insn;
    /* Use 64-bit ops for types >= DILL_L (6), 32-bit for smaller types */
    int is_64bit = (data2 >= DILL_L);
    unsigned int sf = is_64bit ? 0x80000000 : 0;  /* bit 31 = sf (size flag) */

    switch (data1) {
    case 0: /* add: ADD Rd, Rn, Rm */
	/* 64-bit: 0x8B000000, 32-bit: 0x0B000000 */
	insn = 0x0B000000 | sf | (src2 << 16) | (src1 << 5) | dest;
	break;
    case 1: /* sub: SUB Rd, Rn, Rm */
	/* 64-bit: 0xCB000000, 32-bit: 0x4B000000 */
	insn = 0x4B000000 | sf | (src2 << 16) | (src1 << 5) | dest;
	break;
    case 2: /* and: AND Rd, Rn, Rm */
	/* 64-bit: 0x8A000000, 32-bit: 0x0A000000 */
	insn = 0x0A000000 | sf | (src2 << 16) | (src1 << 5) | dest;
	break;
    case 3: /* or: ORR Rd, Rn, Rm */
	/* 64-bit: 0xAA000000, 32-bit: 0x2A000000 */
	insn = 0x2A000000 | sf | (src2 << 16) | (src1 << 5) | dest;
	break;
    case 4: /* xor: EOR Rd, Rn, Rm */
	/* 64-bit: 0xCA000000, 32-bit: 0x4A000000 */
	insn = 0x4A000000 | sf | (src2 << 16) | (src1 << 5) | dest;
	break;
    case 5: /* lsh: LSLV Rd, Rn, Rm */
	/* 64-bit: 0x9AC02000, 32-bit: 0x1AC02000 */
	insn = 0x1AC02000 | sf | (src2 << 16) | (src1 << 5) | dest;
	break;
    case 6: /* rsh signed: ASRV Rd, Rn, Rm */
	/* 64-bit: 0x9AC02800, 32-bit: 0x1AC02800 */
	insn = 0x1AC02800 | sf | (src2 << 16) | (src1 << 5) | dest;
	break;
    case 7: /* rsh unsigned: LSRV Rd, Rn, Rm */
	/* 64-bit: 0x9AC02400, 32-bit: 0x1AC02400 */
	insn = 0x1AC02400 | sf | (src2 << 16) | (src1 << 5) | dest;
	break;
    default:
	return;
    }
    INSN_OUT(s, insn);
}

void arm64_mul(dill_stream s, int data1, int data2, int dest, int src1, int src2)
{
    /* MUL Rd, Rn, Rm = MADD Rd, Rn, Rm, RZR */
    /* 64-bit: 0x9B007C00, 32-bit: 0x1B007C00 */
    int is_64bit = (data2 >= DILL_L);
    unsigned int sf = is_64bit ? 0x80000000 : 0;
    unsigned int insn = 0x1B007C00 | sf | (src2 << 16) | (src1 << 5) | dest;
    INSN_OUT(s, insn);
}

void arm64_div(dill_stream s, int data1, int data2, int dest, int src1, int src2)
{
    /* data1: 1=unsigned, 0=signed */
    /* data2: type for 32/64-bit selection */
    int is_64bit = (data2 >= DILL_L);
    unsigned int sf = is_64bit ? 0x80000000 : 0;
    unsigned int insn;
    if (data1) {
	/* unsigned: UDIV Rd, Rn, Rm = 0x1AC00800 (32-bit) / 0x9AC00800 (64-bit) */
	insn = 0x1AC00800 | sf | (src2 << 16) | (src1 << 5) | dest;
    } else {
	/* signed: SDIV Rd, Rn, Rm = 0x1AC00C00 (32-bit) / 0x9AC00C00 (64-bit) */
	insn = 0x1AC00C00 | sf | (src2 << 16) | (src1 << 5) | dest;
    }
    INSN_OUT(s, insn);
}

void arm64_mod(dill_stream s, int data1, int data2, int dest, int src1, int src2)
{
    /* ARM64 has no direct MOD instruction. Use: dest = src1 - (src1/src2)*src2 */
    /* Use x17 as temp for quotient (x16 may be used for immediates by caller) */
    /* data1: 0=unsigned, 1=signed (from arm64.ops) */
    int is_64bit = (data2 >= DILL_L);
    unsigned int sf = is_64bit ? 0x80000000 : 0;
    unsigned int insn;
    if (data1) {
	/* signed (data1=1): SDIV x17, Rn, Rm */
	insn = 0x1AC00C00 | sf | (src2 << 16) | (src1 << 5) | _x17;
    } else {
	/* unsigned (data1=0): UDIV x17, Rn, Rm */
	insn = 0x1AC00800 | sf | (src2 << 16) | (src1 << 5) | _x17;
    }
    INSN_OUT(s, insn);
    /* MSUB Rd, Rn, Rm, Ra: Rd = Ra - Rn*Rm */
    /* 64-bit: 0x9B008000, 32-bit: 0x1B008000 */
    insn = 0x1B008000 | sf | (src2 << 16) | (src1 << 10) | (_x17 << 5) | dest;
    INSN_OUT(s, insn);
}

void arm64_farith(dill_stream s, int data1, int data2, int dest, int src1, int src2)
{
    /* data1: 0=add, 1=sub, 2=mul, 3=div */
    unsigned int insn;

    /* data2: 0=float, 1=double */
    int is_double = (data2 == 1);

    switch (data1) {
    case 0: /* fadd */
	if (is_double) {
	    /* FADD Dd, Dn, Dm = 0x1E602800 */
	    insn = 0x1E602800 | (src2 << 16) | (src1 << 5) | dest;
	} else {
	    /* FADD Sd, Sn, Sm = 0x1E202800 */
	    insn = 0x1E202800 | (src2 << 16) | (src1 << 5) | dest;
	}
	break;
    case 1: /* fsub */
	if (is_double) {
	    /* FSUB Dd, Dn, Dm = 0x1E603800 */
	    insn = 0x1E603800 | (src2 << 16) | (src1 << 5) | dest;
	} else {
	    /* FSUB Sd, Sn, Sm = 0x1E203800 */
	    insn = 0x1E203800 | (src2 << 16) | (src1 << 5) | dest;
	}
	break;
    case 2: /* fmul */
	if (is_double) {
	    /* FMUL Dd, Dn, Dm = 0x1E600800 */
	    insn = 0x1E600800 | (src2 << 16) | (src1 << 5) | dest;
	} else {
	    /* FMUL Sd, Sn, Sm = 0x1E200800 */
	    insn = 0x1E200800 | (src2 << 16) | (src1 << 5) | dest;
	}
	break;
    case 3: /* fdiv */
	if (is_double) {
	    /* FDIV Dd, Dn, Dm = 0x1E601800 */
	    insn = 0x1E601800 | (src2 << 16) | (src1 << 5) | dest;
	} else {
	    /* FDIV Sd, Sn, Sm = 0x1E201800 */
	    insn = 0x1E201800 | (src2 << 16) | (src1 << 5) | dest;
	}
	break;
    default:
	return;
    }
    INSN_OUT(s, insn);
}

/* Arithmetic immediate functions - signature: (dill_stream s, int data1, int data2, int dest, int src, IMM_TYPE imm) */
/* data1 values: 0=add, 1=sub, 2=and, 3=or, 4=xor, 5=lsh, 6=rsh(signed), 7=rsh(unsigned) */
/* data2 = type (DILL_I, DILL_U, DILL_L, etc.) - determines 32-bit vs 64-bit ops */
void arm64_arith3i(dill_stream s, int data1, int data2, int dest, int src, IMM_TYPE imm)
{
    unsigned int insn;
    int is_64bit = (data2 >= DILL_L);
    unsigned int sf = is_64bit ? 0x80000000 : 0;
    /* For bit-field instructions: N=1 for 64-bit, N=0 for 32-bit */
    int N = is_64bit ? 1 : 0;
    int reg_size = is_64bit ? 64 : 32;
    int max_imms = is_64bit ? 63 : 31;

    switch (data1) {
    case 0: /* add immediate */
	if (imm >= 0 && imm < 4096) {
	    /* ADD Rd, Rn, #imm: 64-bit: 0x91000000, 32-bit: 0x11000000 */
	    insn = 0x11000000 | sf | ((imm & 0xfff) << 10) | (src << 5) | dest;
	    INSN_OUT(s, insn);
	} else {
	    arm64_set64(s, _x16, imm);
	    arm64_arith3(s, 0, data2, dest, src, _x16);
	}
	return;
    case 1: /* sub immediate */
	if (imm >= 0 && imm < 4096) {
	    /* SUB Rd, Rn, #imm: 64-bit: 0xD1000000, 32-bit: 0x51000000 */
	    insn = 0x51000000 | sf | ((imm & 0xfff) << 10) | (src << 5) | dest;
	    INSN_OUT(s, insn);
	} else {
	    arm64_set64(s, _x16, imm);
	    arm64_arith3(s, 1, data2, dest, src, _x16);
	}
	return;
    case 2: /* and immediate - complex bitmask encoding, use register form */
	arm64_set64(s, _x16, imm);
	arm64_arith3(s, 2, data2, dest, src, _x16);
	return;
    case 3: /* or immediate - complex bitmask encoding, use register form */
	arm64_set64(s, _x16, imm);
	arm64_arith3(s, 3, data2, dest, src, _x16);
	return;
    case 4: /* xor immediate - complex bitmask encoding, use register form */
	arm64_set64(s, _x16, imm);
	arm64_arith3(s, 4, data2, dest, src, _x16);
	return;
    case 5: /* lsh immediate: LSL Rd, Rn, #shift = UBFM Rd, Rn, #(-shift MOD size), #(size-1-shift) */
	{
	    int shift = imm & max_imms;
	    int immr = (reg_size - shift) & max_imms;
	    int imms = max_imms - shift;
	    /* UBFM Rd, Rn, #immr, #imms: 64-bit: 0xD3400000 | N<<22, 32-bit: 0x53000000 */
	    insn = 0x53000000 | sf | (N << 22) | (immr << 16) | (imms << 10) | (src << 5) | dest;
	    INSN_OUT(s, insn);
	}
	return;
    case 6: /* rsh signed immediate: ASR Rd, Rn, #shift = SBFM Rd, Rn, #shift, #(size-1) */
	{
	    int shift = imm & max_imms;
	    /* SBFM Rd, Rn, #immr, #imms: 64-bit: 0x93400000 | N<<22, 32-bit: 0x13000000 */
	    insn = 0x13000000 | sf | (N << 22) | (shift << 16) | (max_imms << 10) | (src << 5) | dest;
	    INSN_OUT(s, insn);
	}
	return;
    case 7: /* rsh unsigned immediate: LSR Rd, Rn, #shift = UBFM Rd, Rn, #shift, #(size-1) */
	{
	    int shift = imm & max_imms;
	    /* UBFM Rd, Rn, #immr, #imms: 64-bit: 0xD3400000 | N<<22, 32-bit: 0x53000000 */
	    insn = 0x53000000 | sf | (N << 22) | (shift << 16) | (max_imms << 10) | (src << 5) | dest;
	    INSN_OUT(s, insn);
	}
	return;
    }
}

void arm64_muli(dill_stream s, int data1, int data2, int dest, int src, IMM_TYPE imm)
{
    arm64_set64(s, _x16, imm);
    arm64_mul(s, data1, data2, dest, src, _x16);
}

void arm64_divi(dill_stream s, int data1, int data2, int dest, int src, IMM_TYPE imm)
{
    arm64_set64(s, _x16, imm);
    arm64_div(s, data1, data2, dest, src, _x16);
}

void arm64_modi(dill_stream s, int data1, int data2, int dest, int src, IMM_TYPE imm)
{
    arm64_set64(s, _x16, imm);
    arm64_mod(s, data1, data2, dest, src, _x16);
}

/* Unary arithmetic functions - signature: (dill_stream s, int data1, int data2, int dest, int src) */
/* data1 values: 0=com (bitwise NOT), 1=neg (negate), 2=not (logical NOT) */
void arm64_arith2(dill_stream s, int data1, int data2, int dest, int src)
{
    /* data2 = type for 32/64-bit selection */
    int is_64bit = (data2 >= DILL_L);
    unsigned int sf = is_64bit ? 0x80000000 : 0;
    unsigned int insn;

    switch (data1) {
    case 0: /* com: MVN Rd, Rm = ORN Rd, RZR, Rm */
	/* 64-bit: 0xAA2003E0, 32-bit: 0x2A2003E0 */
	insn = 0x2A2003E0 | sf | (src << 16) | dest;
	INSN_OUT(s, insn);
	break;
    case 1: /* neg: NEG Rd, Rm = SUB Rd, RZR, Rm */
	/* 64-bit: 0xCB0003E0, 32-bit: 0x4B0003E0 */
	insn = 0x4B0003E0 | sf | (src << 16) | dest;
	INSN_OUT(s, insn);
	break;
    case 2: /* not: logical NOT - compare with 0, return 1 if zero, 0 otherwise */
	/* CMP Rn, #0: SUBS RZR, Rn, #0 */
	/* 64-bit: 0xF100001F, 32-bit: 0x7100001F */
	insn = 0x7100001F | sf | (src << 5);
	INSN_OUT(s, insn);
	/* CSET Rd, EQ: CSINC Rd, RZR, RZR, NE */
	/* 64-bit: 0x9A9F17E0, 32-bit: 0x1A9F17E0 */
	insn = 0x1A9F17E0 | sf | dest;
	INSN_OUT(s, insn);
	break;
    }
}

void arm64_farith2(dill_stream s, int data1, int data2, int dest, int src)
{
    /* data1=0: neg; data2: 0=float, 1=double */
    unsigned int insn;
    int is_double = (data2 == 1);

    switch (data1) {
    case 0: /* fneg */
	if (is_double) {
	    /* FNEG Dd, Dn = 0x1E614000 */
	    insn = 0x1E614000 | (src << 5) | dest;
	} else {
	    /* FNEG Sd, Sn = 0x1E214000 */
	    insn = 0x1E214000 | (src << 5) | dest;
	}
	INSN_OUT(s, insn);
	break;
    }
}
