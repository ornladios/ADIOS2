# line 287 "virtual.ops"
/* This file is generated from virtual.ops.  Do not edit directly. */

#include "config.h"
#include "dill.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
#include <unistd.h>
#endif
#include "dill_internal.h"
#include "virtual.h"

static void 
dill_varith3(dill_stream s, int op3, int op, int dest, int src1, int src2)
{
    virtual_insn i;
    i.class_code = iclass_arith3;
    i.insn_code = op3;
    i.opnds.a3.dest = dest;
    i.opnds.a3.src1 = src1;
    i.opnds.a3.src2 = src2;
    INSN_OUT(s, i);
}

static void 
virtual_compare(dill_stream s, int op, int junk, int dest, int src1, int src2)
{
    virtual_insn i;
    i.class_code = iclass_compare;
    i.insn_code = op;
    i.opnds.a3.dest = dest;
    i.opnds.a3.src1 = src1;
    i.opnds.a3.src2 = src2;
    INSN_OUT(s, i);
}

static void 
dill_varith3i(dill_stream s, int op3, int op, int dest, int src1, IMM_TYPE imm)
{
    virtual_insn i;
    i.class_code = iclass_arith3i;
    i.insn_code = op3;
    i.opnds.a3i.dest = dest;
    i.opnds.a3i.src = src1;
    i.opnds.a3i.u.imm = imm;
    INSN_OUT(s, i);
}

static void dill_varith2(dill_stream s, int op3, int op, int dest, int src)
{
    virtual_insn i;
    i.class_code = iclass_arith2;
    i.insn_code = op3;
    i.opnds.a2.dest = dest;
    i.opnds.a2.src = src;
    INSN_OUT(s, i);
}

extern void virtual_ret(dill_stream s, int data1, int data2, int src)
{
    virtual_insn i;
    i.class_code = iclass_ret;
    i.insn_code = data1;
    memset(&i.opnds, 0, sizeof(i.opnds));
    i.opnds.a1.src = src;
    INSN_OUT(s, i);
}


extern void
virtual_convert(dill_stream s, int from_type, int to_type, 
	      int dest, int src)
{
    virtual_insn i;
    i.class_code = iclass_convert;
    i.insn_code = (unsigned)((from_type<<4) + to_type);
    i.opnds.a2.dest = dest;
    i.opnds.a2.src = src;
    INSN_OUT(s, i);
}

extern void
virtual_load(dill_stream s, int type, int junk, int dest, int src1, int src2)
{
    virtual_insn i;
    i.class_code = iclass_loadstore;
    i.insn_code = type;
    i.opnds.a3.dest = dest;
    i.opnds.a3.src1 = src1;
    i.opnds.a3.src2 = src2;
    INSN_OUT(s, i);
}

extern void
virtual_loadi(dill_stream s, int type, int junk, int dest, int src1, IMM_TYPE imm)
{
    virtual_insn i;
    i.class_code = iclass_loadstorei;
    i.insn_code = type;
    i.opnds.a3i.dest = dest;
    i.opnds.a3i.src = src1;
    i.opnds.a3i.u.imm = imm;
    INSN_OUT(s, i);
}

extern void
virtual_pbsload(dill_stream s, int type, int junk, int dest, int src1, int src2)
{
    virtual_insn i;
    i.class_code = iclass_loadstore;
    i.insn_code = 0x20 | type;
    i.opnds.a3.dest = dest;
    i.opnds.a3.src1 = src1;
    i.opnds.a3.src2 = src2;
    INSN_OUT(s, i);
}

extern void
virtual_pbsloadi(dill_stream s, int type, int junk, int dest, int src1, IMM_TYPE imm)
{
    virtual_insn i;
    i.class_code = iclass_loadstorei;
    i.insn_code = 0x20 | type;
    i.opnds.a3i.dest = dest;
    i.opnds.a3i.src = src1;
    i.opnds.a3i.u.imm = imm;
    INSN_OUT(s, i);
}

extern void
virtual_store(dill_stream s, int type, int junk, int dest, int src1, int src2)
{
    virtual_insn i;
    i.class_code = iclass_loadstore;
    i.insn_code = 0x10 | type;
    i.opnds.a3.dest = dest;
    i.opnds.a3.src1 = src1;
    i.opnds.a3.src2 = src2;
    INSN_OUT(s, i);
}

extern void
virtual_storei(dill_stream s, int type, int junk, int dest, int src1, IMM_TYPE imm)
{
    virtual_insn i;
    i.class_code = iclass_loadstorei;
    i.insn_code = 0x10 | type;
    i.opnds.a3i.dest = dest;
    i.opnds.a3i.src = src1;
    i.opnds.a3i.u.imm = imm;
    INSN_OUT(s, i);
}

extern void
virtual_mov(dill_stream s, int type, int junk, int dest, int src)
{
    virtual_insn i;
    i.class_code = iclass_mov;
    i.insn_code = type;
    i.opnds.a2.dest = dest;
    i.opnds.a2.src = src;
    INSN_OUT(s, i);
}

extern void
virtual_pset(dill_stream s, int type, int junk, int dest, IMM_TYPE imm)
{
    virtual_insn i;
    i.class_code = iclass_set;
    i.insn_code = type;
    i.opnds.a3i.dest = dest;
    i.opnds.a3i.u.imm = imm;
    INSN_OUT(s, i);
}

extern void
virtual_setp(dill_stream s, int type, int junk, int dest, void *imm)
{
    virtual_insn i;
    i.class_code = iclass_set;
    i.insn_code = type;
    i.opnds.a3i.dest = dest;
    i.opnds.a3i.u.imm_a = imm;
    INSN_OUT(s, i);
}

extern void
virtual_setf(dill_stream s, int type, int junk, int dest, double imm)
{
    virtual_insn i;
    i.class_code = iclass_setf;
    i.insn_code = type;
    i.opnds.sf.dest = dest;
    i.opnds.sf.imm = imm;
    INSN_OUT(s, i);
}

extern void
virtual_reti(dill_stream s, int type, int junk, IMM_TYPE imm)
{
    virtual_insn i;
    i.class_code = iclass_reti;
    i.insn_code = type;
    i.opnds.a3i.u.imm = imm;
    INSN_OUT(s, i);
}

static void
virtual_branch(dill_stream s, int op, int type, int src1, int src2, int label)
{
    virtual_insn i;
    i.class_code = iclass_branch;
    i.insn_code = op;
    i.opnds.br.src1 = src1;
    i.opnds.br.src2 = src2;
    i.opnds.br.label = label;
    INSN_OUT(s, i);
}

static void
virtual_mark_label(dill_stream s, int op, int type, int src1, int src2, int label)
{
    virtual_insn i;
    i.class_code = iclass_mark_label;
    i.insn_code = 0;
    i.opnds.label.label = label;
    i.opnds.label.label_name = NULL;
    INSN_OUT(s, i);
}

static void
virtual_branchi(dill_stream s, int op, int type, int src, IMM_TYPE imm, int label)
{
    virtual_insn i;
    i.class_code = iclass_branchi;
    i.insn_code = op;
    i.opnds.bri.src = src;
    i.opnds.bri.imm_l = imm;
    i.opnds.bri.label = label;
    INSN_OUT(s, i);
}

static void
virtual_jump_to_label(dill_stream s, unsigned long label)
{
    virtual_insn i;
    i.class_code = iclass_jump_to_label;
    i.insn_code = 0;
    i.opnds.br.label = (unsigned short) label;
    INSN_OUT(s, i);
}

