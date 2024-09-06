#include "config.h"
#undef NDEBUG
#define assert(EX) ((EX) ? (void)0 : (fprintf(stderr, "\"%s\" failed, file %s, line %d\n", #EX, __FILE__, __LINE__), exit(1)))
#ifndef LINUX_KERNEL_MODULE
#include <stdio.h>
#endif
#ifdef LINUX_KERNEL_MODULE
#ifndef MODULE
#define MODULE
#endif
#ifndef __KERNEL__
#define __KERNEL__
#endif
#include <linux/kernel.h>
#include <linux/module.h>
#endif
#include "cod.h"
#include "cod_internal.h"
#include "structs.h"
#ifndef LINUX_KERNEL_MODULE
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <string.h>
#else
#include <linux/string.h>
#include "kcod.h"
#define malloc (void *)DAllocMM
#define free(a) DFreeMM((addrs_t)a)
#define fprintf(fmt, args...) printk(args)
#define printf printk
#endif
#ifndef NULL
#define NULL 0
#endif
#if defined(_MSC_VER)
#define strdup _strdup
#endif
extern sm_ref
cod_new_compound_statement()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_compound_statement;
    return tmp;
}

extern sm_ref
cod_new_enumerator()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_enumerator;
    return tmp;
}

extern sm_ref
cod_new_declaration()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_declaration;
    return tmp;
}

extern sm_ref
cod_new_label_statement()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_label_statement;
    return tmp;
}

extern sm_ref
cod_new_return_statement()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_return_statement;
    return tmp;
}

extern sm_ref
cod_new_jump_statement()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_jump_statement;
    return tmp;
}

extern sm_ref
cod_new_selection_statement()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_selection_statement;
    return tmp;
}

extern sm_ref
cod_new_iteration_statement()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_iteration_statement;
    return tmp;
}

extern sm_ref
cod_new_expression_statement()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_expression_statement;
    return tmp;
}

extern sm_ref
cod_new_assignment_expression()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_assignment_expression;
    return tmp;
}

extern sm_ref
cod_new_comma_expression()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_comma_expression;
    return tmp;
}

extern sm_ref
cod_new_operator()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_operator;
    return tmp;
}

extern sm_ref
cod_new_conditional_operator()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_conditional_operator;
    return tmp;
}

extern sm_ref
cod_new_identifier()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_identifier;
    return tmp;
}

extern sm_ref
cod_new_constant()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_constant;
    return tmp;
}

extern sm_ref
cod_new_type_specifier()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_type_specifier;
    return tmp;
}

extern sm_ref
cod_new_struct_type_decl()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_struct_type_decl;
    return tmp;
}

extern sm_ref
cod_new_enum_type_decl()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_enum_type_decl;
    return tmp;
}

extern sm_ref
cod_new_array_type_decl()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_array_type_decl;
    return tmp;
}

extern sm_ref
cod_new_reference_type_decl()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_reference_type_decl;
    return tmp;
}

extern sm_ref
cod_new_field()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_field;
    return tmp;
}

extern sm_ref
cod_new_field_ref()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_field_ref;
    return tmp;
}

extern sm_ref
cod_new_subroutine_call()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_subroutine_call;
    return tmp;
}

extern sm_ref
cod_new_element_ref()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_element_ref;
    return tmp;
}

extern sm_ref
cod_new_cast()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_cast;
    return tmp;
}

extern sm_ref
cod_new_initializer_list()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_initializer_list;
    return tmp;
}

extern sm_ref
cod_new_initializer()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_initializer;
    return tmp;
}

extern sm_ref
cod_new_designator()
{
    sm_ref tmp = malloc(sizeof(*tmp));
    memset(tmp, 0, sizeof(*tmp));
    tmp->visited = 0;
    tmp->node_type = cod_designator;
    return tmp;
}

static void cod_apply_list(sm_list node, cod_apply_func pre_func, cod_apply_func post_func, cod_apply_list_func list_func, void *data)
{
    sm_list orig = node;
    while (node != NULL) {
        cod_apply(node->node, pre_func, post_func, list_func, data);
        node = node->next;
    }
    if (list_func) (list_func)(orig, data);
}

