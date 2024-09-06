#ifndef __STRUCTS_H
#define __STRUCTS_H
struct list_struct {
    sm_ref node;
    struct list_struct *next;
};

typedef enum {
    cod_compound_statement,
    cod_enumerator,
    cod_declaration,
    cod_label_statement,
    cod_return_statement,
    cod_jump_statement,
    cod_selection_statement,
    cod_iteration_statement,
    cod_expression_statement,
    cod_assignment_expression,
    cod_comma_expression,
    cod_operator,
    cod_conditional_operator,
    cod_identifier,
    cod_constant,
    cod_type_specifier,
    cod_struct_type_decl,
    cod_enum_type_decl,
    cod_array_type_decl,
    cod_reference_type_decl,
    cod_field,
    cod_field_ref,
    cod_subroutine_call,
    cod_element_ref,
    cod_cast,
    cod_initializer_list,
    cod_initializer,
    cod_designator,
    cod_last_node_type
} cod_node_type;

typedef struct {
    sm_list decls;
    sm_list statements;
} compound_statement;

typedef struct {
    char* id;
    sm_ref const_expression;
    int enum_value;
} enumerator;

typedef struct {
    sm_list type_spec;
    sm_ref sm_complex_type;
    sm_ref freeable_complex_type;
    int static_var;
    int const_var;
    int param_num;
    char* id;
    sm_ref init_value;
    srcpos lx_srcpos;
    int is_subroutine;
    int varidiac_subroutine_param_count;
    int is_typedef;
    int addr_taken;
    int is_extern;
    sm_list params;
    int cg_oprnd;
    int cg_type;
    void* closure_id;
    void* cg_address;
} declaration;

typedef struct {
    char* name;
    int cg_label;
    sm_ref statement;
} label_statement;

typedef struct {
    sm_ref expression;
    int cg_func_type;
    srcpos lx_srcpos;
} return_statement;

typedef struct {
    int continue_flag;
    char* goto_target;
    sm_ref sm_target_stmt;
    srcpos lx_srcpos;
} jump_statement;

typedef struct {
    sm_ref conditional;
    sm_ref then_part;
    sm_ref else_part;
    srcpos lx_srcpos;
} selection_statement;

typedef struct {
    sm_ref init_expr;
    sm_ref test_expr;
    sm_ref post_test_expr;
    sm_ref iter_expr;
    sm_ref statement;
    int cg_iter_label;
    int cg_end_label;
    srcpos lx_srcpos;
} iteration_statement;

typedef struct {
    sm_ref expression;
} expression_statement;

typedef struct {
    sm_ref left;
    sm_ref right;
    srcpos lx_srcpos;
    operator_t op;
    int cg_type;
} assignment_expression;

typedef struct {
    sm_ref left;
    sm_ref right;
    srcpos lx_srcpos;
} comma_expression;

typedef struct {
    operator_t op;
    sm_ref left;
    sm_ref right;
    srcpos lx_srcpos;
    int operation_type;
    int result_type;
} operator;

typedef struct {
    sm_ref condition;
    sm_ref e1;
    sm_ref e2;
    srcpos lx_srcpos;
    int result_type;
} conditional_operator;

typedef struct {
    char* id;
    srcpos lx_srcpos;
    sm_ref sm_declaration;
    int cg_type;
} identifier;

typedef struct {
    int token;
    char* const_val;
    char* freeable_name;
    srcpos lx_srcpos;
} constant;

typedef struct {
    int token;
    srcpos lx_srcpos;
    sm_ref created_type_decl;
} type_specifier;

typedef struct {
    srcpos lx_srcpos;
    char* id;
    sm_list fields;
    int cg_size;
    enc_info encode_info;
} struct_type_decl;

typedef struct {
    srcpos lx_srcpos;
    char* id;
    sm_list enums;
} enum_type_decl;

typedef struct {
    srcpos lx_srcpos;
    sm_ref size_expr;
    sm_ref element_ref;
    int static_var;
    sm_list type_spec;
    dimen_p dimensions;
    int cg_static_size;
    sm_ref sm_dynamic_size;
    int cg_element_type;
    sm_ref sm_complex_element_type;
    sm_ref freeable_complex_element_type;
    int cg_element_size;
} array_type_decl;

