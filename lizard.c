#include "lizard.h"
#include "env.h"
#include "lang.h"
#include "mem.h"
#include "primitives.h"
#include <ctype.h>
#include <ds.h>
#include <gmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

void print_ast(lizard_ast_node_t *node, int depth) {
  int i;
  list_node_t *expr, *arg, *param_node, *iter;
  if (!node) {
    return;
  }

  for (i = 0; i < depth; i++) {
    printf("  ");
  }
  switch (node->type) {
  case AST_STRING:
    printf("String: \"%s\"\n", node->data.string);
    break;
  case AST_NUMBER:
    gmp_printf("Number: %Zd\n", node->data.number);
    break;
  case AST_SYMBOL:
    printf("Symbol: %s\n", node->data.variable);
    break;
  case AST_BOOL:
    printf("Boolean: %s\n", node->data.boolean ? "#t" : "#f");
    break;
  case AST_NIL:
    printf("Nil\n");
    break;
  case AST_QUOTED:
    printf("Quote:\n");
    print_ast(node->data.quoted, depth + 1);
    break;
  case AST_ASSIGNMENT:
    printf("Assignment:\n");
    print_ast(node->data.assignment.variable, depth + 1);
    print_ast(node->data.assignment.value, depth + 1);
    break;
  case AST_DEFINITION:
    printf("Definition:\n");
    print_ast(node->data.definition.variable, depth + 1);
    print_ast(node->data.definition.value, depth + 1);
    break;
  case AST_IF:
    printf("If clause:\n");
    print_ast(node->data.if_clause.pred, depth + 1);
    print_ast(node->data.if_clause.cons, depth + 1);
    if (node->data.if_clause.alt)
      print_ast(node->data.if_clause.alt, depth + 1);
    break;

  case AST_LAMBDA:
    printf("Lambda parameters:\n");
    param_node = node->data.lambda.parameters->head;
    while (param_node != node->data.lambda.parameters->nil) {
      print_ast(&((lizard_ast_list_node_t *)param_node)->ast, depth + 1);
      param_node = param_node->next;
    }

    break;

  case AST_BEGIN:
    printf("Begin:\n");
    expr = node->data.begin_expressions->head;
    while (expr != node->data.begin_expressions->nil) {
      print_ast(&((lizard_ast_list_node_t *)expr)->ast, depth + 1);
      expr = expr->next;
    }
    break;
  case AST_APPLICATION:
    printf("Application:\n");
    arg = node->data.application_arguments->head;
    while (arg != node->data.application_arguments->nil) {
      print_ast(&((lizard_ast_list_node_t *)arg)->ast, depth + 1);
      arg = arg->next;
    }
    if (node->data.application_arguments->head ==
        node->data.application_arguments->nil) {
      for (i = 0; i < depth + 1; i++) {
        printf("  ");
      }
      printf("Nil\n");
    }
    break;
  case AST_MACRO:
    printf("Macro definition:\n");
    print_ast(node->data.macro_def.variable, depth + 1);
    print_ast(node->data.macro_def.transformer, depth + 1);
    break;
  case AST_ERROR:
    printf("Error (code %d):\n", node->data.error.code);
    for (iter = node->data.error.data->head; iter != node->data.error.data->nil;
         iter = iter->next) {
      print_ast(&((lizard_ast_list_node_t *)iter)->ast, depth + 1);
    }
    break;
  default:
    printf("Unknown AST node type.\n");
  }
}

