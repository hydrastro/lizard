#include "lizard.h"
#include "env.h"
#include "lang.h"
#include "mem.h"
#include "primitives.h"
#include <ctype.h>
#include <ds.h>
#include <gmp.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
jmp_buf callcc_buf;
int callcc_active = 0;
lizard_ast_node_t *callcc_value = NULL;

int lizard_continuation_jumped = 0;
lizard_ast_node_t *lizard_jump_value = NULL;

lizard_ast_node_t *lizard_convert_list_literal(lizard_ast_node_t *node,
                                               lizard_heap_t *heap) {
  ds_list_t *args, *rest_args;
  lizard_ast_list_node_t *first;
  lizard_ast_node_t *car, *rest_app, *cdr, *pair;
  if (node->type != AST_APPLICATION) {
    return node;
  }
  args = node->data.application_arguments;
  if (args->head == args->nil) {
    return lizard_make_nil(heap);
  }
  first = (lizard_ast_list_node_t *)args->head;
  car = first->ast;
  rest_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  rest_args->head = args->head->next;
  rest_args->nil = args->nil;
  rest_app = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  rest_app->type = AST_APPLICATION;
  rest_app->data.application_arguments = rest_args;
  cdr = lizard_convert_list_literal(rest_app, heap);
  pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  pair->type = AST_PAIR;
  pair->data.pair.car = lizard_ast_deep_copy(car, heap);
  pair->data.pair.cdr = cdr;
  return pair;
}

lizard_ast_node_t *lizard_make_promise(lizard_heap_t *heap,
                                       lizard_ast_node_t *expr,
                                       lizard_env_t *env) {
  lizard_ast_node_t *promise;
  promise = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  promise->type = AST_PROMISE;
  promise->data.promise.expr = expr;
  promise->data.promise.env = env;
  promise->data.promise.value = NULL;
  promise->data.promise.forced = false;
  return promise;
}

lizard_ast_node_t *lizard_force(lizard_ast_node_t *node, lizard_heap_t *heap) {
  if (node->type == AST_PROMISE) {
    if (!node->data.promise.forced) {
      node->data.promise.value =
          lizard_eval(node->data.promise.expr, node->data.promise.env, heap,
                      lizard_identity_cont);
      node->data.promise.forced = true;
    }
    return node->data.promise.value;
  }
  return node;
}

lizard_ast_node_t *lizard_primitive_delay(ds_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_INVALID_DELAY);
  }
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  return lizard_make_promise(heap, expr, env);
}

lizard_ast_node_t *lizard_primitive_force(ds_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *node;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_INVALID_FORCE);
  }
  node = ((lizard_ast_list_node_t *)args->head)->ast;
  return lizard_force(node, heap);
}