extern void cod_apply(sm_ref node, cod_apply_func pre_func, cod_apply_func post_func, cod_apply_list_func list_func, void *data)
{
    if (node == NULL) return;
    if (node->visited) return;
    node->visited++;
    if(pre_func) (pre_func)(node, data);
    switch(node->node_type) {
      case cod_compound_statement: {
          cod_apply_list(node->node.compound_statement.decls, pre_func, post_func, list_func, data);
          cod_apply_list(node->node.compound_statement.statements, pre_func, post_func, list_func, data);
          break;
      }
      case cod_enumerator: {
          cod_apply(node->node.enumerator.const_expression, pre_func, post_func, list_func, data);
          break;
      }
      case cod_declaration: {
          cod_apply_list(node->node.declaration.type_spec, pre_func, post_func, list_func, data);
          cod_apply(node->node.declaration.freeable_complex_type, pre_func, post_func, list_func, data);
          cod_apply(node->node.declaration.init_value, pre_func, post_func, list_func, data);
          cod_apply_list(node->node.declaration.params, pre_func, post_func, list_func, data);
          break;
      }
      case cod_label_statement: {
          cod_apply(node->node.label_statement.statement, pre_func, post_func, list_func, data);
          break;
      }
      case cod_return_statement: {
          cod_apply(node->node.return_statement.expression, pre_func, post_func, list_func, data);
          break;
      }
      case cod_jump_statement: {
          break;
      }
      case cod_selection_statement: {
          cod_apply(node->node.selection_statement.conditional, pre_func, post_func, list_func, data);
          cod_apply(node->node.selection_statement.then_part, pre_func, post_func, list_func, data);
          cod_apply(node->node.selection_statement.else_part, pre_func, post_func, list_func, data);
          break;
      }
      case cod_iteration_statement: {
          cod_apply(node->node.iteration_statement.init_expr, pre_func, post_func, list_func, data);
          cod_apply(node->node.iteration_statement.test_expr, pre_func, post_func, list_func, data);
          cod_apply(node->node.iteration_statement.post_test_expr, pre_func, post_func, list_func, data);
          cod_apply(node->node.iteration_statement.iter_expr, pre_func, post_func, list_func, data);
          cod_apply(node->node.iteration_statement.statement, pre_func, post_func, list_func, data);
          break;
      }
      case cod_expression_statement: {
          cod_apply(node->node.expression_statement.expression, pre_func, post_func, list_func, data);
          break;
      }
      case cod_assignment_expression: {
          cod_apply(node->node.assignment_expression.left, pre_func, post_func, list_func, data);
          cod_apply(node->node.assignment_expression.right, pre_func, post_func, list_func, data);
          break;
      }
      case cod_comma_expression: {
          cod_apply(node->node.comma_expression.left, pre_func, post_func, list_func, data);
          cod_apply(node->node.comma_expression.right, pre_func, post_func, list_func, data);
          break;
      }
      case cod_operator: {
          cod_apply(node->node.operator.left, pre_func, post_func, list_func, data);
          cod_apply(node->node.operator.right, pre_func, post_func, list_func, data);
          break;
      }
      case cod_conditional_operator: {
          cod_apply(node->node.conditional_operator.condition, pre_func, post_func, list_func, data);
          cod_apply(node->node.conditional_operator.e1, pre_func, post_func, list_func, data);
          cod_apply(node->node.conditional_operator.e2, pre_func, post_func, list_func, data);
          break;
      }
      case cod_identifier: {
          break;
      }
      case cod_constant: {
          break;
      }
      case cod_type_specifier: {
          cod_apply(node->node.type_specifier.created_type_decl, pre_func, post_func, list_func, data);
          break;
      }
      case cod_struct_type_decl: {
          cod_apply_list(node->node.struct_type_decl.fields, pre_func, post_func, list_func, data);
          break;
      }
      case cod_enum_type_decl: {
          cod_apply_list(node->node.enum_type_decl.enums, pre_func, post_func, list_func, data);
          break;
      }
      case cod_array_type_decl: {
          cod_apply(node->node.array_type_decl.size_expr, pre_func, post_func, list_func, data);
          cod_apply(node->node.array_type_decl.element_ref, pre_func, post_func, list_func, data);
          cod_apply_list(node->node.array_type_decl.type_spec, pre_func, post_func, list_func, data);
          cod_apply(node->node.array_type_decl.freeable_complex_element_type, pre_func, post_func, list_func, data);
          break;
      }
      case cod_reference_type_decl: {
          cod_apply_list(node->node.reference_type_decl.type_spec, pre_func, post_func, list_func, data);
          cod_apply(node->node.reference_type_decl.freeable_complex_referenced_type, pre_func, post_func, list_func, data);
          break;
      }
      case cod_field: {
          cod_apply_list(node->node.field.type_spec, pre_func, post_func, list_func, data);
          cod_apply(node->node.field.freeable_complex_type, pre_func, post_func, list_func, data);
          break;
      }
      case cod_field_ref: {
          cod_apply(node->node.field_ref.struct_ref, pre_func, post_func, list_func, data);
          break;
      }
      case cod_subroutine_call: {
          cod_apply_list(node->node.subroutine_call.arguments, pre_func, post_func, list_func, data);
          break;
      }
      case cod_element_ref: {
          cod_apply(node->node.element_ref.array_ref, pre_func, post_func, list_func, data);
          cod_apply(node->node.element_ref.expression, pre_func, post_func, list_func, data);
          break;
      }
      case cod_cast: {
          cod_apply_list(node->node.cast.type_spec, pre_func, post_func, list_func, data);
          cod_apply(node->node.cast.expression, pre_func, post_func, list_func, data);
          break;
      }
      case cod_initializer_list: {
          cod_apply_list(node->node.initializer_list.initializers, pre_func, post_func, list_func, data);
          break;
      }
      case cod_initializer: {
          cod_apply_list(node->node.initializer.designation, pre_func, post_func, list_func, data);
          cod_apply(node->node.initializer.initializer, pre_func, post_func, list_func, data);
          break;
      }
      case cod_designator: {
          cod_apply(node->node.designator.expression, pre_func, post_func, list_func, data);
          break;
      }
      default: printf("Unhandled case in cod_apply\n");
    }
    node->visited--;
    if(post_func) (post_func)(node, data);
}

static void cod_print_sm_list(sm_list list)
{
    while (list != NULL) {
        printf(" %p", list->node);
        list = list->next;
    }
}

