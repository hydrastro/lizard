#include "lizard_internal.h"
#include "env.h"
#include "lang.h"
#include "mem.h"
#include "primitives.h"
#include "runtime.h"
#include <ctype.h>
#include <ds.h>
#include <gmp.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
/* Phase 0: callcc state moved to runtime (heap->runtime->callcc_*).
 * Fallback globals for standalone heaps without a runtime. These are
 * non-static because primitives.c also needs them until all callers
 * go through a runtime. Will be removed when the heap always has
 * a runtime (Phase 0 Turn 2). */
jmp_buf callcc_buf_fallback;
int callcc_active_fallback = 0;
lizard_ast_node_t *callcc_value_fallback = NULL;

int lizard_continuation_jumped = 0;
lizard_ast_node_t *lizard_jump_value = NULL;

lizard_ast_node_t *lizard_convert_list_literal(lizard_ast_node_t *node,
                                               lizard_heap_t *heap) {
  lz_list_t *args, *rest_args;
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
  /* Recurse into the car too — nested parenthesized forms inside a
     quote are also list literals, not procedure calls. Without this,
     `'((1 2) (3 4))` left the inner lists as AST_APPLICATION and
     `(car (car ...))` later failed with "car expects a list". */
  if (car->type == AST_APPLICATION) {
    pair->data.pair.car = lizard_convert_list_literal(car, heap);
  } else {
    pair->data.pair.car = lizard_ast_deep_copy(car, heap);
  }
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

lizard_ast_node_t *lizard_primitive_delay(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_INVALID_DELAY);
  }
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  return lizard_make_promise(heap, expr, env);
}

