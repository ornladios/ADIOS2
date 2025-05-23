#ifndef __DILL_H__
#define __DILL_H__
/*! \file */

/* This file is generated from base.ops.  Do not edit directly. */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif


#define IMM_TYPE long
#define UIMM_TYPE unsigned long

struct dill_private_ctx;

typedef struct dill_private_ctx *private_ctx;

struct jmp_table_s;

typedef struct jmp_table_s *jmp_table;

/*!
 * dill_stream is the basic handle for a dynamic instruction stream.
 * It is used in nearly all calls to setup parameter profiles generate
 * instructions.  Using a separate handle rather than static variables to
 * store this information over the multiple calls needed for code generation
 * ensures thread safety. 
 */
typedef struct dill_stream_s *dill_stream;

typedef struct dill_exec_s *dill_exec_handle;

typedef struct dec *dill_exec_ctx;

typedef int dill_reg;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
struct dill_stream_s {
    jmp_table j;
    private_ctx p;
    int dill_local_pointer;
    int dill_param_reg_pointer;
    int dill_debug;
};

struct dec;

typedef enum {DILL_NOP = 0, DILL_SEGMENTED_FOLLOWS} special_operations;
typedef enum {DILL_X86_CS_PREFIX = 0, DILL_X86_SS_PREFIX,
              DILL_X86_DS_PREFIX, DILL_X86_ES_PREFIX,
              DILL_X86_FS_PREFIX, DILL_X86_GS_PREFIX} segment_prefix;

typedef void (*dill_mach_init_routine)(dill_stream s);
typedef void (*dill_mach_proc_start)(dill_stream s, char *name, int param_count, void *params, dill_reg *arg_reg_list);
typedef void (*dill_mach_end)(dill_stream s);
typedef void (*dill_mach_package_end)(dill_stream s);
typedef void (*dill_mach_package_stitch)(void *package, void *externs);
typedef void *(*dill_mach_clone_code)(dill_stream s, void *new_base, int size);
typedef void (*arith_op3)(dill_stream s, int data1, int data2, int dest, int src1, int src2);
typedef void (*arith_op3i)(dill_stream s, int data1, int data2, int dest, int src1, IMM_TYPE imm);
typedef void (*arith_op3fi)(dill_stream s, int data1, int data2, int dest, int src1, double imm);
typedef void (*arith_op2)(dill_stream s, int data1, int data2, int dest, int src);
typedef void (*arith_op2i)(dill_stream s, int data1, int data2, int dest, IMM_TYPE imm);
typedef void (*branch_op)(dill_stream s, int data1, int data2, int src1, int src2, int label);
typedef void (*branch_opi)(dill_stream s, int data1, int data2, int src, IMM_TYPE imm, int label);
typedef void (*compare_op)(dill_stream s, int data1, int data2, int dest, int src1, int src2);
typedef void (*compare_opi)(dill_stream s, int data1, int data2, int dest, int src, IMM_TYPE imm);
typedef void (*cvt_op)(dill_stream s, int data1, int data2, int dest, int src);
typedef void (*cvt_opi)(dill_stream s, int data1, int data2, int dest, IMM_TYPE imm);
typedef void (*ldst_op)(dill_stream s, int data1, int data2, int dest, int src1, int src2);
typedef void (*ldst_opi)(dill_stream s, int data1, int data2, int dest, int src1, IMM_TYPE imm);
typedef void (*ret_op)(dill_stream s, int data1, int data2, int src);
typedef void (*ret_opi)(dill_stream s, int data1, int data2, IMM_TYPE imm);
typedef void (*ret_opf)(dill_stream s, int data1, int data2, double imm);
typedef void (*jmp_op)(dill_stream s, unsigned long arg);
typedef void (*jmp_opa)(dill_stream s, void *arg);
typedef void (*jal_op)(dill_stream s, int dest, int target);
typedef void (*special_op)(dill_stream s, special_operations type, IMM_TYPE param);
typedef int (*call_opi)(dill_stream s, int type, void *xfer_address, const char *name);
typedef int (*call_opr)(dill_stream s, int type, int src);
typedef void (*setf_opi)(dill_stream s, int data1, int data2, int dest, double imm);
typedef void (*setp_opi)(dill_stream s, int data1, int data2, int dest, void *imm);
typedef void (*push_op)(dill_stream s, int type, int reg);
typedef void (*push_opi)(dill_stream s, int type, IMM_TYPE value);
typedef void (*push_oppi)(dill_stream s, int type, void *value);
typedef void (*push_opfi)(dill_stream s, int type, double value);
typedef int (*local_op)(dill_stream s, int flag, int val);
typedef void (*save_restore_op)(dill_stream s, int save_restore_flag, int type, int reg);
typedef void (*lea_op)(dill_stream s, int data1, int data2, int dest, int src, IMM_TYPE offset);
typedef int (*init_disassembly_op)(dill_stream s, void *dis_info);
typedef int (*print_insn_op)(dill_stream s, void *dis_info, void *insn);
typedef void (*print_reg_op)(dill_stream s, int typ, int reg);
typedef int (*count_insn_op)(dill_stream s, int start, int end);

typedef struct jmp_data_s {
    short data1;
    short data2;
} jmp_data;

struct jmp_table_s {
    dill_mach_init_routine init;
    dill_mach_proc_start proc_start;
    dill_mach_end end;
    dill_mach_package_end package_end;
    dill_mach_clone_code clone_code;
    int *type_align;
    arith_op3 *jmp_a3;
    jmp_data *a3_data;
    arith_op3i *jmp_a3i;
    jmp_data *a3i_data;
    arith_op3fi *jmp_f3i;
    jmp_data *f3i_data;
    arith_op2 *jmp_a2;
    arith_op2i *jmp_a2i;
    jmp_data *a2_data;
    branch_op *jmp_b;
    branch_opi *jmp_bi;
    jmp_data *b_data;
    compare_op *jmp_c;
    compare_opi *jmp_ci;
    jmp_data *c_data;
    cvt_op convert;
    ldst_op load;
    ldst_op bsload;
    ldst_opi loadi;
    ldst_opi bsloadi;
    ldst_op store;
    ldst_opi storei;
    ret_op ret;
    ret_opi reti;
    ret_opf retf;
    arith_op2 mov;
    arith_op2i set;
    setf_opi setf;
    setp_opi setp;
    jmp_op jv;
    jmp_op jp;
    jmp_opa jpi;
    jal_op jal;
    special_op special;
    call_opi calli;
    call_opr callr;
    push_op push;
    push_opi pushi;
    push_opfi pushfi;
    push_oppi pushpi;
    local_op local;
    save_restore_op save_restore;
    branch_op mark_label;
    
    init_disassembly_op init_disassembly;
    print_insn_op print_insn;
    print_reg_op print_reg;
    count_insn_op count_insn;
    int do_reverse_push;
    int *type_size;
    lea_op lea;
    int target_byte_order;
    int target_float_format;
};

typedef struct { 
    int is_register;		/* true if parameter is in register */
    unsigned reg;		/* register it's in */
    int offset;			/* otherwise at this offset from stack */
} dill_parameter_type;

#endif

/*! Start a DCG native instruction stream
 *
 * This, or dill_create_stream(), is usually the first call in dynamically
 * generating a subroutine or function.  The dill_stream created here is
 * used in all future calls, setting up the parameter profile and generating
 * the instructions into the stream.  If dill_create_raw_stream() is used,
 * DILL will start generating native instructions directly and all
 * register parameters passed to subsequent calls are assumed to be the IDs
 * of physical registers.
 * \return Will return a new initialized dill_stream unless there is
 * no available memory.
 */
extern dill_stream dill_create_raw_stream(void);

/*! Start a DCG virtual instruction stream
 *
 * This, or dill_create_raw_stream(), is usually the first call in
 * dynamically generating a subroutine or function.  The dill_stream created
 * here is used in all future calls, setting up the parameter profile and
 * generating the instructions into the stream.  If dill_create_stream()
 * is used, DILL will operate in virtual mode.  Register parameters passed
 * to subsequent calls are assumed to be the IDs out of a set of infinite
 * virtual registers.  DILL will assign those virtual registers to physical
 * registers and generate native instructions only at dr_end().
 * \return Will return a new initialized dill_stream unless there is
 * no available memory.
 */
extern dill_stream dill_create_stream(void);
extern dill_stream dill_dup_stream(dill_stream s);

extern dill_exec_ctx dill_get_exec_context(dill_stream x);
extern void dill_free_stream(dill_stream s);
extern void dill_free_exec_context(dill_exec_ctx c);
extern void dill_assoc_client_data(dill_exec_ctx ec, int key, IMM_TYPE value);
extern IMM_TYPE dill_get_client_data(dill_exec_ctx ec, int key);
extern void* dill_take_code(dill_stream s);
extern int dill_alloc_label(dill_stream s, char *name);
extern void dill_mark_label(dill_stream s, int label);
extern int dill_is_label_mark(dill_stream s);
extern int dill_raw_getreg(dill_stream s, dill_reg *reg_p, int type, int reg_class);
extern int dill_getreg(dill_stream s, int typ);
extern void dill_raw_putreg(dill_stream s, dill_reg reg_p, int type);
extern void dill_raw_unavailreg(dill_stream s, int type, dill_reg reg);
extern void dill_raw_availreg(dill_stream s, int type, dill_reg reg);
extern void dill_alloc_specific(dill_stream s, dill_reg reg_p, int type, int reg_class);
extern void dill_dealloc_specific(dill_stream s, dill_reg reg_p, int type, int reg_class);
extern void dill_start_simple_proc(dill_stream s, const char *name, int ret_type);
extern void dill_param_alloc(dill_stream s, int argno, int type, 
			  dill_reg *regp);
extern void dill_param_struct_alloc(dill_stream s, int argno, int type, 
				 dill_parameter_type *paramp);
extern dill_reg dill_param_reg(dill_stream s, int argno);
extern dill_reg dill_vparam(dill_stream s, int argno);
extern void dill_start_proc(dill_stream s, char *name, int ret_type, char *arg_str);
extern void dill_dump(dill_stream s);
extern dill_exec_handle dill_finalize(dill_stream s);
extern void dill_begin_prefix_code(dill_stream s);
extern dill_exec_handle dill_get_handle(dill_stream s);
extern char *dill_finalize_package(dill_stream s, int *pkg_len);
typedef struct _dill_extern_entry {
    /*! the textual name of the external entry */
    char *extern_name;
    /*! the address of the external entry */
    void *extern_value;
} dill_extern_entry;

extern dill_exec_handle dill_package_stitch(char *package, dill_extern_entry* externs);
extern void *dill_package_entry(char* package);
extern void dill_free_handle(dill_exec_handle h);
extern void dill_ref_handle(dill_exec_handle h);
extern void *dill_get_fp(dill_exec_handle h);
extern void *dill_clone_code(dill_stream s, void *new_base, int size);
extern int dill_code_size(dill_stream s);
extern int dill_do_reverse_vararg_push(dill_stream s);
extern int dill_getvblock(dill_stream s, int size);
extern void dill_virtual_lea(dill_stream s, int dest, int src);
extern void dill_pbr(dill_stream s, int op_type, int data_type, dill_reg src1, 
		   dill_reg src2, int label);
extern void dill_markused(dill_stream s, int type, int reg);
extern void dill_pcompare(dill_stream s, int op_type, int data_type, 
			  dill_reg dest, dill_reg src1, 
			  dill_reg src2);

/*! Return the 'this' pointer to be used in a DCG method call
 *
 * Given a method pointer and an object pointer, return the 'this'
 * pointer that must be provided during call to that method.  This may be
 * the same as the object address, or in the case of multiple inheritance it
 * may be different.  This routine is used in conjunction with
 * get_xfer_ptr() to support DCG of C++ method calls.
 * \return the address (as a void*) of the method's code
 */
extern void *get_this_ptr(void *method_ptr, void *object_ptr);

/*!  Return the transfer address to be used in a DCG method call
 *
 * Given a method pointer and an object pointer, return the transfer address
 * that should be used to call the method.  I.E. the base address of the
 * subroutine that implements it.  This routine is used in conjunction with
 * get_this_ptr() to support DCG of C++ method calls.  
 * \return the address (as a void*) of the method's code
 */
extern void *get_xfer_ptr(void *method_ptr, void *object_ptr);

#define dill_type_align(s, t) s->j->type_align[t]
#define dill_type_size(s, t) s->j->type_size[t]
#define dill_target_byte_order(s) s->j->target_byte_order
#define dill_target_float_format(s) s->j->target_float_format
#define dill_lp(s) s->dill_local_pointer
#define dill_pp(s) s->dill_param_reg_pointer
#define dill_local(s, type) s->j->local(s, 0, type)
#define dill_localb(s, bytes) s->j->local(s, 1, bytes)


/* atomic types */
enum {
    DILL_C,    /* char */
    DILL_UC,   /* unsigned char */
    DILL_S,    /* short */
    DILL_US,   /* unsigned short */
    DILL_I,    /* int */
    DILL_U,    /* unsigned */
    DILL_L,    /* long (full register size */
    DILL_UL,   /* unsigned long (full register size) */
    DILL_P,    /* pointer */
    DILL_F,    /* floating */
    DILL_D,    /* double */
    DILL_V,    /* void */
    DILL_B,    /* block structure */
    DILL_EC,   /* execution context */
    DILL_ERR   /* no type */
};

enum {DILL_VAR, DILL_TEMP};