extern void cod_print(sm_ref node)
{
    switch(node->node_type) {
      case cod_compound_statement: {
          printf("0x%p  --  compound_statement ->\n", node);
          printf("	decls : ");
          cod_print_sm_list(node->node.compound_statement.decls);
          printf("\n");
          printf("	statements : ");
          cod_print_sm_list(node->node.compound_statement.statements);
          printf("\n");
          break;
      }
      case cod_enumerator: {
          printf("0x%p  --  enumerator ->\n", node);
          printf("	id : %s\n", (node->node.enumerator.id == NULL) ? "<NULL>" : node->node.enumerator.id);
          printf("	const_expression : %p\n", node->node.enumerator.const_expression);
          printf("	enum_value : %d\n", node->node.enumerator.enum_value);
          break;
      }
      case cod_declaration: {
          printf("0x%p  --  declaration ->\n", node);
          printf("	type_spec : ");
          cod_print_sm_list(node->node.declaration.type_spec);
          printf("\n");
          printf("	sm_complex_type : %p\n", node->node.declaration.sm_complex_type);
          printf("	freeable_complex_type : %p\n", node->node.declaration.freeable_complex_type);
          printf("	static_var : %d\n", node->node.declaration.static_var);
          printf("	const_var : %d\n", node->node.declaration.const_var);
          printf("	param_num : %d\n", node->node.declaration.param_num);
          printf("	id : %s\n", (node->node.declaration.id == NULL) ? "<NULL>" : node->node.declaration.id);
          printf("	init_value : %p\n", node->node.declaration.init_value);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.declaration.lx_srcpos);
          printf("\n");
          printf("	is_subroutine : %d\n", node->node.declaration.is_subroutine);
          printf("	varidiac_subroutine_param_count : %d\n", node->node.declaration.varidiac_subroutine_param_count);
          printf("	is_typedef : %d\n", node->node.declaration.is_typedef);
          printf("	addr_taken : %d\n", node->node.declaration.addr_taken);
          printf("	is_extern : %d\n", node->node.declaration.is_extern);
          printf("	params : ");
          cod_print_sm_list(node->node.declaration.params);
          printf("\n");
          printf("	cg_oprnd : %d\n", node->node.declaration.cg_oprnd);
          printf("	cg_type : %d\n", node->node.declaration.cg_type);
          printf("	closure_id : %p\n", node->node.declaration.closure_id);
          printf("	cg_address : %p\n", node->node.declaration.cg_address);
          break;
      }
      case cod_label_statement: {
          printf("0x%p  --  label_statement ->\n", node);
          printf("	name : %s\n", (node->node.label_statement.name == NULL) ? "<NULL>" : node->node.label_statement.name);
          printf("	cg_label : %d\n", node->node.label_statement.cg_label);
          printf("	statement : %p\n", node->node.label_statement.statement);
          break;
      }
      case cod_return_statement: {
          printf("0x%p  --  return_statement ->\n", node);
          printf("	expression : %p\n", node->node.return_statement.expression);
          printf("	cg_func_type : %d\n", node->node.return_statement.cg_func_type);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.return_statement.lx_srcpos);
          printf("\n");
          break;
      }
      case cod_jump_statement: {
          printf("0x%p  --  jump_statement ->\n", node);
          printf("	continue_flag : %d\n", node->node.jump_statement.continue_flag);
          printf("	goto_target : %s\n", (node->node.jump_statement.goto_target == NULL) ? "<NULL>" : node->node.jump_statement.goto_target);
          printf("	sm_target_stmt : %p\n", node->node.jump_statement.sm_target_stmt);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.jump_statement.lx_srcpos);
          printf("\n");
          break;
      }
      case cod_selection_statement: {
          printf("0x%p  --  selection_statement ->\n", node);
          printf("	conditional : %p\n", node->node.selection_statement.conditional);
          printf("	then_part : %p\n", node->node.selection_statement.then_part);
          printf("	else_part : %p\n", node->node.selection_statement.else_part);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.selection_statement.lx_srcpos);
          printf("\n");
          break;
      }
      case cod_iteration_statement: {
          printf("0x%p  --  iteration_statement ->\n", node);
          printf("	init_expr : %p\n", node->node.iteration_statement.init_expr);
          printf("	test_expr : %p\n", node->node.iteration_statement.test_expr);
          printf("	post_test_expr : %p\n", node->node.iteration_statement.post_test_expr);
          printf("	iter_expr : %p\n", node->node.iteration_statement.iter_expr);
          printf("	statement : %p\n", node->node.iteration_statement.statement);
          printf("	cg_iter_label : %d\n", node->node.iteration_statement.cg_iter_label);
          printf("	cg_end_label : %d\n", node->node.iteration_statement.cg_end_label);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.iteration_statement.lx_srcpos);
          printf("\n");
          break;
      }
      case cod_expression_statement: {
          printf("0x%p  --  expression_statement ->\n", node);
          printf("	expression : %p\n", node->node.expression_statement.expression);
          break;
      }
      case cod_assignment_expression: {
          printf("0x%p  --  assignment_expression ->\n", node);
          printf("	left : %p\n", node->node.assignment_expression.left);
          printf("	right : %p\n", node->node.assignment_expression.right);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.assignment_expression.lx_srcpos);
          printf("\n");
          printf("	op : ");
          cod_print_operator_t(node->node.assignment_expression.op);
          printf("\n");
          printf("	cg_type : %d\n", node->node.assignment_expression.cg_type);
          break;
      }
      case cod_comma_expression: {
          printf("0x%p  --  comma_expression ->\n", node);
          printf("	left : %p\n", node->node.comma_expression.left);
          printf("	right : %p\n", node->node.comma_expression.right);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.comma_expression.lx_srcpos);
          printf("\n");
          break;
      }
      case cod_operator: {
          printf("0x%p  --  operator ->\n", node);
          printf("	op : ");
          cod_print_operator_t(node->node.operator.op);
          printf("\n");
          printf("	left : %p\n", node->node.operator.left);
          printf("	right : %p\n", node->node.operator.right);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.operator.lx_srcpos);
          printf("\n");
          printf("	operation_type : %d\n", node->node.operator.operation_type);
          printf("	result_type : %d\n", node->node.operator.result_type);
          break;
      }
      case cod_conditional_operator: {
          printf("0x%p  --  conditional_operator ->\n", node);
          printf("	condition : %p\n", node->node.conditional_operator.condition);
          printf("	e1 : %p\n", node->node.conditional_operator.e1);
          printf("	e2 : %p\n", node->node.conditional_operator.e2);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.conditional_operator.lx_srcpos);
          printf("\n");
          printf("	result_type : %d\n", node->node.conditional_operator.result_type);
          break;
      }
      case cod_identifier: {
          printf("0x%p  --  identifier ->\n", node);
          printf("	id : %s\n", (node->node.identifier.id == NULL) ? "<NULL>" : node->node.identifier.id);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.identifier.lx_srcpos);
          printf("\n");
          printf("	sm_declaration : %p\n", node->node.identifier.sm_declaration);
          printf("	cg_type : %d\n", node->node.identifier.cg_type);
          break;
      }
      case cod_constant: {
          printf("0x%p  --  constant ->\n", node);
          printf("	token : %d\n", node->node.constant.token);
          printf("	const_val : %s\n", (node->node.constant.const_val == NULL) ? "<NULL>" : node->node.constant.const_val);
          printf("	freeable_name : %s\n", (node->node.constant.freeable_name == NULL) ? "<NULL>" : node->node.constant.freeable_name);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.constant.lx_srcpos);
          printf("\n");
          break;
      }
      case cod_type_specifier: {
          printf("0x%p  --  type_specifier ->\n", node);
          printf("	token : %d\n", node->node.type_specifier.token);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.type_specifier.lx_srcpos);
          printf("\n");
          printf("	created_type_decl : %p\n", node->node.type_specifier.created_type_decl);
          break;
      }
      case cod_struct_type_decl: {
          printf("0x%p  --  struct_type_decl ->\n", node);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.struct_type_decl.lx_srcpos);
          printf("\n");
          printf("	id : %s\n", (node->node.struct_type_decl.id == NULL) ? "<NULL>" : node->node.struct_type_decl.id);
          printf("	fields : ");
          cod_print_sm_list(node->node.struct_type_decl.fields);
          printf("\n");
          printf("	cg_size : %d\n", node->node.struct_type_decl.cg_size);
          printf("	encode_info : ");
          cod_print_enc_info(node->node.struct_type_decl.encode_info);
          printf("\n");
          break;
      }
      case cod_enum_type_decl: {
          printf("0x%p  --  enum_type_decl ->\n", node);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.enum_type_decl.lx_srcpos);
          printf("\n");
          printf("	id : %s\n", (node->node.enum_type_decl.id == NULL) ? "<NULL>" : node->node.enum_type_decl.id);
          printf("	enums : ");
          cod_print_sm_list(node->node.enum_type_decl.enums);
          printf("\n");
          break;
      }
      case cod_array_type_decl: {
          printf("0x%p  --  array_type_decl ->\n", node);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.array_type_decl.lx_srcpos);
          printf("\n");
          printf("	size_expr : %p\n", node->node.array_type_decl.size_expr);
          printf("	element_ref : %p\n", node->node.array_type_decl.element_ref);
          printf("	static_var : %d\n", node->node.array_type_decl.static_var);
          printf("	type_spec : ");
          cod_print_sm_list(node->node.array_type_decl.type_spec);
          printf("\n");
          printf("	dimensions : ");
          cod_print_dimen_p(node->node.array_type_decl.dimensions);
          printf("\n");
          printf("	cg_static_size : %d\n", node->node.array_type_decl.cg_static_size);
          printf("	sm_dynamic_size : %p\n", node->node.array_type_decl.sm_dynamic_size);
          printf("	cg_element_type : %d\n", node->node.array_type_decl.cg_element_type);
          printf("	sm_complex_element_type : %p\n", node->node.array_type_decl.sm_complex_element_type);
          printf("	freeable_complex_element_type : %p\n", node->node.array_type_decl.freeable_complex_element_type);
          printf("	cg_element_size : %d\n", node->node.array_type_decl.cg_element_size);
          break;
      }
      case cod_reference_type_decl: {
          printf("0x%p  --  reference_type_decl ->\n", node);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.reference_type_decl.lx_srcpos);
          printf("\n");
          printf("	name : %s\n", (node->node.reference_type_decl.name == NULL) ? "<NULL>" : node->node.reference_type_decl.name);
          printf("	type_spec : ");
          cod_print_sm_list(node->node.reference_type_decl.type_spec);
          printf("\n");
          printf("	cg_referenced_type : %d\n", node->node.reference_type_decl.cg_referenced_type);
          printf("	sm_complex_referenced_type : %p\n", node->node.reference_type_decl.sm_complex_referenced_type);
          printf("	freeable_complex_referenced_type : %p\n", node->node.reference_type_decl.freeable_complex_referenced_type);
          printf("	cg_referenced_size : %d\n", node->node.reference_type_decl.cg_referenced_size);
          printf("	kernel_ref : %d\n", node->node.reference_type_decl.kernel_ref);
          break;
      }
      case cod_field: {
          printf("0x%p  --  field ->\n", node);
          printf("	name : %s\n", (node->node.field.name == NULL) ? "<NULL>" : node->node.field.name);
          printf("	string_type : %s\n", (node->node.field.string_type == NULL) ? "<NULL>" : node->node.field.string_type);
          printf("	type_spec : ");
          cod_print_sm_list(node->node.field.type_spec);
          printf("\n");
          printf("	sm_complex_type : %p\n", node->node.field.sm_complex_type);
          printf("	freeable_complex_type : %p\n", node->node.field.freeable_complex_type);
          printf("	cg_size : %d\n", node->node.field.cg_size);
          printf("	cg_offset : %d\n", node->node.field.cg_offset);
          printf("	cg_type : %d\n", node->node.field.cg_type);
          break;
      }
      case cod_field_ref: {
          printf("0x%p  --  field_ref ->\n", node);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.field_ref.lx_srcpos);
          printf("\n");
          printf("	struct_ref : %p\n", node->node.field_ref.struct_ref);
          printf("	lx_field : %s\n", (node->node.field_ref.lx_field == NULL) ? "<NULL>" : node->node.field_ref.lx_field);
          printf("	sm_field_ref : %p\n", node->node.field_ref.sm_field_ref);
          break;
      }
      case cod_subroutine_call: {
          printf("0x%p  --  subroutine_call ->\n", node);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.subroutine_call.lx_srcpos);
          printf("\n");
          printf("	sm_func_ref : %p\n", node->node.subroutine_call.sm_func_ref);
          printf("	arguments : ");
          cod_print_sm_list(node->node.subroutine_call.arguments);
          printf("\n");
          break;
      }
      case cod_element_ref: {
          printf("0x%p  --  element_ref ->\n", node);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.element_ref.lx_srcpos);
          printf("\n");
          printf("	array_ref : %p\n", node->node.element_ref.array_ref);
          printf("	sm_complex_element_type : %p\n", node->node.element_ref.sm_complex_element_type);
          printf("	sm_containing_structure_ref : %p\n", node->node.element_ref.sm_containing_structure_ref);
          printf("	cg_element_type : %d\n", node->node.element_ref.cg_element_type);
          printf("	this_index_dimension : %d\n", node->node.element_ref.this_index_dimension);
          printf("	expression : %p\n", node->node.element_ref.expression);
          break;
      }
      case cod_cast: {
          printf("0x%p  --  cast ->\n", node);
          printf("	lx_srcpos : ");
          cod_print_srcpos(node->node.cast.lx_srcpos);
          printf("\n");
          printf("	type_spec : ");
          cod_print_sm_list(node->node.cast.type_spec);
          printf("\n");
          printf("	cg_type : %d\n", node->node.cast.cg_type);
          printf("	expression : %p\n", node->node.cast.expression);
          printf("	sm_complex_type : %p\n", node->node.cast.sm_complex_type);
          break;
      }
      case cod_initializer_list: {
          printf("0x%p  --  initializer_list ->\n", node);
          printf("	initializers : ");
          cod_print_sm_list(node->node.initializer_list.initializers);
          printf("\n");
          break;
      }
      case cod_initializer: {
          printf("0x%p  --  initializer ->\n", node);
          printf("	designation : ");
          cod_print_sm_list(node->node.initializer.designation);
          printf("\n");
          printf("	initializer : %p\n", node->node.initializer.initializer);
          break;
      }
      case cod_designator: {
          printf("0x%p  --  designator ->\n", node);
          printf("	expression : %p\n", node->node.designator.expression);
          printf("	id : %s\n", (node->node.designator.id == NULL) ? "<NULL>" : node->node.designator.id);
          break;
      }
      default: printf("Unhandled case in cod_print\n");
    }
    printf("\n");
}