lizard_ast_node_t *lizard_primitive_force(lz_list_t *args, lizard_env_t *env,
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
    case AST_RATIONAL:
    case AST_REAL:
    case AST_STRING:
    case AST_PAIR:
    case AST_PRIMITIVE:
      return cont(node, env, heap);

    case AST_SYMBOL: {
      lizard_ast_node_t *val;
      val = lizard_env_lookup(env, node->data.variable);
      if (!val) {
        return cont(lizard_make_error_at(heap, LIZARD_ERROR_UNBOUND_SYMBOL, node->span), env,
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

    /* (syntax-rules …) is a value — a template store. It evaluates to
       itself so (define-syntax foo (syntax-rules …)) just binds foo
       to the rules. */
    case AST_SYNTAX_RULES:
      return cont(node, env, heap);

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
        return cont(lizard_make_error_at(heap, LIZARD_ERROR_INVALID_DEF, node->span), env,
                    heap);
      }
      value = lizard_eval(node->data.definition.value, env, heap,
                          lizard_identity_cont);
      lizard_env_define(heap, env,
                        node->data.definition.variable->data.variable, value);
      /* R5RS says (define ...) has unspecified result; we return nil so
         script-mode REPLs don't echo `=> <procedure>` after every
         function definition. */
      return cont(lizard_make_nil(heap), env, heap);
    }

    case AST_ASSIGNMENT: {
      lizard_ast_node_t *value;
      if (node->data.assignment.variable->type != AST_SYMBOL) {
        return cont(lizard_make_error_at(heap, LIZARD_ERROR_ASSIGNMENT, node->span), env,
                    heap);
      }
      value = lizard_eval(node->data.assignment.value, env, heap,
                          lizard_identity_cont);
      if (!lizard_env_set(env, node->data.assignment.variable->data.variable,
                          value)) {
        return cont(lizard_make_error_at(heap, LIZARD_ERROR_ASSIGNMENT_UNBOUND, node->span),
                    env, heap);
      }
      /* set! also returns unspecified per R5RS. */
      return cont(lizard_make_nil(heap), env, heap);
    }

    case AST_IF: {
      lizard_ast_node_t *pred_result;
      int is_true;
      pred_result = lizard_force(lizard_eval(node->data.if_clause.pred, env,
                                             heap, lizard_identity_cont),
                                 heap);
      if (pred_result && pred_result->type == AST_ERROR) {
        return cont(pred_result, env, heap);
      }
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
      lz_list_node_t *iter;
      lz_list_node_t *last = node->data.begin_expressions->nil;
      /* find the tail without evaluating */
      for (iter = node->data.begin_expressions->head;
           iter != node->data.begin_expressions->nil; iter = iter->next) {
        last = iter;
      }
      if (last == node->data.begin_expressions->nil) {
        node = lizard_make_nil(heap);
        continue;
      }
      /* evaluate every non-tail expression for its side effects only,
         but propagate errors so (begin x (raise 'oops) y) actually
         raises rather than silently continuing. */
      for (iter = node->data.begin_expressions->head; iter != last;
           iter = iter->next) {
        lizard_ast_node_t *side =
            lizard_eval(((lizard_ast_list_node_t *)iter)->ast, env, heap,
                        lizard_identity_cont);
        if (side && side->type == AST_ERROR) {
          return cont(side, env, heap);
        }
      }
      /* trampoline the tail so it's in tail position */
      node = ((lizard_ast_list_node_t *)last)->ast;
      continue;
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
      lz_list_node_t *clause_iter;
      lizard_ast_node_t *chosen_body_seq = NULL;
      int matched = 0;
      for (clause_iter = node->data.cond_clauses->head;
           clause_iter != node->data.cond_clauses->nil;
           clause_iter = clause_iter->next) {
        lizard_ast_node_t *clause =
            ((lizard_ast_list_node_t *)clause_iter)->ast;
        lz_list_node_t *cl_head;
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
          if (test_val && test_val->type == AST_ERROR) {
            return cont(test_val, env, heap);
          }
        }
        if (lizard_is_true(test_val)) {
          /* Build a BEGIN over the rest of the clause's elements. */
          lz_list_t *body_list;
          lz_list_node_t *body_iter;
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
      lz_list_node_t *func_node;
      lizard_ast_node_t *func;
      lz_list_t *arg_list;
      lz_list_node_t *arg_node;
      func_node = node->data.application_arguments->head;
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
        lz_list_node_t *arg;
        lizard_ast_node_t *value;
        arg = arg_list->head;
        value = (arg != arg_list->nil)
                    ? lizard_force(((lizard_ast_list_node_t *)arg)->ast, heap)
                    : lizard_make_nil(heap);
        if (heap->runtime != NULL
              ? heap->runtime->callcc_active
              : callcc_active_fallback) {
          if (heap->runtime != NULL) {
            heap->runtime->callcc_value = value;
            longjmp(heap->runtime->callcc_buf, 1);
          } else {
            callcc_value_fallback = value;
            longjmp(callcc_buf_fallback, 1);
          }
        }
        return func->data.continuation.captured_cont(value, env, heap);
      }

      if (func->type == AST_PRIMITIVE) {
        /* Short-circuit `and` and `or`. They are registered as
           primitives, but primitive dispatch pre-forces every
           argument — which both defeats the short-circuit and lets
           an error in a later argument (e.g. an accessor on a
           wrong-typed value that earlier arguments would have ruled
           out) propagate spuriously. Handle these two by iterating
           and forcing one argument at a time. */
        if (func->data.primitive == lizard_primitive_and) {
          lz_list_node_t *cur;
          lizard_ast_node_t *last = lizard_make_bool(heap, true);
          for (cur = arg_list->head; cur != arg_list->nil; cur = cur->next) {
            lizard_ast_node_t *v = lizard_force(
                ((lizard_ast_list_node_t *)cur)->ast, heap);
            if (v && v->type == AST_ERROR) {
              return cont(v, env, heap);
            }
            if (lizard_is_false(v)) {
              return cont(v, env, heap);
            }
            last = v;
          }
          return cont(last, env, heap);
        }
        if (func->data.primitive == lizard_primitive_or) {
          lz_list_node_t *cur;
          lizard_ast_node_t *last = lizard_make_bool(heap, false);
          for (cur = arg_list->head; cur != arg_list->nil; cur = cur->next) {
            lizard_ast_node_t *v = lizard_force(
                ((lizard_ast_list_node_t *)cur)->ast, heap);
            if (v && v->type == AST_ERROR) {
              return cont(v, env, heap);
            }
            if (!lizard_is_false(v)) {
              return cont(v, env, heap);
            }
            last = v;
          }
          return cont(last, env, heap);
        }
        {
          lz_list_node_t *cur;
          for (cur = arg_list->head; cur != arg_list->nil; cur = cur->next) {
            lizard_ast_node_t *forced = lizard_force(
                ((lizard_ast_list_node_t *)cur)->ast, heap);
            /* If any argument errored, propagate immediately rather
               than letting display/print/etc. quietly accept it. Two
               primitives are exempt because they exist precisely to
               *receive* errors as values: `error-object?` (testing)
               and `error-value` (unwrapping the payload). */
            if (forced && forced->type == AST_ERROR &&
                func->data.primitive != lizard_primitive_error_objp &&
                func->data.primitive != lizard_primitive_error_value) {
              return cont(forced, env, heap);
            }
            ((lizard_ast_list_node_t *)cur)->ast = forced;
          }
        }
        return cont(func->data.primitive(arg_list, env, heap), env, heap);
      } else if (func->type == AST_LAMBDA) {
        {
          lizard_env_t *new_env;
          lizard_ast_node_t *param_list;
          lz_list_t *formal_params;
          lz_list_node_t *param_node, *arg_iter;
          lizard_ast_list_node_t *param;
          new_env = lizard_env_create(heap, func->data.lambda.closure_env);
          param_list =
              ((lizard_ast_list_node_t *)func->data.lambda.parameters->head)
                  ->ast;
          if (param_list->type == AST_NIL) {
            /* zero-parameter lambda: nothing to bind */
            if (arg_list->head != arg_list->nil) {
              return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_ARITY_MORE),
                          env, heap);
            }
            param_node = NULL;
            arg_iter = NULL;
            formal_params = NULL;
          } else if (param_list->type == AST_SYMBOL) {
            /* Varargs: (lambda args body) — bind all args as a list to
               the single symbol. Build a proper pair-chain from the
               (already-forced) AST list of args. */
            lz_list_node_t *it;
            lizard_ast_node_t *xs = lizard_make_nil(heap);
            lizard_ast_node_t *pair;
            /* Walk arg_list in order, build list in reverse, then
               flip. Cheaper: collect args into a temporary array
               and build cons chain right-to-left. */
            {
              size_t count = 0;
              lizard_ast_node_t **buf;
              size_t i;
              for (it = arg_list->head; it != arg_list->nil; it = it->next) {
                count++;
              }
              buf = lizard_heap_alloc(count * sizeof(lizard_ast_node_t *) + 1);
              i = 0;
              for (it = arg_list->head; it != arg_list->nil; it = it->next) {
                buf[i++] = lizard_force(
                    ((lizard_ast_list_node_t *)it)->ast, heap);
              }
              for (i = count; i > 0; i--) {
                pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
                pair->type = AST_PAIR;
                pair->data.pair.car = buf[i - 1];
                pair->data.pair.cdr = xs;
                xs = pair;
              }
            }
            lizard_env_define(heap, new_env, param_list->data.variable, xs);
            param_node = NULL;
            arg_iter = NULL;
            formal_params = NULL;
          } else if (param_list->type == AST_APPLICATION) {
            formal_params = param_list->data.application_arguments;
            param_node = formal_params->head;
            arg_iter = arg_list->head;
          } else {
            return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMS),
                        env, heap);
          }
          while (formal_params && param_node != formal_params->nil) {
            param = (lizard_ast_list_node_t *)param_node;
            if (param->ast->type != AST_SYMBOL) {
              return cont(
                  lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMETER), env,
                  heap);
            }
            /* Dotted-rest: a `.` symbol means the next param captures
               all remaining args as a list. (lambda (a b . rest) ...) */
            if (strcmp(param->ast->data.variable, ".") == 0) {
              lizard_ast_list_node_t *rest_param;
              lz_list_node_t *it;
              lizard_ast_node_t *xs = lizard_make_nil(heap);
              lizard_ast_node_t **buf;
              size_t count = 0, i;
              if (param_node->next == formal_params->nil) {
                return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMS),
                            env, heap);
              }
              rest_param = (lizard_ast_list_node_t *)param_node->next;
              if (rest_param->ast->type != AST_SYMBOL) {
                return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMETER),
                            env, heap);
              }
              for (it = arg_iter; it != arg_list->nil; it = it->next) count++;
              buf = lizard_heap_alloc(count * sizeof(lizard_ast_node_t *) + 1);
              i = 0;
              for (it = arg_iter; it != arg_list->nil; it = it->next) {
                buf[i++] = lizard_force(
                    ((lizard_ast_list_node_t *)it)->ast, heap);
              }
              for (i = count; i > 0; i--) {
                lizard_ast_node_t *pair =
                    lizard_heap_alloc(sizeof(lizard_ast_node_t));
                pair->type = AST_PAIR;
                pair->data.pair.car = buf[i - 1];
                pair->data.pair.cdr = xs;
                xs = pair;
              }
              lizard_env_define(heap, new_env,
                                rest_param->ast->data.variable, xs);
              param_node = formal_params->nil;  /* done */
              arg_iter = arg_list->nil;         /* consumed all */
              break;
            }
            if (arg_iter == arg_list->nil) {
              return cont(
                  lizard_make_error(heap, LIZARD_ERROR_LAMBDA_ARITY_LESS), env,
                  heap);
            }
            {
              /* Force the arg eagerly. R5RS is applicative-order: the
                 binding receives a value, not a thunk that re-evaluates
                 in the original env where set! may have mutated things.
                 We deliberately do NOT propagate errors here — handlers
                 passed to `try` receive error values as arguments. */
              lizard_ast_node_t *arg_val = lizard_force(
                  ((lizard_ast_list_node_t *)arg_iter)->ast, heap);
              lizard_env_define(heap, new_env, param->ast->data.variable,
                                arg_val);
            }
            param_node = param_node->next;
            arg_iter = arg_iter->next;
          }
          if (arg_iter && arg_iter != arg_list->nil) {
            return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_ARITY_MORE),
                        env, heap);
          }
          /* Lambda body trampoline.
             Evaluate every NON-tail body for its side effects with an
             identity continuation, then tail-call the LAST body by
             rewriting (node, env) and continuing the outer for(;;)
             dispatch loop. This eliminates one C stack frame per
             Scheme call — without it a self-recursive function like
                (define (pow2 n) (if (= n 0) 1 (* 2 (pow2 (- n 1)))))
             smashes the C stack at depth ~10⁴, whereas with TCO it
             runs to arbitrary depth limited only by the heap. */
          {
            lz_list_node_t *tail_body;
            lz_list_node_t *iter;
            tail_body = func->data.lambda.parameters->nil;
            for (iter = func->data.lambda.parameters->head->next;
                 iter != func->data.lambda.parameters->nil; iter = iter->next) {
              tail_body = iter;
            }
            if (tail_body == func->data.lambda.parameters->nil) {
              /* lambda with no body — degenerate but valid; returns nil */
              return cont(lizard_make_nil(heap), env, heap);
            }
            /* non-tail bodies: side effects only. But an AST_ERROR
               here must short-circuit — otherwise (lambda () (foo)
               (raise 'x) (bar)) would silently swallow the raise. */
            for (iter = func->data.lambda.parameters->head->next;
                 iter != tail_body; iter = iter->next) {
              lizard_ast_node_t *side =
                  lizard_eval(((lizard_ast_list_node_t *)iter)->ast, new_env,
                              heap, lizard_identity_cont);
              if (lizard_continuation_jumped) {
                lizard_continuation_jumped = 0;
                return cont(lizard_force(lizard_jump_value, heap), env, heap);
              }
              if (side && side->type == AST_ERROR) {
                return cont(side, env, heap);
              }
            }
            /* tail body: trampoline. Reuses the outer `cont`, so the
               result of the tail expression naturally flows out to
               the original caller. */
            node = ((lizard_ast_list_node_t *)tail_body)->ast;
            env = new_env;
            continue;
          }
        }
      } else if (func->type == AST_CALLCC) {
        return lizard_primitive_callcc(arg_list, env, heap, cont);
      } else {
        return cont(lizard_make_error_at(heap, LIZARD_ERROR_INVALID_APPLY, node->span), env,
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
        lz_list_node_t *func_node;
        lz_list_node_t *cur;
        lz_list_t *arg_list;
        lz_list_node_t *arg_node;
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

lizard_ast_node_t *lizard_apply(lizard_ast_node_t *func, lz_list_t *args,
                                lizard_env_t *env, lizard_heap_t *heap,
                                lizard_ast_node_t *(*cont)(lizard_ast_node_t *,
                                                           lizard_env_t *,
                                                           lizard_heap_t *)) {
  if (func->type == AST_PRIMITIVE) {
    {
      lz_list_node_t *cur;
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
      lz_list_t *formal_params;
      lz_list_node_t *param_node, *arg_node;
      lizard_ast_list_node_t *param;
      lizard_ast_node_t *result;
      lz_list_node_t *body_iter;
      new_env = lizard_env_create(heap, func->data.lambda.closure_env);
      param_list =
          ((lizard_ast_list_node_t *)func->data.lambda.parameters->head)->ast;
      if (param_list->type == AST_NIL) {
        /* zero-parameter lambda */
        if (args->head != args->nil) {
          return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_ARITY_MORE_2),
                      env, heap);
        }
        param_node = NULL;
        arg_node = NULL;
        formal_params = NULL;
      } else if (param_list->type == AST_SYMBOL) {
        /* Varargs — bind all args as a list to the single symbol. */
        lz_list_node_t *it;
        lizard_ast_node_t *xs = lizard_make_nil(heap);
        size_t count = 0;
        lizard_ast_node_t **buf;
        size_t i;
        for (it = args->head; it != args->nil; it = it->next) count++;
        buf = lizard_heap_alloc(count * sizeof(lizard_ast_node_t *) + 1);
        i = 0;
        for (it = args->head; it != args->nil; it = it->next) {
          buf[i++] = ((lizard_ast_list_node_t *)it)->ast;
        }
        for (i = count; i > 0; i--) {
          lizard_ast_node_t *pair =
              lizard_heap_alloc(sizeof(lizard_ast_node_t));
          pair->type = AST_PAIR;
          pair->data.pair.car = buf[i - 1];
          pair->data.pair.cdr = xs;
          xs = pair;
        }
        lizard_env_define(heap, new_env, param_list->data.variable, xs);
        param_node = NULL;
        arg_node = NULL;
        formal_params = NULL;
      } else if (param_list->type == AST_APPLICATION) {
        formal_params = param_list->data.application_arguments;
        param_node = formal_params->head;
        arg_node = args->head;
      } else {
        return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMS_2), env,
                    heap);
      }
      while (formal_params && param_node != formal_params->nil) {
        param = (lizard_ast_list_node_t *)param_node;
        if (param->ast->type != AST_SYMBOL) {
          return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMETER_2),
                      env, heap);
        }
        /* Dotted-rest. */
        if (strcmp(param->ast->data.variable, ".") == 0) {
          lizard_ast_list_node_t *rest_param;
          lz_list_node_t *it;
          lizard_ast_node_t *xs = lizard_make_nil(heap);
          lizard_ast_node_t **buf;
          size_t count = 0, i;
          if (param_node->next == formal_params->nil) {
            return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMS_2),
                        env, heap);
          }
          rest_param = (lizard_ast_list_node_t *)param_node->next;
          if (rest_param->ast->type != AST_SYMBOL) {
            return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMETER_2),
                        env, heap);
          }
          for (it = arg_node; it != args->nil; it = it->next) count++;
          buf = lizard_heap_alloc(count * sizeof(lizard_ast_node_t *) + 1);
          i = 0;
          for (it = arg_node; it != args->nil; it = it->next) {
            buf[i++] = ((lizard_ast_list_node_t *)it)->ast;
          }
          for (i = count; i > 0; i--) {
            lizard_ast_node_t *pair =
                lizard_heap_alloc(sizeof(lizard_ast_node_t));
            pair->type = AST_PAIR;
            pair->data.pair.car = buf[i - 1];
            pair->data.pair.cdr = xs;
            xs = pair;
          }
          lizard_env_define(heap, new_env, rest_param->ast->data.variable, xs);
          arg_node = args->nil;
          break;
        }
        if (arg_node == args->nil) {
          return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_ARITY_LESS_2),
                      env, heap);
        }
        {
          /* Eager force; do not propagate errors at this boundary so
             try-handlers can receive errors as their arguments. */
          lizard_ast_node_t *arg_val = lizard_force(
              ((lizard_ast_list_node_t *)arg_node)->ast, heap);
          lizard_env_define(heap, new_env, param->ast->data.variable, arg_val);
        }
        param_node = param_node->next;
        arg_node = arg_node->next;
      }
      if (arg_node && arg_node != args->nil) {
        return cont(lizard_make_error(heap, LIZARD_ERROR_LAMBDA_ARITY_MORE_2),
                    env, heap);
      }
      body_iter = func->data.lambda.parameters->head->next;
      result = NULL;
      while (body_iter->next != func->data.lambda.parameters->nil) {
        lizard_ast_list_node_t *body_expr = (lizard_ast_list_node_t *)body_iter;
        lizard_ast_node_t *side =
            lizard_eval(body_expr->ast, new_env, heap, cont);
        if (side && side->type == AST_ERROR) {
          return cont(side, env, heap);
        }
        body_iter = body_iter->next;
      }
      result = lizard_eval(((lizard_ast_list_node_t *)body_iter)->ast, new_env,
                           heap, cont);
      return cont(lizard_force(result, heap), env, heap);
    }
  } else {
    return cont(lizard_make_error_at(heap, LIZARD_ERROR_INVALID_APPLY_2, func->span), env,
                heap);
  }
  return lizard_make_nil(heap);  /* unreachable — satisfies -Wreturn-type */
}