/** @defgroup insns DILL instruction issue functions
 *
 * This group of functions is used to manage CM-returned memory.
 * They are provided to handle the eventuality when CM uses its own memory
 * manager.  That hasn't happened yet, so these are identical to realloc,
 * malloc and free.
 */
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define dill_jmp_addi 0
#define dill_jmp_addu 1
#define dill_jmp_addul 2
#define dill_jmp_addl 3
#define dill_jmp_subi 4
#define dill_jmp_subu 5
#define dill_jmp_subul 6
#define dill_jmp_subl 7
#define dill_jmp_muli 8
#define dill_jmp_mulu 9
#define dill_jmp_mulul 10
#define dill_jmp_mull 11
#define dill_jmp_divi 12
#define dill_jmp_divu 13
#define dill_jmp_divul 14
#define dill_jmp_divl 15
#define dill_jmp_modi 16
#define dill_jmp_modu 17
#define dill_jmp_modul 18
#define dill_jmp_modl 19
#define dill_jmp_xori 20
#define dill_jmp_xoru 21
#define dill_jmp_xorul 22
#define dill_jmp_xorl 23
#define dill_jmp_andi 24
#define dill_jmp_andu 25
#define dill_jmp_andul 26
#define dill_jmp_andl 27
#define dill_jmp_ori 28
#define dill_jmp_oru 29
#define dill_jmp_orul 30
#define dill_jmp_orl 31
#define dill_jmp_lshi 32
#define dill_jmp_lshu 33
#define dill_jmp_lshul 34
#define dill_jmp_lshl 35
#define dill_jmp_rshi 36
#define dill_jmp_rshu 37
#define dill_jmp_rshul 38
#define dill_jmp_rshl 39
#endif
#define dill_addi(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_addi](s, s->j->a3_data[dill_jmp_addi].data1, s->j->a3_data[dill_jmp_addi].data2, dest, src1, src2)
#define dill_addii(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_addi](s, s->j->a3i_data[dill_jmp_addi].data1, s->j->a3i_data[dill_jmp_addi].data2, dest, src1, imm)
#define dill_addu(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_addu](s, s->j->a3_data[dill_jmp_addu].data1, s->j->a3_data[dill_jmp_addu].data2, dest, src1, src2)
#define dill_addui(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_addu](s, s->j->a3i_data[dill_jmp_addu].data1, s->j->a3i_data[dill_jmp_addu].data2, dest, src1, imm)
#define dill_addul(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_addul](s, s->j->a3_data[dill_jmp_addul].data1, s->j->a3_data[dill_jmp_addul].data2, dest, src1, src2)
#define dill_adduli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_addul](s, s->j->a3i_data[dill_jmp_addul].data1, s->j->a3i_data[dill_jmp_addul].data2, dest, src1, imm)
#define dill_addl(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_addl](s, s->j->a3_data[dill_jmp_addl].data1, s->j->a3_data[dill_jmp_addl].data2, dest, src1, src2)
#define dill_addli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_addl](s, s->j->a3i_data[dill_jmp_addl].data1, s->j->a3i_data[dill_jmp_addl].data2, dest, src1, imm)
#define dill_subi(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_subi](s, s->j->a3_data[dill_jmp_subi].data1, s->j->a3_data[dill_jmp_subi].data2, dest, src1, src2)
#define dill_subii(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_subi](s, s->j->a3i_data[dill_jmp_subi].data1, s->j->a3i_data[dill_jmp_subi].data2, dest, src1, imm)
#define dill_subu(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_subu](s, s->j->a3_data[dill_jmp_subu].data1, s->j->a3_data[dill_jmp_subu].data2, dest, src1, src2)
#define dill_subui(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_subu](s, s->j->a3i_data[dill_jmp_subu].data1, s->j->a3i_data[dill_jmp_subu].data2, dest, src1, imm)
#define dill_subul(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_subul](s, s->j->a3_data[dill_jmp_subul].data1, s->j->a3_data[dill_jmp_subul].data2, dest, src1, src2)
#define dill_subuli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_subul](s, s->j->a3i_data[dill_jmp_subul].data1, s->j->a3i_data[dill_jmp_subul].data2, dest, src1, imm)
#define dill_subl(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_subl](s, s->j->a3_data[dill_jmp_subl].data1, s->j->a3_data[dill_jmp_subl].data2, dest, src1, src2)
#define dill_subli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_subl](s, s->j->a3i_data[dill_jmp_subl].data1, s->j->a3i_data[dill_jmp_subl].data2, dest, src1, imm)
#define dill_muli(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_muli](s, s->j->a3_data[dill_jmp_muli].data1, s->j->a3_data[dill_jmp_muli].data2, dest, src1, src2)
#define dill_mulii(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_muli](s, s->j->a3i_data[dill_jmp_muli].data1, s->j->a3i_data[dill_jmp_muli].data2, dest, src1, imm)
#define dill_mulu(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_mulu](s, s->j->a3_data[dill_jmp_mulu].data1, s->j->a3_data[dill_jmp_mulu].data2, dest, src1, src2)
#define dill_mului(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_mulu](s, s->j->a3i_data[dill_jmp_mulu].data1, s->j->a3i_data[dill_jmp_mulu].data2, dest, src1, imm)
#define dill_mulul(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_mulul](s, s->j->a3_data[dill_jmp_mulul].data1, s->j->a3_data[dill_jmp_mulul].data2, dest, src1, src2)
#define dill_mululi(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_mulul](s, s->j->a3i_data[dill_jmp_mulul].data1, s->j->a3i_data[dill_jmp_mulul].data2, dest, src1, imm)
#define dill_mull(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_mull](s, s->j->a3_data[dill_jmp_mull].data1, s->j->a3_data[dill_jmp_mull].data2, dest, src1, src2)
#define dill_mulli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_mull](s, s->j->a3i_data[dill_jmp_mull].data1, s->j->a3i_data[dill_jmp_mull].data2, dest, src1, imm)
#define dill_divi(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_divi](s, s->j->a3_data[dill_jmp_divi].data1, s->j->a3_data[dill_jmp_divi].data2, dest, src1, src2)
#define dill_divii(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_divi](s, s->j->a3i_data[dill_jmp_divi].data1, s->j->a3i_data[dill_jmp_divi].data2, dest, src1, imm)
#define dill_divu(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_divu](s, s->j->a3_data[dill_jmp_divu].data1, s->j->a3_data[dill_jmp_divu].data2, dest, src1, src2)
#define dill_divui(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_divu](s, s->j->a3i_data[dill_jmp_divu].data1, s->j->a3i_data[dill_jmp_divu].data2, dest, src1, imm)
#define dill_divul(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_divul](s, s->j->a3_data[dill_jmp_divul].data1, s->j->a3_data[dill_jmp_divul].data2, dest, src1, src2)
#define dill_divuli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_divul](s, s->j->a3i_data[dill_jmp_divul].data1, s->j->a3i_data[dill_jmp_divul].data2, dest, src1, imm)
#define dill_divl(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_divl](s, s->j->a3_data[dill_jmp_divl].data1, s->j->a3_data[dill_jmp_divl].data2, dest, src1, src2)
#define dill_divli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_divl](s, s->j->a3i_data[dill_jmp_divl].data1, s->j->a3i_data[dill_jmp_divl].data2, dest, src1, imm)
#define dill_modi(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_modi](s, s->j->a3_data[dill_jmp_modi].data1, s->j->a3_data[dill_jmp_modi].data2, dest, src1, src2)
#define dill_modii(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_modi](s, s->j->a3i_data[dill_jmp_modi].data1, s->j->a3i_data[dill_jmp_modi].data2, dest, src1, imm)
#define dill_modu(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_modu](s, s->j->a3_data[dill_jmp_modu].data1, s->j->a3_data[dill_jmp_modu].data2, dest, src1, src2)
#define dill_modui(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_modu](s, s->j->a3i_data[dill_jmp_modu].data1, s->j->a3i_data[dill_jmp_modu].data2, dest, src1, imm)
#define dill_modul(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_modul](s, s->j->a3_data[dill_jmp_modul].data1, s->j->a3_data[dill_jmp_modul].data2, dest, src1, src2)
#define dill_moduli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_modul](s, s->j->a3i_data[dill_jmp_modul].data1, s->j->a3i_data[dill_jmp_modul].data2, dest, src1, imm)
#define dill_modl(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_modl](s, s->j->a3_data[dill_jmp_modl].data1, s->j->a3_data[dill_jmp_modl].data2, dest, src1, src2)
#define dill_modli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_modl](s, s->j->a3i_data[dill_jmp_modl].data1, s->j->a3i_data[dill_jmp_modl].data2, dest, src1, imm)
#define dill_xori(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_xori](s, s->j->a3_data[dill_jmp_xori].data1, s->j->a3_data[dill_jmp_xori].data2, dest, src1, src2)
#define dill_xorii(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_xori](s, s->j->a3i_data[dill_jmp_xori].data1, s->j->a3i_data[dill_jmp_xori].data2, dest, src1, imm)
#define dill_xoru(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_xoru](s, s->j->a3_data[dill_jmp_xoru].data1, s->j->a3_data[dill_jmp_xoru].data2, dest, src1, src2)
#define dill_xorui(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_xoru](s, s->j->a3i_data[dill_jmp_xoru].data1, s->j->a3i_data[dill_jmp_xoru].data2, dest, src1, imm)
#define dill_xorul(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_xorul](s, s->j->a3_data[dill_jmp_xorul].data1, s->j->a3_data[dill_jmp_xorul].data2, dest, src1, src2)
#define dill_xoruli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_xorul](s, s->j->a3i_data[dill_jmp_xorul].data1, s->j->a3i_data[dill_jmp_xorul].data2, dest, src1, imm)
#define dill_xorl(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_xorl](s, s->j->a3_data[dill_jmp_xorl].data1, s->j->a3_data[dill_jmp_xorl].data2, dest, src1, src2)
#define dill_xorli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_xorl](s, s->j->a3i_data[dill_jmp_xorl].data1, s->j->a3i_data[dill_jmp_xorl].data2, dest, src1, imm)
#define dill_andi(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_andi](s, s->j->a3_data[dill_jmp_andi].data1, s->j->a3_data[dill_jmp_andi].data2, dest, src1, src2)
#define dill_andii(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_andi](s, s->j->a3i_data[dill_jmp_andi].data1, s->j->a3i_data[dill_jmp_andi].data2, dest, src1, imm)
#define dill_andu(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_andu](s, s->j->a3_data[dill_jmp_andu].data1, s->j->a3_data[dill_jmp_andu].data2, dest, src1, src2)
#define dill_andui(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_andu](s, s->j->a3i_data[dill_jmp_andu].data1, s->j->a3i_data[dill_jmp_andu].data2, dest, src1, imm)
#define dill_andul(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_andul](s, s->j->a3_data[dill_jmp_andul].data1, s->j->a3_data[dill_jmp_andul].data2, dest, src1, src2)
#define dill_anduli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_andul](s, s->j->a3i_data[dill_jmp_andul].data1, s->j->a3i_data[dill_jmp_andul].data2, dest, src1, imm)
#define dill_andl(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_andl](s, s->j->a3_data[dill_jmp_andl].data1, s->j->a3_data[dill_jmp_andl].data2, dest, src1, src2)
#define dill_andli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_andl](s, s->j->a3i_data[dill_jmp_andl].data1, s->j->a3i_data[dill_jmp_andl].data2, dest, src1, imm)
#define dill_ori(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_ori](s, s->j->a3_data[dill_jmp_ori].data1, s->j->a3_data[dill_jmp_ori].data2, dest, src1, src2)
#define dill_orii(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_ori](s, s->j->a3i_data[dill_jmp_ori].data1, s->j->a3i_data[dill_jmp_ori].data2, dest, src1, imm)
#define dill_oru(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_oru](s, s->j->a3_data[dill_jmp_oru].data1, s->j->a3_data[dill_jmp_oru].data2, dest, src1, src2)
#define dill_orui(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_oru](s, s->j->a3i_data[dill_jmp_oru].data1, s->j->a3i_data[dill_jmp_oru].data2, dest, src1, imm)
#define dill_orul(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_orul](s, s->j->a3_data[dill_jmp_orul].data1, s->j->a3_data[dill_jmp_orul].data2, dest, src1, src2)
#define dill_oruli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_orul](s, s->j->a3i_data[dill_jmp_orul].data1, s->j->a3i_data[dill_jmp_orul].data2, dest, src1, imm)
#define dill_orl(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_orl](s, s->j->a3_data[dill_jmp_orl].data1, s->j->a3_data[dill_jmp_orl].data2, dest, src1, src2)
#define dill_orli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_orl](s, s->j->a3i_data[dill_jmp_orl].data1, s->j->a3i_data[dill_jmp_orl].data2, dest, src1, imm)
#define dill_lshi(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_lshi](s, s->j->a3_data[dill_jmp_lshi].data1, s->j->a3_data[dill_jmp_lshi].data2, dest, src1, src2)
#define dill_lshii(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_lshi](s, s->j->a3i_data[dill_jmp_lshi].data1, s->j->a3i_data[dill_jmp_lshi].data2, dest, src1, imm)
#define dill_lshu(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_lshu](s, s->j->a3_data[dill_jmp_lshu].data1, s->j->a3_data[dill_jmp_lshu].data2, dest, src1, src2)
#define dill_lshui(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_lshu](s, s->j->a3i_data[dill_jmp_lshu].data1, s->j->a3i_data[dill_jmp_lshu].data2, dest, src1, imm)
#define dill_lshul(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_lshul](s, s->j->a3_data[dill_jmp_lshul].data1, s->j->a3_data[dill_jmp_lshul].data2, dest, src1, src2)
#define dill_lshuli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_lshul](s, s->j->a3i_data[dill_jmp_lshul].data1, s->j->a3i_data[dill_jmp_lshul].data2, dest, src1, imm)
#define dill_lshl(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_lshl](s, s->j->a3_data[dill_jmp_lshl].data1, s->j->a3_data[dill_jmp_lshl].data2, dest, src1, src2)
#define dill_lshli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_lshl](s, s->j->a3i_data[dill_jmp_lshl].data1, s->j->a3i_data[dill_jmp_lshl].data2, dest, src1, imm)
#define dill_rshi(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_rshi](s, s->j->a3_data[dill_jmp_rshi].data1, s->j->a3_data[dill_jmp_rshi].data2, dest, src1, src2)
#define dill_rshii(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_rshi](s, s->j->a3i_data[dill_jmp_rshi].data1, s->j->a3i_data[dill_jmp_rshi].data2, dest, src1, imm)
#define dill_rshu(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_rshu](s, s->j->a3_data[dill_jmp_rshu].data1, s->j->a3_data[dill_jmp_rshu].data2, dest, src1, src2)
#define dill_rshui(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_rshu](s, s->j->a3i_data[dill_jmp_rshu].data1, s->j->a3i_data[dill_jmp_rshu].data2, dest, src1, imm)
#define dill_rshul(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_rshul](s, s->j->a3_data[dill_jmp_rshul].data1, s->j->a3_data[dill_jmp_rshul].data2, dest, src1, src2)
#define dill_rshuli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_rshul](s, s->j->a3i_data[dill_jmp_rshul].data1, s->j->a3i_data[dill_jmp_rshul].data2, dest, src1, imm)
#define dill_rshl(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_rshl](s, s->j->a3_data[dill_jmp_rshl].data1, s->j->a3_data[dill_jmp_rshl].data2, dest, src1, src2)
#define dill_rshli(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_rshl](s, s->j->a3i_data[dill_jmp_rshl].data1, s->j->a3i_data[dill_jmp_rshl].data2, dest, src1, imm)
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define dill_jmp_addp 40
#define dill_jmp_subp 41
#endif
#define dill_addp(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_addp](s, s->j->a3_data[dill_jmp_addp].data1, s->j->a3_data[dill_jmp_addp].data2, dest, src1, src2)
#define dill_addpi(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_addp](s, s->j->a3i_data[dill_jmp_addp].data1, s->j->a3i_data[dill_jmp_addp].data2, dest, src1, imm)
#define dill_subp(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_subp](s, s->j->a3_data[dill_jmp_subp].data1, s->j->a3_data[dill_jmp_subp].data2, dest, src1, src2)
#define dill_subpi(s, dest, src1, imm) (s->j->jmp_a3i)[dill_jmp_subp](s, s->j->a3i_data[dill_jmp_subp].data1, s->j->a3i_data[dill_jmp_subp].data2, dest, src1, imm)
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define dill_jmp_addf 42
#define dill_jmp_addd 43
#define dill_jmp_subf 44
#define dill_jmp_subd 45
#define dill_jmp_mulf 46
#define dill_jmp_muld 47
#define dill_jmp_divf 48
#define dill_jmp_divd 49
#endif
#define dill_addf(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_addf](s, s->j->a3_data[dill_jmp_addf].data1, s->j->a3_data[dill_jmp_addf].data2, dest, src1, src2)
#define dill_addfi(s, dest, src1, imm) (s->j->jmp_f3i)[dill_jmp_addf](s, s->j->a3f_data[dill_jmp_addf].data1, s->j->a3f_data[dill_jmp_addf].data2, dest, src1, imm)
#define dill_addd(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_addd](s, s->j->a3_data[dill_jmp_addd].data1, s->j->a3_data[dill_jmp_addd].data2, dest, src1, src2)
#define dill_adddi(s, dest, src1, imm) (s->j->jmp_f3i)[dill_jmp_addd](s, s->j->a3f_data[dill_jmp_addd].data1, s->j->a3f_data[dill_jmp_addd].data2, dest, src1, imm)
#define dill_subf(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_subf](s, s->j->a3_data[dill_jmp_subf].data1, s->j->a3_data[dill_jmp_subf].data2, dest, src1, src2)
#define dill_subfi(s, dest, src1, imm) (s->j->jmp_f3i)[dill_jmp_subf](s, s->j->a3f_data[dill_jmp_subf].data1, s->j->a3f_data[dill_jmp_subf].data2, dest, src1, imm)
#define dill_subd(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_subd](s, s->j->a3_data[dill_jmp_subd].data1, s->j->a3_data[dill_jmp_subd].data2, dest, src1, src2)
#define dill_subdi(s, dest, src1, imm) (s->j->jmp_f3i)[dill_jmp_subd](s, s->j->a3f_data[dill_jmp_subd].data1, s->j->a3f_data[dill_jmp_subd].data2, dest, src1, imm)
#define dill_mulf(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_mulf](s, s->j->a3_data[dill_jmp_mulf].data1, s->j->a3_data[dill_jmp_mulf].data2, dest, src1, src2)
#define dill_mulfi(s, dest, src1, imm) (s->j->jmp_f3i)[dill_jmp_mulf](s, s->j->a3f_data[dill_jmp_mulf].data1, s->j->a3f_data[dill_jmp_mulf].data2, dest, src1, imm)
#define dill_muld(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_muld](s, s->j->a3_data[dill_jmp_muld].data1, s->j->a3_data[dill_jmp_muld].data2, dest, src1, src2)
#define dill_muldi(s, dest, src1, imm) (s->j->jmp_f3i)[dill_jmp_muld](s, s->j->a3f_data[dill_jmp_muld].data1, s->j->a3f_data[dill_jmp_muld].data2, dest, src1, imm)
#define dill_divf(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_divf](s, s->j->a3_data[dill_jmp_divf].data1, s->j->a3_data[dill_jmp_divf].data2, dest, src1, src2)
#define dill_divfi(s, dest, src1, imm) (s->j->jmp_f3i)[dill_jmp_divf](s, s->j->a3f_data[dill_jmp_divf].data1, s->j->a3f_data[dill_jmp_divf].data2, dest, src1, imm)
#define dill_divd(s, dest, src1, src2) (s->j->jmp_a3)[dill_jmp_divd](s, s->j->a3_data[dill_jmp_divd].data1, s->j->a3_data[dill_jmp_divd].data2, dest, src1, src2)
#define dill_divdi(s, dest, src1, imm) (s->j->jmp_f3i)[dill_jmp_divd](s, s->j->a3f_data[dill_jmp_divd].data1, s->j->a3f_data[dill_jmp_divd].data2, dest, src1, imm)
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define dill_jmp_noti 0
#define dill_jmp_notu 1
#define dill_jmp_notul 2
#define dill_jmp_notl 3
#define dill_jmp_comi 4
#define dill_jmp_comu 5
#define dill_jmp_comul 6
#define dill_jmp_coml 7
#define dill_jmp_negi 8
#define dill_jmp_negu 9
#define dill_jmp_negul 10
#define dill_jmp_negl 11
#endif
/*! DILL not i operator */
#define dill_noti(s, dest, src) (s->j->jmp_a2)[dill_jmp_noti](s, s->j->a2_data[dill_jmp_noti].data1, s->j->a2_data[dill_jmp_noti].data2, dest, src)
/*! DILL not i immediate operator */
#define dill_notii(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_noti](s, s->j->a2_data[dill_jmp_noti].data1, s->j->a2_data[dill_jmp_noti].data2, dest, imm)
/*! DILL not u operator */
#define dill_notu(s, dest, src) (s->j->jmp_a2)[dill_jmp_notu](s, s->j->a2_data[dill_jmp_notu].data1, s->j->a2_data[dill_jmp_notu].data2, dest, src)
/*! DILL not u immediate operator */
#define dill_notui(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_notu](s, s->j->a2_data[dill_jmp_notu].data1, s->j->a2_data[dill_jmp_notu].data2, dest, imm)
/*! DILL not ul operator */
#define dill_notul(s, dest, src) (s->j->jmp_a2)[dill_jmp_notul](s, s->j->a2_data[dill_jmp_notul].data1, s->j->a2_data[dill_jmp_notul].data2, dest, src)
/*! DILL not ul immediate operator */
#define dill_notuli(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_notul](s, s->j->a2_data[dill_jmp_notul].data1, s->j->a2_data[dill_jmp_notul].data2, dest, imm)
/*! DILL not l operator */
#define dill_notl(s, dest, src) (s->j->jmp_a2)[dill_jmp_notl](s, s->j->a2_data[dill_jmp_notl].data1, s->j->a2_data[dill_jmp_notl].data2, dest, src)
/*! DILL not l immediate operator */
#define dill_notli(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_notl](s, s->j->a2_data[dill_jmp_notl].data1, s->j->a2_data[dill_jmp_notl].data2, dest, imm)
/*! DILL com i operator */
#define dill_comi(s, dest, src) (s->j->jmp_a2)[dill_jmp_comi](s, s->j->a2_data[dill_jmp_comi].data1, s->j->a2_data[dill_jmp_comi].data2, dest, src)
/*! DILL com i immediate operator */
#define dill_comii(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_comi](s, s->j->a2_data[dill_jmp_comi].data1, s->j->a2_data[dill_jmp_comi].data2, dest, imm)
/*! DILL com u operator */
#define dill_comu(s, dest, src) (s->j->jmp_a2)[dill_jmp_comu](s, s->j->a2_data[dill_jmp_comu].data1, s->j->a2_data[dill_jmp_comu].data2, dest, src)
/*! DILL com u immediate operator */
#define dill_comui(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_comu](s, s->j->a2_data[dill_jmp_comu].data1, s->j->a2_data[dill_jmp_comu].data2, dest, imm)
/*! DILL com ul operator */
#define dill_comul(s, dest, src) (s->j->jmp_a2)[dill_jmp_comul](s, s->j->a2_data[dill_jmp_comul].data1, s->j->a2_data[dill_jmp_comul].data2, dest, src)
/*! DILL com ul immediate operator */
#define dill_comuli(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_comul](s, s->j->a2_data[dill_jmp_comul].data1, s->j->a2_data[dill_jmp_comul].data2, dest, imm)
/*! DILL com l operator */
#define dill_coml(s, dest, src) (s->j->jmp_a2)[dill_jmp_coml](s, s->j->a2_data[dill_jmp_coml].data1, s->j->a2_data[dill_jmp_coml].data2, dest, src)
/*! DILL com l immediate operator */
#define dill_comli(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_coml](s, s->j->a2_data[dill_jmp_coml].data1, s->j->a2_data[dill_jmp_coml].data2, dest, imm)
/*! DILL neg i operator */
#define dill_negi(s, dest, src) (s->j->jmp_a2)[dill_jmp_negi](s, s->j->a2_data[dill_jmp_negi].data1, s->j->a2_data[dill_jmp_negi].data2, dest, src)
/*! DILL neg i immediate operator */
#define dill_negii(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_negi](s, s->j->a2_data[dill_jmp_negi].data1, s->j->a2_data[dill_jmp_negi].data2, dest, imm)
/*! DILL neg u operator */
#define dill_negu(s, dest, src) (s->j->jmp_a2)[dill_jmp_negu](s, s->j->a2_data[dill_jmp_negu].data1, s->j->a2_data[dill_jmp_negu].data2, dest, src)
/*! DILL neg u immediate operator */
#define dill_negui(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_negu](s, s->j->a2_data[dill_jmp_negu].data1, s->j->a2_data[dill_jmp_negu].data2, dest, imm)
/*! DILL neg ul operator */
#define dill_negul(s, dest, src) (s->j->jmp_a2)[dill_jmp_negul](s, s->j->a2_data[dill_jmp_negul].data1, s->j->a2_data[dill_jmp_negul].data2, dest, src)
/*! DILL neg ul immediate operator */
#define dill_neguli(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_negul](s, s->j->a2_data[dill_jmp_negul].data1, s->j->a2_data[dill_jmp_negul].data2, dest, imm)
/*! DILL neg l operator */
#define dill_negl(s, dest, src) (s->j->jmp_a2)[dill_jmp_negl](s, s->j->a2_data[dill_jmp_negl].data1, s->j->a2_data[dill_jmp_negl].data2, dest, src)
/*! DILL neg l immediate operator */
#define dill_negli(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_negl](s, s->j->a2_data[dill_jmp_negl].data1, s->j->a2_data[dill_jmp_negl].data2, dest, imm)
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define dill_jmp_bswaps 12
#define dill_jmp_bswapus 13
#define dill_jmp_bswapi 14
#define dill_jmp_bswapu 15
#define dill_jmp_bswapul 16
#define dill_jmp_bswapl 17
#define dill_jmp_bswapf 18
#define dill_jmp_bswapd 19
#endif
/*! DILL bswap s operator */
#define dill_bswaps(s, dest, src) (s->j->jmp_a2)[dill_jmp_bswaps](s, s->j->a2_data[dill_jmp_bswaps].data1, s->j->a2_data[dill_jmp_bswaps].data2, dest, src)
/*! DILL bswap s immediate operator */
#define dill_bswapsi(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_bswaps](s, s->j->a2_data[dill_jmp_bswaps].data1, s->j->a2_data[dill_jmp_bswaps].data2, dest, imm)
/*! DILL bswap us operator */
#define dill_bswapus(s, dest, src) (s->j->jmp_a2)[dill_jmp_bswapus](s, s->j->a2_data[dill_jmp_bswapus].data1, s->j->a2_data[dill_jmp_bswapus].data2, dest, src)
/*! DILL bswap us immediate operator */
#define dill_bswapusi(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_bswapus](s, s->j->a2_data[dill_jmp_bswapus].data1, s->j->a2_data[dill_jmp_bswapus].data2, dest, imm)
/*! DILL bswap i operator */
#define dill_bswapi(s, dest, src) (s->j->jmp_a2)[dill_jmp_bswapi](s, s->j->a2_data[dill_jmp_bswapi].data1, s->j->a2_data[dill_jmp_bswapi].data2, dest, src)
/*! DILL bswap i immediate operator */
#define dill_bswapii(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_bswapi](s, s->j->a2_data[dill_jmp_bswapi].data1, s->j->a2_data[dill_jmp_bswapi].data2, dest, imm)
/*! DILL bswap u operator */
#define dill_bswapu(s, dest, src) (s->j->jmp_a2)[dill_jmp_bswapu](s, s->j->a2_data[dill_jmp_bswapu].data1, s->j->a2_data[dill_jmp_bswapu].data2, dest, src)
/*! DILL bswap u immediate operator */
#define dill_bswapui(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_bswapu](s, s->j->a2_data[dill_jmp_bswapu].data1, s->j->a2_data[dill_jmp_bswapu].data2, dest, imm)
/*! DILL bswap ul operator */
#define dill_bswapul(s, dest, src) (s->j->jmp_a2)[dill_jmp_bswapul](s, s->j->a2_data[dill_jmp_bswapul].data1, s->j->a2_data[dill_jmp_bswapul].data2, dest, src)
/*! DILL bswap ul immediate operator */
#define dill_bswapuli(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_bswapul](s, s->j->a2_data[dill_jmp_bswapul].data1, s->j->a2_data[dill_jmp_bswapul].data2, dest, imm)
/*! DILL bswap l operator */
#define dill_bswapl(s, dest, src) (s->j->jmp_a2)[dill_jmp_bswapl](s, s->j->a2_data[dill_jmp_bswapl].data1, s->j->a2_data[dill_jmp_bswapl].data2, dest, src)
/*! DILL bswap l immediate operator */
#define dill_bswapli(s, dest, imm) (s->j->jmp_a2i)[dill_jmp_bswapl](s, s->j->a2_data[dill_jmp_bswapl].data1, s->j->a2_data[dill_jmp_bswapl].data2, dest, imm)
/*! DILL bswap f operator */
#define dill_bswapf(s, dest, src) (s->j->jmp_a2)[dill_jmp_bswapf](s, s->j->a2_data[dill_jmp_bswapf].data1, s->j->a2_data[dill_jmp_bswapf].data2, dest, src)
/*! DILL bswap f immediate operator */
#define dill_bswapfi(s, dest, imm) (s->j->jmp_f2i)[dill_jmp_bswapf](s, s->j->a2f_data[dill_jmp_bswapf].data1, s->j->a2f_data[dill_jmp_bswapf].data2, dest, imm)
/*! DILL bswap d operator */
#define dill_bswapd(s, dest, src) (s->j->jmp_a2)[dill_jmp_bswapd](s, s->j->a2_data[dill_jmp_bswapd].data1, s->j->a2_data[dill_jmp_bswapd].data2, dest, src)
/*! DILL bswap d immediate operator */
#define dill_bswapdi(s, dest, imm) (s->j->jmp_f2i)[dill_jmp_bswapd](s, s->j->a2f_data[dill_jmp_bswapd].data1, s->j->a2f_data[dill_jmp_bswapd].data2, dest, imm)
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define dill_jmp_negf 20
#define dill_jmp_negd 21
#endif
/*! DILL neg f operator */
#define dill_negf(s, dest, src) (s->j->jmp_a2)[dill_jmp_negf](s, s->j->a2_data[dill_jmp_negf].data1, s->j->a2_data[dill_jmp_negf].data2, dest, src)
/*! DILL neg f immediate operator */
#define dill_negfi(s, dest, imm) (s->j->jmp_f2i)[dill_jmp_negf](s, s->j->a2f_data[dill_jmp_negf].data1, s->j->a2f_data[dill_jmp_negf].data2, dest, imm)
/*! DILL neg d operator */
#define dill_negd(s, dest, src) (s->j->jmp_a2)[dill_jmp_negd](s, s->j->a2_data[dill_jmp_negd].data1, s->j->a2_data[dill_jmp_negd].data2, dest, src)
/*! DILL neg d immediate operator */
#define dill_negdi(s, dest, imm) (s->j->jmp_f2i)[dill_jmp_negd](s, s->j->a2f_data[dill_jmp_negd].data1, s->j->a2f_data[dill_jmp_negd].data2, dest, imm)
#define dill_eq_code 0
#define dill_ge_code 1
#define dill_gt_code 2
#define dill_le_code 3
#define dill_lt_code 4
#define dill_ne_code 5
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define dill_jmp_beqc 0
#define dill_jmp_bequc 1
#define dill_jmp_beqs 2
#define dill_jmp_bequs 3
#define dill_jmp_beqi 4
#define dill_jmp_bequ 5
#define dill_jmp_beql 6
#define dill_jmp_bequl 7
#define dill_jmp_beqp 8
#define dill_jmp_beqf 9
#define dill_jmp_beqd 10
#define dill_jmp_bgec 11
#define dill_jmp_bgeuc 12
#define dill_jmp_bges 13
#define dill_jmp_bgeus 14
#define dill_jmp_bgei 15
#define dill_jmp_bgeu 16
#define dill_jmp_bgel 17
#define dill_jmp_bgeul 18
#define dill_jmp_bgep 19
#define dill_jmp_bgef 20
#define dill_jmp_bged 21
#define dill_jmp_bgtc 22
#define dill_jmp_bgtuc 23
#define dill_jmp_bgts 24
#define dill_jmp_bgtus 25
#define dill_jmp_bgti 26
#define dill_jmp_bgtu 27
#define dill_jmp_bgtl 28
#define dill_jmp_bgtul 29
#define dill_jmp_bgtp 30
#define dill_jmp_bgtf 31
#define dill_jmp_bgtd 32
#define dill_jmp_blec 33
#define dill_jmp_bleuc 34
#define dill_jmp_bles 35
#define dill_jmp_bleus 36
#define dill_jmp_blei 37
#define dill_jmp_bleu 38
#define dill_jmp_blel 39
#define dill_jmp_bleul 40
#define dill_jmp_blep 41
#define dill_jmp_blef 42
#define dill_jmp_bled 43
#define dill_jmp_bltc 44
#define dill_jmp_bltuc 45
#define dill_jmp_blts 46
#define dill_jmp_bltus 47
#define dill_jmp_blti 48
#define dill_jmp_bltu 49
#define dill_jmp_bltl 50
#define dill_jmp_bltul 51
#define dill_jmp_bltp 52
#define dill_jmp_bltf 53
#define dill_jmp_bltd 54
#define dill_jmp_bnec 55
#define dill_jmp_bneuc 56
#define dill_jmp_bnes 57
#define dill_jmp_bneus 58
#define dill_jmp_bnei 59
#define dill_jmp_bneu 60
#define dill_jmp_bnel 61
#define dill_jmp_bneul 62
#define dill_jmp_bnep 63
#define dill_jmp_bnef 64
#define dill_jmp_bned 65
#endif
#define dill_beqc(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_beqc](s, s->j->b_data[dill_jmp_beqc].data1, s->j->b_data[dill_jmp_beqc].data2, src1, src2, label)
#define dill_beqci(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_beqc](s, s->j->b_data[dill_jmp_beqc].data1, s->j->b_data[dill_jmp_beqc].data2, src, imm, label)
#define dill_bequc(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bequc](s, s->j->b_data[dill_jmp_bequc].data1, s->j->b_data[dill_jmp_bequc].data2, src1, src2, label)
#define dill_bequci(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bequc](s, s->j->b_data[dill_jmp_bequc].data1, s->j->b_data[dill_jmp_bequc].data2, src, imm, label)
#define dill_beqs(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_beqs](s, s->j->b_data[dill_jmp_beqs].data1, s->j->b_data[dill_jmp_beqs].data2, src1, src2, label)
#define dill_beqsi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_beqs](s, s->j->b_data[dill_jmp_beqs].data1, s->j->b_data[dill_jmp_beqs].data2, src, imm, label)
#define dill_bequs(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bequs](s, s->j->b_data[dill_jmp_bequs].data1, s->j->b_data[dill_jmp_bequs].data2, src1, src2, label)
#define dill_bequsi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bequs](s, s->j->b_data[dill_jmp_bequs].data1, s->j->b_data[dill_jmp_bequs].data2, src, imm, label)
#define dill_beqi(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_beqi](s, s->j->b_data[dill_jmp_beqi].data1, s->j->b_data[dill_jmp_beqi].data2, src1, src2, label)
#define dill_beqii(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_beqi](s, s->j->b_data[dill_jmp_beqi].data1, s->j->b_data[dill_jmp_beqi].data2, src, imm, label)
#define dill_bequ(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bequ](s, s->j->b_data[dill_jmp_bequ].data1, s->j->b_data[dill_jmp_bequ].data2, src1, src2, label)
#define dill_bequi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bequ](s, s->j->b_data[dill_jmp_bequ].data1, s->j->b_data[dill_jmp_bequ].data2, src, imm, label)
#define dill_beql(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_beql](s, s->j->b_data[dill_jmp_beql].data1, s->j->b_data[dill_jmp_beql].data2, src1, src2, label)
#define dill_beqli(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_beql](s, s->j->b_data[dill_jmp_beql].data1, s->j->b_data[dill_jmp_beql].data2, src, imm, label)
#define dill_bequl(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bequl](s, s->j->b_data[dill_jmp_bequl].data1, s->j->b_data[dill_jmp_bequl].data2, src1, src2, label)
#define dill_bequli(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bequl](s, s->j->b_data[dill_jmp_bequl].data1, s->j->b_data[dill_jmp_bequl].data2, src, imm, label)
#define dill_beqp(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_beqp](s, s->j->b_data[dill_jmp_beqp].data1, s->j->b_data[dill_jmp_beqp].data2, src1, src2, label)
#define dill_beqpi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_beqp](s, s->j->b_data[dill_jmp_beqp].data1, s->j->b_data[dill_jmp_beqp].data2, src, imm, label)
#define dill_beqf(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_beqf](s, s->j->b_data[dill_jmp_beqf].data1, s->j->b_data[dill_jmp_beqf].data2, src1, src2, label)
#define dill_beqd(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_beqd](s, s->j->b_data[dill_jmp_beqd].data1, s->j->b_data[dill_jmp_beqd].data2, src1, src2, label)
#define dill_bgec(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgec](s, s->j->b_data[dill_jmp_bgec].data1, s->j->b_data[dill_jmp_bgec].data2, src1, src2, label)
#define dill_bgeci(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgec](s, s->j->b_data[dill_jmp_bgec].data1, s->j->b_data[dill_jmp_bgec].data2, src, imm, label)
#define dill_bgeuc(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgeuc](s, s->j->b_data[dill_jmp_bgeuc].data1, s->j->b_data[dill_jmp_bgeuc].data2, src1, src2, label)
#define dill_bgeuci(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgeuc](s, s->j->b_data[dill_jmp_bgeuc].data1, s->j->b_data[dill_jmp_bgeuc].data2, src, imm, label)
#define dill_bges(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bges](s, s->j->b_data[dill_jmp_bges].data1, s->j->b_data[dill_jmp_bges].data2, src1, src2, label)
#define dill_bgesi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bges](s, s->j->b_data[dill_jmp_bges].data1, s->j->b_data[dill_jmp_bges].data2, src, imm, label)
#define dill_bgeus(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgeus](s, s->j->b_data[dill_jmp_bgeus].data1, s->j->b_data[dill_jmp_bgeus].data2, src1, src2, label)
#define dill_bgeusi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgeus](s, s->j->b_data[dill_jmp_bgeus].data1, s->j->b_data[dill_jmp_bgeus].data2, src, imm, label)
#define dill_bgei(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgei](s, s->j->b_data[dill_jmp_bgei].data1, s->j->b_data[dill_jmp_bgei].data2, src1, src2, label)
#define dill_bgeii(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgei](s, s->j->b_data[dill_jmp_bgei].data1, s->j->b_data[dill_jmp_bgei].data2, src, imm, label)
#define dill_bgeu(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgeu](s, s->j->b_data[dill_jmp_bgeu].data1, s->j->b_data[dill_jmp_bgeu].data2, src1, src2, label)
#define dill_bgeui(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgeu](s, s->j->b_data[dill_jmp_bgeu].data1, s->j->b_data[dill_jmp_bgeu].data2, src, imm, label)
#define dill_bgel(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgel](s, s->j->b_data[dill_jmp_bgel].data1, s->j->b_data[dill_jmp_bgel].data2, src1, src2, label)
#define dill_bgeli(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgel](s, s->j->b_data[dill_jmp_bgel].data1, s->j->b_data[dill_jmp_bgel].data2, src, imm, label)
#define dill_bgeul(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgeul](s, s->j->b_data[dill_jmp_bgeul].data1, s->j->b_data[dill_jmp_bgeul].data2, src1, src2, label)
#define dill_bgeuli(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgeul](s, s->j->b_data[dill_jmp_bgeul].data1, s->j->b_data[dill_jmp_bgeul].data2, src, imm, label)
#define dill_bgep(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgep](s, s->j->b_data[dill_jmp_bgep].data1, s->j->b_data[dill_jmp_bgep].data2, src1, src2, label)
#define dill_bgepi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgep](s, s->j->b_data[dill_jmp_bgep].data1, s->j->b_data[dill_jmp_bgep].data2, src, imm, label)
#define dill_bgef(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgef](s, s->j->b_data[dill_jmp_bgef].data1, s->j->b_data[dill_jmp_bgef].data2, src1, src2, label)
#define dill_bged(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bged](s, s->j->b_data[dill_jmp_bged].data1, s->j->b_data[dill_jmp_bged].data2, src1, src2, label)
#define dill_bgtc(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgtc](s, s->j->b_data[dill_jmp_bgtc].data1, s->j->b_data[dill_jmp_bgtc].data2, src1, src2, label)
#define dill_bgtci(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgtc](s, s->j->b_data[dill_jmp_bgtc].data1, s->j->b_data[dill_jmp_bgtc].data2, src, imm, label)
#define dill_bgtuc(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgtuc](s, s->j->b_data[dill_jmp_bgtuc].data1, s->j->b_data[dill_jmp_bgtuc].data2, src1, src2, label)
#define dill_bgtuci(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgtuc](s, s->j->b_data[dill_jmp_bgtuc].data1, s->j->b_data[dill_jmp_bgtuc].data2, src, imm, label)
#define dill_bgts(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgts](s, s->j->b_data[dill_jmp_bgts].data1, s->j->b_data[dill_jmp_bgts].data2, src1, src2, label)
#define dill_bgtsi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgts](s, s->j->b_data[dill_jmp_bgts].data1, s->j->b_data[dill_jmp_bgts].data2, src, imm, label)
#define dill_bgtus(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgtus](s, s->j->b_data[dill_jmp_bgtus].data1, s->j->b_data[dill_jmp_bgtus].data2, src1, src2, label)
#define dill_bgtusi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgtus](s, s->j->b_data[dill_jmp_bgtus].data1, s->j->b_data[dill_jmp_bgtus].data2, src, imm, label)
#define dill_bgti(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgti](s, s->j->b_data[dill_jmp_bgti].data1, s->j->b_data[dill_jmp_bgti].data2, src1, src2, label)
#define dill_bgtii(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgti](s, s->j->b_data[dill_jmp_bgti].data1, s->j->b_data[dill_jmp_bgti].data2, src, imm, label)
#define dill_bgtu(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgtu](s, s->j->b_data[dill_jmp_bgtu].data1, s->j->b_data[dill_jmp_bgtu].data2, src1, src2, label)
#define dill_bgtui(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgtu](s, s->j->b_data[dill_jmp_bgtu].data1, s->j->b_data[dill_jmp_bgtu].data2, src, imm, label)
#define dill_bgtl(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgtl](s, s->j->b_data[dill_jmp_bgtl].data1, s->j->b_data[dill_jmp_bgtl].data2, src1, src2, label)
#define dill_bgtli(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgtl](s, s->j->b_data[dill_jmp_bgtl].data1, s->j->b_data[dill_jmp_bgtl].data2, src, imm, label)
#define dill_bgtul(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgtul](s, s->j->b_data[dill_jmp_bgtul].data1, s->j->b_data[dill_jmp_bgtul].data2, src1, src2, label)
#define dill_bgtuli(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgtul](s, s->j->b_data[dill_jmp_bgtul].data1, s->j->b_data[dill_jmp_bgtul].data2, src, imm, label)
#define dill_bgtp(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgtp](s, s->j->b_data[dill_jmp_bgtp].data1, s->j->b_data[dill_jmp_bgtp].data2, src1, src2, label)
#define dill_bgtpi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bgtp](s, s->j->b_data[dill_jmp_bgtp].data1, s->j->b_data[dill_jmp_bgtp].data2, src, imm, label)
#define dill_bgtf(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgtf](s, s->j->b_data[dill_jmp_bgtf].data1, s->j->b_data[dill_jmp_bgtf].data2, src1, src2, label)
#define dill_bgtd(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgtd](s, s->j->b_data[dill_jmp_bgtd].data1, s->j->b_data[dill_jmp_bgtd].data2, src1, src2, label)
#define dill_blec(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_blec](s, s->j->b_data[dill_jmp_blec].data1, s->j->b_data[dill_jmp_blec].data2, src1, src2, label)
#define dill_bleci(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_blec](s, s->j->b_data[dill_jmp_blec].data1, s->j->b_data[dill_jmp_blec].data2, src, imm, label)
#define dill_bleuc(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bleuc](s, s->j->b_data[dill_jmp_bleuc].data1, s->j->b_data[dill_jmp_bleuc].data2, src1, src2, label)
#define dill_bleuci(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bleuc](s, s->j->b_data[dill_jmp_bleuc].data1, s->j->b_data[dill_jmp_bleuc].data2, src, imm, label)
#define dill_bles(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bles](s, s->j->b_data[dill_jmp_bles].data1, s->j->b_data[dill_jmp_bles].data2, src1, src2, label)
#define dill_blesi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bles](s, s->j->b_data[dill_jmp_bles].data1, s->j->b_data[dill_jmp_bles].data2, src, imm, label)
#define dill_bleus(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bleus](s, s->j->b_data[dill_jmp_bleus].data1, s->j->b_data[dill_jmp_bleus].data2, src1, src2, label)
#define dill_bleusi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bleus](s, s->j->b_data[dill_jmp_bleus].data1, s->j->b_data[dill_jmp_bleus].data2, src, imm, label)
#define dill_blei(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_blei](s, s->j->b_data[dill_jmp_blei].data1, s->j->b_data[dill_jmp_blei].data2, src1, src2, label)
#define dill_bleii(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_blei](s, s->j->b_data[dill_jmp_blei].data1, s->j->b_data[dill_jmp_blei].data2, src, imm, label)
#define dill_bleu(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bleu](s, s->j->b_data[dill_jmp_bleu].data1, s->j->b_data[dill_jmp_bleu].data2, src1, src2, label)
#define dill_bleui(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bleu](s, s->j->b_data[dill_jmp_bleu].data1, s->j->b_data[dill_jmp_bleu].data2, src, imm, label)
#define dill_blel(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_blel](s, s->j->b_data[dill_jmp_blel].data1, s->j->b_data[dill_jmp_blel].data2, src1, src2, label)
#define dill_bleli(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_blel](s, s->j->b_data[dill_jmp_blel].data1, s->j->b_data[dill_jmp_blel].data2, src, imm, label)
#define dill_bleul(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bleul](s, s->j->b_data[dill_jmp_bleul].data1, s->j->b_data[dill_jmp_bleul].data2, src1, src2, label)
#define dill_bleuli(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bleul](s, s->j->b_data[dill_jmp_bleul].data1, s->j->b_data[dill_jmp_bleul].data2, src, imm, label)
#define dill_blep(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_blep](s, s->j->b_data[dill_jmp_blep].data1, s->j->b_data[dill_jmp_blep].data2, src1, src2, label)
#define dill_blepi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_blep](s, s->j->b_data[dill_jmp_blep].data1, s->j->b_data[dill_jmp_blep].data2, src, imm, label)
#define dill_blef(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_blef](s, s->j->b_data[dill_jmp_blef].data1, s->j->b_data[dill_jmp_blef].data2, src1, src2, label)
#define dill_bled(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bled](s, s->j->b_data[dill_jmp_bled].data1, s->j->b_data[dill_jmp_bled].data2, src1, src2, label)
#define dill_bltc(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bltc](s, s->j->b_data[dill_jmp_bltc].data1, s->j->b_data[dill_jmp_bltc].data2, src1, src2, label)
#define dill_bltci(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bltc](s, s->j->b_data[dill_jmp_bltc].data1, s->j->b_data[dill_jmp_bltc].data2, src, imm, label)
#define dill_bltuc(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bltuc](s, s->j->b_data[dill_jmp_bltuc].data1, s->j->b_data[dill_jmp_bltuc].data2, src1, src2, label)
#define dill_bltuci(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bltuc](s, s->j->b_data[dill_jmp_bltuc].data1, s->j->b_data[dill_jmp_bltuc].data2, src, imm, label)
#define dill_blts(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_blts](s, s->j->b_data[dill_jmp_blts].data1, s->j->b_data[dill_jmp_blts].data2, src1, src2, label)
#define dill_bltsi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_blts](s, s->j->b_data[dill_jmp_blts].data1, s->j->b_data[dill_jmp_blts].data2, src, imm, label)
#define dill_bltus(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bltus](s, s->j->b_data[dill_jmp_bltus].data1, s->j->b_data[dill_jmp_bltus].data2, src1, src2, label)
#define dill_bltusi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bltus](s, s->j->b_data[dill_jmp_bltus].data1, s->j->b_data[dill_jmp_bltus].data2, src, imm, label)
#define dill_blti(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_blti](s, s->j->b_data[dill_jmp_blti].data1, s->j->b_data[dill_jmp_blti].data2, src1, src2, label)
#define dill_bltii(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_blti](s, s->j->b_data[dill_jmp_blti].data1, s->j->b_data[dill_jmp_blti].data2, src, imm, label)
#define dill_bltu(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bltu](s, s->j->b_data[dill_jmp_bltu].data1, s->j->b_data[dill_jmp_bltu].data2, src1, src2, label)
#define dill_bltui(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bltu](s, s->j->b_data[dill_jmp_bltu].data1, s->j->b_data[dill_jmp_bltu].data2, src, imm, label)
#define dill_bltl(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bltl](s, s->j->b_data[dill_jmp_bltl].data1, s->j->b_data[dill_jmp_bltl].data2, src1, src2, label)
#define dill_bltli(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bltl](s, s->j->b_data[dill_jmp_bltl].data1, s->j->b_data[dill_jmp_bltl].data2, src, imm, label)
#define dill_bltul(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bltul](s, s->j->b_data[dill_jmp_bltul].data1, s->j->b_data[dill_jmp_bltul].data2, src1, src2, label)
#define dill_bltuli(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bltul](s, s->j->b_data[dill_jmp_bltul].data1, s->j->b_data[dill_jmp_bltul].data2, src, imm, label)
#define dill_bltp(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bltp](s, s->j->b_data[dill_jmp_bltp].data1, s->j->b_data[dill_jmp_bltp].data2, src1, src2, label)
#define dill_bltpi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bltp](s, s->j->b_data[dill_jmp_bltp].data1, s->j->b_data[dill_jmp_bltp].data2, src, imm, label)
#define dill_bltf(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bltf](s, s->j->b_data[dill_jmp_bltf].data1, s->j->b_data[dill_jmp_bltf].data2, src1, src2, label)
#define dill_bltd(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bltd](s, s->j->b_data[dill_jmp_bltd].data1, s->j->b_data[dill_jmp_bltd].data2, src1, src2, label)
#define dill_bnec(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bnec](s, s->j->b_data[dill_jmp_bnec].data1, s->j->b_data[dill_jmp_bnec].data2, src1, src2, label)
#define dill_bneci(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bnec](s, s->j->b_data[dill_jmp_bnec].data1, s->j->b_data[dill_jmp_bnec].data2, src, imm, label)
#define dill_bneuc(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bneuc](s, s->j->b_data[dill_jmp_bneuc].data1, s->j->b_data[dill_jmp_bneuc].data2, src1, src2, label)
#define dill_bneuci(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bneuc](s, s->j->b_data[dill_jmp_bneuc].data1, s->j->b_data[dill_jmp_bneuc].data2, src, imm, label)
#define dill_bnes(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bnes](s, s->j->b_data[dill_jmp_bnes].data1, s->j->b_data[dill_jmp_bnes].data2, src1, src2, label)
#define dill_bnesi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bnes](s, s->j->b_data[dill_jmp_bnes].data1, s->j->b_data[dill_jmp_bnes].data2, src, imm, label)
#define dill_bneus(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bneus](s, s->j->b_data[dill_jmp_bneus].data1, s->j->b_data[dill_jmp_bneus].data2, src1, src2, label)
#define dill_bneusi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bneus](s, s->j->b_data[dill_jmp_bneus].data1, s->j->b_data[dill_jmp_bneus].data2, src, imm, label)
#define dill_bnei(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bnei](s, s->j->b_data[dill_jmp_bnei].data1, s->j->b_data[dill_jmp_bnei].data2, src1, src2, label)
#define dill_bneii(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bnei](s, s->j->b_data[dill_jmp_bnei].data1, s->j->b_data[dill_jmp_bnei].data2, src, imm, label)
#define dill_bneu(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bneu](s, s->j->b_data[dill_jmp_bneu].data1, s->j->b_data[dill_jmp_bneu].data2, src1, src2, label)
#define dill_bneui(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bneu](s, s->j->b_data[dill_jmp_bneu].data1, s->j->b_data[dill_jmp_bneu].data2, src, imm, label)
#define dill_bnel(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bnel](s, s->j->b_data[dill_jmp_bnel].data1, s->j->b_data[dill_jmp_bnel].data2, src1, src2, label)
#define dill_bneli(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bnel](s, s->j->b_data[dill_jmp_bnel].data1, s->j->b_data[dill_jmp_bnel].data2, src, imm, label)
#define dill_bneul(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bneul](s, s->j->b_data[dill_jmp_bneul].data1, s->j->b_data[dill_jmp_bneul].data2, src1, src2, label)
#define dill_bneuli(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bneul](s, s->j->b_data[dill_jmp_bneul].data1, s->j->b_data[dill_jmp_bneul].data2, src, imm, label)
#define dill_bnep(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bnep](s, s->j->b_data[dill_jmp_bnep].data1, s->j->b_data[dill_jmp_bnep].data2, src1, src2, label)
#define dill_bnepi(s, src, imm, label) (s->j->jmp_bi)[dill_jmp_bnep](s, s->j->b_data[dill_jmp_bnep].data1, s->j->b_data[dill_jmp_bnep].data2, src, imm, label)
#define dill_bnef(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bnef](s, s->j->b_data[dill_jmp_bnef].data1, s->j->b_data[dill_jmp_bnef].data2, src1, src2, label)
#define dill_bned(s, src1, src2, label) (s->j->jmp_b)[dill_jmp_bned](s, s->j->b_data[dill_jmp_bned].data1, s->j->b_data[dill_jmp_bned].data2, src1, src2, label)
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define dill_jmp_ceqc 0
#define dill_jmp_cequc 1
#define dill_jmp_ceqs 2
#define dill_jmp_cequs 3
#define dill_jmp_ceqi 4
#define dill_jmp_cequ 5
#define dill_jmp_ceql 6
#define dill_jmp_cequl 7
#define dill_jmp_ceqp 8
#define dill_jmp_ceqf 9
#define dill_jmp_ceqd 10
#define dill_jmp_cgec 11
#define dill_jmp_cgeuc 12
#define dill_jmp_cges 13
#define dill_jmp_cgeus 14
#define dill_jmp_cgei 15
#define dill_jmp_cgeu 16
#define dill_jmp_cgel 17
#define dill_jmp_cgeul 18
#define dill_jmp_cgep 19
#define dill_jmp_cgef 20
#define dill_jmp_cged 21
#define dill_jmp_cgtc 22
#define dill_jmp_cgtuc 23
#define dill_jmp_cgts 24
#define dill_jmp_cgtus 25
#define dill_jmp_cgti 26
#define dill_jmp_cgtu 27
#define dill_jmp_cgtl 28
#define dill_jmp_cgtul 29
#define dill_jmp_cgtp 30
#define dill_jmp_cgtf 31
#define dill_jmp_cgtd 32
#define dill_jmp_clec 33
#define dill_jmp_cleuc 34
#define dill_jmp_cles 35
#define dill_jmp_cleus 36
#define dill_jmp_clei 37
#define dill_jmp_cleu 38
#define dill_jmp_clel 39
#define dill_jmp_cleul 40
#define dill_jmp_clep 41
#define dill_jmp_clef 42
#define dill_jmp_cled 43
#define dill_jmp_cltc 44
#define dill_jmp_cltuc 45
#define dill_jmp_clts 46
#define dill_jmp_cltus 47
#define dill_jmp_clti 48
#define dill_jmp_cltu 49
#define dill_jmp_cltl 50
#define dill_jmp_cltul 51
#define dill_jmp_cltp 52
#define dill_jmp_cltf 53
#define dill_jmp_cltd 54
#define dill_jmp_cnec 55
#define dill_jmp_cneuc 56
#define dill_jmp_cnes 57
#define dill_jmp_cneus 58
#define dill_jmp_cnei 59
#define dill_jmp_cneu 60
#define dill_jmp_cnel 61
#define dill_jmp_cneul 62
#define dill_jmp_cnep 63
#define dill_jmp_cnef 64
#define dill_jmp_cned 65
#endif
#define dill_eqc(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_ceqc](s, s->j->c_data[dill_jmp_ceqc].data1, s->j->c_data[dill_jmp_ceqc].data2, dest, src1, src2)
#define dill_eqci(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_ceqc](s, s->j->c_data[dill_jmp_ceqc].data1, s->j->c_data[dill_jmp_ceqc].data2, dest, src, imm)
#define dill_equc(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cequc](s, s->j->c_data[dill_jmp_cequc].data1, s->j->c_data[dill_jmp_cequc].data2, dest, src1, src2)
#define dill_equci(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cequc](s, s->j->c_data[dill_jmp_cequc].data1, s->j->c_data[dill_jmp_cequc].data2, dest, src, imm)
#define dill_eqs(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_ceqs](s, s->j->c_data[dill_jmp_ceqs].data1, s->j->c_data[dill_jmp_ceqs].data2, dest, src1, src2)
#define dill_eqsi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_ceqs](s, s->j->c_data[dill_jmp_ceqs].data1, s->j->c_data[dill_jmp_ceqs].data2, dest, src, imm)
#define dill_equs(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cequs](s, s->j->c_data[dill_jmp_cequs].data1, s->j->c_data[dill_jmp_cequs].data2, dest, src1, src2)
#define dill_equsi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cequs](s, s->j->c_data[dill_jmp_cequs].data1, s->j->c_data[dill_jmp_cequs].data2, dest, src, imm)
#define dill_eqi(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_ceqi](s, s->j->c_data[dill_jmp_ceqi].data1, s->j->c_data[dill_jmp_ceqi].data2, dest, src1, src2)
#define dill_eqii(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_ceqi](s, s->j->c_data[dill_jmp_ceqi].data1, s->j->c_data[dill_jmp_ceqi].data2, dest, src, imm)
#define dill_equ(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cequ](s, s->j->c_data[dill_jmp_cequ].data1, s->j->c_data[dill_jmp_cequ].data2, dest, src1, src2)
#define dill_equi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cequ](s, s->j->c_data[dill_jmp_cequ].data1, s->j->c_data[dill_jmp_cequ].data2, dest, src, imm)
#define dill_eql(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_ceql](s, s->j->c_data[dill_jmp_ceql].data1, s->j->c_data[dill_jmp_ceql].data2, dest, src1, src2)
#define dill_eqli(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_ceql](s, s->j->c_data[dill_jmp_ceql].data1, s->j->c_data[dill_jmp_ceql].data2, dest, src, imm)
#define dill_equl(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cequl](s, s->j->c_data[dill_jmp_cequl].data1, s->j->c_data[dill_jmp_cequl].data2, dest, src1, src2)
#define dill_equli(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cequl](s, s->j->c_data[dill_jmp_cequl].data1, s->j->c_data[dill_jmp_cequl].data2, dest, src, imm)
#define dill_eqp(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_ceqp](s, s->j->c_data[dill_jmp_ceqp].data1, s->j->c_data[dill_jmp_ceqp].data2, dest, src1, src2)
#define dill_eqpi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_ceqp](s, s->j->c_data[dill_jmp_ceqp].data1, s->j->c_data[dill_jmp_ceqp].data2, dest, src, imm)
#define dill_eqf(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_ceqf](s, s->j->c_data[dill_jmp_ceqf].data1, s->j->c_data[dill_jmp_ceqf].data2, dest, src1, src2)
#define dill_eqd(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_ceqd](s, s->j->c_data[dill_jmp_ceqd].data1, s->j->c_data[dill_jmp_ceqd].data2, dest, src1, src2)
#define dill_gec(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgec](s, s->j->c_data[dill_jmp_cgec].data1, s->j->c_data[dill_jmp_cgec].data2, dest, src1, src2)
#define dill_geci(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgec](s, s->j->c_data[dill_jmp_cgec].data1, s->j->c_data[dill_jmp_cgec].data2, dest, src, imm)
#define dill_geuc(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgeuc](s, s->j->c_data[dill_jmp_cgeuc].data1, s->j->c_data[dill_jmp_cgeuc].data2, dest, src1, src2)
#define dill_geuci(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgeuc](s, s->j->c_data[dill_jmp_cgeuc].data1, s->j->c_data[dill_jmp_cgeuc].data2, dest, src, imm)
#define dill_ges(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cges](s, s->j->c_data[dill_jmp_cges].data1, s->j->c_data[dill_jmp_cges].data2, dest, src1, src2)
#define dill_gesi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cges](s, s->j->c_data[dill_jmp_cges].data1, s->j->c_data[dill_jmp_cges].data2, dest, src, imm)
#define dill_geus(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgeus](s, s->j->c_data[dill_jmp_cgeus].data1, s->j->c_data[dill_jmp_cgeus].data2, dest, src1, src2)
#define dill_geusi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgeus](s, s->j->c_data[dill_jmp_cgeus].data1, s->j->c_data[dill_jmp_cgeus].data2, dest, src, imm)
#define dill_gei(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgei](s, s->j->c_data[dill_jmp_cgei].data1, s->j->c_data[dill_jmp_cgei].data2, dest, src1, src2)
#define dill_geii(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgei](s, s->j->c_data[dill_jmp_cgei].data1, s->j->c_data[dill_jmp_cgei].data2, dest, src, imm)
#define dill_geu(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgeu](s, s->j->c_data[dill_jmp_cgeu].data1, s->j->c_data[dill_jmp_cgeu].data2, dest, src1, src2)
#define dill_geui(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgeu](s, s->j->c_data[dill_jmp_cgeu].data1, s->j->c_data[dill_jmp_cgeu].data2, dest, src, imm)
#define dill_gel(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgel](s, s->j->c_data[dill_jmp_cgel].data1, s->j->c_data[dill_jmp_cgel].data2, dest, src1, src2)
#define dill_geli(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgel](s, s->j->c_data[dill_jmp_cgel].data1, s->j->c_data[dill_jmp_cgel].data2, dest, src, imm)
#define dill_geul(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgeul](s, s->j->c_data[dill_jmp_cgeul].data1, s->j->c_data[dill_jmp_cgeul].data2, dest, src1, src2)
#define dill_geuli(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgeul](s, s->j->c_data[dill_jmp_cgeul].data1, s->j->c_data[dill_jmp_cgeul].data2, dest, src, imm)
#define dill_gep(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgep](s, s->j->c_data[dill_jmp_cgep].data1, s->j->c_data[dill_jmp_cgep].data2, dest, src1, src2)
#define dill_gepi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgep](s, s->j->c_data[dill_jmp_cgep].data1, s->j->c_data[dill_jmp_cgep].data2, dest, src, imm)
#define dill_gef(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgef](s, s->j->c_data[dill_jmp_cgef].data1, s->j->c_data[dill_jmp_cgef].data2, dest, src1, src2)
#define dill_ged(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cged](s, s->j->c_data[dill_jmp_cged].data1, s->j->c_data[dill_jmp_cged].data2, dest, src1, src2)
#define dill_gtc(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgtc](s, s->j->c_data[dill_jmp_cgtc].data1, s->j->c_data[dill_jmp_cgtc].data2, dest, src1, src2)
#define dill_gtci(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgtc](s, s->j->c_data[dill_jmp_cgtc].data1, s->j->c_data[dill_jmp_cgtc].data2, dest, src, imm)
#define dill_gtuc(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgtuc](s, s->j->c_data[dill_jmp_cgtuc].data1, s->j->c_data[dill_jmp_cgtuc].data2, dest, src1, src2)
#define dill_gtuci(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgtuc](s, s->j->c_data[dill_jmp_cgtuc].data1, s->j->c_data[dill_jmp_cgtuc].data2, dest, src, imm)
#define dill_gts(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgts](s, s->j->c_data[dill_jmp_cgts].data1, s->j->c_data[dill_jmp_cgts].data2, dest, src1, src2)
#define dill_gtsi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgts](s, s->j->c_data[dill_jmp_cgts].data1, s->j->c_data[dill_jmp_cgts].data2, dest, src, imm)
#define dill_gtus(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgtus](s, s->j->c_data[dill_jmp_cgtus].data1, s->j->c_data[dill_jmp_cgtus].data2, dest, src1, src2)
#define dill_gtusi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgtus](s, s->j->c_data[dill_jmp_cgtus].data1, s->j->c_data[dill_jmp_cgtus].data2, dest, src, imm)
#define dill_gti(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgti](s, s->j->c_data[dill_jmp_cgti].data1, s->j->c_data[dill_jmp_cgti].data2, dest, src1, src2)
#define dill_gtii(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgti](s, s->j->c_data[dill_jmp_cgti].data1, s->j->c_data[dill_jmp_cgti].data2, dest, src, imm)
#define dill_gtu(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgtu](s, s->j->c_data[dill_jmp_cgtu].data1, s->j->c_data[dill_jmp_cgtu].data2, dest, src1, src2)
#define dill_gtui(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgtu](s, s->j->c_data[dill_jmp_cgtu].data1, s->j->c_data[dill_jmp_cgtu].data2, dest, src, imm)
#define dill_gtl(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgtl](s, s->j->c_data[dill_jmp_cgtl].data1, s->j->c_data[dill_jmp_cgtl].data2, dest, src1, src2)
#define dill_gtli(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgtl](s, s->j->c_data[dill_jmp_cgtl].data1, s->j->c_data[dill_jmp_cgtl].data2, dest, src, imm)
#define dill_gtul(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgtul](s, s->j->c_data[dill_jmp_cgtul].data1, s->j->c_data[dill_jmp_cgtul].data2, dest, src1, src2)
#define dill_gtuli(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgtul](s, s->j->c_data[dill_jmp_cgtul].data1, s->j->c_data[dill_jmp_cgtul].data2, dest, src, imm)
#define dill_gtp(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgtp](s, s->j->c_data[dill_jmp_cgtp].data1, s->j->c_data[dill_jmp_cgtp].data2, dest, src1, src2)
#define dill_gtpi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cgtp](s, s->j->c_data[dill_jmp_cgtp].data1, s->j->c_data[dill_jmp_cgtp].data2, dest, src, imm)
#define dill_gtf(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgtf](s, s->j->c_data[dill_jmp_cgtf].data1, s->j->c_data[dill_jmp_cgtf].data2, dest, src1, src2)
#define dill_gtd(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cgtd](s, s->j->c_data[dill_jmp_cgtd].data1, s->j->c_data[dill_jmp_cgtd].data2, dest, src1, src2)
#define dill_lec(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_clec](s, s->j->c_data[dill_jmp_clec].data1, s->j->c_data[dill_jmp_clec].data2, dest, src1, src2)
#define dill_leci(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_clec](s, s->j->c_data[dill_jmp_clec].data1, s->j->c_data[dill_jmp_clec].data2, dest, src, imm)
#define dill_leuc(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cleuc](s, s->j->c_data[dill_jmp_cleuc].data1, s->j->c_data[dill_jmp_cleuc].data2, dest, src1, src2)
#define dill_leuci(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cleuc](s, s->j->c_data[dill_jmp_cleuc].data1, s->j->c_data[dill_jmp_cleuc].data2, dest, src, imm)
#define dill_les(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cles](s, s->j->c_data[dill_jmp_cles].data1, s->j->c_data[dill_jmp_cles].data2, dest, src1, src2)
#define dill_lesi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cles](s, s->j->c_data[dill_jmp_cles].data1, s->j->c_data[dill_jmp_cles].data2, dest, src, imm)
#define dill_leus(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cleus](s, s->j->c_data[dill_jmp_cleus].data1, s->j->c_data[dill_jmp_cleus].data2, dest, src1, src2)
#define dill_leusi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cleus](s, s->j->c_data[dill_jmp_cleus].data1, s->j->c_data[dill_jmp_cleus].data2, dest, src, imm)
#define dill_lei(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_clei](s, s->j->c_data[dill_jmp_clei].data1, s->j->c_data[dill_jmp_clei].data2, dest, src1, src2)
#define dill_leii(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_clei](s, s->j->c_data[dill_jmp_clei].data1, s->j->c_data[dill_jmp_clei].data2, dest, src, imm)
#define dill_leu(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cleu](s, s->j->c_data[dill_jmp_cleu].data1, s->j->c_data[dill_jmp_cleu].data2, dest, src1, src2)
#define dill_leui(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cleu](s, s->j->c_data[dill_jmp_cleu].data1, s->j->c_data[dill_jmp_cleu].data2, dest, src, imm)
#define dill_lel(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_clel](s, s->j->c_data[dill_jmp_clel].data1, s->j->c_data[dill_jmp_clel].data2, dest, src1, src2)
#define dill_leli(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_clel](s, s->j->c_data[dill_jmp_clel].data1, s->j->c_data[dill_jmp_clel].data2, dest, src, imm)
#define dill_leul(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cleul](s, s->j->c_data[dill_jmp_cleul].data1, s->j->c_data[dill_jmp_cleul].data2, dest, src1, src2)
#define dill_leuli(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cleul](s, s->j->c_data[dill_jmp_cleul].data1, s->j->c_data[dill_jmp_cleul].data2, dest, src, imm)
#define dill_lep(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_clep](s, s->j->c_data[dill_jmp_clep].data1, s->j->c_data[dill_jmp_clep].data2, dest, src1, src2)
#define dill_lepi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_clep](s, s->j->c_data[dill_jmp_clep].data1, s->j->c_data[dill_jmp_clep].data2, dest, src, imm)
#define dill_lef(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_clef](s, s->j->c_data[dill_jmp_clef].data1, s->j->c_data[dill_jmp_clef].data2, dest, src1, src2)
#define dill_led(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cled](s, s->j->c_data[dill_jmp_cled].data1, s->j->c_data[dill_jmp_cled].data2, dest, src1, src2)
#define dill_ltc(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cltc](s, s->j->c_data[dill_jmp_cltc].data1, s->j->c_data[dill_jmp_cltc].data2, dest, src1, src2)
#define dill_ltci(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cltc](s, s->j->c_data[dill_jmp_cltc].data1, s->j->c_data[dill_jmp_cltc].data2, dest, src, imm)
#define dill_ltuc(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cltuc](s, s->j->c_data[dill_jmp_cltuc].data1, s->j->c_data[dill_jmp_cltuc].data2, dest, src1, src2)
#define dill_ltuci(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cltuc](s, s->j->c_data[dill_jmp_cltuc].data1, s->j->c_data[dill_jmp_cltuc].data2, dest, src, imm)
#define dill_lts(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_clts](s, s->j->c_data[dill_jmp_clts].data1, s->j->c_data[dill_jmp_clts].data2, dest, src1, src2)
#define dill_ltsi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_clts](s, s->j->c_data[dill_jmp_clts].data1, s->j->c_data[dill_jmp_clts].data2, dest, src, imm)
#define dill_ltus(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cltus](s, s->j->c_data[dill_jmp_cltus].data1, s->j->c_data[dill_jmp_cltus].data2, dest, src1, src2)
#define dill_ltusi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cltus](s, s->j->c_data[dill_jmp_cltus].data1, s->j->c_data[dill_jmp_cltus].data2, dest, src, imm)
#define dill_lti(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_clti](s, s->j->c_data[dill_jmp_clti].data1, s->j->c_data[dill_jmp_clti].data2, dest, src1, src2)
#define dill_ltii(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_clti](s, s->j->c_data[dill_jmp_clti].data1, s->j->c_data[dill_jmp_clti].data2, dest, src, imm)
#define dill_ltu(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cltu](s, s->j->c_data[dill_jmp_cltu].data1, s->j->c_data[dill_jmp_cltu].data2, dest, src1, src2)
#define dill_ltui(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cltu](s, s->j->c_data[dill_jmp_cltu].data1, s->j->c_data[dill_jmp_cltu].data2, dest, src, imm)
#define dill_ltl(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cltl](s, s->j->c_data[dill_jmp_cltl].data1, s->j->c_data[dill_jmp_cltl].data2, dest, src1, src2)
#define dill_ltli(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cltl](s, s->j->c_data[dill_jmp_cltl].data1, s->j->c_data[dill_jmp_cltl].data2, dest, src, imm)
#define dill_ltul(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cltul](s, s->j->c_data[dill_jmp_cltul].data1, s->j->c_data[dill_jmp_cltul].data2, dest, src1, src2)
#define dill_ltuli(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cltul](s, s->j->c_data[dill_jmp_cltul].data1, s->j->c_data[dill_jmp_cltul].data2, dest, src, imm)
#define dill_ltp(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cltp](s, s->j->c_data[dill_jmp_cltp].data1, s->j->c_data[dill_jmp_cltp].data2, dest, src1, src2)
#define dill_ltpi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cltp](s, s->j->c_data[dill_jmp_cltp].data1, s->j->c_data[dill_jmp_cltp].data2, dest, src, imm)
#define dill_ltf(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cltf](s, s->j->c_data[dill_jmp_cltf].data1, s->j->c_data[dill_jmp_cltf].data2, dest, src1, src2)
#define dill_ltd(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cltd](s, s->j->c_data[dill_jmp_cltd].data1, s->j->c_data[dill_jmp_cltd].data2, dest, src1, src2)
#define dill_nec(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cnec](s, s->j->c_data[dill_jmp_cnec].data1, s->j->c_data[dill_jmp_cnec].data2, dest, src1, src2)
#define dill_neci(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cnec](s, s->j->c_data[dill_jmp_cnec].data1, s->j->c_data[dill_jmp_cnec].data2, dest, src, imm)
#define dill_neuc(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cneuc](s, s->j->c_data[dill_jmp_cneuc].data1, s->j->c_data[dill_jmp_cneuc].data2, dest, src1, src2)
#define dill_neuci(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cneuc](s, s->j->c_data[dill_jmp_cneuc].data1, s->j->c_data[dill_jmp_cneuc].data2, dest, src, imm)
#define dill_nes(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cnes](s, s->j->c_data[dill_jmp_cnes].data1, s->j->c_data[dill_jmp_cnes].data2, dest, src1, src2)
#define dill_nesi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cnes](s, s->j->c_data[dill_jmp_cnes].data1, s->j->c_data[dill_jmp_cnes].data2, dest, src, imm)
#define dill_neus(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cneus](s, s->j->c_data[dill_jmp_cneus].data1, s->j->c_data[dill_jmp_cneus].data2, dest, src1, src2)
#define dill_neusi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cneus](s, s->j->c_data[dill_jmp_cneus].data1, s->j->c_data[dill_jmp_cneus].data2, dest, src, imm)
#define dill_nei(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cnei](s, s->j->c_data[dill_jmp_cnei].data1, s->j->c_data[dill_jmp_cnei].data2, dest, src1, src2)
#define dill_neii(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cnei](s, s->j->c_data[dill_jmp_cnei].data1, s->j->c_data[dill_jmp_cnei].data2, dest, src, imm)
#define dill_neu(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cneu](s, s->j->c_data[dill_jmp_cneu].data1, s->j->c_data[dill_jmp_cneu].data2, dest, src1, src2)
#define dill_neui(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cneu](s, s->j->c_data[dill_jmp_cneu].data1, s->j->c_data[dill_jmp_cneu].data2, dest, src, imm)
#define dill_nel(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cnel](s, s->j->c_data[dill_jmp_cnel].data1, s->j->c_data[dill_jmp_cnel].data2, dest, src1, src2)
#define dill_neli(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cnel](s, s->j->c_data[dill_jmp_cnel].data1, s->j->c_data[dill_jmp_cnel].data2, dest, src, imm)
#define dill_neul(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cneul](s, s->j->c_data[dill_jmp_cneul].data1, s->j->c_data[dill_jmp_cneul].data2, dest, src1, src2)
#define dill_neuli(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cneul](s, s->j->c_data[dill_jmp_cneul].data1, s->j->c_data[dill_jmp_cneul].data2, dest, src, imm)
#define dill_nep(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cnep](s, s->j->c_data[dill_jmp_cnep].data1, s->j->c_data[dill_jmp_cnep].data2, dest, src1, src2)
#define dill_nepi(s, dest, src, imm) (s->j->jmp_ci)[dill_jmp_cnep](s, s->j->c_data[dill_jmp_cnep].data1, s->j->c_data[dill_jmp_cnep].data2, dest, src, imm)
#define dill_nef(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cnef](s, s->j->c_data[dill_jmp_cnef].data1, s->j->c_data[dill_jmp_cnef].data2, dest, src1, src2)
#define dill_ned(s, dest, src1, src2) (s->j->jmp_c)[dill_jmp_cned](s, s->j->c_data[dill_jmp_cned].data1, s->j->c_data[dill_jmp_cned].data2, dest, src1, src2)
#define dill_cvc2uc(s, dest, src) (s->j->convert)(s, DILL_C, DILL_UC, dest, src)
#define dill_cvc2s(s, dest, src) (s->j->convert)(s, DILL_C, DILL_S, dest, src)
#define dill_cvc2us(s, dest, src) (s->j->convert)(s, DILL_C, DILL_US, dest, src)
#define dill_cvc2i(s, dest, src) (s->j->convert)(s, DILL_C, DILL_I, dest, src)
#define dill_cvc2u(s, dest, src) (s->j->convert)(s, DILL_C, DILL_U, dest, src)
#define dill_cvc2ul(s, dest, src) (s->j->convert)(s, DILL_C, DILL_UL, dest, src)
#define dill_cvc2l(s, dest, src) (s->j->convert)(s, DILL_C, DILL_L, dest, src)
#define dill_cvd2c(s, dest, src) (s->j->convert)(s, DILL_D, DILL_C, dest, src)
#define dill_cvd2uc(s, dest, src) (s->j->convert)(s, DILL_D, DILL_UC, dest, src)
#define dill_cvd2s(s, dest, src) (s->j->convert)(s, DILL_D, DILL_S, dest, src)
#define dill_cvd2us(s, dest, src) (s->j->convert)(s, DILL_D, DILL_US, dest, src)
#define dill_cvd2i(s, dest, src) (s->j->convert)(s, DILL_D, DILL_I, dest, src)
#define dill_cvd2u(s, dest, src) (s->j->convert)(s, DILL_D, DILL_U, dest, src)
#define dill_cvd2ul(s, dest, src) (s->j->convert)(s, DILL_D, DILL_UL, dest, src)
#define dill_cvd2l(s, dest, src) (s->j->convert)(s, DILL_D, DILL_L, dest, src)
#define dill_cvf2c(s, dest, src) (s->j->convert)(s, DILL_F, DILL_C, dest, src)
#define dill_cvf2uc(s, dest, src) (s->j->convert)(s, DILL_F, DILL_UC, dest, src)
#define dill_cvf2s(s, dest, src) (s->j->convert)(s, DILL_F, DILL_S, dest, src)
#define dill_cvf2us(s, dest, src) (s->j->convert)(s, DILL_F, DILL_US, dest, src)
#define dill_cvf2i(s, dest, src) (s->j->convert)(s, DILL_F, DILL_I, dest, src)
#define dill_cvf2u(s, dest, src) (s->j->convert)(s, DILL_F, DILL_U, dest, src)
#define dill_cvf2ul(s, dest, src) (s->j->convert)(s, DILL_F, DILL_UL, dest, src)
#define dill_cvf2l(s, dest, src) (s->j->convert)(s, DILL_F, DILL_L, dest, src)
#define dill_cvi2c(s, dest, src) (s->j->convert)(s, DILL_I, DILL_C, dest, src)
#define dill_cvi2uc(s, dest, src) (s->j->convert)(s, DILL_I, DILL_UC, dest, src)
#define dill_cvi2s(s, dest, src) (s->j->convert)(s, DILL_I, DILL_S, dest, src)
#define dill_cvi2us(s, dest, src) (s->j->convert)(s, DILL_I, DILL_US, dest, src)
#define dill_cvi2u(s, dest, src) (s->j->convert)(s, DILL_I, DILL_U, dest, src)
#define dill_cvi2ul(s, dest, src) (s->j->convert)(s, DILL_I, DILL_UL, dest, src)
#define dill_cvi2l(s, dest, src) (s->j->convert)(s, DILL_I, DILL_L, dest, src)
#define dill_cvl2c(s, dest, src) (s->j->convert)(s, DILL_L, DILL_C, dest, src)
#define dill_cvl2uc(s, dest, src) (s->j->convert)(s, DILL_L, DILL_UC, dest, src)
#define dill_cvl2s(s, dest, src) (s->j->convert)(s, DILL_L, DILL_S, dest, src)
#define dill_cvl2us(s, dest, src) (s->j->convert)(s, DILL_L, DILL_US, dest, src)
#define dill_cvl2i(s, dest, src) (s->j->convert)(s, DILL_L, DILL_I, dest, src)
#define dill_cvl2u(s, dest, src) (s->j->convert)(s, DILL_L, DILL_U, dest, src)
#define dill_cvl2ul(s, dest, src) (s->j->convert)(s, DILL_L, DILL_UL, dest, src)
#define dill_cvs2c(s, dest, src) (s->j->convert)(s, DILL_S, DILL_C, dest, src)
#define dill_cvs2uc(s, dest, src) (s->j->convert)(s, DILL_S, DILL_UC, dest, src)
#define dill_cvs2us(s, dest, src) (s->j->convert)(s, DILL_S, DILL_US, dest, src)
#define dill_cvs2i(s, dest, src) (s->j->convert)(s, DILL_S, DILL_I, dest, src)
#define dill_cvs2u(s, dest, src) (s->j->convert)(s, DILL_S, DILL_U, dest, src)
#define dill_cvs2ul(s, dest, src) (s->j->convert)(s, DILL_S, DILL_UL, dest, src)
#define dill_cvs2l(s, dest, src) (s->j->convert)(s, DILL_S, DILL_L, dest, src)
#define dill_cvu2c(s, dest, src) (s->j->convert)(s, DILL_U, DILL_C, dest, src)
#define dill_cvu2uc(s, dest, src) (s->j->convert)(s, DILL_U, DILL_UC, dest, src)
#define dill_cvu2s(s, dest, src) (s->j->convert)(s, DILL_U, DILL_S, dest, src)
#define dill_cvu2us(s, dest, src) (s->j->convert)(s, DILL_U, DILL_US, dest, src)
#define dill_cvu2i(s, dest, src) (s->j->convert)(s, DILL_U, DILL_I, dest, src)
#define dill_cvu2ul(s, dest, src) (s->j->convert)(s, DILL_U, DILL_UL, dest, src)
#define dill_cvu2l(s, dest, src) (s->j->convert)(s, DILL_U, DILL_L, dest, src)
#define dill_cvul2c(s, dest, src) (s->j->convert)(s, DILL_UL, DILL_C, dest, src)
#define dill_cvul2uc(s, dest, src) (s->j->convert)(s, DILL_UL, DILL_UC, dest, src)
#define dill_cvul2s(s, dest, src) (s->j->convert)(s, DILL_UL, DILL_S, dest, src)
#define dill_cvul2us(s, dest, src) (s->j->convert)(s, DILL_UL, DILL_US, dest, src)
#define dill_cvul2i(s, dest, src) (s->j->convert)(s, DILL_UL, DILL_I, dest, src)
#define dill_cvul2u(s, dest, src) (s->j->convert)(s, DILL_UL, DILL_U, dest, src)
#define dill_cvul2l(s, dest, src) (s->j->convert)(s, DILL_UL, DILL_L, dest, src)
#define dill_cvus2c(s, dest, src) (s->j->convert)(s, DILL_US, DILL_C, dest, src)
#define dill_cvus2uc(s, dest, src) (s->j->convert)(s, DILL_US, DILL_UC, dest, src)
#define dill_cvus2s(s, dest, src) (s->j->convert)(s, DILL_US, DILL_S, dest, src)
#define dill_cvus2i(s, dest, src) (s->j->convert)(s, DILL_US, DILL_I, dest, src)
#define dill_cvus2u(s, dest, src) (s->j->convert)(s, DILL_US, DILL_U, dest, src)
#define dill_cvus2ul(s, dest, src) (s->j->convert)(s, DILL_US, DILL_UL, dest, src)
#define dill_cvus2l(s, dest, src) (s->j->convert)(s, DILL_US, DILL_L, dest, src)
#define dill_cvuc2c(s, dest, src) (s->j->convert)(s, DILL_UC, DILL_C, dest, src)
#define dill_cvuc2s(s, dest, src) (s->j->convert)(s, DILL_UC, DILL_S, dest, src)
#define dill_cvuc2us(s, dest, src) (s->j->convert)(s, DILL_UC, DILL_US, dest, src)
#define dill_cvuc2i(s, dest, src) (s->j->convert)(s, DILL_UC, DILL_I, dest, src)
#define dill_cvuc2u(s, dest, src) (s->j->convert)(s, DILL_UC, DILL_U, dest, src)
#define dill_cvuc2ul(s, dest, src) (s->j->convert)(s, DILL_UC, DILL_UL, dest, src)
#define dill_cvuc2l(s, dest, src) (s->j->convert)(s, DILL_UC, DILL_L, dest, src)
#define dill_pconvert(s, from_type, to_type, dest, src) (s->j->convert)(s, from_type, to_type, dest, src)
#define dill_cvc2f(s, dest, src) (s->j->convert)(s, DILL_C, DILL_F, dest, src)
#define dill_cvc2d(s, dest, src) (s->j->convert)(s, DILL_C, DILL_D, dest, src)
#define dill_cvuc2f(s, dest, src) (s->j->convert)(s, DILL_UC, DILL_F, dest, src)
#define dill_cvuc2d(s, dest, src) (s->j->convert)(s, DILL_UC, DILL_D, dest, src)
#define dill_cvs2f(s, dest, src) (s->j->convert)(s, DILL_S, DILL_F, dest, src)
#define dill_cvs2d(s, dest, src) (s->j->convert)(s, DILL_S, DILL_D, dest, src)
#define dill_cvus2f(s, dest, src) (s->j->convert)(s, DILL_US, DILL_F, dest, src)
#define dill_cvus2d(s, dest, src) (s->j->convert)(s, DILL_US, DILL_D, dest, src)
#define dill_cvd2f(s, dest, src) (s->j->convert)(s, DILL_D, DILL_F, dest, src)
#define dill_cvf2d(s, dest, src) (s->j->convert)(s, DILL_F, DILL_D, dest, src)
#define dill_cvi2f(s, dest, src) (s->j->convert)(s, DILL_I, DILL_F, dest, src)
#define dill_cvi2d(s, dest, src) (s->j->convert)(s, DILL_I, DILL_D, dest, src)
#define dill_cvl2f(s, dest, src) (s->j->convert)(s, DILL_L, DILL_F, dest, src)
#define dill_cvl2d(s, dest, src) (s->j->convert)(s, DILL_L, DILL_D, dest, src)
#define dill_cvu2f(s, dest, src) (s->j->convert)(s, DILL_U, DILL_F, dest, src)
#define dill_cvu2d(s, dest, src) (s->j->convert)(s, DILL_U, DILL_D, dest, src)
#define dill_cvul2f(s, dest, src) (s->j->convert)(s, DILL_UL, DILL_F, dest, src)
#define dill_cvul2d(s, dest, src) (s->j->convert)(s, DILL_UL, DILL_D, dest, src)
#define dill_pconvert(s, from_type, to_type, dest, src) (s->j->convert)(s, from_type, to_type, dest, src)
#define dill_cvp2ul(s, dest, src) (s->j->convert)(s, DILL_P, DILL_UL, dest, src)
#define dill_cvul2p(s, dest, src) (s->j->convert)(s, DILL_UL, DILL_P, dest, src)
#define dill_pconvert(s, from_type, to_type, dest, src) (s->j->convert)(s, from_type, to_type, dest, src)
#define dill_Pload(s, typ, dest, src1, src2) (s->j->load)(s, typ, 0, dest, src1, src2)
#define dill_Ploadi(s, typ, dest, src, imm) (s->j->loadi)(s, typ, 0, dest, src, imm)
#define dill_Pstore(s, typ, dest, src1, src2) (s->j->store)(s, typ, 0, dest, src1, src2)
#define dill_Pstorei(s, typ, dest, src, imm) (s->j->storei)(s, typ, 0, dest, src, imm)
#define dill_Pbne(s, type, src1, src2, label) (s->j->jmp_b)[dill_jmp_bnec+type](s, s->j->b_data[dill_jmp_bnec+type].data1, s->j->b_data[dill_jmp_bnec+type].data2, src1, src2, label)
#define dill_Pbgt(s, type, src1, src2, label) (s->j->jmp_b)[dill_jmp_bgtc+type](s, s->j->b_data[dill_jmp_bgtc+type].data1, s->j->b_data[dill_jmp_bgtc+type].data2, src1, src2, label)
#define dill_Pbeq(s, type, src1, src2, label) (s->j->jmp_b)[dill_jmp_beqc+type](s, s->j->b_data[dill_jmp_beqc+type].data1, s->j->b_data[dill_jmp_beqc+type].data2, src1, src2, label)
#define dill_ldc(s, dest, src1, src2) (s->j->load)(s, DILL_C, 0, dest, src1, src2)
#define dill_ldci(s, dest, src, imm) (s->j->loadi)(s, DILL_C, 0, dest, src, imm)
#define dill_stc(s, dest, src1, src2) (s->j->store)(s, DILL_C, 0, dest, src1, src2)
#define dill_stci(s, dest, src, imm) (s->j->storei)(s, DILL_C, 0, dest, src, imm)
#define dill_has_ldbs(s) (s->j->bsload != 0)
#define dill_ldbsc(s, dest, src1, src2) (s->j->bsload)(s, DILL_C, 0, dest, src1, src2)
#define dill_ldbsci(s, dest, src, imm) (s->j->bsloadi)(s, DILL_C, 0, dest, src, imm)
#define dill_lea(s, dest, src, imm) (s->j->lea)(s, 0, 0, dest, src, imm)
#define dill_ldd(s, dest, src1, src2) (s->j->load)(s, DILL_D, 0, dest, src1, src2)
#define dill_lddi(s, dest, src, imm) (s->j->loadi)(s, DILL_D, 0, dest, src, imm)
#define dill_std(s, dest, src1, src2) (s->j->store)(s, DILL_D, 0, dest, src1, src2)
#define dill_stdi(s, dest, src, imm) (s->j->storei)(s, DILL_D, 0, dest, src, imm)
#define dill_has_ldbs(s) (s->j->bsload != 0)
#define dill_ldbsd(s, dest, src1, src2) (s->j->bsload)(s, DILL_D, 0, dest, src1, src2)
#define dill_ldbsdi(s, dest, src, imm) (s->j->bsloadi)(s, DILL_D, 0, dest, src, imm)
#define dill_lea(s, dest, src, imm) (s->j->lea)(s, 0, 0, dest, src, imm)
#define dill_ldf(s, dest, src1, src2) (s->j->load)(s, DILL_F, 0, dest, src1, src2)
#define dill_ldfi(s, dest, src, imm) (s->j->loadi)(s, DILL_F, 0, dest, src, imm)
#define dill_stf(s, dest, src1, src2) (s->j->store)(s, DILL_F, 0, dest, src1, src2)
#define dill_stfi(s, dest, src, imm) (s->j->storei)(s, DILL_F, 0, dest, src, imm)
#define dill_has_ldbs(s) (s->j->bsload != 0)
#define dill_ldbsf(s, dest, src1, src2) (s->j->bsload)(s, DILL_F, 0, dest, src1, src2)
#define dill_ldbsfi(s, dest, src, imm) (s->j->bsloadi)(s, DILL_F, 0, dest, src, imm)
#define dill_lea(s, dest, src, imm) (s->j->lea)(s, 0, 0, dest, src, imm)
#define dill_ldi(s, dest, src1, src2) (s->j->load)(s, DILL_I, 0, dest, src1, src2)
#define dill_ldii(s, dest, src, imm) (s->j->loadi)(s, DILL_I, 0, dest, src, imm)
#define dill_sti(s, dest, src1, src2) (s->j->store)(s, DILL_I, 0, dest, src1, src2)
#define dill_stii(s, dest, src, imm) (s->j->storei)(s, DILL_I, 0, dest, src, imm)
#define dill_has_ldbs(s) (s->j->bsload != 0)
#define dill_ldbsi(s, dest, src1, src2) (s->j->bsload)(s, DILL_I, 0, dest, src1, src2)
#define dill_ldbsii(s, dest, src, imm) (s->j->bsloadi)(s, DILL_I, 0, dest, src, imm)
#define dill_lea(s, dest, src, imm) (s->j->lea)(s, 0, 0, dest, src, imm)
#define dill_ldl(s, dest, src1, src2) (s->j->load)(s, DILL_L, 0, dest, src1, src2)
#define dill_ldli(s, dest, src, imm) (s->j->loadi)(s, DILL_L, 0, dest, src, imm)
#define dill_stl(s, dest, src1, src2) (s->j->store)(s, DILL_L, 0, dest, src1, src2)
#define dill_stli(s, dest, src, imm) (s->j->storei)(s, DILL_L, 0, dest, src, imm)
#define dill_has_ldbs(s) (s->j->bsload != 0)
#define dill_ldbsl(s, dest, src1, src2) (s->j->bsload)(s, DILL_L, 0, dest, src1, src2)
#define dill_ldbsli(s, dest, src, imm) (s->j->bsloadi)(s, DILL_L, 0, dest, src, imm)
#define dill_lea(s, dest, src, imm) (s->j->lea)(s, 0, 0, dest, src, imm)
#define dill_ldp(s, dest, src1, src2) (s->j->load)(s, DILL_P, 0, dest, src1, src2)
#define dill_ldpi(s, dest, src, imm) (s->j->loadi)(s, DILL_P, 0, dest, src, imm)
#define dill_stp(s, dest, src1, src2) (s->j->store)(s, DILL_P, 0, dest, src1, src2)
#define dill_stpi(s, dest, src, imm) (s->j->storei)(s, DILL_P, 0, dest, src, imm)
#define dill_has_ldbs(s) (s->j->bsload != 0)
#define dill_ldbsp(s, dest, src1, src2) (s->j->bsload)(s, DILL_P, 0, dest, src1, src2)
#define dill_ldbspi(s, dest, src, imm) (s->j->bsloadi)(s, DILL_P, 0, dest, src, imm)
#define dill_lea(s, dest, src, imm) (s->j->lea)(s, 0, 0, dest, src, imm)
#define dill_lds(s, dest, src1, src2) (s->j->load)(s, DILL_S, 0, dest, src1, src2)
#define dill_ldsi(s, dest, src, imm) (s->j->loadi)(s, DILL_S, 0, dest, src, imm)
#define dill_sts(s, dest, src1, src2) (s->j->store)(s, DILL_S, 0, dest, src1, src2)
#define dill_stsi(s, dest, src, imm) (s->j->storei)(s, DILL_S, 0, dest, src, imm)
#define dill_has_ldbs(s) (s->j->bsload != 0)
#define dill_ldbss(s, dest, src1, src2) (s->j->bsload)(s, DILL_S, 0, dest, src1, src2)
#define dill_ldbssi(s, dest, src, imm) (s->j->bsloadi)(s, DILL_S, 0, dest, src, imm)
#define dill_lea(s, dest, src, imm) (s->j->lea)(s, 0, 0, dest, src, imm)
#define dill_ldu(s, dest, src1, src2) (s->j->load)(s, DILL_U, 0, dest, src1, src2)
#define dill_ldui(s, dest, src, imm) (s->j->loadi)(s, DILL_U, 0, dest, src, imm)
#define dill_stu(s, dest, src1, src2) (s->j->store)(s, DILL_U, 0, dest, src1, src2)
#define dill_stui(s, dest, src, imm) (s->j->storei)(s, DILL_U, 0, dest, src, imm)
#define dill_has_ldbs(s) (s->j->bsload != 0)
#define dill_ldbsu(s, dest, src1, src2) (s->j->bsload)(s, DILL_U, 0, dest, src1, src2)
#define dill_ldbsui(s, dest, src, imm) (s->j->bsloadi)(s, DILL_U, 0, dest, src, imm)
#define dill_lea(s, dest, src, imm) (s->j->lea)(s, 0, 0, dest, src, imm)
#define dill_lduc(s, dest, src1, src2) (s->j->load)(s, DILL_UC, 0, dest, src1, src2)
#define dill_lduci(s, dest, src, imm) (s->j->loadi)(s, DILL_UC, 0, dest, src, imm)
#define dill_stuc(s, dest, src1, src2) (s->j->store)(s, DILL_UC, 0, dest, src1, src2)
#define dill_stuci(s, dest, src, imm) (s->j->storei)(s, DILL_UC, 0, dest, src, imm)
#define dill_has_ldbs(s) (s->j->bsload != 0)
#define dill_ldbsuc(s, dest, src1, src2) (s->j->bsload)(s, DILL_UC, 0, dest, src1, src2)
#define dill_ldbsuci(s, dest, src, imm) (s->j->bsloadi)(s, DILL_UC, 0, dest, src, imm)
#define dill_lea(s, dest, src, imm) (s->j->lea)(s, 0, 0, dest, src, imm)
#define dill_ldu(s, dest, src1, src2) (s->j->load)(s, DILL_U, 0, dest, src1, src2)
#define dill_ldui(s, dest, src, imm) (s->j->loadi)(s, DILL_U, 0, dest, src, imm)
#define dill_stu(s, dest, src1, src2) (s->j->store)(s, DILL_U, 0, dest, src1, src2)
#define dill_stui(s, dest, src, imm) (s->j->storei)(s, DILL_U, 0, dest, src, imm)
#define dill_has_ldbs(s) (s->j->bsload != 0)
#define dill_ldbsu(s, dest, src1, src2) (s->j->bsload)(s, DILL_U, 0, dest, src1, src2)
#define dill_ldbsui(s, dest, src, imm) (s->j->bsloadi)(s, DILL_U, 0, dest, src, imm)
#define dill_lea(s, dest, src, imm) (s->j->lea)(s, 0, 0, dest, src, imm)
#define dill_ldul(s, dest, src1, src2) (s->j->load)(s, DILL_UL, 0, dest, src1, src2)
#define dill_lduli(s, dest, src, imm) (s->j->loadi)(s, DILL_UL, 0, dest, src, imm)
#define dill_stul(s, dest, src1, src2) (s->j->store)(s, DILL_UL, 0, dest, src1, src2)
#define dill_stuli(s, dest, src, imm) (s->j->storei)(s, DILL_UL, 0, dest, src, imm)
#define dill_has_ldbs(s) (s->j->bsload != 0)
#define dill_ldbsul(s, dest, src1, src2) (s->j->bsload)(s, DILL_UL, 0, dest, src1, src2)
#define dill_ldbsuli(s, dest, src, imm) (s->j->bsloadi)(s, DILL_UL, 0, dest, src, imm)
#define dill_lea(s, dest, src, imm) (s->j->lea)(s, 0, 0, dest, src, imm)
#define dill_ldus(s, dest, src1, src2) (s->j->load)(s, DILL_US, 0, dest, src1, src2)
#define dill_ldusi(s, dest, src, imm) (s->j->loadi)(s, DILL_US, 0, dest, src, imm)
#define dill_stus(s, dest, src1, src2) (s->j->store)(s, DILL_US, 0, dest, src1, src2)
#define dill_stusi(s, dest, src, imm) (s->j->storei)(s, DILL_US, 0, dest, src, imm)
#define dill_has_ldbs(s) (s->j->bsload != 0)
#define dill_ldbsus(s, dest, src1, src2) (s->j->bsload)(s, DILL_US, 0, dest, src1, src2)
#define dill_ldbsusi(s, dest, src, imm) (s->j->bsloadi)(s, DILL_US, 0, dest, src, imm)
#define dill_lea(s, dest, src, imm) (s->j->lea)(s, 0, 0, dest, src, imm)
#define dill_retc(s, src) (s->j->ret)(s, DILL_I, 0, src)
#define dill_retci(s, imm) (s->j->reti)(s, DILL_C, 0, imm)
#define dill_retuc(s, src) (s->j->ret)(s, DILL_I, 0, src)
#define dill_retuci(s, imm) (s->j->reti)(s, DILL_UC, 0, imm)
#define dill_rets(s, src) (s->j->ret)(s, DILL_I, 0, src)
#define dill_retsi(s, imm) (s->j->reti)(s, DILL_S, 0, imm)
#define dill_retus(s, src) (s->j->ret)(s, DILL_I, 0, src)
#define dill_retusi(s, imm) (s->j->reti)(s, DILL_US, 0, imm)
#define dill_reti(s, src) (s->j->ret)(s, DILL_I, 0, src)
#define dill_retii(s, imm) (s->j->reti)(s, DILL_I, 0, imm)
#define dill_retu(s, src) (s->j->ret)(s, DILL_U, 0, src)
#define dill_retui(s, imm) (s->j->reti)(s, DILL_U, 0, imm)
#define dill_retl(s, src) (s->j->ret)(s, DILL_L, 0, src)
#define dill_retli(s, imm) (s->j->reti)(s, DILL_L, 0, imm)
#define dill_retul(s, src) (s->j->ret)(s, DILL_UL, 0, src)
#define dill_retuli(s, imm) (s->j->reti)(s, DILL_UL, 0, imm)
#define dill_retp(s, src) (s->j->ret)(s, DILL_P, 0, src)
#define dill_retpi(s, imm) (s->j->reti)(s, DILL_P, 0, (IMM_TYPE) imm)
#define dill_retd(s, src) (s->j->ret)(s, DILL_D, 0, src)
#define dill_retdi(s, imm) (s->j->retf)(s, DILL_D, 0, imm)
#define dill_retf(s, src) (s->j->ret)(s, DILL_F, 0, src)
#define dill_retfi(s, imm) (s->j->retf)(s, DILL_F, 0, imm)
#define dill_pret(s, type, src) (s->j->ret)(s, type, 0, src)
#define dill_movc(s, dest, src) (s->j->mov)(s, DILL_C, 0, dest, src)
#define dill_movuc(s, dest, src) (s->j->mov)(s, DILL_UC, 0, dest, src)
#define dill_movs(s, dest, src) (s->j->mov)(s, DILL_S, 0, dest, src)
#define dill_movus(s, dest, src) (s->j->mov)(s, DILL_US, 0, dest, src)
#define dill_movi(s, dest, src) (s->j->mov)(s, DILL_I, 0, dest, src)
#define dill_movu(s, dest, src) (s->j->mov)(s, DILL_U, 0, dest, src)
#define dill_movl(s, dest, src) (s->j->mov)(s, DILL_L, 0, dest, src)
#define dill_movul(s, dest, src) (s->j->mov)(s, DILL_UL, 0, dest, src)
#define dill_movp(s, dest, src) (s->j->mov)(s, DILL_P, 0, dest, src)
#define dill_movd(s, dest, src) (s->j->mov)(s, DILL_D, 0, dest, src)
#define dill_movf(s, dest, src) (s->j->mov)(s, DILL_F, 0, dest, src)
#define dill_pmov(s, type, dest, src) (s->j->mov)(s, type, 0, dest, src)
#define dill_setc(s, dest, imm) (s->j->set)(s, DILL_C, 0, dest, imm)
#define dill_setuc(s, dest, imm) (s->j->set)(s, DILL_UC, 0, dest, imm)
#define dill_sets(s, dest, imm) (s->j->set)(s, DILL_S, 0, dest, imm)
#define dill_setus(s, dest, imm) (s->j->set)(s, DILL_US, 0, dest, imm)
#define dill_seti(s, dest, imm) (s->j->set)(s, DILL_I, 0, dest, imm)
#define dill_setu(s, dest, imm) (s->j->set)(s, DILL_U, 0, dest, imm)
#define dill_setl(s, dest, imm) (s->j->set)(s, DILL_L, 0, dest, imm)
#define dill_setul(s, dest, imm) (s->j->set)(s, DILL_UL, 0, dest, imm)
#define dill_setp(s, dest, imm) (s->j->setp)(s, DILL_P, 0, dest, imm)
#define dill_setd(s, dest, imm) (s->j->setf)(s, DILL_D, 0, dest, imm)
#define dill_setf(s, dest, imm) (s->j->setf)(s, DILL_F, 0, dest, imm)
#define dill_piset(s, type, dest, imm) (s->j->set)(s, type, 0, dest, imm)
#define dill_push_arg(s, type, reg) (s->j->push)(s, type, reg)
#define dill_push_init(s) (s->j->push)(s, DILL_V, -1)
#define dill_push_varidiac_init(s, non_var_arg_count) (s->j->push)(s, DILL_V, -2-(non_var_arg_count))
#define dill_push_argi(s, reg) (s->j->push)(s, DILL_I, reg)
#define dill_push_argii(s, imm) (s->j->pushi)(s, DILL_I, imm)
#define dill_push_argu(s, reg) (s->j->push)(s, DILL_U, reg)
#define dill_push_argui(s, imm) (s->j->pushi)(s, DILL_U, imm)
#define dill_push_argl(s, reg) (s->j->push)(s, DILL_L, reg)
#define dill_push_argli(s, imm) (s->j->pushi)(s, DILL_L, imm)
#define dill_push_argul(s, reg) (s->j->push)(s, DILL_UL, reg)
#define dill_push_arguli(s, imm) (s->j->pushi)(s, DILL_UL, imm)
#define dill_push_argp(s, reg) (s->j->push)(s, DILL_P, reg)
#define dill_push_argpi(s, imm) (s->j->pushpi)(s, DILL_P, imm)
#define dill_push_argf(s, reg) (s->j->push)(s, DILL_F, reg)
#define dill_push_argfi(s, imm) (s->j->pushfi)(s, DILL_F, imm)
#define dill_push_argd(s, reg) (s->j->push)(s, DILL_D, reg)
#define dill_push_argdi(s, imm) (s->j->pushfi)(s, DILL_D, imm)
extern int dill_scalli(dill_stream s, void *ptr, const char *name, const char *arg_str, ...);
#define dill_calli(s, ptr, name) s->j->calli(s, DILL_I , ptr, name)
#define dill_callri(s, src) s->j->callr(s, DILL_I , src)
extern int dill_scallu(dill_stream s, void *ptr, const char *name, const char *arg_str, ...);
#define dill_callu(s, ptr, name) s->j->calli(s, DILL_U , ptr, name)
#define dill_callru(s, src) s->j->callr(s, DILL_U , src)
extern int dill_scalll(dill_stream s, void *ptr, const char *name, const char *arg_str, ...);
#define dill_calll(s, ptr, name) s->j->calli(s, DILL_L , ptr, name)
#define dill_callrl(s, src) s->j->callr(s, DILL_L , src)
extern int dill_scallul(dill_stream s, void *ptr, const char *name, const char *arg_str, ...);
#define dill_callul(s, ptr, name) s->j->calli(s, DILL_UL , ptr, name)
#define dill_callrul(s, src) s->j->callr(s, DILL_UL , src)
extern int dill_scallp(dill_stream s, void *ptr, const char *name, const char *arg_str, ...);
#define dill_callp(s, ptr, name) s->j->calli(s, DILL_P , ptr, name)
#define dill_callrp(s, src) s->j->callr(s, DILL_P , src)
extern int dill_scallf(dill_stream s, void *ptr, const char *name, const char *arg_str, ...);
#define dill_callf(s, ptr, name) s->j->calli(s, DILL_F , ptr, name)
#define dill_callrf(s, src) s->j->callr(s, DILL_F , src)
extern int dill_scalld(dill_stream s, void *ptr, const char *name, const char *arg_str, ...);
#define dill_calld(s, ptr, name) s->j->calli(s, DILL_D , ptr, name)
#define dill_callrd(s, src) s->j->callr(s, DILL_D , src)
extern void dill_scallv(dill_stream s, void *ptr, const char *name, const char *arg_str, ...);
#define dill_callv(s, ptr, name) s->j->calli(s, DILL_V , ptr, name)
#define dill_callrv(s, src) s->j->callr(s, DILL_V , src)
#define dill_pcall(s, type, ptr, name) s->j->calli(s, type, ptr, name)
#define dill_pcallr(s, type, src) s->j->callr(s, type, src)
#define dill_savei(s, reg) s->j->save_restore(s, 0, DILL_I, reg)
#define dill_restorei(s, reg) s->j->save_restore(s, 1, DILL_I, reg)
#define dill_saveu(s, reg) s->j->save_restore(s, 0, DILL_U, reg)
#define dill_restoreu(s, reg) s->j->save_restore(s, 1, DILL_U, reg)
#define dill_savel(s, reg) s->j->save_restore(s, 0, DILL_L, reg)
#define dill_restorel(s, reg) s->j->save_restore(s, 1, DILL_L, reg)
#define dill_saveul(s, reg) s->j->save_restore(s, 0, DILL_UL, reg)
#define dill_restoreul(s, reg) s->j->save_restore(s, 1, DILL_UL, reg)
#define dill_savep(s, reg) s->j->save_restore(s, 0, DILL_P, reg)
#define dill_restorep(s, reg) s->j->save_restore(s, 1, DILL_P, reg)
#define dill_saved(s, reg) s->j->save_restore(s, 0, DILL_D, reg)
#define dill_restored(s, reg) s->j->save_restore(s, 1, DILL_D, reg)
#define dill_savef(s, reg) s->j->save_restore(s, 0, DILL_F, reg)
#define dill_restoref(s, reg) s->j->save_restore(s, 1, DILL_F, reg)
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define dill_jmp_a3_size 50
#define dill_jmp_a2_size 22
#define dill_jmp_branch_size 66
#define dill_jmp_compare_size 66
#define dill_jmp_convert_size 0
#endif
#define dill_jv(s, label) (s->j->jv)(s, (unsigned long) label)
#define dill_jp(s, dest_reg) (s->j->jp)(s, (unsigned long) dest_reg)
#define dill_jpi(s, dest_imm) (s->j->jpi)(s, dest_imm)
#define dill_jalp(s, return_addr_reg, target) (s->j->jal)(s, return_addr_reg, target)
#define dill_special(s, type, param) if (s->j->special) (s->j->special)(s, type, param)
#if defined(_MSC_VER) && !defined(DILL_SRC)
#define DILL_DECLSPEC    __declspec(dllimport)
#else
#define DILL_DECLSPEC
#endif
extern DILL_DECLSPEC int dill_add_poly_map[];
#define dill_Padd(s, typ, dest, src1, src2) (s->j->jmp_a3)[dill_add_poly_map[typ]](s, s->j->a3_data[dill_add_poly_map[typ]].data1, s->j->a3_data[dill_add_poly_map[typ]].data2, dest, src1, src2)
extern DILL_DECLSPEC int dill_and_poly_map[];
#define dill_Pand(s, typ, dest, src1, src2) (s->j->jmp_a3)[dill_and_poly_map[typ]](s, s->j->a3_data[dill_and_poly_map[typ]].data1, s->j->a3_data[dill_and_poly_map[typ]].data2, dest, src1, src2)
extern DILL_DECLSPEC int dill_div_poly_map[];
#define dill_Pdiv(s, typ, dest, src1, src2) (s->j->jmp_a3)[dill_div_poly_map[typ]](s, s->j->a3_data[dill_div_poly_map[typ]].data1, s->j->a3_data[dill_div_poly_map[typ]].data2, dest, src1, src2)
extern DILL_DECLSPEC int dill_lsh_poly_map[];
#define dill_Plsh(s, typ, dest, src1, src2) (s->j->jmp_a3)[dill_lsh_poly_map[typ]](s, s->j->a3_data[dill_lsh_poly_map[typ]].data1, s->j->a3_data[dill_lsh_poly_map[typ]].data2, dest, src1, src2)
extern DILL_DECLSPEC int dill_mod_poly_map[];
#define dill_Pmod(s, typ, dest, src1, src2) (s->j->jmp_a3)[dill_mod_poly_map[typ]](s, s->j->a3_data[dill_mod_poly_map[typ]].data1, s->j->a3_data[dill_mod_poly_map[typ]].data2, dest, src1, src2)
extern DILL_DECLSPEC int dill_mul_poly_map[];
#define dill_Pmul(s, typ, dest, src1, src2) (s->j->jmp_a3)[dill_mul_poly_map[typ]](s, s->j->a3_data[dill_mul_poly_map[typ]].data1, s->j->a3_data[dill_mul_poly_map[typ]].data2, dest, src1, src2)
extern DILL_DECLSPEC int dill_or_poly_map[];
#define dill_Por(s, typ, dest, src1, src2) (s->j->jmp_a3)[dill_or_poly_map[typ]](s, s->j->a3_data[dill_or_poly_map[typ]].data1, s->j->a3_data[dill_or_poly_map[typ]].data2, dest, src1, src2)
extern DILL_DECLSPEC int dill_rsh_poly_map[];
#define dill_Prsh(s, typ, dest, src1, src2) (s->j->jmp_a3)[dill_rsh_poly_map[typ]](s, s->j->a3_data[dill_rsh_poly_map[typ]].data1, s->j->a3_data[dill_rsh_poly_map[typ]].data2, dest, src1, src2)
extern DILL_DECLSPEC int dill_sub_poly_map[];
#define dill_Psub(s, typ, dest, src1, src2) (s->j->jmp_a3)[dill_sub_poly_map[typ]](s, s->j->a3_data[dill_sub_poly_map[typ]].data1, s->j->a3_data[dill_sub_poly_map[typ]].data2, dest, src1, src2)
extern DILL_DECLSPEC int dill_xor_poly_map[];
#define dill_Pxor(s, typ, dest, src1, src2) (s->j->jmp_a3)[dill_xor_poly_map[typ]](s, s->j->a3_data[dill_xor_poly_map[typ]].data1, s->j->a3_data[dill_xor_poly_map[typ]].data2, dest, src1, src2)

