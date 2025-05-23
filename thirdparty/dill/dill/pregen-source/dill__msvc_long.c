char *branch_op_names[] = {"eqc", "equc", "eqs", "equs", "eqi", "equ", "eql", "equl", "eqp", "eqf", "eqd", "gec", "geuc", "ges", "geus", "gei", "geu", "gel", "geul", "gep", "gef", "ged", "gtc", "gtuc", "gts", "gtus", "gti", "gtu", "gtl", "gtul", "gtp", "gtf", "gtd", "lec", "leuc", "les", "leus", "lei", "leu", "lel", "leul", "lep", "lef", "led", "ltc", "ltuc", "lts", "ltus", "lti", "ltu", "ltl", "ltul", "ltp", "ltf", "ltd", "nec", "neuc", "nes", "neus", "nei", "neu", "nel", "neul", "nep", "nef", "ned", 0};
char *compare_op_names[] = {"eqc", "equc", "eqs", "equs", "eqi", "equ", "eql", "equl", "eqp", "eqf", "eqd", "gec", "geuc", "ges", "geus", "gei", "geu", "gel", "geul", "gep", "gef", "ged", "gtc", "gtuc", "gts", "gtus", "gti", "gtu", "gtl", "gtul", "gtp", "gtf", "gtd", "lec", "leuc", "les", "leus", "lei", "leu", "lel", "leul", "lep", "lef", "led", "ltc", "ltuc", "lts", "ltus", "lti", "ltu", "ltl", "ltul", "ltp", "ltf", "ltd", "nec", "neuc", "nes", "neus", "nei", "neu", "nel", "neul", "nep", "nef", "ned", 0};

/* This file is generated from base.ops.  Do not edit directly. */

#include "dill.h"

char *arith3_name[] = {"addi", "addu", "addul", "addl", "subi", "subu", "subul", "subl", "muli", "mulu", "mulul", "mull", "divi", "divu", "divul", "divl", "modi", "modu", "modul", "modl", "xori", "xoru", "xorul", "xorl", "andi", "andu", "andul", "andl", "ori", "oru", "orul", "orl", "lshi", "lshu", "lshul", "lshl", "rshi", "rshu", "rshul", "rshl", "addp", "subp", "addf", "addd", "subf", "subd", "mulf", "muld", "divf", "divd"};

char *arith2_name[] = {"noti", "notu", "notul", "notl", "comi", "comu", "comul", "coml", "negi", "negu", "negul", "negl", "bswaps", "bswapus", "bswapi", "bswapu", "bswapul", "bswapl", "bswapf", "bswapd", "negf", "negd"};
int dill_add_poly_map[] = {
dill_jmp_addl, dill_jmp_addul, dill_jmp_addl, dill_jmp_addul, dill_jmp_addi, dill_jmp_addu, dill_jmp_addl, dill_jmp_addul, dill_jmp_addp, dill_jmp_addf, dill_jmp_addd, dill_jmp_addl, dill_jmp_addl, 0};
int dill_and_poly_map[] = {
dill_jmp_andl, dill_jmp_andul, dill_jmp_andl, dill_jmp_andul, dill_jmp_andi, dill_jmp_andu, dill_jmp_andl, dill_jmp_andul, dill_jmp_andl, dill_jmp_andl, dill_jmp_andl, dill_jmp_andl, dill_jmp_andl, 0};
int dill_div_poly_map[] = {
dill_jmp_divl, dill_jmp_divul, dill_jmp_divl, dill_jmp_divul, dill_jmp_divi, dill_jmp_divu, dill_jmp_divl, dill_jmp_divul, dill_jmp_divl, dill_jmp_divf, dill_jmp_divd, dill_jmp_divl, dill_jmp_divl, 0};
int dill_lsh_poly_map[] = {
dill_jmp_lshl, dill_jmp_lshul, dill_jmp_lshl, dill_jmp_lshul, dill_jmp_lshi, dill_jmp_lshu, dill_jmp_lshl, dill_jmp_lshul, dill_jmp_lshl, dill_jmp_lshl, dill_jmp_lshl, dill_jmp_lshl, dill_jmp_lshl, 0};
int dill_mod_poly_map[] = {
dill_jmp_modl, dill_jmp_modul, dill_jmp_modl, dill_jmp_modul, dill_jmp_modi, dill_jmp_modu, dill_jmp_modl, dill_jmp_modul, dill_jmp_modl, dill_jmp_modl, dill_jmp_modl, dill_jmp_modl, dill_jmp_modl, 0};
int dill_mul_poly_map[] = {
dill_jmp_mull, dill_jmp_mulul, dill_jmp_mull, dill_jmp_mulul, dill_jmp_muli, dill_jmp_mulu, dill_jmp_mull, dill_jmp_mulul, dill_jmp_mull, dill_jmp_mulf, dill_jmp_muld, dill_jmp_mull, dill_jmp_mull, 0};
int dill_or_poly_map[] = {
dill_jmp_orl, dill_jmp_orul, dill_jmp_orl, dill_jmp_orul, dill_jmp_ori, dill_jmp_oru, dill_jmp_orl, dill_jmp_orul, dill_jmp_orl, dill_jmp_orl, dill_jmp_orl, dill_jmp_orl, dill_jmp_orl, 0};
int dill_rsh_poly_map[] = {
dill_jmp_rshl, dill_jmp_rshul, dill_jmp_rshl, dill_jmp_rshul, dill_jmp_rshi, dill_jmp_rshu, dill_jmp_rshl, dill_jmp_rshul, dill_jmp_rshl, dill_jmp_rshl, dill_jmp_rshl, dill_jmp_rshl, dill_jmp_rshl, 0};
int dill_sub_poly_map[] = {
dill_jmp_subl, dill_jmp_subul, dill_jmp_subl, dill_jmp_subul, dill_jmp_subi, dill_jmp_subu, dill_jmp_subl, dill_jmp_subul, dill_jmp_subp, dill_jmp_subf, dill_jmp_subd, dill_jmp_subl, dill_jmp_subl, 0};
int dill_xor_poly_map[] = {
dill_jmp_xorl, dill_jmp_xorul, dill_jmp_xorl, dill_jmp_xorul, dill_jmp_xori, dill_jmp_xoru, dill_jmp_xorl, dill_jmp_xorul, dill_jmp_xorl, dill_jmp_xorl, dill_jmp_xorl, dill_jmp_xorl, dill_jmp_xorl, 0};
char *dill_type_names[] = {
    "c",    /* char */
    "uc",   /* unsigned char */
    "s",    /* short */
    "us",   /* unsigned short */
    "i",    /* int */
    "u",    /* unsigned */
    "l",    /* long (full register size) */
    "ul",   /* unsigned long (full register size) */
    "p",    /* pointer */
    "f",    /* floating */
    "d",    /* double */
    "v",    /* void */
    "b",    /* block structure */
};