extern void cod_free_list(sm_list list, void *junk)
{
    while (list != NULL) {
        sm_list next = list->next;
        free(list);
        list = next;
    }
}

extern void cod_free(sm_ref node)
{
    switch(node->node_type) {
      case cod_compound_statement: {
          break;
      }
      case cod_enumerator: {
	    free(node->node.enumerator.id);
          break;
      }
      case cod_declaration: {
	    free(node->node.declaration.id);
          break;
      }
      case cod_label_statement: {
	    free(node->node.label_statement.name);
          break;
      }
      case cod_return_statement: {
          break;
      }
      case cod_jump_statement: {
	    free(node->node.jump_statement.goto_target);
          break;
      }
      case cod_selection_statement: {
          break;
      }
      case cod_iteration_statement: {
          break;
      }
      case cod_expression_statement: {
          break;
      }
      case cod_assignment_expression: {
          break;
      }
      case cod_comma_expression: {
          break;
      }
      case cod_operator: {
          break;
      }
      case cod_conditional_operator: {
          break;
      }
      case cod_identifier: {
	    free(node->node.identifier.id);
          break;
      }
      case cod_constant: {
	    free(node->node.constant.const_val);
	    free(node->node.constant.freeable_name);
          break;
      }
      case cod_type_specifier: {
          break;
      }
      case cod_struct_type_decl: {
	    free(node->node.struct_type_decl.id);
	    free_enc_info(node->node.struct_type_decl.encode_info);
          break;
      }
      case cod_enum_type_decl: {
	    free(node->node.enum_type_decl.id);
          break;
      }
      case cod_array_type_decl: {
	    free(node->node.array_type_decl.dimensions);
          break;
      }
      case cod_reference_type_decl: {
	    free(node->node.reference_type_decl.name);
          break;
      }
      case cod_field: {
	    free(node->node.field.name);
	    free(node->node.field.string_type);
          break;
      }
      case cod_field_ref: {
	    free(node->node.field_ref.lx_field);
          break;
      }
      case cod_subroutine_call: {
          break;
      }
      case cod_element_ref: {
          break;
      }
      case cod_cast: {
          break;
      }
      case cod_initializer_list: {
          break;
      }
      case cod_initializer: {
          break;
      }
      case cod_designator: {
	    free(node->node.designator.id);
          break;
      }
      default: printf("Unhandled case in cod_free\n");
    }
    free(node);
}