/* ---------------------------------------------------------------------
 * syntax-rules pattern matcher + template expander.
 *
 * The matcher walks a pattern (parsed via lizard_parse_datum, so a
 * pair-chain) against the macro call's arguments (also turned into a
 * pair-chain). On success it accumulates pattern-variable bindings;
 * the expander then walks the template, substituting variables.
 *
 * Subset of R5RS we support:
 *   - literal symbols (from the (syntax-rules (lits) ...) header)
 *     must match exactly
 *   - the symbol _ matches anything without capture
 *   - any other symbol is a pattern variable, capturing the matched
 *     sub-form
 *   - list patterns match lists of identical structure
 *   - other atoms (numbers, strings, booleans, nil) must match by value
 *
 * Not yet supported: ellipsis (...) and proper hygiene. We do provide
 * a lightweight hygiene pass that renames identifiers introduced in
 * obvious binding positions (`let`, `lambda`, `define`) inside the
 * template; this catches the classic (swap! x tmp) capture.
 * ------------------------------------------------------------------- */

typedef struct lizard_sr_binding {
  const char *name;
  lizard_ast_node_t *value;
  int depth;                       /* ellipsis depth: 0 = ordinary var */
  struct lizard_sr_binding *next;
} lizard_sr_binding_t;

static int lizard_is_literal_symbol(lz_list_t *literals, const char *name) {
  lz_list_node_t *it;
  for (it = literals->head; it != literals->nil; it = it->next) {
    lizard_ast_node_t *lit = ((lizard_ast_list_node_t *)it)->ast;
    if (lit && lit->type == AST_SYMBOL &&
        strcmp(lit->data.variable, name) == 0) {
      return 1;
    }
  }
  return 0;
}

static lizard_ast_node_t *lizard_sr_lookup(lizard_sr_binding_t *bs,
                                           const char *name) {
  for (; bs; bs = bs->next) {
    if (strcmp(bs->name, name) == 0) return bs->value;
  }
  return NULL;
}

/* ---- ellipsis (...) support ----------------------------------------------
 * A pattern element immediately followed by the symbol `...` matches zero or
 * more consecutive forms; each pattern variable underneath it binds to a
 * SEQUENCE (depth one greater).  The template expander then iterates such a
 * sub-template once per element.  This generalises to nested ellipses because
 * a depth-(d) variable's value is a pair-chain of depth-(d-1) values. */
static int lizard_sr_match(lizard_ast_node_t *pattern, lizard_ast_node_t *form,
                           lz_list_t *literals, lizard_sr_binding_t **out,
                           lizard_heap_t *heap);