extern void virtual_jump_to_reg(dill_stream s, unsigned long reg)
{
    virtual_insn i;
    i.class_code = iclass_jump_to_reg;
    i.insn_code = 0;
    i.opnds.br.src1 = (unsigned short) reg;
    INSN_OUT(s, i);
}

extern void virtual_jump_to_imm(dill_stream s, void *imm)
{
    virtual_insn i;
    i.class_code = iclass_jump_to_imm;
    i.insn_code = 0;
    i.opnds.bri.imm_a = imm;
    INSN_OUT(s, i);
}

extern int virtual_calli(dill_stream s, int type, void *xfer_address, const char *name)
{
    virtual_insn i;
    i.class_code = iclass_call;
    i.insn_code = type;
    i.opnds.calli.src = -1;
    if (type != DILL_V) i.opnds.calli.src = dill_getreg(s, type);
    i.opnds.calli.imm_a = xfer_address;
    i.opnds.calli.xfer_name = name;
    INSN_OUT(s, i);
    return i.opnds.calli.src;
}

extern int virtual_callr(dill_stream s, int type, int src)
{
    virtual_insn i;
    i.class_code = iclass_call;
    i.insn_code = 0x10 | type;
    i.opnds.bri.src = -1;
    if (type != DILL_V) i.opnds.bri.src = dill_getreg(s, type);
    i.opnds.bri.imm_l = src;
    INSN_OUT(s, i);
    return i.opnds.bri.src;
}

extern void virtual_push(dill_stream s, int type, int reg)
{
    virtual_insn i;
    i.class_code = iclass_push;
    i.insn_code = type;
    i.opnds.a1.src = reg;
    INSN_OUT(s, i);
}

extern void virtual_pushi(dill_stream s, int type, IMM_TYPE value)
{
    virtual_insn i;
    i.class_code = iclass_pushi;
    i.insn_code = type;
    i.opnds.a3i.u.imm = value;
    INSN_OUT(s, i);
}

extern void virtual_pushpi(dill_stream s, int type, void *value)
{
    virtual_insn i;
    i.class_code = iclass_pushi;
    i.insn_code = type;
    i.opnds.a3i.u.imm_a = value;
    INSN_OUT(s, i);
}

extern void virtual_pushfi(dill_stream s, int type, double value)
{
    virtual_insn i;
    i.class_code = iclass_pushf;
    i.insn_code = type;
    i.opnds.sf.imm = value;
    INSN_OUT(s, i);
}

extern void virtual_lea(dill_stream s, int junk1, int junk2, int dest, int src,
			IMM_TYPE imm)
{
    virtual_insn i;
    i.class_code = iclass_lea;
    i.insn_code = 0;
    i.opnds.a3i.dest = dest;
    i.opnds.a3i.src = src;
    i.opnds.a3i.u.imm = imm;
    INSN_OUT(s, i);
}

extern void virtual_special(dill_stream s, special_operations type, IMM_TYPE param)
{
    virtual_insn i;
    i.class_code = iclass_special;
    i.insn_code = 0;
    i.opnds.spec.type = type;
    i.opnds.spec.param = param;
    INSN_OUT(s, i);
}

static int
virtual_init_disassembly_info(dill_stream s, void *ptr)
{
    return 1;
}

int virtual_type_align[] = {
        1, /* C */
        1, /* UC */
        2, /* S */
        2, /* US */
        4, /* I */
        4, /* U */
        sizeof(uintptr_t), /* UL */
        sizeof(intptr_t), /* L */
        sizeof(char*), /* P */
        4, /* F */
        8, /* D */
        4, /* V */
        4, /* B */
        sizeof(char*), /* EC */
};

int virtual_type_size[] = {
        1, /* C */
        1, /* UC */
        2, /* S */
        2, /* US */
        4, /* I */
        4, /* U */
        sizeof(uintptr_t), /* UL */
        sizeof(intptr_t), /* L */
        sizeof(char*), /* P */
        sizeof(float), /* F */
        sizeof(double), /* D */
        4, /* V */
        0, /* B */
        sizeof(char*), /* EC */
};

DECLARE_JUMP_TABLE(virtual);
#ifdef __has_feature
#if __has_feature(thread_sanitizer)
#define NO_SANITIZE_THREAD __attribute__((no_sanitize("thread")))
#endif
#endif
#ifndef NO_SANITIZE_THREAD
#define NO_SANITIZE_THREAD
#endif
extern void NO_SANITIZE_THREAD dill_virtual_init(dill_stream s)
{
	  FILL_JUMP_STRUCTURE(virtual);
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_addi] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_addi].data1 = dill_jmp_addi;
	  virtual_jump_table->a3_data[dill_jmp_addi].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_addu] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_addu].data1 = dill_jmp_addu;
	  virtual_jump_table->a3_data[dill_jmp_addu].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_addul] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_addul].data1 = dill_jmp_addul;
	  virtual_jump_table->a3_data[dill_jmp_addul].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_addl] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_addl].data1 = dill_jmp_addl;
	  virtual_jump_table->a3_data[dill_jmp_addl].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_addp] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_addp].data1 = dill_jmp_addp;
	  virtual_jump_table->a3_data[dill_jmp_addp].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_subi] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_subi].data1 = dill_jmp_subi;
	  virtual_jump_table->a3_data[dill_jmp_subi].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_subu] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_subu].data1 = dill_jmp_subu;
	  virtual_jump_table->a3_data[dill_jmp_subu].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_subul] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_subul].data1 = dill_jmp_subul;
	  virtual_jump_table->a3_data[dill_jmp_subul].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_subl] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_subl].data1 = dill_jmp_subl;
	  virtual_jump_table->a3_data[dill_jmp_subl].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_subp] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_subp].data1 = dill_jmp_subp;
	  virtual_jump_table->a3_data[dill_jmp_subp].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_mulu] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_mulu].data1 = dill_jmp_mulu;
	  virtual_jump_table->a3_data[dill_jmp_mulu].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_mulul] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_mulul].data1 = dill_jmp_mulul;
	  virtual_jump_table->a3_data[dill_jmp_mulul].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_muli] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_muli].data1 = dill_jmp_muli;
	  virtual_jump_table->a3_data[dill_jmp_muli].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_mull] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_mull].data1 = dill_jmp_mull;
	  virtual_jump_table->a3_data[dill_jmp_mull].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_divu] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_divu].data1 = dill_jmp_divu;
	  virtual_jump_table->a3_data[dill_jmp_divu].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_divul] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_divul].data1 = dill_jmp_divul;
	  virtual_jump_table->a3_data[dill_jmp_divul].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_divi] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_divi].data1 = dill_jmp_divi;
	  virtual_jump_table->a3_data[dill_jmp_divi].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_divl] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_divl].data1 = dill_jmp_divl;
	  virtual_jump_table->a3_data[dill_jmp_divl].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_modu] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_modu].data1 = dill_jmp_modu;
	  virtual_jump_table->a3_data[dill_jmp_modu].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_modul] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_modul].data1 = dill_jmp_modul;
	  virtual_jump_table->a3_data[dill_jmp_modul].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_modi] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_modi].data1 = dill_jmp_modi;
	  virtual_jump_table->a3_data[dill_jmp_modi].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_modl] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_modl].data1 = dill_jmp_modl;
	  virtual_jump_table->a3_data[dill_jmp_modl].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_andu] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_andu].data1 = dill_jmp_andu;
	  virtual_jump_table->a3_data[dill_jmp_andu].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_andul] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_andul].data1 = dill_jmp_andul;
	  virtual_jump_table->a3_data[dill_jmp_andul].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_andi] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_andi].data1 = dill_jmp_andi;
	  virtual_jump_table->a3_data[dill_jmp_andi].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_andl] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_andl].data1 = dill_jmp_andl;
	  virtual_jump_table->a3_data[dill_jmp_andl].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_oru] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_oru].data1 = dill_jmp_oru;
	  virtual_jump_table->a3_data[dill_jmp_oru].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_orul] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_orul].data1 = dill_jmp_orul;
	  virtual_jump_table->a3_data[dill_jmp_orul].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_ori] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_ori].data1 = dill_jmp_ori;
	  virtual_jump_table->a3_data[dill_jmp_ori].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_orl] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_orl].data1 = dill_jmp_orl;
	  virtual_jump_table->a3_data[dill_jmp_orl].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_xoru] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_xoru].data1 = dill_jmp_xoru;
	  virtual_jump_table->a3_data[dill_jmp_xoru].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_xorul] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_xorul].data1 = dill_jmp_xorul;
	  virtual_jump_table->a3_data[dill_jmp_xorul].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_xori] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_xori].data1 = dill_jmp_xori;
	  virtual_jump_table->a3_data[dill_jmp_xori].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_xorl] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_xorl].data1 = dill_jmp_xorl;
	  virtual_jump_table->a3_data[dill_jmp_xorl].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_lshu] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_lshu].data1 = dill_jmp_lshu;
	  virtual_jump_table->a3_data[dill_jmp_lshu].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_lshul] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_lshul].data1 = dill_jmp_lshul;
	  virtual_jump_table->a3_data[dill_jmp_lshul].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_lshi] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_lshi].data1 = dill_jmp_lshi;
	  virtual_jump_table->a3_data[dill_jmp_lshi].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_lshl] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_lshl].data1 = dill_jmp_lshl;
	  virtual_jump_table->a3_data[dill_jmp_lshl].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_rshu] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_rshu].data1 = dill_jmp_rshu;
	  virtual_jump_table->a3_data[dill_jmp_rshu].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_rshul] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_rshul].data1 = dill_jmp_rshul;
	  virtual_jump_table->a3_data[dill_jmp_rshul].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_rshi] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_rshi].data1 = dill_jmp_rshi;
	  virtual_jump_table->a3_data[dill_jmp_rshi].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_rshl] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_rshl].data1 = dill_jmp_rshl;
	  virtual_jump_table->a3_data[dill_jmp_rshl].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_addf] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_addf].data1 = dill_jmp_addf;
	  virtual_jump_table->a3_data[dill_jmp_addf].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_addd] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_addd].data1 = dill_jmp_addd;
	  virtual_jump_table->a3_data[dill_jmp_addd].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_subf] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_subf].data1 = dill_jmp_subf;
	  virtual_jump_table->a3_data[dill_jmp_subf].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_subd] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_subd].data1 = dill_jmp_subd;
	  virtual_jump_table->a3_data[dill_jmp_subd].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_mulf] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_mulf].data1 = dill_jmp_mulf;
	  virtual_jump_table->a3_data[dill_jmp_mulf].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_muld] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_muld].data1 = dill_jmp_muld;
	  virtual_jump_table->a3_data[dill_jmp_muld].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_divf] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_divf].data1 = dill_jmp_divf;
	  virtual_jump_table->a3_data[dill_jmp_divf].data2 = 0;