lizard_ast_node_t *lizard_eval(
    lizard_ast_node_t *node, lizard_env_t *env, lizard_heap_t *heap,
    lizard_ast_node_t *(*cont)(lizard_ast_node_t *result, lizard_env_t *env,
                               lizard_heap_t *heap)) {
  for (;;) {
    if (node->type == AST_PROMISE) {
      node = lizard_force(node, heap);
      continue;
    }
    switch (node->type) {
    case AST_BOOL:
    case AST_NIL:
    case AST_NUMBER:
    case AST_STRING:
    case AST_PAIR:
    case AST_PRIMITIVE:
      return cont(node, env, heap);

    case AST_SYMBOL: {
      lizard_ast_node_t *val;
      val = lizard_env_lookup(env, node->data.variable);
      if (!val) {
        return cont(lizard_make_error(heap, LIZARD_ERROR_UNBOUND_SYMBOL), env,
                    heap);
      }
      return cont(lizard_force(val, heap), env, heap);
    }

    case AST_QUOTE: {
      lizard_ast_node_t *quoted = node->data.quoted;
      if (quoted->type == AST_APPLICATION) {
        quoted = lizard_convert_list_literal(quoted, heap);
      }
      return cont(quoted, env, heap);
    }

    case AST_QUASIQUOTE: {
      lizard_ast_node_t *expanded;
      lizard_ast_node_t *quoted;
      expanded = lizard_expand_quasiquote(node->data.quoted, env, heap);
      quoted = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      quoted->type = AST_QUOTE;
      quoted->data.quoted = expanded;
      return cont(quoted, env, heap);
    }

    case AST_UNQUOTE:
      return cont(node->data.quoted, env, heap);

    case AST_DEFINITION: {
      lizard_ast_node_t *value;
      if (node->data.definition.variable->type != AST_SYMBOL) {
        return cont(lizard_make_error(heap, LIZARD_ERROR_INVALID_DEF), env,
                    heap);
      }
      value = lizard_eval(node->data.definition.value, env, heap,
                          lizard_identity_cont);
      lizard_env_define(heap, env,
                        node->data.definition.variable->data.variable, value);
      return cont(value, env, heap);
    }

    case AST_ASSIGNMENT: {
      lizard_ast_node_t *value;
      if (node->data.assignment.variable->type != AST_SYMBOL) {
        return cont(lizard_make_error(heap, LIZARD_ERROR_ASSIGNMENT), env,
                    heap);
      }
      value = lizard_eval(node->data.assignment.value, env, heap,
                          lizard_identity_cont);
      if (!lizard_env_set(env, node->data.assignment.variable->data.variable,
                          value)) {
        return cont(lizard_make_error(heap, LIZARD_ERROR_ASSIGNMENT_UNBOUND),
                    env, heap);
      }
      return cont(value, env, heap);
    }

    case AST_IF: {
      lizard_ast_node_t *pred_result;
      int is_true;
      pred_result = lizard_force(lizard_eval(node->data.if_clause.pred, env,
                                             heap, lizard_identity_cont),
                                 heap);
      if (pred_result->type == AST_BOOL) {
        is_true = pred_result->data.boolean;
      } else if (pred_result->type == AST_NIL) {
        is_true = 0;
      } else {
        is_true = 1;
      }
      if (is_true) {
        node = node->data.if_clause.cons;
      } else {
        node = (node->data.if_clause.alt ? node->data.if_clause.alt
                                         : lizard_make_nil(heap));
      }
      continue;
    }

    case AST_BEGIN: {
<<<<<<< HEAD
      ds_list_node_t *iter;
      ds_list_node_t *last = node->data.begin_expressions->nil;
      /* find the tail without evaluating */
      for (iter = node->data.begin_expressions->head;
           iter != node->data.begin_expressions->nil; iter = iter->next) {
        last = iter;
      }
      if (last == node->data.begin_expressions->nil) {
        node = lizard_make_nil(heap);
        continue;
      }
      /* evaluate every non-tail expression for its side effects only */
      for (iter = node->data.begin_expressions->head; iter != last;
           iter = iter->next) {
        lizard_eval(((lizard_ast_list_node_t *)iter)->ast, env, heap,
                    lizard_identity_cont);
      }
      /* trampoline the tail so it's in tail position */
      node = ((lizard_ast_list_node_t *)last)->ast;
      continue;
=======
      list_node_t *iter;
      lizard_ast_node_t *result;
      result = lizard_make_nil(heap);
      for (iter = node->data.begin_expressions->head;
           iter != node->data.begin_expressions->nil; iter = iter->next) {
        lizard_ast_list_node_t *expr;
        expr = (lizard_ast_list_node_t *)iter;
        result = lizard_eval(expr->ast, env, heap, lizard_identity_cont);
        if (result->type == AST_ERROR) {
          return cont(result, env, heap);
        }
      }
      return cont(lizard_force(result, heap), env, heap);
>>>>>>> refs/remotes/origin/master
    }

    case AST_LAMBDA: {
      lizard_ast_node_t *closure;
      closure = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      closure->type = AST_LAMBDA;
      closure->data.lambda.parameters = node->data.lambda.parameters;
      closure->data.lambda.closure_env = env;
      return cont(closure, env, heap);
    }

    case AST_COND: {
      /* (cond (test body...) ... (else body...))
         Each clause is itself an AST_APPLICATION whose head is the test
         and whose tail is the body to evaluate when the test holds.
         A clause whose head is the symbol `else` is unconditional. */
      ds_list_node_t *clause_iter;
      lizard_ast_node_t *chosen_body_seq = NULL;
      int matched = 0;
      for (clause_iter = node->data.cond_clauses->head;
           clause_iter != node->data.cond_clauses->nil;
           clause_iter = clause_iter->next) {
        lizard_ast_node_t *clause =
            ((lizard_ast_list_node_t *)clause_iter)->ast;
        ds_list_node_t *cl_head;
        lizard_ast_node_t *test_expr;
        lizard_ast_node_t *test_val;
        int is_else;
        if (clause->type != AST_APPLICATION ||
            clause->data.application_arguments->head ==
                clause->data.application_arguments->nil) {
          continue;
        }
        cl_head = clause->data.application_arguments->head;
        test_expr = ((lizard_ast_list_node_t *)cl_head)->ast;
        is_else = (test_expr->type == AST_SYMBOL &&
                   strcmp(test_expr->data.variable, "else") == 0);
        if (is_else) {
          test_val = lizard_make_bool(heap, true);
        } else {
          test_val = lizard_force(
              lizard_eval(test_expr, env, heap, lizard_identity_cont), heap);
        }
        if (lizard_is_true(test_val)) {
          /* Build a BEGIN over the rest of the clause's elements. */
          ds_list_t *body_list;
          ds_list_node_t *body_iter;
          lizard_ast_node_t *begin_node;
          body_list = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
          for (body_iter = cl_head->next;
               body_iter != clause->data.application_arguments->nil;
               body_iter = body_iter->next) {
            lizard_ast_list_node_t *wrap =
                lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
            wrap->ast = ((lizard_ast_list_node_t *)body_iter)->ast;
            list_append(body_list, &wrap->node);
          }
          if (body_list->head == body_list->nil) {
            /* (cond (test)) — return the test value itself, per R5RS. */
            return cont(test_val, env, heap);
          }
          begin_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
          begin_node->type = AST_BEGIN;
          begin_node->data.begin_expressions = body_list;
          chosen_body_seq = begin_node;
          matched = 1;
          break;
        }
      }
      if (!matched) {
        return cont(lizard_make_nil(heap), env, heap);
      }
      node = chosen_body_seq;
      continue;
    }

    case AST_APPLICATION: {
      ds_list_node_t *func_node;
      lizard_ast_node_t *func;
      ds_list_t *arg_list;
      ds_list_node_t *arg_node;
      func_node = node->data.application_arguments->head;
      if (func_node == node->data.application_arguments->nil) {
        return cont(lizard_make_error(heap, LIZARD_ERROR_INVALID_APPLY), env,
                    heap);
      }
      func =
          lizard_force(lizard_eval(((lizard_ast_list_node_t *)func_node)->ast,
                                   env, heap, lizard_identity_cont),
                       heap);
      /* If evaluating the operator already produced an error (e.g. unbound */
      /* symbol), surface that error instead of reporting "non-function". */
      if (func->type == AST_ERROR) {
        return cont(func, env, heap);
      }
      arg_list = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
      for (arg_node = func_node->next;
           arg_node != node->data.application_arguments->nil;
           arg_node = arg_node->next) {
        lizard_ast_node_t *arg_expr;
        lizard_ast_node_t *promise;
        lizard_ast_list_node_t *new_arg_node;
        arg_expr = ((lizard_ast_list_node_t *)arg_node)->ast;
        promise = lizard_make_promise(heap, arg_expr, env);
        new_arg_node = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
        new_arg_node->ast = promise;
        list_append(arg_list, &new_arg_node->node);
      }
      if (func->type == AST_CONTINUATION) {
<<<<<<< HEAD
        ds_list_node_t *arg;
=======
        list_node_t *arg;
>>>>>>> refs/remotes/origin/master
        lizard_ast_node_t *value;
        arg = arg_list->head;
        value = (arg != arg_list->nil)
                    ? lizard_force(((lizard_ast_list_node_t *)arg)->ast, heap)
                    : lizard_make_nil(heap);
        if (callcc_active) {
          callcc_value = value;
          longjmp(callcc_buf, 1);
        }
        return func->data.continuation.captured_cont(value, env, heap);
      }

      if (func->type == AST_PRIMITIVE) {
        {
          ds_list_node_t *cur;
          for (cur = arg_list->head; cur != arg_list->nil; cur = cur->next) {
            ((lizard_ast_list_node_t *)cur)->ast =
                lizard_force(((lizard_ast_list_node_t *)cur)->ast, heap);
          }
        }
        return cont(func->data.primitive(arg_list, env, heap), env, heap);
      } else if (func->type == AST_LAMBDA) {
        {
          lizard_env_t *new_env;
          lizard_ast_node_t *param_list;
          ds_list_t *formal_params;
          ds_list_node_t *param_node, *arg_iter;
          lizard_ast_list_node_t *param;
<<<<<<< HEAD
          ds_list_node_t *body_node;
=======
          list_node_t *body_node;
>>>>>>> refs/remotes/origin/master
          lizard_ast_node_t *result;
          new_env = lizard_env_create(heap, func->data.lambda.closure_env);
          param_list =
              ((lizard_ast_list_node_t *)func->data.lambda.parameters->head)
                  ->ast;
          if (param_list->type != AST_APPLICATION) {
            return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMS),
                        env, heap);
          }
          formal_params = param_list->data.application_arguments;
          param_node = formal_params->head;
          arg_iter = arg_list->head;
          while (param_node != formal_params->nil) {
            if (arg_iter == arg_list->nil) {
              return cont(
                  lizard_make_error(heap, LIZARD_ERROR_LAMBDA_ARITY_LESS), env,
                  heap);
            }
            param = (lizard_ast_list_node_t *)param_node;
            if (param->ast->type != AST_SYMBOL) {
              return cont(
                  lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMETER), env,
                  heap);
            }
            lizard_env_define(heap, new_env, param->ast->data.variable,
                              ((lizard_ast_list_node_t *)arg_iter)->ast);
            param_node = param_node->next;
            arg_iter = arg_iter->next;
          }
          if (arg_iter != arg_list->nil) {
            return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_ARITY_MORE),
                        env, heap);
          }
          result = NULL;
          for (body_node = func->data.lambda.parameters->head->next;
               body_node != func->data.lambda.parameters->nil;
               body_node = body_node->next) {
            lizard_ast_list_node_t *body_expr =
                (lizard_ast_list_node_t *)body_node;
            result = lizard_eval(body_expr->ast, new_env, heap, cont);
            if (lizard_continuation_jumped) {
              lizard_continuation_jumped = 0;
              return cont(lizard_force(lizard_jump_value, heap), env, heap);
            }
          }
          return cont(lizard_force(result, heap), env, heap);
        }
      } else if (func->type == AST_CALLCC) {
        return lizard_primitive_callcc(arg_list, env, heap, cont);
      } else {
        return cont(lizard_make_error(heap, LIZARD_ERROR_INVALID_APPLY), env,
                    heap);
      }
    }

    case AST_MACRO: {
      {
        lizard_ast_node_t *transformer;
        lizard_ast_node_t *macro_obj;
        transformer = lizard_eval(node->data.macro_def.transformer, env, heap,
                                  lizard_identity_cont);
        if (node->data.macro_def.variable->type != AST_SYMBOL) {
          return cont(lizard_make_error(heap, LIZARD_ERROR_INVALID_MACRO_NAME),
                      env, heap);
        }
        macro_obj = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        macro_obj->type = AST_MACRO;
        macro_obj->data.macro_def.variable = node->data.macro_def.variable;
        macro_obj->data.macro_def.transformer = transformer;
        lizard_env_define(heap, env,
                          node->data.macro_def.variable->data.variable,
                          macro_obj);
        return cont(lizard_make_nil(heap), env, heap);
      }
    }

    case AST_CALLCC: {
      {
        ds_list_node_t *func_node;
        ds_list_node_t *cur;
        ds_list_t *arg_list;
        ds_list_node_t *arg_node;
        func_node = node->data.application_arguments->head;
        arg_list = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
        for (arg_node = func_node->next;
             arg_node != node->data.application_arguments->nil;
             arg_node = arg_node->next) {
          lizard_ast_node_t *arg_expr;
          lizard_ast_node_t *promise;
          lizard_ast_list_node_t *new_arg_node;
          arg_expr = ((lizard_ast_list_node_t *)arg_node)->ast;
          promise = lizard_make_promise(heap, arg_expr, env);
          new_arg_node = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
          new_arg_node->ast = promise;
          list_append(arg_list, &new_arg_node->node);
        }
        for (cur = arg_list->head; cur != arg_list->nil; cur = cur->next) {
          ((lizard_ast_list_node_t *)cur)->ast =
              lizard_force(((lizard_ast_list_node_t *)cur)->ast, heap);
        }
        return lizard_primitive_callcc(arg_list, env, heap, cont);
      }
    }

    case AST_ERROR:
      return cont(node, env, heap);

    default:
      return cont(lizard_make_error(heap, LIZARD_ERROR_NODE_TYPE), env, heap);
    }
  }
}