lizard_ast_node_t *lizard_eval(lizard_ast_node_t *node, lizard_env_t *env,
                               lizard_heap_t *heap) {
  lizard_ast_node_t *closure, *result, *func, *lambda_node, *params_app, *var;
  lizard_ast_list_node_t *name_node, *params_wrapper, *body_wrapper;
  const char *fn_name, *macro_name;
  list_t *param_list, *evaled_args;
  list_node_t *p, *iter, *arg_node;
  if (!node) {
    return NULL;
  }
  node = lizard_expand_macros(node, env, heap);

  switch (node->type) {
  case AST_BOOL:
  case AST_NIL:
  case AST_NUMBER:
  case AST_STRING:
  case AST_PRIMITIVE:
    return node;
  case AST_SYMBOL: {
    lizard_ast_node_t *val = lizard_env_lookup(env, node->data.variable);
    if (!val) {
      return lizard_make_error(heap, LIZARD_ERROR_UNBOUND_SYMBOL);
    }
    return val;
  }
  case AST_QUOTED:
    return node->data.quoted;

  case AST_DEFINITION:
    var = node->data.definition.variable;
    if (var->type == AST_SYMBOL) {
      {
        lizard_ast_node_t *value =
            lizard_eval(node->data.definition.value, env, heap);
        lizard_env_define(heap, env, var->data.variable, value);
        return value;
      }
    } else if (var->type == AST_APPLICATION) {
      list_t *app_args = var->data.application_arguments;
      if (app_args->head == app_args->nil) {
        return lizard_make_error(heap, LIZARD_ERROR_INVALID_FUNCTION_DEF);
      }
      name_node = (lizard_ast_list_node_t *)app_args->head;
      if (name_node->ast.type != AST_SYMBOL) {
        return lizard_make_error(heap, LIZARD_ERROR_INVALID_FUNCTION_NAME);
      }
      fn_name = name_node->ast.data.variable;
      param_list = list_create();
      for (p = app_args->head->next; p != app_args->nil; p = p->next) {
        lizard_ast_list_node_t *param_node = (lizard_ast_list_node_t *)p;
        if (param_node->ast.type != AST_SYMBOL) {
          return lizard_make_error(heap, LIZARD_ERROR_INVALID_PARAMETER);
        }
        list_append(param_list, p);
      }

      lambda_node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
      lambda_node->type = AST_LAMBDA;
      lambda_node->data.lambda.closure_env = NULL;

      lambda_node->data.lambda.parameters = list_create();

      params_app = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
      params_app->type = AST_APPLICATION;
      params_app->data.application_arguments = param_list;
      params_wrapper = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
      params_wrapper->ast = *params_app;
      list_append(lambda_node->data.lambda.parameters, &params_wrapper->node);

      body_wrapper = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
      body_wrapper->ast = *node->data.definition.value;
      list_append(lambda_node->data.lambda.parameters, &body_wrapper->node);

      lizard_env_define(heap, env, fn_name, lambda_node);
      return lambda_node;
    } else {
      return lizard_make_error(heap, LIZARD_ERROR_INVALID_DEF);
    }

  case AST_ASSIGNMENT: {
    lizard_ast_node_t *var = node->data.assignment.variable;
    lizard_ast_node_t *value =
        lizard_eval(node->data.assignment.value, env, heap);
    if (var->type != AST_SYMBOL) {
      return lizard_make_error(heap, LIZARD_ERROR_ASSIGNMENT);
    }
    if (!lizard_env_set(env, var->data.variable, value)) {
      return lizard_make_error(heap, LIZARD_ERROR_ASSIGNMENT_UNBOUND);
    }
    return value;
  }

  case AST_IF: {
    lizard_ast_node_t *pred = lizard_eval(node->data.if_clause.pred, env, heap);
    bool is_true = false;

    if (pred->type == AST_BOOL) {
      is_true = pred->data.boolean;
    } else if (pred->type == AST_NIL) {
      is_true = false;
    } else {
      is_true = true;
    }

    if (is_true) {
      return lizard_eval(node->data.if_clause.cons, env, heap);
    } else {
      if (node->data.if_clause.alt)
        return lizard_eval(node->data.if_clause.alt, env, heap);
      else
        return lizard_make_nil(heap);
    }
  }

  case AST_BEGIN: {
    result = NULL;
    for (iter = node->data.begin_expressions->head;
         iter != node->data.begin_expressions->nil; iter = iter->next) {
      lizard_ast_list_node_t *expr_node = CAST(iter, lizard_ast_list_node_t);
      result = lizard_eval(&expr_node->ast, env, heap);
    }
    return result;
  }

  case AST_LAMBDA:
    closure = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
    closure->type = AST_LAMBDA;
    closure->data.lambda.parameters = node->data.lambda.parameters;
    closure->data.lambda.closure_env = env;
    return closure;

  case AST_APPLICATION: {
    list_node_t *func_node = node->data.application_arguments->head;
    func = lizard_eval(&((lizard_ast_list_node_t *)func_node)->ast, env, heap);

    evaled_args = list_create();
    for (arg_node = func_node->next;
         arg_node != node->data.application_arguments->nil;
         arg_node = arg_node->next) {
      lizard_ast_node_t *arg_val =
          lizard_eval(&((lizard_ast_list_node_t *)arg_node)->ast, env, heap);
      lizard_ast_list_node_t *new_arg_node =
          lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
      new_arg_node->ast = *arg_val;
      list_append(evaled_args, &new_arg_node->node);
    }
    return lizard_apply(func, evaled_args, env, heap);
  }
  case AST_MACRO: {
    lizard_ast_node_t *transformer =
        lizard_eval(node->data.macro_def.transformer, env, heap);
    if (node->data.macro_def.variable->type != AST_SYMBOL) {
      return lizard_make_error(heap, LIZARD_ERROR_INVALID_MACRO_NAME);
    }
    macro_name = node->data.macro_def.variable->data.variable;
    lizard_env_define(heap, env, macro_name, transformer);
    return lizard_make_nil(heap);
  }

  default:
    return lizard_make_error(heap, LIZARD_ERROR_NODE_TYPE);
  }
}