# line 156 "virtual.ops"
	  virtual_jump_table->jmp_a3[dill_jmp_divd] = dill_varith3;
	  virtual_jump_table->a3_data[dill_jmp_divd].data1 = dill_jmp_divd;
	  virtual_jump_table->a3_data[dill_jmp_divd].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_addi] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_addi].data1 = dill_jmp_addi;
	  virtual_jump_table->a3i_data[dill_jmp_addi].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_addu] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_addu].data1 = dill_jmp_addu;
	  virtual_jump_table->a3i_data[dill_jmp_addu].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_addul] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_addul].data1 = dill_jmp_addul;
	  virtual_jump_table->a3i_data[dill_jmp_addul].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_addl] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_addl].data1 = dill_jmp_addl;
	  virtual_jump_table->a3i_data[dill_jmp_addl].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_addp] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_addp].data1 = dill_jmp_addp;
	  virtual_jump_table->a3i_data[dill_jmp_addp].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_subi] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_subi].data1 = dill_jmp_subi;
	  virtual_jump_table->a3i_data[dill_jmp_subi].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_subu] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_subu].data1 = dill_jmp_subu;
	  virtual_jump_table->a3i_data[dill_jmp_subu].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_subul] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_subul].data1 = dill_jmp_subul;
	  virtual_jump_table->a3i_data[dill_jmp_subul].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_subl] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_subl].data1 = dill_jmp_subl;
	  virtual_jump_table->a3i_data[dill_jmp_subl].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_subp] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_subp].data1 = dill_jmp_subp;
	  virtual_jump_table->a3i_data[dill_jmp_subp].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_mulu] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_mulu].data1 = dill_jmp_mulu;
	  virtual_jump_table->a3i_data[dill_jmp_mulu].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_mulul] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_mulul].data1 = dill_jmp_mulul;
	  virtual_jump_table->a3i_data[dill_jmp_mulul].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_muli] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_muli].data1 = dill_jmp_muli;
	  virtual_jump_table->a3i_data[dill_jmp_muli].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_mull] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_mull].data1 = dill_jmp_mull;
	  virtual_jump_table->a3i_data[dill_jmp_mull].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_divu] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_divu].data1 = dill_jmp_divu;
	  virtual_jump_table->a3i_data[dill_jmp_divu].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_divul] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_divul].data1 = dill_jmp_divul;
	  virtual_jump_table->a3i_data[dill_jmp_divul].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_divi] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_divi].data1 = dill_jmp_divi;
	  virtual_jump_table->a3i_data[dill_jmp_divi].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_divl] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_divl].data1 = dill_jmp_divl;
	  virtual_jump_table->a3i_data[dill_jmp_divl].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_modu] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_modu].data1 = dill_jmp_modu;
	  virtual_jump_table->a3i_data[dill_jmp_modu].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_modul] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_modul].data1 = dill_jmp_modul;
	  virtual_jump_table->a3i_data[dill_jmp_modul].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_modi] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_modi].data1 = dill_jmp_modi;
	  virtual_jump_table->a3i_data[dill_jmp_modi].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_modl] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_modl].data1 = dill_jmp_modl;
	  virtual_jump_table->a3i_data[dill_jmp_modl].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_andi] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_andi].data1 = dill_jmp_andi;
	  virtual_jump_table->a3i_data[dill_jmp_andi].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_andu] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_andu].data1 = dill_jmp_andu;
	  virtual_jump_table->a3i_data[dill_jmp_andu].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_andul] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_andul].data1 = dill_jmp_andul;
	  virtual_jump_table->a3i_data[dill_jmp_andul].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_andl] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_andl].data1 = dill_jmp_andl;
	  virtual_jump_table->a3i_data[dill_jmp_andl].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_ori] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_ori].data1 = dill_jmp_ori;
	  virtual_jump_table->a3i_data[dill_jmp_ori].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_oru] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_oru].data1 = dill_jmp_oru;
	  virtual_jump_table->a3i_data[dill_jmp_oru].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_orul] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_orul].data1 = dill_jmp_orul;
	  virtual_jump_table->a3i_data[dill_jmp_orul].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_orl] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_orl].data1 = dill_jmp_orl;
	  virtual_jump_table->a3i_data[dill_jmp_orl].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_xori] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_xori].data1 = dill_jmp_xori;
	  virtual_jump_table->a3i_data[dill_jmp_xori].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_xoru] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_xoru].data1 = dill_jmp_xoru;
	  virtual_jump_table->a3i_data[dill_jmp_xoru].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_xorul] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_xorul].data1 = dill_jmp_xorul;
	  virtual_jump_table->a3i_data[dill_jmp_xorul].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_xorl] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_xorl].data1 = dill_jmp_xorl;
	  virtual_jump_table->a3i_data[dill_jmp_xorl].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_lshi] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_lshi].data1 = dill_jmp_lshi;
	  virtual_jump_table->a3i_data[dill_jmp_lshi].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_lshu] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_lshu].data1 = dill_jmp_lshu;
	  virtual_jump_table->a3i_data[dill_jmp_lshu].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_lshul] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_lshul].data1 = dill_jmp_lshul;
	  virtual_jump_table->a3i_data[dill_jmp_lshul].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_lshl] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_lshl].data1 = dill_jmp_lshl;
	  virtual_jump_table->a3i_data[dill_jmp_lshl].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_rshi] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_rshi].data1 = dill_jmp_rshi;
	  virtual_jump_table->a3i_data[dill_jmp_rshi].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_rshu] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_rshu].data1 = dill_jmp_rshu;
	  virtual_jump_table->a3i_data[dill_jmp_rshu].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_rshul] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_rshul].data1 = dill_jmp_rshul;
	  virtual_jump_table->a3i_data[dill_jmp_rshul].data2 = 0;
	  virtual_jump_table->jmp_a3i[dill_jmp_rshl] = dill_varith3i;
	  virtual_jump_table->a3i_data[dill_jmp_rshl].data1 = dill_jmp_rshl;
	  virtual_jump_table->a3i_data[dill_jmp_rshl].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_comi] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_comi].data1 = dill_jmp_comi;
	  virtual_jump_table->a2_data[dill_jmp_comi].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_comu] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_comu].data1 = dill_jmp_comu;
	  virtual_jump_table->a2_data[dill_jmp_comu].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_comul] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_comul].data1 = dill_jmp_comul;
	  virtual_jump_table->a2_data[dill_jmp_comul].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_coml] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_coml].data1 = dill_jmp_coml;
	  virtual_jump_table->a2_data[dill_jmp_coml].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_noti] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_noti].data1 = dill_jmp_noti;
	  virtual_jump_table->a2_data[dill_jmp_noti].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_notu] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_notu].data1 = dill_jmp_notu;
	  virtual_jump_table->a2_data[dill_jmp_notu].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_notul] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_notul].data1 = dill_jmp_notul;
	  virtual_jump_table->a2_data[dill_jmp_notul].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_notl] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_notl].data1 = dill_jmp_notl;
	  virtual_jump_table->a2_data[dill_jmp_notl].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_negi] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_negi].data1 = dill_jmp_negi;
	  virtual_jump_table->a2_data[dill_jmp_negi].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_negu] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_negu].data1 = dill_jmp_negu;
	  virtual_jump_table->a2_data[dill_jmp_negu].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_negul] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_negul].data1 = dill_jmp_negul;
	  virtual_jump_table->a2_data[dill_jmp_negul].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_negl] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_negl].data1 = dill_jmp_negl;
	  virtual_jump_table->a2_data[dill_jmp_negl].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_negf] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_negf].data1 = dill_jmp_negf;
	  virtual_jump_table->a2_data[dill_jmp_negf].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_negd] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_negd].data1 = dill_jmp_negd;
	  virtual_jump_table->a2_data[dill_jmp_negd].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_bswaps] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_bswaps].data1 = dill_jmp_bswaps;
	  virtual_jump_table->a2_data[dill_jmp_bswaps].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_bswapus] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_bswapus].data1 = dill_jmp_bswapus;
	  virtual_jump_table->a2_data[dill_jmp_bswapus].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_bswapi] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_bswapi].data1 = dill_jmp_bswapi;
	  virtual_jump_table->a2_data[dill_jmp_bswapi].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_bswapu] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_bswapu].data1 = dill_jmp_bswapu;
	  virtual_jump_table->a2_data[dill_jmp_bswapu].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_bswapul] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_bswapul].data1 = dill_jmp_bswapul;
	  virtual_jump_table->a2_data[dill_jmp_bswapul].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_bswapl] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_bswapl].data1 = dill_jmp_bswapl;
	  virtual_jump_table->a2_data[dill_jmp_bswapl].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_bswapf] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_bswapf].data1 = dill_jmp_bswapf;
	  virtual_jump_table->a2_data[dill_jmp_bswapf].data2 = 0;
	  virtual_jump_table->jmp_a2[dill_jmp_bswapd] = dill_varith2;
	  virtual_jump_table->a2_data[dill_jmp_bswapd].data1 = dill_jmp_bswapd;
	  virtual_jump_table->a2_data[dill_jmp_bswapd].data2 = 0;
	  virtual_jump_table->jmp_b[dill_jmp_beqc] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_beqc].data1 = dill_jmp_beqc;
	  virtual_jump_table->b_data[dill_jmp_beqc].data2 = DILL_C;
	  virtual_jump_table->jmp_b[dill_jmp_bequc] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bequc].data1 = dill_jmp_bequc;
	  virtual_jump_table->b_data[dill_jmp_bequc].data2 = DILL_UC;
	  virtual_jump_table->jmp_b[dill_jmp_beqs] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_beqs].data1 = dill_jmp_beqs;
	  virtual_jump_table->b_data[dill_jmp_beqs].data2 = DILL_S;
	  virtual_jump_table->jmp_b[dill_jmp_bequs] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bequs].data1 = dill_jmp_bequs;
	  virtual_jump_table->b_data[dill_jmp_bequs].data2 = DILL_US;
	  virtual_jump_table->jmp_b[dill_jmp_beqi] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_beqi].data1 = dill_jmp_beqi;
	  virtual_jump_table->b_data[dill_jmp_beqi].data2 = DILL_I;
	  virtual_jump_table->jmp_b[dill_jmp_bequ] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bequ].data1 = dill_jmp_bequ;
	  virtual_jump_table->b_data[dill_jmp_bequ].data2 = DILL_U;
	  virtual_jump_table->jmp_b[dill_jmp_bequl] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bequl].data1 = dill_jmp_bequl;
	  virtual_jump_table->b_data[dill_jmp_bequl].data2 = DILL_UL;
	  virtual_jump_table->jmp_b[dill_jmp_beql] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_beql].data1 = dill_jmp_beql;
	  virtual_jump_table->b_data[dill_jmp_beql].data2 = DILL_L;
	  virtual_jump_table->jmp_b[dill_jmp_beqp] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_beqp].data1 = dill_jmp_beqp;
	  virtual_jump_table->b_data[dill_jmp_beqp].data2 = DILL_P;
	  virtual_jump_table->jmp_b[dill_jmp_beqd] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_beqd].data1 = dill_jmp_beqd;
	  virtual_jump_table->b_data[dill_jmp_beqd].data2 = DILL_D;
	  virtual_jump_table->jmp_b[dill_jmp_beqf] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_beqf].data1 = dill_jmp_beqf;
	  virtual_jump_table->b_data[dill_jmp_beqf].data2 = DILL_F;
	  virtual_jump_table->jmp_b[dill_jmp_bgec] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgec].data1 = dill_jmp_bgec;
	  virtual_jump_table->b_data[dill_jmp_bgec].data2 = DILL_C;
	  virtual_jump_table->jmp_b[dill_jmp_bgeuc] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgeuc].data1 = dill_jmp_bgeuc;
	  virtual_jump_table->b_data[dill_jmp_bgeuc].data2 = DILL_UC;
	  virtual_jump_table->jmp_b[dill_jmp_bges] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bges].data1 = dill_jmp_bges;
	  virtual_jump_table->b_data[dill_jmp_bges].data2 = DILL_S;
	  virtual_jump_table->jmp_b[dill_jmp_bgeus] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgeus].data1 = dill_jmp_bgeus;
	  virtual_jump_table->b_data[dill_jmp_bgeus].data2 = DILL_US;
	  virtual_jump_table->jmp_b[dill_jmp_bgei] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgei].data1 = dill_jmp_bgei;
	  virtual_jump_table->b_data[dill_jmp_bgei].data2 = DILL_I;
	  virtual_jump_table->jmp_b[dill_jmp_bgeu] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgeu].data1 = dill_jmp_bgeu;
	  virtual_jump_table->b_data[dill_jmp_bgeu].data2 = DILL_U;
	  virtual_jump_table->jmp_b[dill_jmp_bgeul] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgeul].data1 = dill_jmp_bgeul;
	  virtual_jump_table->b_data[dill_jmp_bgeul].data2 = DILL_UL;
	  virtual_jump_table->jmp_b[dill_jmp_bgel] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgel].data1 = dill_jmp_bgel;
	  virtual_jump_table->b_data[dill_jmp_bgel].data2 = DILL_L;
	  virtual_jump_table->jmp_b[dill_jmp_bgep] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgep].data1 = dill_jmp_bgep;
	  virtual_jump_table->b_data[dill_jmp_bgep].data2 = DILL_P;
	  virtual_jump_table->jmp_b[dill_jmp_bged] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bged].data1 = dill_jmp_bged;
	  virtual_jump_table->b_data[dill_jmp_bged].data2 = DILL_D;
	  virtual_jump_table->jmp_b[dill_jmp_bgef] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgef].data1 = dill_jmp_bgef;
	  virtual_jump_table->b_data[dill_jmp_bgef].data2 = DILL_F;
	  virtual_jump_table->jmp_b[dill_jmp_bgtc] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgtc].data1 = dill_jmp_bgtc;
	  virtual_jump_table->b_data[dill_jmp_bgtc].data2 = DILL_C;
	  virtual_jump_table->jmp_b[dill_jmp_bgtuc] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgtuc].data1 = dill_jmp_bgtuc;
	  virtual_jump_table->b_data[dill_jmp_bgtuc].data2 = DILL_UC;
	  virtual_jump_table->jmp_b[dill_jmp_bgts] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgts].data1 = dill_jmp_bgts;
	  virtual_jump_table->b_data[dill_jmp_bgts].data2 = DILL_S;
	  virtual_jump_table->jmp_b[dill_jmp_bgtus] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgtus].data1 = dill_jmp_bgtus;
	  virtual_jump_table->b_data[dill_jmp_bgtus].data2 = DILL_US;
	  virtual_jump_table->jmp_b[dill_jmp_bgti] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgti].data1 = dill_jmp_bgti;
	  virtual_jump_table->b_data[dill_jmp_bgti].data2 = DILL_I;
	  virtual_jump_table->jmp_b[dill_jmp_bgtu] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgtu].data1 = dill_jmp_bgtu;
	  virtual_jump_table->b_data[dill_jmp_bgtu].data2 = DILL_U;
	  virtual_jump_table->jmp_b[dill_jmp_bgtul] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgtul].data1 = dill_jmp_bgtul;
	  virtual_jump_table->b_data[dill_jmp_bgtul].data2 = DILL_UL;
	  virtual_jump_table->jmp_b[dill_jmp_bgtl] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgtl].data1 = dill_jmp_bgtl;
	  virtual_jump_table->b_data[dill_jmp_bgtl].data2 = DILL_L;
	  virtual_jump_table->jmp_b[dill_jmp_bgtp] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgtp].data1 = dill_jmp_bgtp;
	  virtual_jump_table->b_data[dill_jmp_bgtp].data2 = DILL_P;
	  virtual_jump_table->jmp_b[dill_jmp_bgtd] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgtd].data1 = dill_jmp_bgtd;
	  virtual_jump_table->b_data[dill_jmp_bgtd].data2 = DILL_D;
	  virtual_jump_table->jmp_b[dill_jmp_bgtf] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bgtf].data1 = dill_jmp_bgtf;
	  virtual_jump_table->b_data[dill_jmp_bgtf].data2 = DILL_F;
	  virtual_jump_table->jmp_b[dill_jmp_blec] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_blec].data1 = dill_jmp_blec;
	  virtual_jump_table->b_data[dill_jmp_blec].data2 = DILL_C;
	  virtual_jump_table->jmp_b[dill_jmp_bleuc] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bleuc].data1 = dill_jmp_bleuc;
	  virtual_jump_table->b_data[dill_jmp_bleuc].data2 = DILL_UC;
	  virtual_jump_table->jmp_b[dill_jmp_bles] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bles].data1 = dill_jmp_bles;
	  virtual_jump_table->b_data[dill_jmp_bles].data2 = DILL_S;
	  virtual_jump_table->jmp_b[dill_jmp_bleus] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bleus].data1 = dill_jmp_bleus;
	  virtual_jump_table->b_data[dill_jmp_bleus].data2 = DILL_US;
	  virtual_jump_table->jmp_b[dill_jmp_blei] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_blei].data1 = dill_jmp_blei;
	  virtual_jump_table->b_data[dill_jmp_blei].data2 = DILL_I;
	  virtual_jump_table->jmp_b[dill_jmp_bleu] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bleu].data1 = dill_jmp_bleu;
	  virtual_jump_table->b_data[dill_jmp_bleu].data2 = DILL_U;
	  virtual_jump_table->jmp_b[dill_jmp_bleul] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bleul].data1 = dill_jmp_bleul;
	  virtual_jump_table->b_data[dill_jmp_bleul].data2 = DILL_UL;
	  virtual_jump_table->jmp_b[dill_jmp_blel] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_blel].data1 = dill_jmp_blel;
	  virtual_jump_table->b_data[dill_jmp_blel].data2 = DILL_L;
	  virtual_jump_table->jmp_b[dill_jmp_blep] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_blep].data1 = dill_jmp_blep;
	  virtual_jump_table->b_data[dill_jmp_blep].data2 = DILL_P;
	  virtual_jump_table->jmp_b[dill_jmp_bled] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bled].data1 = dill_jmp_bled;
	  virtual_jump_table->b_data[dill_jmp_bled].data2 = DILL_D;
	  virtual_jump_table->jmp_b[dill_jmp_blef] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_blef].data1 = dill_jmp_blef;
	  virtual_jump_table->b_data[dill_jmp_blef].data2 = DILL_F;
	  virtual_jump_table->jmp_b[dill_jmp_bltc] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bltc].data1 = dill_jmp_bltc;
	  virtual_jump_table->b_data[dill_jmp_bltc].data2 = DILL_C;
	  virtual_jump_table->jmp_b[dill_jmp_bltuc] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bltuc].data1 = dill_jmp_bltuc;
	  virtual_jump_table->b_data[dill_jmp_bltuc].data2 = DILL_UC;
	  virtual_jump_table->jmp_b[dill_jmp_blts] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_blts].data1 = dill_jmp_blts;
	  virtual_jump_table->b_data[dill_jmp_blts].data2 = DILL_S;
	  virtual_jump_table->jmp_b[dill_jmp_bltus] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bltus].data1 = dill_jmp_bltus;
	  virtual_jump_table->b_data[dill_jmp_bltus].data2 = DILL_US;
	  virtual_jump_table->jmp_b[dill_jmp_blti] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_blti].data1 = dill_jmp_blti;
	  virtual_jump_table->b_data[dill_jmp_blti].data2 = DILL_I;
	  virtual_jump_table->jmp_b[dill_jmp_bltu] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bltu].data1 = dill_jmp_bltu;
	  virtual_jump_table->b_data[dill_jmp_bltu].data2 = DILL_U;
	  virtual_jump_table->jmp_b[dill_jmp_bltul] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bltul].data1 = dill_jmp_bltul;
	  virtual_jump_table->b_data[dill_jmp_bltul].data2 = DILL_UL;
	  virtual_jump_table->jmp_b[dill_jmp_bltl] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bltl].data1 = dill_jmp_bltl;
	  virtual_jump_table->b_data[dill_jmp_bltl].data2 = DILL_L;
	  virtual_jump_table->jmp_b[dill_jmp_bltp] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bltp].data1 = dill_jmp_bltp;
	  virtual_jump_table->b_data[dill_jmp_bltp].data2 = DILL_P;
	  virtual_jump_table->jmp_b[dill_jmp_bltd] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bltd].data1 = dill_jmp_bltd;
	  virtual_jump_table->b_data[dill_jmp_bltd].data2 = DILL_D;
	  virtual_jump_table->jmp_b[dill_jmp_bltf] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bltf].data1 = dill_jmp_bltf;
	  virtual_jump_table->b_data[dill_jmp_bltf].data2 = DILL_F;
	  virtual_jump_table->jmp_b[dill_jmp_bnec] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bnec].data1 = dill_jmp_bnec;
	  virtual_jump_table->b_data[dill_jmp_bnec].data2 = DILL_C;
	  virtual_jump_table->jmp_b[dill_jmp_bneuc] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bneuc].data1 = dill_jmp_bneuc;
	  virtual_jump_table->b_data[dill_jmp_bneuc].data2 = DILL_UC;
	  virtual_jump_table->jmp_b[dill_jmp_bnes] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bnes].data1 = dill_jmp_bnes;
	  virtual_jump_table->b_data[dill_jmp_bnes].data2 = DILL_S;
	  virtual_jump_table->jmp_b[dill_jmp_bneus] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bneus].data1 = dill_jmp_bneus;
	  virtual_jump_table->b_data[dill_jmp_bneus].data2 = DILL_US;
	  virtual_jump_table->jmp_b[dill_jmp_bnei] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bnei].data1 = dill_jmp_bnei;
	  virtual_jump_table->b_data[dill_jmp_bnei].data2 = DILL_I;
	  virtual_jump_table->jmp_b[dill_jmp_bneu] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bneu].data1 = dill_jmp_bneu;
	  virtual_jump_table->b_data[dill_jmp_bneu].data2 = DILL_U;
	  virtual_jump_table->jmp_b[dill_jmp_bneul] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bneul].data1 = dill_jmp_bneul;
	  virtual_jump_table->b_data[dill_jmp_bneul].data2 = DILL_UL;
	  virtual_jump_table->jmp_b[dill_jmp_bnel] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bnel].data1 = dill_jmp_bnel;
	  virtual_jump_table->b_data[dill_jmp_bnel].data2 = DILL_L;
	  virtual_jump_table->jmp_b[dill_jmp_bnep] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bnep].data1 = dill_jmp_bnep;
	  virtual_jump_table->b_data[dill_jmp_bnep].data2 = DILL_P;
	  virtual_jump_table->jmp_b[dill_jmp_bned] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bned].data1 = dill_jmp_bned;
	  virtual_jump_table->b_data[dill_jmp_bned].data2 = DILL_D;
	  virtual_jump_table->jmp_b[dill_jmp_bnef] = virtual_branch;
	  virtual_jump_table->b_data[dill_jmp_bnef].data1 = dill_jmp_bnef;
	  virtual_jump_table->b_data[dill_jmp_bnef].data2 = DILL_F;
	  virtual_jump_table->jmp_bi[dill_jmp_beqc] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bequc] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_beqs] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bequs] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_beqi] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bequ] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bequl] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_beql] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_beqp] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgec] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgeuc] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bges] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgeus] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgei] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgeu] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgeul] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgel] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgep] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgtc] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgtuc] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgts] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgtus] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgti] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgtu] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgtul] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgtl] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bgtp] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_blec] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bleuc] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bles] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bleus] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_blei] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bleu] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bleul] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_blel] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_blep] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bltc] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bltuc] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_blts] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bltus] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_blti] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bltu] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bltul] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bltl] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bltp] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bnec] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bneuc] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bnes] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bneus] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bnei] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bneu] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bneul] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bnel] = virtual_branchi;
	  virtual_jump_table->jmp_bi[dill_jmp_bnep] = virtual_branchi;
	  virtual_jump_table->jmp_c[dill_jmp_ceqc] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_ceqc].data1 = dill_jmp_ceqc;
	  virtual_jump_table->c_data[dill_jmp_ceqc].data2 = DILL_C;
	  virtual_jump_table->jmp_c[dill_jmp_cequc] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cequc].data1 = dill_jmp_cequc;
	  virtual_jump_table->c_data[dill_jmp_cequc].data2 = DILL_UC;
	  virtual_jump_table->jmp_c[dill_jmp_ceqs] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_ceqs].data1 = dill_jmp_ceqs;
	  virtual_jump_table->c_data[dill_jmp_ceqs].data2 = DILL_S;
	  virtual_jump_table->jmp_c[dill_jmp_cequs] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cequs].data1 = dill_jmp_cequs;
	  virtual_jump_table->c_data[dill_jmp_cequs].data2 = DILL_US;
	  virtual_jump_table->jmp_c[dill_jmp_ceqi] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_ceqi].data1 = dill_jmp_ceqi;
	  virtual_jump_table->c_data[dill_jmp_ceqi].data2 = DILL_I;
	  virtual_jump_table->jmp_c[dill_jmp_cequ] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cequ].data1 = dill_jmp_cequ;
	  virtual_jump_table->c_data[dill_jmp_cequ].data2 = DILL_U;
	  virtual_jump_table->jmp_c[dill_jmp_cequl] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cequl].data1 = dill_jmp_cequl;
	  virtual_jump_table->c_data[dill_jmp_cequl].data2 = DILL_UL;
	  virtual_jump_table->jmp_c[dill_jmp_ceql] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_ceql].data1 = dill_jmp_ceql;
	  virtual_jump_table->c_data[dill_jmp_ceql].data2 = DILL_L;
	  virtual_jump_table->jmp_c[dill_jmp_ceqp] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_ceqp].data1 = dill_jmp_ceqp;
	  virtual_jump_table->c_data[dill_jmp_ceqp].data2 = DILL_P;
	  virtual_jump_table->jmp_c[dill_jmp_ceqd] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_ceqd].data1 = dill_jmp_ceqd;
	  virtual_jump_table->c_data[dill_jmp_ceqd].data2 = DILL_D;
	  virtual_jump_table->jmp_c[dill_jmp_ceqf] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_ceqf].data1 = dill_jmp_ceqf;
	  virtual_jump_table->c_data[dill_jmp_ceqf].data2 = DILL_F;
	  virtual_jump_table->jmp_c[dill_jmp_cgec] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgec].data1 = dill_jmp_cgec;
	  virtual_jump_table->c_data[dill_jmp_cgec].data2 = DILL_C;
	  virtual_jump_table->jmp_c[dill_jmp_cgeuc] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgeuc].data1 = dill_jmp_cgeuc;
	  virtual_jump_table->c_data[dill_jmp_cgeuc].data2 = DILL_UC;
	  virtual_jump_table->jmp_c[dill_jmp_cges] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cges].data1 = dill_jmp_cges;
	  virtual_jump_table->c_data[dill_jmp_cges].data2 = DILL_S;
	  virtual_jump_table->jmp_c[dill_jmp_cgeus] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgeus].data1 = dill_jmp_cgeus;
	  virtual_jump_table->c_data[dill_jmp_cgeus].data2 = DILL_US;
	  virtual_jump_table->jmp_c[dill_jmp_cgei] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgei].data1 = dill_jmp_cgei;
	  virtual_jump_table->c_data[dill_jmp_cgei].data2 = DILL_I;
	  virtual_jump_table->jmp_c[dill_jmp_cgeu] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgeu].data1 = dill_jmp_cgeu;
	  virtual_jump_table->c_data[dill_jmp_cgeu].data2 = DILL_U;
	  virtual_jump_table->jmp_c[dill_jmp_cgeul] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgeul].data1 = dill_jmp_cgeul;
	  virtual_jump_table->c_data[dill_jmp_cgeul].data2 = DILL_UL;
	  virtual_jump_table->jmp_c[dill_jmp_cgel] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgel].data1 = dill_jmp_cgel;
	  virtual_jump_table->c_data[dill_jmp_cgel].data2 = DILL_L;
	  virtual_jump_table->jmp_c[dill_jmp_cgep] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgep].data1 = dill_jmp_cgep;
	  virtual_jump_table->c_data[dill_jmp_cgep].data2 = DILL_P;
	  virtual_jump_table->jmp_c[dill_jmp_cged] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cged].data1 = dill_jmp_cged;
	  virtual_jump_table->c_data[dill_jmp_cged].data2 = DILL_D;
	  virtual_jump_table->jmp_c[dill_jmp_cgef] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgef].data1 = dill_jmp_cgef;
	  virtual_jump_table->c_data[dill_jmp_cgef].data2 = DILL_F;
	  virtual_jump_table->jmp_c[dill_jmp_cgtc] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgtc].data1 = dill_jmp_cgtc;
	  virtual_jump_table->c_data[dill_jmp_cgtc].data2 = DILL_C;
	  virtual_jump_table->jmp_c[dill_jmp_cgtuc] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgtuc].data1 = dill_jmp_cgtuc;
	  virtual_jump_table->c_data[dill_jmp_cgtuc].data2 = DILL_UC;
	  virtual_jump_table->jmp_c[dill_jmp_cgts] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgts].data1 = dill_jmp_cgts;
	  virtual_jump_table->c_data[dill_jmp_cgts].data2 = DILL_S;
	  virtual_jump_table->jmp_c[dill_jmp_cgtus] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgtus].data1 = dill_jmp_cgtus;
	  virtual_jump_table->c_data[dill_jmp_cgtus].data2 = DILL_US;
	  virtual_jump_table->jmp_c[dill_jmp_cgti] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgti].data1 = dill_jmp_cgti;
	  virtual_jump_table->c_data[dill_jmp_cgti].data2 = DILL_I;
	  virtual_jump_table->jmp_c[dill_jmp_cgtu] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgtu].data1 = dill_jmp_cgtu;
	  virtual_jump_table->c_data[dill_jmp_cgtu].data2 = DILL_U;
	  virtual_jump_table->jmp_c[dill_jmp_cgtul] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgtul].data1 = dill_jmp_cgtul;
	  virtual_jump_table->c_data[dill_jmp_cgtul].data2 = DILL_UL;
	  virtual_jump_table->jmp_c[dill_jmp_cgtl] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgtl].data1 = dill_jmp_cgtl;
	  virtual_jump_table->c_data[dill_jmp_cgtl].data2 = DILL_L;
	  virtual_jump_table->jmp_c[dill_jmp_cgtp] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgtp].data1 = dill_jmp_cgtp;
	  virtual_jump_table->c_data[dill_jmp_cgtp].data2 = DILL_P;
	  virtual_jump_table->jmp_c[dill_jmp_cgtd] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgtd].data1 = dill_jmp_cgtd;
	  virtual_jump_table->c_data[dill_jmp_cgtd].data2 = DILL_D;
	  virtual_jump_table->jmp_c[dill_jmp_cgtf] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cgtf].data1 = dill_jmp_cgtf;
	  virtual_jump_table->c_data[dill_jmp_cgtf].data2 = DILL_F;
	  virtual_jump_table->jmp_c[dill_jmp_clec] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_clec].data1 = dill_jmp_clec;
	  virtual_jump_table->c_data[dill_jmp_clec].data2 = DILL_C;
	  virtual_jump_table->jmp_c[dill_jmp_cleuc] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cleuc].data1 = dill_jmp_cleuc;
	  virtual_jump_table->c_data[dill_jmp_cleuc].data2 = DILL_UC;
	  virtual_jump_table->jmp_c[dill_jmp_cles] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cles].data1 = dill_jmp_cles;
	  virtual_jump_table->c_data[dill_jmp_cles].data2 = DILL_S;
	  virtual_jump_table->jmp_c[dill_jmp_cleus] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cleus].data1 = dill_jmp_cleus;
	  virtual_jump_table->c_data[dill_jmp_cleus].data2 = DILL_US;
	  virtual_jump_table->jmp_c[dill_jmp_clei] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_clei].data1 = dill_jmp_clei;
	  virtual_jump_table->c_data[dill_jmp_clei].data2 = DILL_I;
	  virtual_jump_table->jmp_c[dill_jmp_cleu] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cleu].data1 = dill_jmp_cleu;
	  virtual_jump_table->c_data[dill_jmp_cleu].data2 = DILL_U;
	  virtual_jump_table->jmp_c[dill_jmp_cleul] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cleul].data1 = dill_jmp_cleul;
	  virtual_jump_table->c_data[dill_jmp_cleul].data2 = DILL_UL;
	  virtual_jump_table->jmp_c[dill_jmp_clel] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_clel].data1 = dill_jmp_clel;
	  virtual_jump_table->c_data[dill_jmp_clel].data2 = DILL_L;
	  virtual_jump_table->jmp_c[dill_jmp_clep] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_clep].data1 = dill_jmp_clep;
	  virtual_jump_table->c_data[dill_jmp_clep].data2 = DILL_P;
	  virtual_jump_table->jmp_c[dill_jmp_cled] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cled].data1 = dill_jmp_cled;
	  virtual_jump_table->c_data[dill_jmp_cled].data2 = DILL_D;
	  virtual_jump_table->jmp_c[dill_jmp_clef] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_clef].data1 = dill_jmp_clef;
	  virtual_jump_table->c_data[dill_jmp_clef].data2 = DILL_F;
	  virtual_jump_table->jmp_c[dill_jmp_cltc] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cltc].data1 = dill_jmp_cltc;
	  virtual_jump_table->c_data[dill_jmp_cltc].data2 = DILL_C;
	  virtual_jump_table->jmp_c[dill_jmp_cltuc] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cltuc].data1 = dill_jmp_cltuc;
	  virtual_jump_table->c_data[dill_jmp_cltuc].data2 = DILL_UC;
	  virtual_jump_table->jmp_c[dill_jmp_clts] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_clts].data1 = dill_jmp_clts;
	  virtual_jump_table->c_data[dill_jmp_clts].data2 = DILL_S;
	  virtual_jump_table->jmp_c[dill_jmp_cltus] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cltus].data1 = dill_jmp_cltus;
	  virtual_jump_table->c_data[dill_jmp_cltus].data2 = DILL_US;
	  virtual_jump_table->jmp_c[dill_jmp_clti] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_clti].data1 = dill_jmp_clti;
	  virtual_jump_table->c_data[dill_jmp_clti].data2 = DILL_I;
	  virtual_jump_table->jmp_c[dill_jmp_cltu] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cltu].data1 = dill_jmp_cltu;
	  virtual_jump_table->c_data[dill_jmp_cltu].data2 = DILL_U;
	  virtual_jump_table->jmp_c[dill_jmp_cltul] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cltul].data1 = dill_jmp_cltul;
	  virtual_jump_table->c_data[dill_jmp_cltul].data2 = DILL_UL;
	  virtual_jump_table->jmp_c[dill_jmp_cltl] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cltl].data1 = dill_jmp_cltl;
	  virtual_jump_table->c_data[dill_jmp_cltl].data2 = DILL_L;
	  virtual_jump_table->jmp_c[dill_jmp_cltp] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cltp].data1 = dill_jmp_cltp;
	  virtual_jump_table->c_data[dill_jmp_cltp].data2 = DILL_P;
	  virtual_jump_table->jmp_c[dill_jmp_cltd] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cltd].data1 = dill_jmp_cltd;
	  virtual_jump_table->c_data[dill_jmp_cltd].data2 = DILL_D;
	  virtual_jump_table->jmp_c[dill_jmp_cltf] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cltf].data1 = dill_jmp_cltf;
	  virtual_jump_table->c_data[dill_jmp_cltf].data2 = DILL_F;
	  virtual_jump_table->jmp_c[dill_jmp_cnec] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cnec].data1 = dill_jmp_cnec;
	  virtual_jump_table->c_data[dill_jmp_cnec].data2 = DILL_C;
	  virtual_jump_table->jmp_c[dill_jmp_cneuc] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cneuc].data1 = dill_jmp_cneuc;
	  virtual_jump_table->c_data[dill_jmp_cneuc].data2 = DILL_UC;
	  virtual_jump_table->jmp_c[dill_jmp_cnes] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cnes].data1 = dill_jmp_cnes;
	  virtual_jump_table->c_data[dill_jmp_cnes].data2 = DILL_S;
	  virtual_jump_table->jmp_c[dill_jmp_cneus] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cneus].data1 = dill_jmp_cneus;
	  virtual_jump_table->c_data[dill_jmp_cneus].data2 = DILL_US;
	  virtual_jump_table->jmp_c[dill_jmp_cnei] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cnei].data1 = dill_jmp_cnei;
	  virtual_jump_table->c_data[dill_jmp_cnei].data2 = DILL_I;
	  virtual_jump_table->jmp_c[dill_jmp_cneu] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cneu].data1 = dill_jmp_cneu;
	  virtual_jump_table->c_data[dill_jmp_cneu].data2 = DILL_U;
	  virtual_jump_table->jmp_c[dill_jmp_cneul] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cneul].data1 = dill_jmp_cneul;
	  virtual_jump_table->c_data[dill_jmp_cneul].data2 = DILL_UL;
	  virtual_jump_table->jmp_c[dill_jmp_cnel] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cnel].data1 = dill_jmp_cnel;
	  virtual_jump_table->c_data[dill_jmp_cnel].data2 = DILL_L;
	  virtual_jump_table->jmp_c[dill_jmp_cnep] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cnep].data1 = dill_jmp_cnep;
	  virtual_jump_table->c_data[dill_jmp_cnep].data2 = DILL_P;
	  virtual_jump_table->jmp_c[dill_jmp_cned] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cned].data1 = dill_jmp_cned;
	  virtual_jump_table->c_data[dill_jmp_cned].data2 = DILL_D;
	  virtual_jump_table->jmp_c[dill_jmp_cnef] = virtual_compare;
	  virtual_jump_table->c_data[dill_jmp_cnef].data1 = dill_jmp_cnef;
	  virtual_jump_table->c_data[dill_jmp_cnef].data2 = DILL_F;