lizard_ast_node_t *lizard_apply(lizard_ast_node_t *func, ds_list_t *args,
                                lizard_env_t *env, lizard_heap_t *heap,
                                lizard_ast_node_t *(*cont)(lizard_ast_node_t *,
                                                           lizard_env_t *,
                                                           lizard_heap_t *)) {
  if (func->type == AST_PRIMITIVE) {
    {
      ds_list_node_t *cur;
      for (cur = args->head; cur != args->nil; cur = cur->next) {
        ((lizard_ast_list_node_t *)cur)->ast =
            lizard_force(((lizard_ast_list_node_t *)cur)->ast, heap);
      }
    }
    return cont(func->data.primitive(args, env, heap), env, heap);
  } else if (func->type == AST_LAMBDA) {
    {
      lizard_env_t *new_env;
      lizard_ast_node_t *param_list;
      ds_list_t *formal_params;
      ds_list_node_t *param_node, *arg_node;
      lizard_ast_list_node_t *param;
      lizard_ast_node_t *result;
<<<<<<< HEAD
      ds_list_node_t *body_iter;
=======
      list_node_t *body_iter;
>>>>>>> refs/remotes/origin/master
      new_env = lizard_env_create(heap, func->data.lambda.closure_env);
      param_list =
          ((lizard_ast_list_node_t *)func->data.lambda.parameters->head)->ast;
      if (param_list->type != AST_APPLICATION) {
        return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMS_2), env,
                    heap);
      }
      formal_params = param_list->data.application_arguments;
      param_node = formal_params->head;
      arg_node = args->head;
      while (param_node != formal_params->nil) {
        if (arg_node == args->nil) {
          return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_ARITY_LESS_2),
                      env, heap);
        }
        param = (lizard_ast_list_node_t *)param_node;
        if (param->ast->type != AST_SYMBOL) {
          return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMETER_2),
                      env, heap);
        }
        lizard_env_define(heap, new_env, param->ast->data.variable,
                          ((lizard_ast_list_node_t *)arg_node)->ast);
        param_node = param_node->next;
        arg_node = arg_node->next;
      }
      if (arg_node != args->nil) {
        return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_ARITY_MORE_2),
                    env, heap);
      }
      body_iter = func->data.lambda.parameters->head->next;
      result = NULL;
      while (body_iter->next != func->data.lambda.parameters->nil) {
        lizard_ast_list_node_t *body_expr = (lizard_ast_list_node_t *)body_iter;
        lizard_eval(body_expr->ast, new_env, heap, cont);
        body_iter = body_iter->next;
      }
      result = lizard_eval(((lizard_ast_list_node_t *)body_iter)->ast, new_env,
                           heap, cont);
      return cont(lizard_force(result, heap), env, heap);
    }
  } else {
    return cont(lizard_make_error(heap, LIZARD_ERROR_INVALID_APPLY_2), env,
                heap);
  }
}