static sm_list free_list = NULL;
extern void cod_make_free(sm_ref node, void *junk)
{
    sm_list new_free = malloc(sizeof(*new_free));
    new_free->next = free_list;
    new_free->node = node;
    free_list = new_free;
    switch(node->node_type) {
      case cod_compound_statement: {
          node->node.compound_statement.decls = NULL;
          node->node.compound_statement.statements = NULL;
          break;
      }
      case cod_enumerator: {
          break;
      }
      case cod_declaration: {
          node->node.declaration.type_spec = NULL;
          node->node.declaration.params = NULL;
          break;
      }
      case cod_label_statement: {
          break;
      }
      case cod_return_statement: {
          break;
      }
      case cod_jump_statement: {
          break;
      }
      case cod_selection_statement: {
          break;
      }
      case cod_iteration_statement: {
          break;
      }
      case cod_expression_statement: {
          break;
      }
      case cod_assignment_expression: {
          break;
      }
      case cod_comma_expression: {
          break;
      }
      case cod_operator: {
          break;
      }
      case cod_conditional_operator: {
          break;
      }
      case cod_identifier: {
          break;
      }
      case cod_constant: {
          break;
      }
      case cod_type_specifier: {
          break;
      }
      case cod_struct_type_decl: {
          node->node.struct_type_decl.fields = NULL;
          break;
      }
      case cod_enum_type_decl: {
          node->node.enum_type_decl.enums = NULL;
          break;
      }
      case cod_array_type_decl: {
          node->node.array_type_decl.type_spec = NULL;
          break;
      }
      case cod_reference_type_decl: {
          node->node.reference_type_decl.type_spec = NULL;
          break;
      }
      case cod_field: {
          node->node.field.type_spec = NULL;
          break;
      }
      case cod_field_ref: {
          break;
      }
      case cod_subroutine_call: {
          node->node.subroutine_call.arguments = NULL;
          break;
      }
      case cod_element_ref: {
          break;
      }
      case cod_cast: {
          node->node.cast.type_spec = NULL;
          break;
      }
      case cod_initializer_list: {
          node->node.initializer_list.initializers = NULL;
          break;
      }
      case cod_initializer: {
          node->node.initializer.designation = NULL;
          break;
      }
      case cod_designator: {
          break;
      }
      default: printf("Unhandled case in cod_make_free\n");
    }
}