static int lizard_sr_match_ellipsis(lizard_ast_node_t *subpat,
                                     lizard_ast_node_t *restpat,
                                     lizard_ast_node_t *form,
                                     lz_list_t *literals,
                                     lizard_sr_binding_t **out,
                                     lizard_heap_t *heap);

/* Is `node`'s cdr a pair whose car is the literal symbol `...`? */
static int lizard_sr_followed_by_ellipsis(lizard_ast_node_t *node) {
  lizard_ast_node_t *nx;
  if (!node || node->type != AST_PAIR) return 0;
  nx = node->data.pair.cdr;
  return nx && nx->type == AST_PAIR && nx->data.pair.car &&
         nx->data.pair.car->type == AST_SYMBOL &&
         strcmp(nx->data.pair.car->data.variable, "...") == 0;
}

static lizard_ast_node_t *lizard_sr_nil(lizard_heap_t *heap) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_NIL;
  return n;
}
static lizard_ast_node_t *lizard_sr_cons(lizard_ast_node_t *a,
                                         lizard_ast_node_t *d,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_PAIR;
  n->data.pair.car = a;
  n->data.pair.cdr = d;
  return n;
}
static int lizard_sr_len(lizard_ast_node_t *f) {
  int n = 0;
  while (f && f->type == AST_PAIR) { n++; f = f->data.pair.cdr; }
  return n;
}
static lizard_ast_node_t *lizard_sr_drop(lizard_ast_node_t *f, int k) {
  while (k-- > 0 && f && f->type == AST_PAIR) f = f->data.pair.cdr;
  return f;
}

/* Collect every pattern variable in `pat` together with its ellipsis depth
 * (number of enclosing `...` within pat).  Used so an ellipsis matching ZERO
 * forms still binds its variables (to empty sequences). */
static void lizard_sr_collect_vars(lizard_ast_node_t *pat, lz_list_t *literals,
                                   int depth, lizard_sr_binding_t **acc,
                                   lizard_heap_t *heap) {
  if (!pat) return;
  if (pat->type == AST_PAIR) {
    if (lizard_sr_followed_by_ellipsis(pat)) {
      lizard_sr_collect_vars(pat->data.pair.car, literals, depth + 1, acc, heap);
      lizard_sr_collect_vars(pat->data.pair.cdr->data.pair.cdr, literals, depth,
                             acc, heap);
    } else {
      lizard_sr_collect_vars(pat->data.pair.car, literals, depth, acc, heap);
      lizard_sr_collect_vars(pat->data.pair.cdr, literals, depth, acc, heap);
    }
    return;
  }
  if (pat->type == AST_SYMBOL) {
    const char *n = pat->data.variable;
    if (strcmp(n, "_") == 0 || strcmp(n, "...") == 0) return;
    if (lizard_is_literal_symbol(literals, n)) return;
    {
      lizard_sr_binding_t *b = lizard_heap_alloc(sizeof(lizard_sr_binding_t));
      b->name = n;
      b->value = NULL;
      b->depth = depth;
      b->next = *acc;
      *acc = b;
    }
  }
}

/* Returns 1 on match and prepends any captured variables to *out. */
static int lizard_sr_match(lizard_ast_node_t *pattern, lizard_ast_node_t *form,
                           lz_list_t *literals, lizard_sr_binding_t **out,
                           lizard_heap_t *heap) {
  if (!pattern && !form) return 1;
  if (!pattern || !form) return 0;
  /* literal-pair pattern */
  if (pattern->type == AST_PAIR) {
    if (lizard_sr_followed_by_ellipsis(pattern)) {
      return lizard_sr_match_ellipsis(pattern->data.pair.car,
                                      pattern->data.pair.cdr->data.pair.cdr,
                                      form, literals, out, heap);
    }
    if (form->type != AST_PAIR) return 0;
    if (!lizard_sr_match(pattern->data.pair.car, form->data.pair.car,
                         literals, out, heap)) return 0;
    if (!lizard_sr_match(pattern->data.pair.cdr, form->data.pair.cdr,
                         literals, out, heap)) return 0;
    return 1;
  }
  if (pattern->type == AST_NIL) {
    return form->type == AST_NIL;
  }
  if (pattern->type == AST_SYMBOL) {
    const char *name = pattern->data.variable;
    if (strcmp(name, "_") == 0) return 1;  /* wildcard */
    if (lizard_is_literal_symbol(literals, name)) {
      /* Literal: must match an identical symbol. */
      return form->type == AST_SYMBOL &&
             strcmp(form->data.variable, name) == 0;
    }
    /* Pattern variable: capture. */
    {
      lizard_sr_binding_t *b = lizard_heap_alloc(sizeof(lizard_sr_binding_t));
      b->name = name;
      b->value = form;
      b->depth = 0;
      b->next = *out;
      *out = b;
      return 1;
    }
  }
  if (pattern->type == AST_NUMBER) {
    return form->type == AST_NUMBER &&
           mpz_cmp(pattern->data.number, form->data.number) == 0;
  }
  if (pattern->type == AST_STRING) {
    return form->type == AST_STRING &&
           strcmp(pattern->data.string, form->data.string) == 0;
  }
  if (pattern->type == AST_BOOL) {
    return form->type == AST_BOOL &&
           pattern->data.boolean == form->data.boolean;
  }
  return 0;
}

/* Match  (subpat ... . restpat)  against `form`.  subpat consumes
 * (len(form) - len(restpat)) leading elements; each pattern variable in subpat
 * binds to the sequence of its per-element matches (depth + 1). */
static int lizard_sr_match_ellipsis(lizard_ast_node_t *subpat,
                                     lizard_ast_node_t *restpat,
                                     lizard_ast_node_t *form,
                                     lz_list_t *literals,
                                     lizard_sr_binding_t **out,
                                     lizard_heap_t *heap) {
  int formlen = lizard_sr_len(form);
  int restlen = lizard_sr_len(restpat);
  int nmatch = formlen - restlen;
  int i;
  lizard_sr_binding_t *pvars = NULL;
  lizard_sr_binding_t *vp;
  lizard_sr_binding_t **subs;
  lizard_ast_node_t *cur;
  if (nmatch < 0) return 0;
  lizard_sr_collect_vars(subpat, literals, 0, &pvars, heap);
  subs = lizard_heap_alloc(sizeof(lizard_sr_binding_t *) *
                           (size_t)(nmatch > 0 ? nmatch : 1));
  cur = form;
  for (i = 0; i < nmatch; i++) {
    lizard_sr_binding_t *sub = NULL;
    if (!cur || cur->type != AST_PAIR) return 0;
    if (!lizard_sr_match(subpat, cur->data.pair.car, literals, &sub, heap))
      return 0;
    subs[i] = sub;
    cur = cur->data.pair.cdr;
  }
  /* For each variable in subpat, assemble its ordered sequence (depth+1). */
  for (vp = pvars; vp; vp = vp->next) {
    lizard_ast_node_t *seq = lizard_sr_nil(heap);
    lizard_sr_binding_t *b;
    for (i = nmatch - 1; i >= 0; i--) {
      lizard_ast_node_t *val = lizard_sr_lookup(subs[i], vp->name);
      seq = lizard_sr_cons(val ? val : lizard_sr_nil(heap), seq, heap);
    }
    b = lizard_heap_alloc(sizeof(lizard_sr_binding_t));
    b->name = vp->name;
    b->value = seq;
    b->depth = vp->depth + 1;
    b->next = *out;
    *out = b;
  }
  /* The forms after the ellipsis must match restpat. */
  return lizard_sr_match(restpat, lizard_sr_drop(form, nmatch), literals, out,
                         heap);
}

/* Hygiene rename map: for identifiers introduced inside the template
 * (e.g. `let` bindings, `lambda` params, `define` names) we replace
 * them with fresh gensyms so they can't accidentally capture
 * identifiers the caller passed in. */
typedef struct lizard_sr_rename {
  const char *original;
  const char *fresh;
  struct lizard_sr_rename *next;
} lizard_sr_rename_t;

/* Phase 0: counter lives in runtime (heap->runtime->sr_counter).
 * Falls back to a static counter for standalone heaps. */
static unsigned long lizard_sr_counter_fallback = 0;