lizard_ast_node_t *lizard_expand_macros(lizard_ast_node_t *node,
                                        lizard_env_t *env,
                                        lizard_heap_t *heap) {
  ds_list_node_t *first, *arg, *expr;
  lizard_ast_node_t *operator, * binding, *expanded;
  lizard_ast_list_node_t *copy;
  ds_list_t *macro_args;
  if (!node) {
    return NULL;
  }
  if (node->type == AST_APPLICATION) {
    first = node->data.application_arguments->head;
    if (first != node->data.application_arguments->nil) {
      operator=((lizard_ast_list_node_t *)first)->ast;
      if (operator->type == AST_SYMBOL) {
        binding = lizard_env_lookup(env, operator->data.variable);
        if (binding && binding->type == AST_MACRO) {
          macro_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
          for (arg = first->next; arg != node->data.application_arguments->nil;
               arg = arg->next) {
            copy = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
            copy->ast = ((lizard_ast_list_node_t *)arg)->ast;
            list_append(macro_args, &copy->node);
          }
          expanded = lizard_apply(binding->data.macro_def.transformer,
                                  macro_args, env, heap, lizard_identity_cont);
          /* A quasiquote-built body evaluates to AST_QUOTE wrapping code; */
          /* unwrap so the result is treated as code, not data. */
          while (expanded && expanded->type == AST_QUOTE) {
            expanded = expanded->data.quoted;
          }
          return lizard_expand_macros(expanded, env, heap);
        }
      }
    }
    for (arg = node->data.application_arguments->head;
         arg != node->data.application_arguments->nil; arg = arg->next) {
      ((lizard_ast_list_node_t *)arg)->ast =
          lizard_expand_macros(((lizard_ast_list_node_t *)arg)->ast, env, heap);
    }
  }
  switch (node->type) {
  case AST_QUOTE:
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
    if (node->data.if_clause.alt) {
      node->data.if_clause.alt =
          lizard_expand_macros(node->data.if_clause.alt, env, heap);
    }
    break;
  case AST_LAMBDA: {
    ds_list_node_t *body_node;
    lizard_ast_list_node_t *body_expr;
    if (node->data.lambda.parameters->head !=
        node->data.lambda.parameters->nil) {
      body_node = node->data.lambda.parameters->head->next;
      while (body_node != node->data.lambda.parameters->nil) {
        body_expr = (lizard_ast_list_node_t *)body_node;
        body_expr->ast = lizard_expand_macros(body_expr->ast, env, heap);
        body_node = body_node->next;
      }
    }
    break;
  }
  case AST_BEGIN:
    for (expr = node->data.begin_expressions->head;
         expr != node->data.begin_expressions->nil; expr = expr->next) {
      ((lizard_ast_list_node_t *)expr)->ast = lizard_expand_macros(
          ((lizard_ast_list_node_t *)expr)->ast, env, heap);
    }
    break;
  default:
    break;
  }
  return node;
}