lizard_ast_node_t *lizard_apply(lizard_ast_node_t *func, list_t *args,
                                lizard_env_t *env, lizard_heap_t *heap) {

  list_t *formal_params;
  list_node_t *param_node, *arg_node, *body_node;
  lizard_ast_list_node_t *var_param;
  lizard_ast_node_t *rest_list, *result;
  bool variadic;
  const char *variadic_name;

  if (func->type == AST_PRIMITIVE) {
    return func->data.primitive(args, env, heap);
  } else if (func->type == AST_LAMBDA) {
    lizard_env_t *new_env =
        lizard_env_create(heap, func->data.lambda.closure_env);

    lizard_ast_node_t *param_list =
        &((lizard_ast_list_node_t *)func->data.lambda.parameters->head)->ast;
    if (param_list->type != AST_APPLICATION) {
      return lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMS);
    }

    formal_params = param_list->data.application_arguments;
    param_node = formal_params->head;
    arg_node = args->head;
    variadic = false;
    variadic_name = NULL;

    while (param_node != formal_params->nil) {
      lizard_ast_list_node_t *param = (lizard_ast_list_node_t *)param_node;
      if (param->ast.type == AST_SYMBOL &&
          (strcmp(param->ast.data.variable, ".") == 0 ||
           strcmp(param->ast.data.variable, "...") == 0)) {
        param_node = param_node->next;
        if (param_node == formal_params->nil) {
          return lizard_make_error(heap, LIZARD_ERROR_VARIADIC_UNFOLLOWED);
        }
        var_param = (lizard_ast_list_node_t *)param_node;
        if (var_param->ast.type != AST_SYMBOL) {
          return lizard_make_error(heap, LIZARD_ERROR_VARIADIC_SYMBOL);
        }
        variadic = true;
        variadic_name = var_param->ast.data.variable;
        param_node = param_node->next;
        break;
      }

      if (arg_node == args->nil) {
        return lizard_make_error(heap, LIZARD_ERROR_LAMBDA_ARITY_LESS);
      }

      if (param->ast.type != AST_SYMBOL) {
        return lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMETER);
      }
      lizard_env_define(heap, new_env, param->ast.data.variable,
                        &((lizard_ast_list_node_t *)arg_node)->ast);
      param_node = param_node->next;
      arg_node = arg_node->next;
    }

    if (!variadic && arg_node != args->nil) {
      return lizard_make_error(heap, LIZARD_ERROR_LAMBDA_ARITY_MORE);
    }

    if (variadic) {
      list_t temp_args;
      temp_args.head = arg_node;
      temp_args.nil = args->nil;
      rest_list = lizard_primitive_list(&temp_args, env, heap);
      lizard_env_define(heap, new_env, variadic_name, rest_list);
    }

    result = NULL;
    for (body_node = func->data.lambda.parameters->head->next;
         body_node != func->data.lambda.parameters->nil;
         body_node = body_node->next) {
      lizard_ast_list_node_t *body_expr = (lizard_ast_list_node_t *)body_node;
      result = lizard_eval(&body_expr->ast, new_env, heap);
    }

    return result;

  } else {
    return lizard_make_error(heap, LIZARD_ERROR_INVALID_APPLY);
  }
}