static const char *lizard_sr_fresh(const char *base, lizard_heap_t *heap) {
  unsigned long *counter;
  char *buf = lizard_heap_alloc(strlen(base) + 24);
  counter = (heap->runtime != NULL)
              ? &heap->runtime->sr_counter
              : &lizard_sr_counter_fallback;
  (*counter)++;
  sprintf(buf, "%s.h%lu", base, *counter);
  return buf;
}

static const char *lizard_sr_renamed(lizard_sr_rename_t *r,
                                     const char *name) {
  for (; r; r = r->next) {
    if (strcmp(r->original, name) == 0) return r->fresh;
  }
  return name;
}

/* Expand a template, substituting pattern vars and applying renames. */
static lizard_ast_node_t *lizard_sr_expand(lizard_ast_node_t *tmpl,
                                           lizard_sr_binding_t *bs,
                                           lizard_sr_rename_t *renames,
                                           lizard_heap_t *heap);
static lizard_ast_node_t *lizard_sr_expand_ellipsis(lizard_ast_node_t *tmpl,
                                                    lizard_ast_node_t *rest,
                                                    lizard_sr_binding_t *bs,
                                                    lizard_sr_rename_t *renames,
                                                    lizard_heap_t *heap);

/* nth element of a sequence pair-chain (NULL if out of range). */
static lizard_ast_node_t *lizard_sr_nth(lizard_ast_node_t *seq, int i) {
  while (i-- > 0 && seq && seq->type == AST_PAIR) seq = seq->data.pair.cdr;
  return (seq && seq->type == AST_PAIR) ? seq->data.pair.car : NULL;
}

/* Collect the bindings (from bs) for variables appearing in `tmpl` that have
 * ellipsis depth >= 1 — the ones an enclosing `...` iterates over. */
static void lizard_sr_iter_vars(lizard_ast_node_t *tmpl,
                                lizard_sr_binding_t *bs,
                                lizard_sr_binding_t **acc,
                                lizard_heap_t *heap) {
  if (!tmpl) return;
  if (tmpl->type == AST_PAIR) {
    lizard_sr_iter_vars(tmpl->data.pair.car, bs, acc, heap);
    lizard_sr_iter_vars(tmpl->data.pair.cdr, bs, acc, heap);
    return;
  }
  if (tmpl->type == AST_SYMBOL) {
    lizard_sr_binding_t *b;
    for (b = bs; b; b = b->next) {
      if (strcmp(b->name, tmpl->data.variable) == 0) {
        if (b->depth >= 1) {
          lizard_sr_binding_t *a;
          for (a = *acc; a; a = a->next)
            if (strcmp(a->name, b->name) == 0) return;  /* already noted */
          a = lizard_heap_alloc(sizeof(lizard_sr_binding_t));
          a->name = b->name;
          a->value = b->value;
          a->depth = b->depth;
          a->next = *acc;
          *acc = a;
        }
        return;  /* first (innermost) binding for this name wins */
      }
    }
  }
}

static lizard_ast_node_t *lizard_sr_expand(lizard_ast_node_t *tmpl,
                                           lizard_sr_binding_t *bs,
                                           lizard_sr_rename_t *renames,
                                           lizard_heap_t *heap) {
  if (!tmpl) return NULL;
  if (tmpl->type == AST_PAIR) {
    lizard_ast_node_t *head = tmpl->data.pair.car;
    /* Ellipsis: (sub ... . rest) expands `sub` once per iteration. */
    if (lizard_sr_followed_by_ellipsis(tmpl)) {
      return lizard_sr_expand_ellipsis(tmpl->data.pair.car,
                                       tmpl->data.pair.cdr->data.pair.cdr,
                                       bs, renames, heap);
    }
    /* Detect binding-introducing forms — let, lambda, define — and
       generate fresh names for the introduced identifiers. This is
       the hygiene step. */
    if (head && head->type == AST_SYMBOL) {
      const char *hname = head->data.variable;
      if (strcmp(hname, "let") == 0 || strcmp(hname, "let*") == 0 ||
          strcmp(hname, "letrec") == 0) {
        /* (let ((n1 v1) (n2 v2) ...) body...) — rename each ni. */
        lizard_ast_node_t *bindings_form;
        lizard_ast_node_t *cur;
        lizard_sr_rename_t *new_renames = renames;
        if (tmpl->data.pair.cdr && tmpl->data.pair.cdr->type == AST_PAIR) {
          bindings_form = tmpl->data.pair.cdr->data.pair.car;
          for (cur = bindings_form; cur && cur->type == AST_PAIR;
               cur = cur->data.pair.cdr) {
            lizard_ast_node_t *binding = cur->data.pair.car;
            if (binding && binding->type == AST_PAIR &&
                binding->data.pair.car &&
                binding->data.pair.car->type == AST_SYMBOL) {
              const char *orig = binding->data.pair.car->data.variable;
              /* Only rename identifiers NOT already substituted as
                 pattern variables — otherwise we'd break legitimate
                 user-supplied names. */
              if (!lizard_sr_lookup(bs, orig)) {
                lizard_sr_rename_t *r =
                    lizard_heap_alloc(sizeof(lizard_sr_rename_t));
                r->original = orig;
                r->fresh = lizard_sr_fresh(orig, heap);
                r->next = new_renames;
                new_renames = r;
              }
            }
          }
        }
        /* Fall through to generic pair rebuild with the augmented
           rename set in scope. */
        {
          lizard_ast_node_t *result =
              lizard_heap_alloc(sizeof(lizard_ast_node_t));
          result->type = AST_PAIR;
          result->data.pair.car =
              lizard_sr_expand(tmpl->data.pair.car, bs, new_renames, heap);
          result->data.pair.cdr =
              lizard_sr_expand(tmpl->data.pair.cdr, bs, new_renames, heap);
          return result;
        }
      }
      if (strcmp(hname, "lambda") == 0) {
        /* (lambda (p1 p2 . rest) body...) — rename each parameter. */
        lizard_ast_node_t *params;
        lizard_ast_node_t *cur;
        lizard_sr_rename_t *new_renames = renames;
        if (tmpl->data.pair.cdr && tmpl->data.pair.cdr->type == AST_PAIR) {
          params = tmpl->data.pair.cdr->data.pair.car;
          for (cur = params; cur && cur->type == AST_PAIR;
               cur = cur->data.pair.cdr) {
            lizard_ast_node_t *p = cur->data.pair.car;
            if (p && p->type == AST_SYMBOL &&
                strcmp(p->data.variable, ".") != 0) {
              if (!lizard_sr_lookup(bs, p->data.variable)) {
                lizard_sr_rename_t *r =
                    lizard_heap_alloc(sizeof(lizard_sr_rename_t));
                r->original = p->data.variable;
                r->fresh = lizard_sr_fresh(p->data.variable, heap);
                r->next = new_renames;
                new_renames = r;
              }
            }
          }
          /* single-symbol varargs */
          if (params && params->type == AST_SYMBOL) {
            if (!lizard_sr_lookup(bs, params->data.variable)) {
              lizard_sr_rename_t *r =
                  lizard_heap_alloc(sizeof(lizard_sr_rename_t));
              r->original = params->data.variable;
              r->fresh = lizard_sr_fresh(params->data.variable, heap);
              r->next = new_renames;
              new_renames = r;
            }
          }
        }
        {
          lizard_ast_node_t *result =
              lizard_heap_alloc(sizeof(lizard_ast_node_t));
          result->type = AST_PAIR;
          result->data.pair.car =
              lizard_sr_expand(tmpl->data.pair.car, bs, new_renames, heap);
          result->data.pair.cdr =
              lizard_sr_expand(tmpl->data.pair.cdr, bs, new_renames, heap);
          return result;
        }
      }
    }
    {
      lizard_ast_node_t *result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      result->type = AST_PAIR;
      result->data.pair.car =
          lizard_sr_expand(tmpl->data.pair.car, bs, renames, heap);
      result->data.pair.cdr =
          lizard_sr_expand(tmpl->data.pair.cdr, bs, renames, heap);
      return result;
    }
  }
  if (tmpl->type == AST_SYMBOL) {
    /* First try pattern variable. */
    lizard_ast_node_t *bound = lizard_sr_lookup(bs, tmpl->data.variable);
    if (bound) return bound;
    /* Then try hygienic rename. */
    {
      const char *rn = lizard_sr_renamed(renames, tmpl->data.variable);
      if (rn != tmpl->data.variable) {
        lizard_ast_node_t *sym =
            lizard_heap_alloc(sizeof(lizard_ast_node_t));
        sym->type = AST_SYMBOL;
        sym->data.variable = rn;
        return sym;
      }
    }
    return tmpl;
  }
  /* Literals and nil pass through. */
  return tmpl;
}