lizard_ast_node_t *lizard_expand_quasiquote(lizard_ast_node_t *node,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  ds_list_t *expanded_list;
  ds_list_node_t *iter;
  lizard_ast_list_node_t *elem;
  lizard_ast_node_t *expanded_elem;
  lizard_ast_list_node_t *new_elem;
  lizard_ast_node_t *new_node;
  if (!node) {
    return NULL;
  }
  switch (node->type) {
  case AST_NUMBER:
  case AST_STRING:
  case AST_SYMBOL:
  case AST_BOOL:
  case AST_NIL:
    return node;
  case AST_UNQUOTE:
    return lizard_eval(node->data.quoted, env, heap, lizard_identity_cont);
  case AST_APPLICATION: {
    expanded_list = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    for (iter = node->data.application_arguments->head;
         iter != node->data.application_arguments->nil; iter = iter->next) {
      elem = (lizard_ast_list_node_t *)iter;
      expanded_elem = lizard_expand_quasiquote(elem->ast, env, heap);
      if (expanded_elem->type == AST_UNQUOTE) {
        expanded_elem = expanded_elem->data.quoted;
      } else if (expanded_elem->type == AST_UNQUOTE_SPLICING) {
        {
          ds_list_node_t *splice_iter;
          lizard_ast_node_t *spliced;
          lizard_ast_node_t *quoted_value = expanded_elem->data.quoted;
          spliced = lizard_eval(quoted_value, env, heap, lizard_identity_cont);
          /* The value being spliced may be an AST_APPLICATION (legacy list
             literal) or a cons-pair chain (what quoted lists become at
             runtime after lizard_convert_list_literal). Handle both. */
          if (spliced && spliced->type == AST_APPLICATION) {
            for (splice_iter = spliced->data.application_arguments->head;
                 splice_iter != spliced->data.application_arguments->nil;
                 splice_iter = splice_iter->next) {
              new_elem = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
              new_elem->ast = ((lizard_ast_list_node_t *)splice_iter)->ast;
              list_append(expanded_list, &new_elem->node);
            }
          } else if (spliced && spliced->type == AST_PAIR) {
            lizard_ast_node_t *cur = spliced;
            while (cur && cur->type == AST_PAIR) {
              new_elem = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
              new_elem->ast = cur->data.pair.car;
              list_append(expanded_list, &new_elem->node);
              cur = cur->data.pair.cdr;
            }
          } else if (spliced && spliced->type == AST_NIL) {
            /* splicing an empty list inserts nothing */
          } else {
            return lizard_make_error(heap, LIZARD_ERROR_INVALID_SPLICE);
          }
        }
        continue;
      }
      new_elem = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
      new_elem->ast = expanded_elem;
      list_append(expanded_list, &new_elem->node);
    }
    new_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    new_node->type = AST_APPLICATION;
    new_node->data.application_arguments = expanded_list;
    return new_node;
  }
  case AST_QUASIQUOTE:
    return lizard_expand_quasiquote(node->data.quoted, env, heap);
  case AST_IF: {
    /* Parser eagerly parses (if ...) as AST_IF even inside `(...). */
    /* Recurse into each subexpression so ,unquote works. */
    new_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    new_node->type = AST_IF;
    new_node->data.if_clause.pred =
        lizard_expand_quasiquote(node->data.if_clause.pred, env, heap);
    new_node->data.if_clause.cons =
        lizard_expand_quasiquote(node->data.if_clause.cons, env, heap);
    new_node->data.if_clause.alt =
        node->data.if_clause.alt
            ? lizard_expand_quasiquote(node->data.if_clause.alt, env, heap)
            : NULL;
    return new_node;
  }
  case AST_DEFINITION: {
    new_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    new_node->type = AST_DEFINITION;
    new_node->data.definition.variable = lizard_expand_quasiquote(
        node->data.definition.variable, env, heap);
    new_node->data.definition.value =
        lizard_expand_quasiquote(node->data.definition.value, env, heap);
    return new_node;
  }
  case AST_ASSIGNMENT: {
    new_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    new_node->type = AST_ASSIGNMENT;
    new_node->data.assignment.variable = lizard_expand_quasiquote(
        node->data.assignment.variable, env, heap);
    new_node->data.assignment.value =
        lizard_expand_quasiquote(node->data.assignment.value, env, heap);
    return new_node;
  }
  case AST_BEGIN: {
    ds_list_t *exprs;
    exprs = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    for (iter = node->data.begin_expressions->head;
         iter != node->data.begin_expressions->nil; iter = iter->next) {
      new_elem = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
      new_elem->ast = lizard_expand_quasiquote(
          ((lizard_ast_list_node_t *)iter)->ast, env, heap);
      list_append(exprs, &new_elem->node);
    }
    new_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    new_node->type = AST_BEGIN;
    new_node->data.begin_expressions = exprs;
    return new_node;
  }
  case AST_LAMBDA: {
    ds_list_t *params;
    params = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    for (iter = node->data.lambda.parameters->head;
         iter != node->data.lambda.parameters->nil; iter = iter->next) {
      new_elem = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
      new_elem->ast = lizard_expand_quasiquote(
          ((lizard_ast_list_node_t *)iter)->ast, env, heap);
      list_append(params, &new_elem->node);
    }
    new_node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    new_node->type = AST_LAMBDA;
    new_node->data.lambda.parameters = params;
    new_node->data.lambda.closure_env = node->data.lambda.closure_env;
    return new_node;
  }
  default:
    return node;
  }
}