extern void cod_rfree(sm_ref node) {
    free_list = NULL;
    cod_apply(node, NULL, cod_make_free, cod_free_list, NULL);
    while(free_list != NULL) {
        sm_list next = free_list->next;
        cod_free(free_list->node);
        free(free_list);
        free_list = next;
    }
}
extern void cod_rfree_list(sm_list list, void *junk) {
    free_list = NULL;
    cod_apply_list(list, NULL, cod_make_free, cod_free_list, junk);
    while(free_list != NULL) {
        sm_list next = free_list->next;
        cod_free(free_list->node);
        free(free_list);
        free_list = next;
    }
}
extern sm_list cod_copy_list(sm_list list)
{
    sm_list new_list = NULL;
    if (list != NULL) {
        new_list = malloc(sizeof(*new_list));
        new_list->node = cod_copy(list->node);
        new_list->next = cod_copy_list(list->next);
    }
    return new_list;
}

extern sm_list cod_append_list(sm_list list, sm_list list2)
{
    sm_list tmp = list;
    if (tmp == NULL) {
        return list2;
    }
    while (tmp->next != NULL) {
        tmp = tmp->next;
    }
    tmp->next = list2;
    return list;
}

extern sm_ref cod_copy(sm_ref node)
{
    sm_ref new_node = NULL;
    if (node == NULL) return NULL;

    switch(node->node_type) {
      case cod_compound_statement: {
	    new_node = cod_new_compound_statement();
	    new_node->node.compound_statement = node->node.compound_statement;
	    new_node->node.compound_statement.decls = cod_copy_list(node->node.compound_statement.decls);
	    new_node->node.compound_statement.statements = cod_copy_list(node->node.compound_statement.statements);
          break;
      }
      case cod_enumerator: {
	    new_node = cod_new_enumerator();
	    new_node->node.enumerator = node->node.enumerator;
	    new_node->node.enumerator.id = node->node.enumerator.id? strdup(node->node.enumerator.id):NULL;
	    new_node->node.enumerator.const_expression = cod_copy(node->node.enumerator.const_expression);
          break;
      }
      case cod_declaration: {
	    new_node = cod_new_declaration();
	    new_node->node.declaration = node->node.declaration;
	    new_node->node.declaration.type_spec = cod_copy_list(node->node.declaration.type_spec);
	    new_node->node.declaration.freeable_complex_type = cod_copy(node->node.declaration.freeable_complex_type);
	    new_node->node.declaration.id = node->node.declaration.id? strdup(node->node.declaration.id):NULL;
	    new_node->node.declaration.init_value = cod_copy(node->node.declaration.init_value);
	    new_node->node.declaration.params = cod_copy_list(node->node.declaration.params);
          break;
      }
      case cod_label_statement: {
	    new_node = cod_new_label_statement();
	    new_node->node.label_statement = node->node.label_statement;
	    new_node->node.label_statement.name = node->node.label_statement.name? strdup(node->node.label_statement.name):NULL;
	    new_node->node.label_statement.statement = cod_copy(node->node.label_statement.statement);
          break;
      }
      case cod_return_statement: {
	    new_node = cod_new_return_statement();
	    new_node->node.return_statement = node->node.return_statement;
	    new_node->node.return_statement.expression = cod_copy(node->node.return_statement.expression);
          break;
      }
      case cod_jump_statement: {
	    new_node = cod_new_jump_statement();
	    new_node->node.jump_statement = node->node.jump_statement;
	    new_node->node.jump_statement.goto_target = node->node.jump_statement.goto_target? strdup(node->node.jump_statement.goto_target):NULL;
          break;
      }
      case cod_selection_statement: {
	    new_node = cod_new_selection_statement();
	    new_node->node.selection_statement = node->node.selection_statement;
	    new_node->node.selection_statement.conditional = cod_copy(node->node.selection_statement.conditional);
	    new_node->node.selection_statement.then_part = cod_copy(node->node.selection_statement.then_part);
	    new_node->node.selection_statement.else_part = cod_copy(node->node.selection_statement.else_part);
          break;
      }
      case cod_iteration_statement: {
	    new_node = cod_new_iteration_statement();
	    new_node->node.iteration_statement = node->node.iteration_statement;
	    new_node->node.iteration_statement.init_expr = cod_copy(node->node.iteration_statement.init_expr);
	    new_node->node.iteration_statement.test_expr = cod_copy(node->node.iteration_statement.test_expr);
	    new_node->node.iteration_statement.post_test_expr = cod_copy(node->node.iteration_statement.post_test_expr);
	    new_node->node.iteration_statement.iter_expr = cod_copy(node->node.iteration_statement.iter_expr);
	    new_node->node.iteration_statement.statement = cod_copy(node->node.iteration_statement.statement);
          break;
      }
      case cod_expression_statement: {
	    new_node = cod_new_expression_statement();
	    new_node->node.expression_statement = node->node.expression_statement;
	    new_node->node.expression_statement.expression = cod_copy(node->node.expression_statement.expression);
          break;
      }
      case cod_assignment_expression: {
	    new_node = cod_new_assignment_expression();
	    new_node->node.assignment_expression = node->node.assignment_expression;
	    new_node->node.assignment_expression.left = cod_copy(node->node.assignment_expression.left);
	    new_node->node.assignment_expression.right = cod_copy(node->node.assignment_expression.right);
          break;
      }
      case cod_comma_expression: {
	    new_node = cod_new_comma_expression();
	    new_node->node.comma_expression = node->node.comma_expression;
	    new_node->node.comma_expression.left = cod_copy(node->node.comma_expression.left);
	    new_node->node.comma_expression.right = cod_copy(node->node.comma_expression.right);
          break;
      }
      case cod_operator: {
	    new_node = cod_new_operator();
	    new_node->node.operator = node->node.operator;
	    new_node->node.operator.left = cod_copy(node->node.operator.left);
	    new_node->node.operator.right = cod_copy(node->node.operator.right);
          break;
      }
      case cod_conditional_operator: {
	    new_node = cod_new_conditional_operator();
	    new_node->node.conditional_operator = node->node.conditional_operator;
	    new_node->node.conditional_operator.condition = cod_copy(node->node.conditional_operator.condition);
	    new_node->node.conditional_operator.e1 = cod_copy(node->node.conditional_operator.e1);
	    new_node->node.conditional_operator.e2 = cod_copy(node->node.conditional_operator.e2);
          break;
      }
      case cod_identifier: {
	    new_node = cod_new_identifier();
	    new_node->node.identifier = node->node.identifier;
	    new_node->node.identifier.id = node->node.identifier.id? strdup(node->node.identifier.id):NULL;
          break;
      }
      case cod_constant: {
	    new_node = cod_new_constant();
	    new_node->node.constant = node->node.constant;
	    new_node->node.constant.const_val = node->node.constant.const_val? strdup(node->node.constant.const_val):NULL;
	    new_node->node.constant.freeable_name = node->node.constant.freeable_name? strdup(node->node.constant.freeable_name):NULL;
          break;
      }
      case cod_type_specifier: {
	    new_node = cod_new_type_specifier();
	    new_node->node.type_specifier = node->node.type_specifier;
	    new_node->node.type_specifier.created_type_decl = cod_copy(node->node.type_specifier.created_type_decl);
          break;
      }
      case cod_struct_type_decl: {
	    new_node = cod_new_struct_type_decl();
	    new_node->node.struct_type_decl = node->node.struct_type_decl;
	    new_node->node.struct_type_decl.id = node->node.struct_type_decl.id? strdup(node->node.struct_type_decl.id):NULL;
	    new_node->node.struct_type_decl.fields = cod_copy_list(node->node.struct_type_decl.fields);
          break;
      }
      case cod_enum_type_decl: {
	    new_node = cod_new_enum_type_decl();
	    new_node->node.enum_type_decl = node->node.enum_type_decl;
	    new_node->node.enum_type_decl.id = node->node.enum_type_decl.id? strdup(node->node.enum_type_decl.id):NULL;
	    new_node->node.enum_type_decl.enums = cod_copy_list(node->node.enum_type_decl.enums);
          break;
      }
      case cod_array_type_decl: {
	    new_node = cod_new_array_type_decl();
	    new_node->node.array_type_decl = node->node.array_type_decl;
	    new_node->node.array_type_decl.size_expr = cod_copy(node->node.array_type_decl.size_expr);
	    new_node->node.array_type_decl.element_ref = cod_copy(node->node.array_type_decl.element_ref);
	    new_node->node.array_type_decl.type_spec = cod_copy_list(node->node.array_type_decl.type_spec);
	    new_node->node.array_type_decl.freeable_complex_element_type = cod_copy(node->node.array_type_decl.freeable_complex_element_type);
          break;
      }
      case cod_reference_type_decl: {
	    new_node = cod_new_reference_type_decl();
	    new_node->node.reference_type_decl = node->node.reference_type_decl;
	    new_node->node.reference_type_decl.name = node->node.reference_type_decl.name? strdup(node->node.reference_type_decl.name):NULL;
	    new_node->node.reference_type_decl.type_spec = cod_copy_list(node->node.reference_type_decl.type_spec);
	    new_node->node.reference_type_decl.freeable_complex_referenced_type = cod_copy(node->node.reference_type_decl.freeable_complex_referenced_type);
          break;
      }
      case cod_field: {
	    new_node = cod_new_field();
	    new_node->node.field = node->node.field;
	    new_node->node.field.name = node->node.field.name? strdup(node->node.field.name):NULL;
	    new_node->node.field.string_type = node->node.field.string_type? strdup(node->node.field.string_type):NULL;
	    new_node->node.field.type_spec = cod_copy_list(node->node.field.type_spec);
	    new_node->node.field.freeable_complex_type = cod_copy(node->node.field.freeable_complex_type);
          break;
      }
      case cod_field_ref: {
	    new_node = cod_new_field_ref();
	    new_node->node.field_ref = node->node.field_ref;
	    new_node->node.field_ref.struct_ref = cod_copy(node->node.field_ref.struct_ref);
	    new_node->node.field_ref.lx_field = node->node.field_ref.lx_field? strdup(node->node.field_ref.lx_field):NULL;
          break;
      }
      case cod_subroutine_call: {
	    new_node = cod_new_subroutine_call();
	    new_node->node.subroutine_call = node->node.subroutine_call;
	    new_node->node.subroutine_call.arguments = cod_copy_list(node->node.subroutine_call.arguments);
          break;
      }
      case cod_element_ref: {
	    new_node = cod_new_element_ref();
	    new_node->node.element_ref = node->node.element_ref;
	    new_node->node.element_ref.array_ref = cod_copy(node->node.element_ref.array_ref);
	    new_node->node.element_ref.expression = cod_copy(node->node.element_ref.expression);
          break;
      }
      case cod_cast: {
	    new_node = cod_new_cast();
	    new_node->node.cast = node->node.cast;
	    new_node->node.cast.type_spec = cod_copy_list(node->node.cast.type_spec);
	    new_node->node.cast.expression = cod_copy(node->node.cast.expression);
          break;
      }
      case cod_initializer_list: {
	    new_node = cod_new_initializer_list();
	    new_node->node.initializer_list = node->node.initializer_list;
	    new_node->node.initializer_list.initializers = cod_copy_list(node->node.initializer_list.initializers);
          break;
      }
      case cod_initializer: {
	    new_node = cod_new_initializer();
	    new_node->node.initializer = node->node.initializer;
	    new_node->node.initializer.designation = cod_copy_list(node->node.initializer.designation);
	    new_node->node.initializer.initializer = cod_copy(node->node.initializer.initializer);
          break;
      }
      case cod_designator: {
	    new_node = cod_new_designator();
	    new_node->node.designator = node->node.designator;
	    new_node->node.designator.expression = cod_copy(node->node.designator.expression);
	    new_node->node.designator.id = node->node.designator.id? strdup(node->node.designator.id):NULL;
          break;
      }
      default: printf("Unhandled case in cod_copy\n");
    }
    return new_node;
}