# line 74 "virtual.ops"
	  virtual_jump_table->proc_start = (dill_mach_proc_start) virtual_proc_start;
	  virtual_jump_table->end = (dill_mach_end)virtual_end;
	  virtual_jump_table->package_end = (dill_mach_end)virtual_package_end;
	  if (s->p->native.mach_jump != NULL) {
	    virtual_jump_table->type_align = s->p->native.mach_jump->type_align;
	    virtual_jump_table->type_size = s->p->native.mach_jump->type_size;
	  } else {
	    virtual_jump_table->type_align = virtual_type_align;
	    virtual_jump_table->type_size = virtual_type_size;
	  }
	  virtual_jump_table->ret = virtual_ret;
	  virtual_jump_table->reti = virtual_reti;
	  virtual_jump_table->retf = (ret_opf)virtual_reti;
	  virtual_jump_table->load = virtual_load;
	  virtual_jump_table->bsload = virtual_pbsload;
	  virtual_jump_table->loadi = virtual_loadi;
	  virtual_jump_table->bsloadi = virtual_pbsloadi;
	  virtual_jump_table->store = virtual_store;
	  virtual_jump_table->storei = virtual_storei;
	  virtual_jump_table->convert = virtual_convert;
	  virtual_jump_table->mov = virtual_mov;
	  virtual_jump_table->set = virtual_pset;
	  virtual_jump_table->setf = virtual_setf;
	  virtual_jump_table->setp = virtual_setp;
	  virtual_jump_table->jv = virtual_jump_to_label;
	  virtual_jump_table->jp = virtual_jump_to_reg;
	  virtual_jump_table->jpi = virtual_jump_to_imm;
	  virtual_jump_table->special = virtual_special;
	  virtual_jump_table->push = virtual_push;
	  virtual_jump_table->pushi = virtual_pushi;
	  virtual_jump_table->pushfi = virtual_pushfi;
	  virtual_jump_table->pushpi = virtual_pushpi;
	  virtual_jump_table->calli = virtual_calli;
	  virtual_jump_table->callr = virtual_callr;
	virtual_jump_table->lea = virtual_lea;
	  virtual_jump_table->mark_label = virtual_mark_label;
	  virtual_jump_table->init_disassembly = virtual_init_disassembly_info;
	  virtual_jump_table->print_insn = virtual_print_insn;
	  virtual_jump_table->print_reg = NULL;
	  virtual_jump_table->count_insn = NULL;
	  if (s->p->native.mach_jump != NULL) {
	    virtual_jump_table->do_reverse_push = s->p->native.mach_jump->do_reverse_push;
	    virtual_jump_table->target_byte_order = s->p->native.mach_jump->target_byte_order;
	    virtual_jump_table->target_float_format = s->p->native.mach_jump->target_float_format;
	  } else {
	    virtual_jump_table->do_reverse_push = 0;
#ifdef WORDS_BIGENDIAN
	    virtual_jump_table->target_byte_order = 1;  /* Format_Integer_bigendian */
	    virtual_jump_table->target_float_format = 1;  /* Format_IEEE_754_bigendian */ 
#else
	    virtual_jump_table->target_byte_order = 2;  /* Format_Integer_littleendian */
	    virtual_jump_table->target_float_format = 2;  /* Format_IEEE_754_littleendian */ 
#endif
	  }
# line 132 "virtual.ops"
	if (s->p->virtual.mach_info == NULL) {
	    s->p->virtual.mach_info = malloc(sizeof(struct mach_info));
	}
	s->p->mach_info = s->p->virtual.mach_info;
	s->p->mach_reset = s->p->virtual.mach_reset;
	s->p->code_base = s->p->virtual.code_base;
	s->p->cur_ip = s->p->code_base;
	s->p->code_limit = s->p->virtual.code_limit;
	s->p->vreg_count = 0;
	s->dill_local_pointer = dill_getreg(s, DILL_P);
	s->dill_param_reg_pointer = s->dill_local_pointer;
	s->j = virtual_jump_table;
	return;
}