typedef struct {
    srcpos lx_srcpos;
    char* name;
    sm_list type_spec;
    int cg_referenced_type;
    sm_ref sm_complex_referenced_type;
    sm_ref freeable_complex_referenced_type;
    int cg_referenced_size;
    int kernel_ref;
} reference_type_decl;

typedef struct {
    char* name;
    char* string_type;
    sm_list type_spec;
    sm_ref sm_complex_type;
    sm_ref freeable_complex_type;
    int cg_size;
    int cg_offset;
    int cg_type;
} field;

typedef struct {
    srcpos lx_srcpos;
    sm_ref struct_ref;
    char* lx_field;
    sm_ref sm_field_ref;
} field_ref;

typedef struct {
    srcpos lx_srcpos;
    sm_ref sm_func_ref;
    sm_list arguments;
} subroutine_call;

typedef struct {
    srcpos lx_srcpos;
    sm_ref array_ref;
    sm_ref sm_complex_element_type;
    sm_ref sm_containing_structure_ref;
    int cg_element_type;
    int this_index_dimension;
    sm_ref expression;
} element_ref;

typedef struct {
    srcpos lx_srcpos;
    sm_list type_spec;
    int cg_type;
    sm_ref expression;
    sm_ref sm_complex_type;
} cast;

typedef struct {
    sm_list initializers;
} initializer_list;

typedef struct {
    sm_list designation;
    sm_ref initializer;
} initializer;

typedef struct {
    sm_ref expression;
    char* id;
} designator;

typedef union {
   compound_statement compound_statement;
   enumerator enumerator;
   declaration declaration;
   label_statement label_statement;
   return_statement return_statement;
   jump_statement jump_statement;
   selection_statement selection_statement;
   iteration_statement iteration_statement;
   expression_statement expression_statement;
   assignment_expression assignment_expression;
   comma_expression comma_expression;
   operator operator;
   conditional_operator conditional_operator;
   identifier identifier;
   constant constant;
   type_specifier type_specifier;
   struct_type_decl struct_type_decl;
   enum_type_decl enum_type_decl;
   array_type_decl array_type_decl;
   reference_type_decl reference_type_decl;
   field field;
   field_ref field_ref;
   subroutine_call subroutine_call;
   element_ref element_ref;
   cast cast;
   initializer_list initializer_list;
   initializer initializer;
   designator designator;
} sm_union;

struct sm_struct {
    cod_node_type node_type;
    int visited;
    sm_union node;
};
#endif
extern sm_ref cod_new_compound_statement();
extern sm_ref cod_new_enumerator();
extern sm_ref cod_new_declaration();
extern sm_ref cod_new_label_statement();
extern sm_ref cod_new_return_statement();
extern sm_ref cod_new_jump_statement();
extern sm_ref cod_new_selection_statement();
extern sm_ref cod_new_iteration_statement();
extern sm_ref cod_new_expression_statement();
extern sm_ref cod_new_assignment_expression();
extern sm_ref cod_new_comma_expression();
extern sm_ref cod_new_operator();
extern sm_ref cod_new_conditional_operator();
extern sm_ref cod_new_identifier();
extern sm_ref cod_new_constant();
extern sm_ref cod_new_type_specifier();
extern sm_ref cod_new_struct_type_decl();
extern sm_ref cod_new_enum_type_decl();
extern sm_ref cod_new_array_type_decl();
extern sm_ref cod_new_reference_type_decl();
extern sm_ref cod_new_field();
extern sm_ref cod_new_field_ref();
extern sm_ref cod_new_subroutine_call();
extern sm_ref cod_new_element_ref();
extern sm_ref cod_new_cast();
extern sm_ref cod_new_initializer_list();
extern sm_ref cod_new_initializer();
extern sm_ref cod_new_designator();
typedef void (*cod_apply_func)(sm_ref node, void *data);
typedef void (*cod_apply_list_func)(sm_list list, void *data);
extern void cod_apply(sm_ref node, cod_apply_func pre_func, cod_apply_func post_func, cod_apply_list_func list_func, void *data);
extern void cod_print(sm_ref node);
extern void cod_free(sm_ref node);
extern void cod_free_list(sm_list list, void *junk);
extern void cod_rfree(sm_ref node);
extern void cod_rfree_list(sm_list list, void *junk);
extern sm_ref cod_copy(sm_ref node);
extern sm_list cod_copy_list(sm_list list);
extern sm_list cod_append_list(sm_list list1, sm_list list2);
extern srcpos cod_get_srcpos(sm_ref expr);