lizard_ast_node_t *lizard_expand_macros(lizard_ast_node_t *node,
                                        lizard_env_t *env,
                                        lizard_heap_t *heap) {
  list_node_t *first, *arg, *body, *expr;
  lizard_ast_node_t *operator, * binding, *expanded;
  lizard_ast_list_node_t *copy;
  list_t *macro_args;
  if (!node) {
    return NULL;
  }

  if (node->type == AST_APPLICATION) {
    first = node->data.application_arguments->head;
    if (first != node->data.application_arguments->nil) {
      operator= &((lizard_ast_list_node_t *)first)->ast;
      if (operator->type == AST_SYMBOL) {
        binding = lizard_env_lookup(env, operator->data.variable);
        if (binding && binding->type == AST_MACRO) {
          macro_args = list_create();
          for (arg = first->next; arg != node->data.application_arguments->nil;
               arg = arg->next) {
            copy = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
            copy->ast = ((lizard_ast_list_node_t *)arg)->ast; /* shallow copy */
            list_append(macro_args, &copy->node);
          }
          expanded = lizard_apply(binding, macro_args, env, heap);
          return lizard_expand_macros(expanded, env, heap);
        }
      }
    }
    for (arg = node->data.application_arguments->head;
         arg != node->data.application_arguments->nil; arg = arg->next) {
      lizard_ast_list_node_t *arg_node = (lizard_ast_list_node_t *)arg;
      arg_node->ast = *lizard_expand_macros(&arg_node->ast, env, heap);
    }
  }

  switch (node->type) {
  case AST_QUOTED:
    node->data.quoted = lizard_expand_macros(node->data.quoted, env, heap);
    break;
  case AST_ASSIGNMENT:
    node->data.assignment.variable =
        lizard_expand_macros(node->data.assignment.variable, env, heap);
    node->data.assignment.value =
        lizard_expand_macros(node->data.assignment.value, env, heap);
    break;
  case AST_DEFINITION:
    node->data.definition.variable =
        lizard_expand_macros(node->data.definition.variable, env, heap);
    node->data.definition.value =
        lizard_expand_macros(node->data.definition.value, env, heap);
    break;
  case AST_IF:
    node->data.if_clause.pred =
        lizard_expand_macros(node->data.if_clause.pred, env, heap);
    node->data.if_clause.cons =
        lizard_expand_macros(node->data.if_clause.cons, env, heap);
    if (node->data.if_clause.alt)
      node->data.if_clause.alt =
          lizard_expand_macros(node->data.if_clause.alt, env, heap);
    break;
  case AST_LAMBDA:
    for (body = node->data.lambda.parameters->head->next;
         body != node->data.lambda.parameters->nil; body = body->next) {
      lizard_ast_list_node_t *body_expr = (lizard_ast_list_node_t *)body;
      body_expr->ast = *lizard_expand_macros(&body_expr->ast, env, heap);
    }
    break;
  case AST_BEGIN:
    for (expr = node->data.begin_expressions->head;
         expr != node->data.begin_expressions->nil; expr = expr->next) {
      lizard_ast_list_node_t *expr_node = (lizard_ast_list_node_t *)expr;
      expr_node->ast = *lizard_expand_macros(&expr_node->ast, env, heap);
    }
    break;
  default:
    break;
  }
  return node;
}