/* Expand  (sub ... . rest):  expand `sub` once per element of the iteration
 * variables' sequences (shadowing each with its i-th element at depth-1), then
 * append the expansion of `rest`. */
static lizard_ast_node_t *lizard_sr_expand_ellipsis(lizard_ast_node_t *sub,
                                                    lizard_ast_node_t *rest,
                                                    lizard_sr_binding_t *bs,
                                                    lizard_sr_rename_t *renames,
                                                    lizard_heap_t *heap) {
  lizard_sr_binding_t *iters = NULL;
  lizard_sr_binding_t *it;
  int count = -1;
  int i;
  lizard_ast_node_t *acc;
  lizard_sr_iter_vars(sub, bs, &iters, heap);
  for (it = iters; it; it = it->next) {
    int len = lizard_sr_len(it->value);
    if (count < 0 || len < count) count = len;  /* lockstep over the shortest */
  }
  if (count < 0) count = 0;  /* no iteration variable -> zero copies */
  acc = lizard_sr_expand(rest, bs, renames, heap);
  for (i = count - 1; i >= 0; i--) {
    lizard_sr_binding_t *scope = bs;
    lizard_ast_node_t *ei;
    for (it = iters; it; it = it->next) {
      lizard_sr_binding_t *nb = lizard_heap_alloc(sizeof(lizard_sr_binding_t));
      nb->name = it->name;
      nb->value = lizard_sr_nth(it->value, i);
      nb->depth = it->depth - 1;
      nb->next = scope;
      scope = nb;
    }
    ei = lizard_sr_expand(sub, scope, renames, heap);
    acc = lizard_sr_cons(ei, acc, heap);
  }
  return acc;
}

/* Build a pair-chain from an AST_APPLICATION's argument list. Used to
 * turn a macro call's argument list into a form the matcher can walk. */
static lizard_ast_node_t *lizard_app_to_pair(lz_list_t *args,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *head = lizard_make_nil(heap);
  lizard_ast_node_t **tail = &head;
  lz_list_node_t *it;
  for (it = args->head; it != args->nil; it = it->next) {
    lizard_ast_node_t *p = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    p->type = AST_PAIR;
    p->data.pair.car = ((lizard_ast_list_node_t *)it)->ast;
    p->data.pair.cdr = lizard_make_nil(heap);
    *tail = p;
    tail = &p->data.pair.cdr;
  }
  return head;
}

/* Convert an arbitrary AST (which may contain AST_APPLICATION because
 * the macro call was parsed by the normal parser) back into a
 * pair-chain so the matcher can compare it against the datum-shaped
 * pattern. Non-application leaves pass through. */
static lizard_ast_node_t *lizard_datumize(lizard_ast_node_t *n,
                                          lizard_heap_t *heap) {
  if (!n) return NULL;
  if (n->type == AST_APPLICATION) {
    lizard_ast_node_t *head = lizard_make_nil(heap);
    lizard_ast_node_t **tail = &head;
    lz_list_node_t *it;
    for (it = n->data.application_arguments->head;
         it != n->data.application_arguments->nil; it = it->next) {
      lizard_ast_node_t *p = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      p->type = AST_PAIR;
      p->data.pair.car =
          lizard_datumize(((lizard_ast_list_node_t *)it)->ast, heap);
      p->data.pair.cdr = lizard_make_nil(heap);
      *tail = p;
      tail = &p->data.pair.cdr;
    }
    return head;
  }
  if (n->type == AST_PAIR) {
    lizard_ast_node_t *p = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    p->type = AST_PAIR;
    p->data.pair.car = lizard_datumize(n->data.pair.car, heap);
    p->data.pair.cdr = lizard_datumize(n->data.pair.cdr, heap);
    return p;
  }
  return n;
}

/* Re-parse an expanded template (pair-chain form) into proper
 * specialized AST by serializing it back to text and tokenizing+
 * parsing. This is the easiest way to get AST_LAMBDA / AST_IF / etc.
 * out of the pair-chain produced by lizard_sr_expand.
 *
 * For now we use a simpler approach: when the macro expansion result
 * is a pair-chain, we run lizard_reparse_datum on it which walks the
 * pair-chain and reconstructs specialized AST nodes by recognizing
 * special heads (`lambda`, `if`, etc.). This skips the text-rendering
 * roundtrip and keeps everything in-memory. */
lizard_ast_node_t *lizard_reparse_datum(lizard_ast_node_t *n,
                                        lizard_heap_t *heap);

static lz_list_t *pair_to_app_args(lizard_ast_node_t *list_form,
                                   lizard_heap_t *heap) {
  lz_list_t *args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  lizard_ast_node_t *cur;
  for (cur = list_form; cur && cur->type == AST_PAIR;
       cur = cur->data.pair.cdr) {
    lizard_ast_list_node_t *wrap =
        lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    wrap->ast = lizard_reparse_datum(cur->data.pair.car, heap);
    list_append(args, &wrap->node);
  }
  return args;
}