typedef enum {iclass_arith3, iclass_arith3i, iclass_arith2, iclass_ret, iclass_convert, iclass_loadstore, iclass_loadstorei, iclass_set, iclass_setf, iclass_mov, iclass_reti, iclass_branch, iclass_branchi, iclass_jump_to_label, iclass_special, iclass_jump_to_reg, iclass_jump_to_imm, iclass_push, iclass_pushi, iclass_pushf, iclass_call, iclass_lea, iclass_compare, iclass_mark_label, iclass_nop} insn_class;

struct arith3 {
    int dest;
    int src1;
    int src2;
};

struct arith3i {
    int dest;
    int src;
    union {
	IMM_TYPE imm;
	void *imm_a;
    }u;
};

struct arith2 {
    int dest;
    int src;
};

struct arith1 {
    int src;
};

struct setf {
    int dest;
    double imm;
};

struct branch {
    int src1;
    int src2;
    int label;
};

struct branchi {
    int src;
    int label;
    void *imm_a;
    IMM_TYPE imm_l;
};

struct calli {
    int src;
    void *imm_a;
    IMM_TYPE imm_l;
    const char *xfer_name;
};

struct label {
    int label;
    char *label_name;
};

struct special {
    special_operations type;
    IMM_TYPE param;
};

union operands {
    struct arith3 a3;
    struct arith3i a3i;
    struct arith2 a2;
    struct arith1 a1;
    struct setf sf;
    struct branch br;
    struct branchi bri;
    struct calli calli;
    struct label label;
    struct special spec;
};

typedef struct {
    char class_code;
    char insn_code;
    union operands opnds;
} virtual_insn;

typedef void (*dill_foreign_cg_func)(dill_stream s, virtual_insn *start, virtual_insn *end);

extern void
dill_set_foreign_cg(dill_foreign_cg_func foreign);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* __DILL_H__ */