extern srcpos cod_get_srcpos(sm_ref expr)
{
    switch(expr->node_type) {
      case cod_declaration: return expr->node.declaration.lx_srcpos;
      case cod_return_statement: return expr->node.return_statement.lx_srcpos;
      case cod_jump_statement: return expr->node.jump_statement.lx_srcpos;
      case cod_selection_statement: return expr->node.selection_statement.lx_srcpos;
      case cod_iteration_statement: return expr->node.iteration_statement.lx_srcpos;
      case cod_assignment_expression: return expr->node.assignment_expression.lx_srcpos;
      case cod_comma_expression: return expr->node.comma_expression.lx_srcpos;
      case cod_operator: return expr->node.operator.lx_srcpos;
      case cod_conditional_operator: return expr->node.conditional_operator.lx_srcpos;
      case cod_identifier: return expr->node.identifier.lx_srcpos;
      case cod_constant: return expr->node.constant.lx_srcpos;
      case cod_type_specifier: return expr->node.type_specifier.lx_srcpos;
      case cod_struct_type_decl: return expr->node.struct_type_decl.lx_srcpos;
      case cod_enum_type_decl: return expr->node.enum_type_decl.lx_srcpos;
      case cod_array_type_decl: return expr->node.array_type_decl.lx_srcpos;
      case cod_reference_type_decl: return expr->node.reference_type_decl.lx_srcpos;
      case cod_field_ref: return expr->node.field_ref.lx_srcpos;
      case cod_subroutine_call: return expr->node.subroutine_call.lx_srcpos;
      case cod_element_ref: return expr->node.element_ref.lx_srcpos;
      case cod_cast: return expr->node.cast.lx_srcpos;
      default: {
          srcpos tmp;
          tmp.line = 0;
          tmp.character = 0;
          return tmp;
       };
    };
}