lizard_ast_node_t *lizard_reparse_datum(lizard_ast_node_t *n,
                                        lizard_heap_t *heap) {
  if (!n) return NULL;
  if (n->type != AST_PAIR) return n;
  {
    lizard_ast_node_t *head = n->data.pair.car;
    if (head && head->type == AST_SYMBOL) {
      const char *h = head->data.variable;
      /* (lambda (params) body...) */
      if (strcmp(h, "lambda") == 0 &&
          n->data.pair.cdr && n->data.pair.cdr->type == AST_PAIR) {
        lizard_ast_node_t *params_form = n->data.pair.cdr->data.pair.car;
        lizard_ast_node_t *body_form = n->data.pair.cdr->data.pair.cdr;
        lizard_ast_node_t *lam =
            lizard_heap_alloc(sizeof(lizard_ast_node_t));
        lz_list_t *plist;
        lizard_ast_list_node_t *pwrap;
        lizard_ast_node_t *params_app;
        lizard_ast_node_t *cur;
        lam->type = AST_LAMBDA;
        plist = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
        /* parameter spec — turn `(a b c)` pair-chain into an
           AST_APPLICATION containing AST_SYMBOL elements; varargs
           single-symbol becomes itself. */
        if (params_form && params_form->type == AST_SYMBOL) {
          pwrap = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
          pwrap->ast = params_form;
          list_append(plist, &pwrap->node);
        } else if (params_form && params_form->type == AST_NIL) {
          pwrap = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
          pwrap->ast = params_form;
          list_append(plist, &pwrap->node);
        } else {
          params_app = lizard_heap_alloc(sizeof(lizard_ast_node_t));
          params_app->type = AST_APPLICATION;
          params_app->data.application_arguments =
              list_create_alloc(lizard_heap_alloc, lizard_heap_free);
          for (cur = params_form; cur && cur->type == AST_PAIR;
               cur = cur->data.pair.cdr) {
            lizard_ast_list_node_t *w =
                lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
            w->ast = cur->data.pair.car;  /* leave symbols as-is */
            list_append(params_app->data.application_arguments, &w->node);
          }
          pwrap = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
          pwrap->ast = params_app;
          list_append(plist, &pwrap->node);
        }
        /* body */
        for (cur = body_form; cur && cur->type == AST_PAIR;
             cur = cur->data.pair.cdr) {
          lizard_ast_list_node_t *w =
              lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
          w->ast = lizard_reparse_datum(cur->data.pair.car, heap);
          list_append(plist, &w->node);
        }
        lam->data.lambda.parameters = plist;
        lam->data.lambda.closure_env = NULL;
        return lam;
      }
      /* (if pred cons alt) */
      if (strcmp(h, "if") == 0 &&
          n->data.pair.cdr && n->data.pair.cdr->type == AST_PAIR) {
        lizard_ast_node_t *iff =
            lizard_heap_alloc(sizeof(lizard_ast_node_t));
        lizard_ast_node_t *rest = n->data.pair.cdr;
        iff->type = AST_IF;
        iff->data.if_clause.pred =
            lizard_reparse_datum(rest->data.pair.car, heap);
        rest = rest->data.pair.cdr;
        iff->data.if_clause.cons =
            (rest && rest->type == AST_PAIR)
                ? lizard_reparse_datum(rest->data.pair.car, heap)
                : lizard_make_nil(heap);
        if (rest && rest->type == AST_PAIR) rest = rest->data.pair.cdr;
        iff->data.if_clause.alt =
            (rest && rest->type == AST_PAIR)
                ? lizard_reparse_datum(rest->data.pair.car, heap)
                : NULL;
        return iff;
      }
      /* (begin body...) */
      if (strcmp(h, "begin") == 0) {
        lizard_ast_node_t *bn =
            lizard_heap_alloc(sizeof(lizard_ast_node_t));
        lizard_ast_node_t *cur;
        bn->type = AST_BEGIN;
        bn->data.begin_expressions =
            list_create_alloc(lizard_heap_alloc, lizard_heap_free);
        for (cur = n->data.pair.cdr; cur && cur->type == AST_PAIR;
             cur = cur->data.pair.cdr) {
          lizard_ast_list_node_t *w =
              lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
          w->ast = lizard_reparse_datum(cur->data.pair.car, heap);
          list_append(bn->data.begin_expressions, &w->node);
        }
        return bn;
      }
      /* (set! var val) */
      if (strcmp(h, "set!") == 0 &&
          n->data.pair.cdr && n->data.pair.cdr->type == AST_PAIR &&
          n->data.pair.cdr->data.pair.cdr &&
          n->data.pair.cdr->data.pair.cdr->type == AST_PAIR) {
        lizard_ast_node_t *as =
            lizard_heap_alloc(sizeof(lizard_ast_node_t));
        as->type = AST_ASSIGNMENT;
        as->data.assignment.variable = n->data.pair.cdr->data.pair.car;
        as->data.assignment.value = lizard_reparse_datum(
            n->data.pair.cdr->data.pair.cdr->data.pair.car, heap);
        return as;
      }
      /* (define name val) — only the simple form. */
      if (strcmp(h, "define") == 0 &&
          n->data.pair.cdr && n->data.pair.cdr->type == AST_PAIR &&
          n->data.pair.cdr->data.pair.cdr &&
          n->data.pair.cdr->data.pair.cdr->type == AST_PAIR) {
        lizard_ast_node_t *de =
            lizard_heap_alloc(sizeof(lizard_ast_node_t));
        lizard_ast_node_t *name = n->data.pair.cdr->data.pair.car;
        lizard_ast_node_t *val =
            n->data.pair.cdr->data.pair.cdr->data.pair.car;
        if (name->type == AST_SYMBOL) {
          de->type = AST_DEFINITION;
          de->data.definition.variable = name;
          de->data.definition.value = lizard_reparse_datum(val, heap);
          return de;
        }
      }
      /* (let ((n1 v1) ...) body...) — desugar to ((lambda (n1 ...)
         body...) v1 ...). */
      if (strcmp(h, "let") == 0 &&
          n->data.pair.cdr && n->data.pair.cdr->type == AST_PAIR) {
        lizard_ast_node_t *bindings = n->data.pair.cdr->data.pair.car;
        lizard_ast_node_t *body_form = n->data.pair.cdr->data.pair.cdr;
        lizard_ast_node_t *lam;
        lz_list_t *params_list;
        lizard_ast_node_t *params_app;
        lizard_ast_node_t *app;
        lizard_ast_list_node_t *fwrap;
        lizard_ast_node_t *cur;
        /* Build lambda. */
        lam = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        lam->type = AST_LAMBDA;
        params_list = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
        params_app = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        params_app->type = AST_APPLICATION;
        params_app->data.application_arguments =
            list_create_alloc(lizard_heap_alloc, lizard_heap_free);
        for (cur = bindings; cur && cur->type == AST_PAIR;
             cur = cur->data.pair.cdr) {
          lizard_ast_list_node_t *pw =
              lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
          pw->ast = cur->data.pair.car->data.pair.car; /* name */
          list_append(params_app->data.application_arguments, &pw->node);
        }
        fwrap = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
        fwrap->ast = params_app;
        list_append(params_list, &fwrap->node);
        for (cur = body_form; cur && cur->type == AST_PAIR;
             cur = cur->data.pair.cdr) {
          lizard_ast_list_node_t *bw =
              lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
          bw->ast = lizard_reparse_datum(cur->data.pair.car, heap);
          list_append(params_list, &bw->node);
        }
        lam->data.lambda.parameters = params_list;
        lam->data.lambda.closure_env = NULL;
        /* Build application: (lambda v1 v2 ...) */
        app = lizard_heap_alloc(sizeof(lizard_ast_node_t));
        app->type = AST_APPLICATION;
        app->data.application_arguments =
            list_create_alloc(lizard_heap_alloc, lizard_heap_free);
        {
          lizard_ast_list_node_t *lw =
              lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
          lw->ast = lam;
          list_append(app->data.application_arguments, &lw->node);
        }
        for (cur = bindings; cur && cur->type == AST_PAIR;
             cur = cur->data.pair.cdr) {
          lizard_ast_list_node_t *vw =
              lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
          vw->ast = lizard_reparse_datum(
              cur->data.pair.car->data.pair.cdr->data.pair.car, heap);
          list_append(app->data.application_arguments, &vw->node);
        }
        return app;
      }
      /* (quote x) — re-wrap */
      if (strcmp(h, "quote") == 0 &&
          n->data.pair.cdr && n->data.pair.cdr->type == AST_PAIR) {
        lizard_ast_node_t *q =
            lizard_heap_alloc(sizeof(lizard_ast_node_t));
        q->type = AST_QUOTE;
        q->data.quoted = n->data.pair.cdr->data.pair.car;
        return q;
      }
    }
    /* Generic application. */
    {
      lizard_ast_node_t *app =
          lizard_heap_alloc(sizeof(lizard_ast_node_t));
      app->type = AST_APPLICATION;
      app->data.application_arguments = pair_to_app_args(n, heap);
      return app;
    }
  }
}

/* ---- Phase 3: built-in module forms ----
 * module/export/import are rewritten here, in the macro expander, into calls to
 * the lz:* primitives with their payloads quoted.  Doing it structurally (not
 * via syntax-rules) gives full control and works inside hermetic module bodies
 * with no bootstrap.  The module body is passed as a pair-chain of the parsed
 * body forms (still typed AST); lz:module re-expands and evaluates each one in
 * the module's own environment. */
static lizard_ast_node_t *mod_qsym(lizard_heap_t *heap, const char *s) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_SYMBOL;
  n->data.variable = s;
  return n;
}
static lizard_ast_node_t *mod_quote(lizard_heap_t *heap, lizard_ast_node_t *d) {
  lizard_ast_node_t *q = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  q->type = AST_QUOTE;
  q->data.quoted = d;
  return q;
}
static void mod_push(lz_list_t *L, lizard_ast_node_t *ast, lizard_heap_t *heap) {
  lizard_ast_list_node_t *w = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
  w->ast = ast;
  list_append(L, &w->node);
  (void)heap;
}
static lizard_ast_node_t *mod_call(lizard_heap_t *heap, const char *op,
                                   lizard_ast_node_t *a1, lizard_ast_node_t *a2) {
  lz_list_t *L = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  lizard_ast_node_t *app = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  mod_push(L, mod_qsym(heap, op), heap);
  mod_push(L, a1, heap);
  if (a2 != NULL) mod_push(L, a2, heap);
  app->type = AST_APPLICATION;
  app->data.application_arguments = L;
  return app;
}
static lizard_ast_node_t *expand_module_form(lizard_ast_node_t *node,
                                             lizard_heap_t *heap) {
  lz_list_t *args = node->data.application_arguments;
  lz_list_node_t *it = args->head;            /* operator "module" */
  lizard_ast_node_t *name_ast;
  lz_list_t *body;
  if (it == args->nil || it->next == args->nil) return node;
  it = it->next;                               /* module name */
  name_ast = ((lizard_ast_list_node_t *)it)->ast;
  body = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (it = it->next; it != args->nil; it = it->next)
    mod_push(body, ((lizard_ast_list_node_t *)it)->ast, heap);
  return mod_call(heap, "lz:module", mod_quote(heap, name_ast),
                  mod_quote(heap, lizard_app_to_pair(body, heap)));
}
static lizard_ast_node_t *expand_export_form(lizard_ast_node_t *node,
                                             lizard_heap_t *heap) {
  lz_list_t *args = node->data.application_arguments;
  lz_list_node_t *it;
  lz_list_t *names = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  if (args->head == args->nil) return node;
  for (it = args->head->next; it != args->nil; it = it->next)
    mod_push(names, ((lizard_ast_list_node_t *)it)->ast, heap);
  return mod_call(heap, "lz:export",
                  mod_quote(heap, lizard_app_to_pair(names, heap)), NULL);
}
static lizard_ast_node_t *expand_import_form(lizard_ast_node_t *node,
                                             lizard_heap_t *heap) {
  lz_list_t *args = node->data.application_arguments;
  lz_list_node_t *it;
  lz_list_t *all = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  if (args->head == args->nil) return node;
  /* datumize each option so a list like :only (a b) becomes a pair-chain. */
  for (it = args->head->next; it != args->nil; it = it->next)
    mod_push(all, lizard_datumize(((lizard_ast_list_node_t *)it)->ast, heap), heap);
  return mod_call(heap, "lz:import",
                  mod_quote(heap, lizard_app_to_pair(all, heap)), NULL);
}

lizard_ast_node_t *lizard_expand_macros(lizard_ast_node_t *node,
                                        lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lz_list_node_t *first, *arg, *expr;
  lizard_ast_node_t *operator, * binding, *expanded;
  lizard_ast_list_node_t *copy;
  lz_list_t *macro_args;
  if (!node) {
    return NULL;
  }
  if (node->type == AST_APPLICATION) {
    first = node->data.application_arguments->head;
    if (first != node->data.application_arguments->nil) {
      operator=((lizard_ast_list_node_t *)first)->ast;
      if (operator->type == AST_SYMBOL) {
        if (strcmp(operator->data.variable, "module") == 0)
          return expand_module_form(node, heap);
        if (strcmp(operator->data.variable, "export") == 0)
          return expand_export_form(node, heap);
        if (strcmp(operator->data.variable, "import") == 0)
          return expand_import_form(node, heap);
        binding = lizard_env_lookup(env, operator->data.variable);
        if (binding && binding->type == AST_MACRO) {
          lizard_ast_node_t *transformer = binding->data.macro_def.transformer;
          /* syntax-rules transformer: try each clause in order. */
          if (transformer && transformer->type == AST_SYNTAX_RULES) {
            lz_list_node_t *cit;
            lizard_ast_node_t *call_pair;
            /* Convert the macro call's argument AST list into a
               pair-chain so it can be matched against the pattern
               (which is datum-shaped). */
            {
              lz_list_t *call_args =
                  list_create_alloc(lizard_heap_alloc, lizard_heap_free);
              for (arg = first->next;
                   arg != node->data.application_arguments->nil;
                   arg = arg->next) {
                lizard_ast_list_node_t *w =
                    lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
                w->ast = lizard_datumize(
                    ((lizard_ast_list_node_t *)arg)->ast, heap);
                list_append(call_args, &w->node);
              }
              call_pair = lizard_app_to_pair(call_args, heap);
            }
            for (cit = transformer->data.syntax_rules.clauses->head;
                 cit != transformer->data.syntax_rules.clauses->nil;
                 cit = cit->next) {
              lizard_ast_node_t *clause =
                  ((lizard_ast_list_node_t *)cit)->ast;
              lizard_ast_node_t *pattern;
              lizard_ast_node_t *template;
              lizard_ast_node_t *pat_args;
              lizard_sr_binding_t *bs = NULL;
              if (!clause || clause->type != AST_PAIR ||
                  !clause->data.pair.cdr ||
                  clause->data.pair.cdr->type != AST_PAIR) {
                continue;
              }
              pattern = clause->data.pair.car;
              template = clause->data.pair.cdr->data.pair.car;
              /* The pattern's first element is the macro name slot —
                 skip it and match the rest against the call's args. */
              if (!pattern || pattern->type != AST_PAIR) continue;
              pat_args = pattern->data.pair.cdr;
              if (lizard_sr_match(pat_args, call_pair,
                                  transformer->data.syntax_rules.literals,
                                  &bs, heap)) {
                lizard_ast_node_t *expanded_datum =
                    lizard_sr_expand(template, bs, NULL, heap);
                lizard_ast_node_t *reparsed =
                    lizard_reparse_datum(expanded_datum, heap);
                return lizard_expand_macros(reparsed, env, heap);
              }
            }
            /* No clause matched — leave the form alone. */
          } else {
            /* Legacy lambda-transformer macro. */
            macro_args =
                list_create_alloc(lizard_heap_alloc, lizard_heap_free);
            for (arg = first->next;
                 arg != node->data.application_arguments->nil;
                 arg = arg->next) {
              copy = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
              copy->ast = ((lizard_ast_list_node_t *)arg)->ast;
              list_append(macro_args, &copy->node);
            }
            expanded = lizard_apply(transformer, macro_args, env, heap,
                                    lizard_identity_cont);
            while (expanded && expanded->type == AST_QUOTE) {
              expanded = expanded->data.quoted;
            }
            return lizard_expand_macros(expanded, env, heap);
          }
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
    lz_list_node_t *body_node;
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

/* (macroexpand-1 form) — perform a SINGLE syntax-rules expansion step on a
 * datum `form`.  If form is (m args...) with m a syntax-rules macro in scope,
 * returns the template instantiated once (hygienically, via the same matcher
 * and expander the full expander uses); otherwise returns form unchanged.
 * The result is a datum, so it can be displayed and fed back in — this is what
 * the macro stepper iterates over (Phase 4 tooling). */
lizard_ast_node_t *lizard_primitive_macroexpand1(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *form, *head, *binding, *transformer, *call_pair;
  lz_list_node_t *cit;
  if (!args || args->head == args->nil) return lizard_make_nil(heap);
  form = ((lizard_ast_list_node_t *)args->head)->ast;
  if (!form || form->type != AST_PAIR) return form;
  head = form->data.pair.car;
  if (!head || head->type != AST_SYMBOL) return form;
  binding = lizard_env_lookup(env, head->data.variable);
  if (!binding || binding->type != AST_MACRO) return form;
  transformer = binding->data.macro_def.transformer;
  if (!transformer || transformer->type != AST_SYNTAX_RULES) return form;
  call_pair = form->data.pair.cdr;
  for (cit = transformer->data.syntax_rules.clauses->head;
       cit != transformer->data.syntax_rules.clauses->nil; cit = cit->next) {
    lizard_ast_node_t *clause = ((lizard_ast_list_node_t *)cit)->ast;
    lizard_ast_node_t *pattern, *template, *pat_args;
    lizard_sr_binding_t *bs = NULL;
    if (!clause || clause->type != AST_PAIR || !clause->data.pair.cdr ||
        clause->data.pair.cdr->type != AST_PAIR)
      continue;
    pattern = clause->data.pair.car;
    template = clause->data.pair.cdr->data.pair.car;
    if (!pattern || pattern->type != AST_PAIR) continue;
    pat_args = pattern->data.pair.cdr;
    if (lizard_sr_match(pat_args, call_pair,
                        transformer->data.syntax_rules.literals, &bs, heap)) {
      return lizard_sr_expand(template, bs, NULL, heap);
    }
  }
  return form; /* no clause matched: leave unchanged */
}

lizard_ast_node_t *lizard_expand_quasiquote(lizard_ast_node_t *node,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lz_list_t *expanded_list;
  lz_list_node_t *iter;
  lizard_ast_list_node_t *elem;
  lizard_ast_node_t *expanded_elem;
  lizard_ast_list_node_t *new_elem;
  lizard_ast_node_t *new_node;
  if (!node) {
    return NULL;
  }
  switch (node->type) {
  case AST_NUMBER:
  case AST_RATIONAL:
  case AST_REAL:
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
          lz_list_node_t *splice_iter;
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
    lz_list_t *exprs;
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
    lz_list_t *params;
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
