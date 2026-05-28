#include "primitives.h"
#include "env.h"
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "parser.h"
#include "printer.h"
#include "runtime.h"
#include "tokenizer.h"
#include <setjmp.h>
#include <stdint.h>
/* Phase 0: callcc state accessed via heap->runtime. Fallback
 * globals defined in lizard.c for standalone-heap compat. */
extern jmp_buf callcc_buf_fallback;
extern int callcc_active_fallback;
extern lizard_ast_node_t *callcc_value_fallback;

/* Forward declarations for arg-count validators (defined later). */
static int single_arg(lz_list_t *args);
static int two_args(lz_list_t *args);
static int three_args(lz_list_t *args);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static const char *lizard_heap_strdup(lizard_heap_t *heap, const char *src) {
  char *dst;
  size_t len;
  if (src == NULL) {
    return NULL;
  }
  len = strlen(src) + 1U;
  dst = lizard_heap_alloc(len);
  memcpy(dst, src, len);
  return dst;
}

static lizard_ast_node_t *lizard_make_number_copy(lizard_heap_t *heap,
                                                   lizard_ast_node_t *source) {
  lizard_ast_node_t *copy;
  copy = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  copy->type = AST_NUMBER;
  mpz_init(copy->data.number);
  mpz_set(copy->data.number, source->data.number);
  return copy;
}

lizard_ast_node_t *lizard_primitive_plus(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lz_list_node_t *node;
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *arg, *result;
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init(result->data.number);

  node = args->head;
  if (node == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGC);
  }
  while (node != args->nil) {
    arg_node = (lizard_ast_list_node_t *)node;
    arg = arg_node->ast;
    if (arg->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
    }
    mpz_add(result->data.number, result->data.number, arg->data.number);
    node = node->next;
  }

  return result;
}

lizard_ast_node_t *lizard_primitive_minus(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lz_list_node_t *node;
  lizard_ast_list_node_t *first_arg_node, *arg_node;
  lizard_ast_node_t *acc, *arg;

  node = args->head;
  if (node == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_MINUS_ARGC);
  }
  first_arg_node = (lizard_ast_list_node_t *)node;
  arg = first_arg_node->ast;
  if (arg->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_MINUS_ARGT);
  }
  /* Fast path: exactly two number arguments — subtract directly
   * into a fresh result without copying the first operand. */
  if (node->next != args->nil && node->next->next == args->nil) {
    lizard_ast_node_t *b = ((lizard_ast_list_node_t *)node->next)->ast;
    if (b->type == AST_NUMBER) {
      lizard_ast_node_t *result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      result->type = AST_NUMBER;
      mpz_init(result->data.number);
      mpz_sub(result->data.number, arg->data.number, b->data.number);
      return result;
    }
  }
  acc = lizard_make_number_copy(heap, arg);
  node = node->next;
  if (node == args->nil) {
    mpz_neg(acc->data.number, acc->data.number);
    return acc;
  }
  while (node != args->nil) {
    arg_node = (lizard_ast_list_node_t *)node;
    arg = arg_node->ast;
    if (arg->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_MINUS_ARGT_2);
    }
    mpz_sub(acc->data.number, acc->data.number, arg->data.number);
    node = node->next;
  }
  return acc;
}

lizard_ast_node_t *lizard_primitive_multiply(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lz_list_node_t *node;
  lizard_ast_list_node_t *first_arg_node;
  lizard_ast_node_t *acc;
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *arg;

  node = args->head;
  if (node == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_MUL_ARGC);
  }
  first_arg_node = (lizard_ast_list_node_t *)node;
  acc = first_arg_node->ast;
  if (acc->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_MUL_ARGT);
  }
  /* Fast path: exactly two number arguments. Avoid copying the
   * (potentially huge) accumulator — multiply directly into a fresh
   * result. This is the dominant case for bignum power loops. */
  if (node->next != args->nil && node->next->next == args->nil) {
    lizard_ast_node_t *b = ((lizard_ast_list_node_t *)node->next)->ast;
    if (b->type == AST_NUMBER) {
      lizard_ast_node_t *result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      result->type = AST_NUMBER;
      mpz_init(result->data.number);
      mpz_mul(result->data.number, acc->data.number, b->data.number);
      return result;
    }
  }
  acc = lizard_make_number_copy(heap, acc);
  node = node->next;
  while (node != args->nil) {
    arg_node = (lizard_ast_list_node_t *)node;
    arg = arg_node->ast;
    if (arg->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_MUL_ARGT_2);
    }
    mpz_mul(acc->data.number, acc->data.number, arg->data.number);
    node = node->next;
  }
  return acc;
}

lizard_ast_node_t *lizard_primitive_divide(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lz_list_node_t *node;
  lizard_ast_list_node_t *first_arg_node, *arg_node;
  lizard_ast_node_t *acc, *arg;

  node = args->head;
  if (node == args->nil || node->next == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_DIV_ARGC);
  }
  first_arg_node = (lizard_ast_list_node_t *)node;
  acc = first_arg_node->ast;
  if (acc->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_DIV_ARGT);
  }
  acc = lizard_make_number_copy(heap, acc);
  node = node->next;
  while (node != args->nil) {
    arg_node = (lizard_ast_list_node_t *)node;
    arg = arg_node->ast;
    if (arg->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_DIV_ARGT_2);
    }
    if (mpz_cmp_ui(arg->data.number, 0) == 0) {
      return lizard_make_error(heap, LIZARD_ERROR_DIV_ZERO);
    }
    mpz_tdiv_q(acc->data.number, acc->data.number, arg->data.number);
    node = node->next;
  }
  return acc;
}

int lizard_is_empty_list(lizard_ast_node_t *node) {
  if (!node)
    return 1;
  if (node->type == AST_NIL)
    return 1;
  if (node->type == AST_APPLICATION) {
    lz_list_t *args = node->data.application_arguments;
    if (args->head == args->nil)
      return 1;
  }
  return 0;
}

int lizard_ast_equal(lizard_ast_node_t *a, lizard_ast_node_t *b) {
  if (a == b)
    return 1;
  if (!a || !b)
    return 0;

  if (lizard_is_empty_list(a) && lizard_is_empty_list(b))
    return 1;

  if (a->type != b->type) {
    if ((a->type == AST_NIL && lizard_is_empty_list(b)) ||
        (b->type == AST_NIL && lizard_is_empty_list(a))) {
      return 1;
    }
    return 0;
  }

  switch (a->type) {
  case AST_NUMBER:
    return (mpz_cmp(a->data.number, b->data.number) == 0);
  case AST_SYMBOL:
    return (strcmp(a->data.variable, b->data.variable) == 0);
  case AST_BOOL:
    return (a->data.boolean == b->data.boolean);
  case AST_STRING:
    return (strcmp(a->data.string, b->data.string) == 0);
  case AST_NIL:
    return 1;
  case AST_QUOTE:
  case AST_QUASIQUOTE:
  case AST_UNQUOTE:
  case AST_UNQUOTE_SPLICING:
    return lizard_ast_equal(a->data.quoted, b->data.quoted);
  case AST_ASSIGNMENT:
    return lizard_ast_equal(a->data.assignment.variable,
                            b->data.assignment.variable) &&
           lizard_ast_equal(a->data.assignment.value, b->data.assignment.value);
  case AST_DEFINITION:
    return lizard_ast_equal(a->data.definition.variable,
                            b->data.definition.variable) &&
           lizard_ast_equal(a->data.definition.value, b->data.definition.value);
  case AST_IF:
    return lizard_ast_equal(a->data.if_clause.pred, b->data.if_clause.pred) &&
           lizard_ast_equal(a->data.if_clause.cons, b->data.if_clause.cons) &&
           lizard_ast_equal(a->data.if_clause.alt, b->data.if_clause.alt);
  case AST_LAMBDA: {
    lz_list_t *params_a = a->data.lambda.parameters;
    lz_list_t *params_b = b->data.lambda.parameters;
    lz_list_node_t *pa = params_a->head, *pb = params_b->head;
    while (pa != params_a->nil && pb != params_b->nil) {
      lizard_ast_list_node_t *pna = (lizard_ast_list_node_t *)pa;
      lizard_ast_list_node_t *pnb = (lizard_ast_list_node_t *)pb;
      if (!lizard_ast_equal(pna->ast, pnb->ast))
        return 0;
      pa = pa->next;
      pb = pb->next;
    }
    if (pa != params_a->nil || pb != params_b->nil)
      return 0;
    return 1;
  }
  case AST_BEGIN: {
    lz_list_t *exprs_a = a->data.begin_expressions;
    lz_list_t *exprs_b = b->data.begin_expressions;
    lz_list_node_t *na = exprs_a->head, *nb = exprs_b->head;
    while (na != exprs_a->nil && nb != exprs_b->nil) {
      lizard_ast_list_node_t *ena = (lizard_ast_list_node_t *)na;
      lizard_ast_list_node_t *enb = (lizard_ast_list_node_t *)nb;
      if (!lizard_ast_equal(ena->ast, enb->ast))
        return 0;
      na = na->next;
      nb = nb->next;
    }
    return (na == exprs_a->nil && nb == exprs_b->nil);
  }
  case AST_APPLICATION: {
    lz_list_t *args_a = a->data.application_arguments;
    lz_list_t *args_b = b->data.application_arguments;
    lz_list_node_t *na = args_a->head, *nb = args_b->head;
    while (na != args_a->nil && nb != args_b->nil) {
      lizard_ast_list_node_t *ena = (lizard_ast_list_node_t *)na;
      lizard_ast_list_node_t *enb = (lizard_ast_list_node_t *)nb;
      if (!lizard_ast_equal(ena->ast, enb->ast))
        return 0;
      na = na->next;
      nb = nb->next;
    }
    return (na == args_a->nil && nb == args_b->nil);
  }
  case AST_PRIMITIVE:
    return (a->data.primitive == b->data.primitive);
  case AST_MACRO:
    return lizard_ast_equal(a->data.macro_def.variable,
                            b->data.macro_def.variable) &&
           lizard_ast_equal(a->data.macro_def.transformer,
                            b->data.macro_def.transformer);
  case AST_PROMISE:
    if (a->data.promise.forced && b->data.promise.forced)
      return lizard_ast_equal(a->data.promise.value, b->data.promise.value);
    return lizard_ast_equal(a->data.promise.expr, b->data.promise.expr);
  case AST_CONTINUATION:
    return (a->data.continuation.captured_cont ==
            b->data.continuation.captured_cont);
  case AST_CALLCC:
    return (a == b);
  case AST_ERROR:
    if (a->data.error.code != b->data.error.code)
      return 0;
    return lizard_ast_equal((lizard_ast_node_t *)a->data.error.data,
                            (lizard_ast_node_t *)b->data.error.data);
  default:
    return (a == b);
  }
}

lizard_ast_node_t *lizard_primitive_equal(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lz_list_node_t *node;
  lizard_ast_list_node_t *first_node;
  lizard_ast_node_t *first;
  lizard_ast_node_t *result;
  int eq = 1;
  if (args->head == args->nil || args->head->next == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_EQ_ARGC);
  }
  first_node = (lizard_ast_list_node_t *)args->head;
  first = first_node->ast;
  for (node = args->head->next; node != args->nil; node = node->next) {
    lizard_ast_list_node_t *other_node = (lizard_ast_list_node_t *)node;
    lizard_ast_node_t *other = other_node->ast;
    if (!lizard_ast_equal(first, other)) {
      eq = 0;
      break;
    }
  }
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_BOOL;
  result->data.boolean = (eq != 0);
  return result;
}

lizard_ast_node_t *lizard_primitive_pow(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *base_node, *exp_node, *ret;
  unsigned long exp;

  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_POW_ARGC);
  }

  arg_node = CAST(args->head, lizard_ast_list_node_t);
  base_node = arg_node->ast;
  arg_node = CAST(args->head->next, lizard_ast_list_node_t);
  exp_node = arg_node->ast;

  if (base_node->type != AST_NUMBER || exp_node->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_POW_ARGT);
  }

  exp = mpz_get_ui(exp_node->data.number);

  ret = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  ret->type = AST_NUMBER;
  mpz_init(ret->data.number);
  mpz_pow_ui(ret->data.number, base_node->data.number, exp);

  return ret;
}

lizard_ast_node_t *lizard_primitive_lt(lz_list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *prev, *current;
  lz_list_node_t *node;
  int cmp;

  if (args->head == args->nil || args->head->next == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_LT_ARGC);
  }

  arg_node = CAST(args->head, lizard_ast_list_node_t);
  prev = arg_node->ast;
  if (prev->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_LT_ARGT);
  }

  node = args->head->next;
  while (node != args->nil) {
    arg_node = CAST(node, lizard_ast_list_node_t);
    current = arg_node->ast;
    if (current->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_LT_ARGT_2);
    }
    cmp = mpz_cmp(prev->data.number, current->data.number);
    if (cmp >= 0) {
      return lizard_make_bool(heap, 0);
    }
    prev = current;
    node = node->next;
  }
  return lizard_make_bool(heap, 1);
}

lizard_ast_node_t *lizard_primitive_le(lz_list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *prev, *current;
  lz_list_node_t *node;
  int cmp;

  if (args->head == args->nil || args->head->next == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_LE_ARGC);
  }

  arg_node = CAST(args->head, lizard_ast_list_node_t);
  prev = arg_node->ast;
  if (prev->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_LE_ARGT);
  }

  node = args->head->next;
  while (node != args->nil) {
    arg_node = CAST(node, lizard_ast_list_node_t);
    current = arg_node->ast;
    if (current->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_LE_ARGT_2);
    }
    cmp = mpz_cmp(prev->data.number, current->data.number);
    if (cmp > 0) {
      return lizard_make_bool(heap, 0);
    }
    prev = current;
    node = node->next;
  }
  return lizard_make_bool(heap, 1);
}

lizard_ast_node_t *lizard_primitive_gt(lz_list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *prev, *current;
  lz_list_node_t *node;
  int cmp;

  if (args->head == args->nil || args->head->next == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_GT_ARGC);
  }

  arg_node = CAST(args->head, lizard_ast_list_node_t);
  prev = arg_node->ast;
  if (prev->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_GT_ARGT);
  }

  node = args->head->next;
  while (node != args->nil) {
    arg_node = CAST(node, lizard_ast_list_node_t);
    current = arg_node->ast;
    if (current->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_GT_ARGT_2);
    }
    cmp = mpz_cmp(prev->data.number, current->data.number);
    if (cmp <= 0) {
      return lizard_make_bool(heap, 0);
    }
    prev = current;
    node = node->next;
  }
  return lizard_make_bool(heap, 1);
}

lizard_ast_node_t *lizard_primitive_ge(lz_list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *prev, *current;
  lz_list_node_t *node;
  int cmp;

  if (args->head == args->nil || args->head->next == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_GE_ARGC);
  }

  arg_node = CAST(args->head, lizard_ast_list_node_t);
  prev = arg_node->ast;
  if (prev->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_GE_ARGT);
  }

  node = args->head->next;
  while (node != args->nil) {
    arg_node = CAST(node, lizard_ast_list_node_t);
    current = arg_node->ast;
    if (current->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_GE_ARGT_2);
    }
    cmp = mpz_cmp(prev->data.number, current->data.number);
    if (cmp < 0) {
      return lizard_make_bool(heap, 0);
    }
    prev = current;
    node = node->next;
  }
  return lizard_make_bool(heap, 1);
}

lizard_ast_node_t *lizard_primitive_mod(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *dividend, *divisor, *ret;

  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_MOD_ARGC);
  }

  arg_node = CAST(args->head, lizard_ast_list_node_t);
  dividend = arg_node->ast;
  arg_node = CAST(args->head->next, lizard_ast_list_node_t);
  divisor = arg_node->ast;

  if (dividend->type != AST_NUMBER || divisor->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_MOD_ARGT);
  }
  if (mpz_cmp_ui(divisor->data.number, 0) == 0) {
    return lizard_make_error(heap, LIZARD_ERROR_DIV_ZERO);
  }

  ret = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  ret->type = AST_NUMBER;
  mpz_init(ret->data.number);
  mpz_mod(ret->data.number, dividend->data.number, divisor->data.number);

  return ret;
}

lizard_ast_node_t *lizard_primitive_cons(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_list_node_t *car_node, *cdr_node;
  lizard_ast_node_t *node;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CONS_ARGC);
  }

  car_node = (lizard_ast_list_node_t *)args->head;
  cdr_node = (lizard_ast_list_node_t *)args->head->next;

  node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  node->type = AST_PAIR;

  node->data.pair.car = lizard_ast_deep_copy(car_node->ast, heap);
  node->data.pair.cdr = lizard_ast_deep_copy(cdr_node->ast, heap);
  return node;
}

lizard_ast_node_t *lizard_primitive_car(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CAR_ARGC);
  }
  node = (lizard_ast_list_node_t *)args->head;
  if (node->ast->type == AST_NIL) {
    return lizard_make_error(heap, LIZARD_ERROR_CAR_NIL);
  }
  if (node->ast->type != AST_PAIR) {
    return lizard_make_error(heap, LIZARD_ERROR_CAR_ARGT);
  }
  return node->ast->data.pair.car;
}

lizard_ast_node_t *lizard_primitive_cdr(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CDR_ARGC);
  }
  node = (lizard_ast_list_node_t *)args->head;
  if (node->ast->type == AST_NIL) {
    return lizard_make_error(heap, LIZARD_ERROR_CDR_NIL);
  }
  if (node->ast->type != AST_PAIR) {
    return lizard_make_error(heap, LIZARD_ERROR_CDR_ARGT);
  }
  return node->ast->data.pair.cdr;
}

lizard_ast_node_t *lizard_make_primitive(lizard_heap_t *heap,
                                         lizard_primitive_func_t func) {
  lizard_ast_node_t *node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  node->type = AST_PRIMITIVE;
  node->data.primitive = func;
  return node;
}

lizard_ast_node_t *lizard_primitive_list(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *first, *rest;
  lz_list_t *rest_args, *cons_args;
  lizard_ast_list_node_t *first_node, *rest_node;
  if (args->head == args->nil) {
    return lizard_make_nil(heap);
  }

  first = CAST(args->head, lizard_ast_list_node_t)->ast;
  rest_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  rest_args->head = args->head->next;
  rest_args->nil = args->nil;
  rest = lizard_primitive_list(rest_args, env, heap);

  cons_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  first_node = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
  first_node->ast = first;
  list_append(cons_args, &first_node->node);
  rest_node = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
  rest_node->ast = rest;
  list_append(cons_args, &rest_node->node);

  return lizard_primitive_cons(cons_args, env, heap);
}

lizard_ast_node_t *lizard_primitive_tokens(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  const char *input;
  lz_list_t *tokens;
  lz_list_node_t *token_node;
  lizard_token_t *token;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_TOKENS_ARGC);
  }

  node = CAST(args->head, lizard_ast_list_node_t);
  if (node->ast->type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_TOKENS_ARGT);
  }

  input = node->ast->data.string;
  tokens = lizard_tokenize_source(input, "<tokens>", NULL);
  if (!tokens) {
    return lizard_make_error(heap, LIZARD_ERROR_TOKENIZATION);
  }

  for (token_node = tokens->head; token_node != tokens->nil;
       token_node = token_node->next) {
    token = &CAST(token_node, lizard_token_list_node_t)->token;
    switch (token->type) {
    case TOKEN_LEFT_PAREN:
      printf("TOKEN_LEFT_PAREN\n");
      break;
    case TOKEN_RIGHT_PAREN:
      printf("TOKEN_RIGHT_PAREN\n");
      break;
    case TOKEN_SYMBOL:
      printf("TOKEN_SYMBOL: %s\n", token->data.symbol);
      break;
    case TOKEN_NUMBER:
      gmp_printf("TOKEN_NUMBER: %Zd\n", token->data.number);
      break;
    case TOKEN_STRING:
      printf("TOKEN_STRING: \"%s\"\n", token->data.string);
      break;
    default:
      printf("Unknown token type.\n");
    }
  }

  return lizard_make_nil(heap);
}

lizard_ast_node_t *lizard_primitive_ast(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  const char *input;
  lz_list_t *tokens, *ast_list;
  lz_list_node_t *current_node;

  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_AST_ARGC);
  }

  node = CAST(args->head, lizard_ast_list_node_t);
  if (node->ast->type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_AST_ARGT);
  }

  input = node->ast->data.string;
  tokens = lizard_tokenize_source(input, "<ast>", NULL);
  if (!tokens) {
    return lizard_make_error(heap, LIZARD_ERROR_TOKENIZATION);
  }
  ast_list = lizard_parse(tokens, heap);
  if (!ast_list) {
    return lizard_make_error(heap, LIZARD_ERROR_PARSE);
  }
  current_node = ast_list->head;
  while (current_node != ast_list->nil) {
    printf("=> ");
    lizard_print_ast(CAST(current_node, lizard_ast_list_node_t)->ast, 0);
    current_node = current_node->next;
  }
  return lizard_make_nil(heap);
}

lizard_ast_node_t *lizard_primitive_eval(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  lizard_ast_node_t *expr;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_EVAL_ARGC);
  }

  node = CAST(args->head, lizard_ast_list_node_t);
  expr = node->ast;
  return lizard_eval(expr, env, heap, lizard_identity_cont);
}

lizard_ast_node_t *lizard_primitive_unquote(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  lizard_ast_node_t *expr;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_UNQUOTE_ARGC);
  }

  node = CAST(args->head, lizard_ast_list_node_t);
  expr = node->ast;

  if (expr->type != AST_QUOTE) {
    return lizard_make_error(heap, LIZARD_ERROR_UNQUOTE_ARGT);
  }

  return expr->data.quoted;
}

lizard_ast_node_t *lizard_primitive_callcc(
    lz_list_t *args, lizard_env_t *env, lizard_heap_t *heap,
    lizard_ast_node_t *(*current_cont)(lizard_ast_node_t *, lizard_env_t *,
                                       lizard_heap_t *)) {
  lizard_ast_list_node_t *arg0;
  lizard_ast_node_t *proc;
  lizard_ast_node_t *cont_obj;
  lz_list_t *arg_list;
  lizard_ast_list_node_t *node_arg;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CALLCC_ARGC);
  }
  arg0 = (lizard_ast_list_node_t *)args->head;
  proc = arg0->ast;
  if (proc->type != AST_LAMBDA && proc->type != AST_PRIMITIVE) {
    return lizard_make_error(heap, LIZARD_ERROR_CALLCC_APPLY);
  }
  if (heap->runtime != NULL) {
    if (!heap->runtime->callcc_active) {
      heap->runtime->callcc_active = 1;
      if (setjmp(heap->runtime->callcc_buf) != 0) {
        heap->runtime->callcc_active = 0;
        return heap->runtime->callcc_value;
      }
    }
  } else {
    if (!callcc_active_fallback) {
      callcc_active_fallback = 1;
      if (setjmp(callcc_buf_fallback) != 0) {
        callcc_active_fallback = 0;
        return callcc_value_fallback;
      }
    }
  }
  cont_obj = lizard_make_continuation(current_cont, heap);

  arg_list = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  node_arg = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
  node_arg->ast = cont_obj; /* shallow copy */
  list_append(arg_list, &node_arg->node);

  return lizard_apply(proc, arg_list, env, heap, current_cont);
}

lizard_ast_node_t *lizard_identity_cont(lizard_ast_node_t *result,
                                        lizard_env_t *env,
                                        lizard_heap_t *heap) {
  (void)heap;
  (void)env;
  return result;
}

int lizard_is_false(lizard_ast_node_t *node) {
  if (!node) {
    return 1;
  }
  if (node->type == AST_NIL) {
    return 1;
  }
  if (node->type == AST_BOOL && node->data.boolean == 0) {
    return 1;
  }
  return 0;
}

int lizard_is_true(lizard_ast_node_t *node) { return !lizard_is_false(node); }

lizard_ast_node_t *lizard_primitive_nullp(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_list_node_t *node_arg;
  (void)env; /* unused */
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_NULLP_ARGC);
  }
  node_arg = (lizard_ast_list_node_t *)args->head;
  if (node_arg->ast->type == AST_NIL)
    return lizard_make_bool(heap, true);
  else
    return lizard_make_bool(heap, false);
}

lizard_ast_node_t *lizard_primitive_pairp(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_list_node_t *node_arg;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PAIRP_ARGC);
  }
  node_arg = (lizard_ast_list_node_t *)args->head;
  if (node_arg->ast->type == AST_PAIR)
    return lizard_make_bool(heap, true);
  else
    return lizard_make_bool(heap, false);
}

lizard_ast_node_t *lizard_primitive_stringp(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_list_node_t *node_arg;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_STRINGP_ARGC);
  }
  node_arg = (lizard_ast_list_node_t *)args->head;
  if (node_arg->ast->type == AST_STRING)
    return lizard_make_bool(heap, true);
  else
    return lizard_make_bool(heap, false);
}

lizard_ast_node_t *lizard_primitive_boolp(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_list_node_t *node_arg;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_BOOLP_ARGC);
  }
  node_arg = (lizard_ast_list_node_t *)args->head;
  if (node_arg->ast->type == AST_BOOL)
    return lizard_make_bool(heap, true);
  else
    return lizard_make_bool(heap, false);
}

lizard_ast_node_t *lizard_primitive_numberp(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_list_node_t *node_arg;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  node_arg = (lizard_ast_list_node_t *)args->head;
  return lizard_make_bool(heap, node_arg->ast->type == AST_NUMBER);
}

lizard_ast_node_t *lizard_primitive_symbolp(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_list_node_t *node_arg;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  node_arg = (lizard_ast_list_node_t *)args->head;
  return lizard_make_bool(heap, node_arg->ast->type == AST_SYMBOL);
}

lizard_ast_node_t *lizard_primitive_procedurep(lz_list_t *args, lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_list_node_t *node_arg;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  node_arg = (lizard_ast_list_node_t *)args->head;
  return lizard_make_bool(heap, node_arg->ast->type == AST_LAMBDA ||
                                    node_arg->ast->type == AST_PRIMITIVE);
}

lizard_ast_node_t *lizard_primitive_and(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lz_list_node_t *iter;
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *result = lizard_make_bool(heap, true);
  (void)env;
  for (iter = args->head; iter != args->nil; iter = iter->next) {
    arg_node = (lizard_ast_list_node_t *)iter;
    result = arg_node->ast;
    if (lizard_is_false(result)) {
      return result;
    }
  }
  return result;
}

lizard_ast_node_t *lizard_primitive_or(lz_list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap) {
  lz_list_node_t *iter;
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *result =
      lizard_make_bool(heap, false); /* default for no args */
  (void)env;
  for (iter = args->head; iter != args->nil; iter = iter->next) {
    arg_node = (lizard_ast_list_node_t *)iter;
    result = arg_node->ast;
    if (lizard_is_true(result)) {
      return result;
    }
  }
  return result;
}

lizard_ast_node_t *lizard_primitive_not(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_list_node_t *node_arg;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_NOT_ARGC);
  }
  node_arg = (lizard_ast_list_node_t *)args->head;
  if (lizard_is_false(node_arg->ast))
    return lizard_make_bool(heap, true);
  else
    return lizard_make_bool(heap, false);
}

lizard_ast_node_t *lizard_primitive_xor(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lz_list_node_t *iter;
  lizard_ast_list_node_t *arg_node;
  int true_count = 0;
  (void)env;
  for (iter = args->head; iter != args->nil; iter = iter->next) {
    arg_node = (lizard_ast_list_node_t *)iter;
    if (lizard_is_true(arg_node->ast))
      true_count++;
  }
  return lizard_make_bool(heap, (true_count % 2) != 0);
}

lizard_ast_node_t *lizard_primitive_nand(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *and_result = lizard_primitive_and(args, env, heap);
  return lizard_make_bool(heap, lizard_is_false(and_result));
}

lizard_ast_node_t *lizard_primitive_nor(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_node_t *or_result = lizard_primitive_or(args, env, heap);
  return lizard_make_bool(heap, lizard_is_false(or_result));
}

lizard_ast_node_t *lizard_primitive_xnor(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *xor_result = lizard_primitive_xor(args, env, heap);
  return lizard_make_bool(heap, lizard_is_false(xor_result));
}

lizard_ast_node_t *lizard_ast_deep_copy(lizard_ast_node_t *node,
                                        lizard_heap_t *heap) {
  lizard_ast_node_t *copy;
  if (!node) {
    return NULL;
  }
  copy = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  copy->type = node->type;
  copy->span = node->span;
  switch (node->type) {
  case AST_NUMBER:
    mpz_init(copy->data.number);
    mpz_set(copy->data.number, node->data.number);
    break;
  case AST_STRING:
    copy->data.string = lizard_heap_strdup(heap, node->data.string);
    break;
  case AST_SYMBOL:
    copy->data.variable = lizard_heap_strdup(heap, node->data.variable);
    break;
  case AST_BOOL:
    copy->data.boolean = node->data.boolean;
    break;
  case AST_NIL:
    return node;
  case AST_QUOTE:
  case AST_QUASIQUOTE:
  case AST_UNQUOTE:
  case AST_UNQUOTE_SPLICING:
    copy->data.quoted = lizard_ast_deep_copy(node->data.quoted, heap);
    break;
  case AST_APPLICATION:
    copy->data.application_arguments =
        list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    {
      lz_list_node_t *iter;
      for (iter = node->data.application_arguments->head;
           iter != node->data.application_arguments->nil; iter = iter->next) {
        lizard_ast_list_node_t *orig = (lizard_ast_list_node_t *)iter;
        lizard_ast_list_node_t *copy_node =
            lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
        copy_node->ast = lizard_ast_deep_copy(orig->ast, heap);
        list_append(copy->data.application_arguments, &copy_node->node);
      }
    }
    break;
  case AST_PAIR:

    copy->data.pair.car = lizard_ast_deep_copy(node->data.pair.car, heap);
    copy->data.pair.cdr = lizard_ast_deep_copy(node->data.pair.cdr, heap);
    break;
  default:
    memcpy(&(copy->data), &(node->data), sizeof(node->data));
  }
  return copy;
}

/* ---- I/O primitives ----
   display: print a value Scheme-style without trailing newline;
            strings are printed without surrounding quotes.
   write:   like display, but keeps quotes around strings.
   newline: print a single newline.
   error:   raise a user-defined error with a message.
*/

lizard_ast_node_t *lizard_primitive_display(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lz_list_node_t *iter;
  (void)env;
  for (iter = args->head; iter != args->nil; iter = iter->next) {
    lizard_ast_node_t *v = ((lizard_ast_list_node_t *)iter)->ast;
    if (v && v->type == AST_STRING) {
      fputs(v->data.string, stdout);
    } else {
      lizard_print_value(v);
    }
  }
  return lizard_make_nil(heap);
}

lizard_ast_node_t *lizard_primitive_write(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lz_list_node_t *iter;
  (void)env;
  for (iter = args->head; iter != args->nil; iter = iter->next) {
    lizard_ast_node_t *v = ((lizard_ast_list_node_t *)iter)->ast;
    if (v && v->type == AST_STRING) {
      /* R5RS: write escapes strings: write "hi\n" prints "hi\n"
         (literal backslash-n), not a real newline. */
      const char *s = v->data.string;
      fputc('"', stdout);
      while (*s) {
        switch (*s) {
        case '"':  fputs("\\\"", stdout); break;
        case '\\': fputs("\\\\", stdout); break;
        case '\n': fputs("\\n",  stdout); break;
        case '\t': fputs("\\t",  stdout); break;
        case '\r': fputs("\\r",  stdout); break;
        default:   fputc(*s,     stdout); break;
        }
        s++;
      }
      fputc('"', stdout);
    } else {
      lizard_print_value(v);
    }
  }
  return lizard_make_nil(heap);
}

lizard_ast_node_t *lizard_primitive_newline(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  (void)args;
  (void)env;
  fputc('\n', stdout);
  return lizard_make_nil(heap);
}

/* (load "path") — read a file and evaluate every top-level form in env. */

/* ============================================================
 * Phase C — Module loader primitives.
 *
 * (import "path.lisp")       — load-once with caching
 * (module-loaded? "path")    — check if already imported
 * (module-search-path)       — return the search path as a list
 * (add-module-path! "dir")   — prepend a directory to the search path
 * ============================================================ */

/* Try to open a file by searching: raw path first, then each search
 * path directory prepended. Returns an fopen'd FILE* and writes the
 * resolved path into out_resolved (heap-allocated). */



/* (import "path.lisp")
 * Like load, but:
 *   1. Resolves via the module search path
 *   2. Caches the result — second import of the same path is a no-op
 *   3. Returns the cached result of the last expression */

/* (module-loaded? "path") — #t if already imported, #f otherwise. */

/* (module-search-path) — returns the search path as a list of strings. */

/* (add-module-path! "dir") — prepend a directory to the search path. */

/* ---- primitive registration ----
   Install every built-in primitive into the given environment.
   Called by both the REPL and the test harness so the set of
   primitives stays in one place. */

/* Phase D: GC stats primitive. */
#include "gc.h"

/* Helper: build an alist pair (key . value) where value is an unsigned long. */



/* (gc) — run a full GC cycle: mark from roots, free dead segments.
 * Returns an alist with before/after stats and bytes freed. */

/* (error-location err) — return source location of an error as an alist,
 * or '() if no location is available. */

/* ============================================================
 * Practical list operations — essential R5RS subset.
 * ============================================================ */

/* (length lst) — number of elements in a proper list. */

/* (append lst1 lst2) — concatenate two lists. */

/* (reverse lst) — reverse a list. */

/* (list? x) — #t if x is a proper list (nil-terminated chain of pairs). */

/* (apply f args-list) — apply function to a list of arguments. */

/* (map f lst) — apply f to each element, return list of results. */

/* (filter pred lst) — keep elements where (pred elem) is truthy. */

/* (for-each f lst) — apply f to each element for side effects. */

/* (fold-left f init lst) — left fold: (f (f (f init e0) e1) e2) ... */

/* (assoc key alist) — find pair with matching car (using equal?). */

/* (member x lst) — find first sublist starting with x, or #f. */

/* (list-ref lst n) — 0-indexed element access. */

/* (iota n) — list of integers 0..n-1. (iota n start) — start..start+n-1. */

/* ============================================================
 * Track R: Syntax object primitives.
 * ============================================================ */

/* (datum->syntax context datum) — wrap datum with lexical context. */

/* (syntax->datum stx) / (syntax-e stx) — unwrap syntax object. */

/* (syntax-source stx) — return source location as alist. */

/* (syntax? x) — predicate. */

/* ============================================================
 * Track C: Persistent vector primitives.
 *
 * Initial implementation: copy-on-write flat array. Will be
 * upgraded to a 32-way trie in Track C Phase 2 for O(log32 n)
 * updates on large vectors.
 * ============================================================ */

/* (pvec e1 e2 ...) — create a persistent vector from elements. */

/* (pvec-ref v i) — 0-indexed access. */

/* (pvec-set v i val) — functional update, returns new vector. */

/* (pvec-push v val) — append, returns new vector. */

/* (pvec-count v) — number of elements. */

/* (pvec->list v) — convert to a Scheme list. */

/* (pvec? x) — predicate. */

/* ============================================================
 * Track C.2: Persistent hash map (initial flat-array impl).
 *
 * Stored as AST_HAMT with root pointing to a flat array of
 * key-value pair nodes. Will be upgraded to a proper 32-way
 * HAMT trie for large maps.
 * ============================================================ */

typedef struct {
  lizard_ast_node_t *key;
  lizard_ast_node_t *val;
} hamt_entry_t;

typedef struct {
  hamt_entry_t *entries;
  int count;
  int capacity;
} hamt_flat_t;


/* (hash-map k1 v1 k2 v2 ...) — create from alternating key-value pairs. */

/* (hash-get m key [default]) — lookup key, return value or default/#f. */

/* (hash-set m key val) — functional update, returns new map. */

/* (hash-keys m) — list of keys. */

/* (hash-count m) — number of entries. */

/* (hash-map? x) — predicate. */

/* ============================================================
 * Track K: Kernel type-checker primitives.
 * ============================================================ */
#include "kernel.h"

/* Helper: convert Lisp S-expression to kernel term. */
static kterm_t *sexp_to_kterm(lizard_heap_t *heap, lizard_ast_node_t *e) {
  if (e == NULL) return NULL;
  if (e->type == AST_NUMBER) {
    /* Build a Nat literal: 0 → kt_zero, n → succ^n(zero) */
    long n = mpz_get_si(e->data.number);
    kterm_t *t = kt_zero(heap);
    long i;
    for (i = 0; i < n; i++) t = kt_succ(heap, t);
    return t;
  }
  if (e->type == AST_SYMBOL) {
    const char *s = e->data.variable;
    if (strcmp(s, "Nat") == 0) return kt_nat(heap);
    if (strcmp(s, "zero") == 0) return kt_zero(heap);
      if (strcmp(s, "Bool") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_BOOL; return b;
      }
      if (strcmp(s, "true") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_TRUE; return b;
      }
      if (strcmp(s, "false") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_FALSE; return b;
      }
      if (strcmp(s, "Unit") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_UNIT; return b;
      }
      if (strcmp(s, "*") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_STAR; return b;
      }
      if (strcmp(s, "nil") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_NIL_K; return b;
      }
      if (strcmp(s, "nothing") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_NOTHING; return b;
      }
      if (strcmp(s, "Empty") == 0) {
        kterm_t *b = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(b, 0, sizeof(*b)); b->tag = KT_EMPTY; return b;
      }
    /* de Bruijn variable: #0, #1, etc. */
    if (s[0] == '#' && s[1] >= '0' && s[1] <= '9') {
      return kt_var(heap, s[1] - '0');
    }
    /* metavariable: ?0, ?1, etc. */
    if (s[0] == '?' && s[1] >= '0' && s[1] <= '9') {
      kterm_t *m = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
      memset(m, 0, sizeof(*m));
      m->tag = KT_META;
      m->data.meta.id = s[1] - '0';
      return m;
    }
    return NULL;
  }
  if (e->type == AST_PAIR || e->type == AST_APPLICATION) {
    /* Parse (Sort n), (succ e), (Pi (x A) B), (lam (x A) body),
     * (app f a), (Id A a b), (refl a) */
    lz_list_t *parts;
    lizard_ast_node_t *head;
    if (e->type == AST_APPLICATION) {
      parts = e->data.application_arguments;
      if (parts == NULL || parts->head == parts->nil) return NULL;
      head = ((lizard_ast_list_node_t *)parts->head)->ast;
    } else {
      head = e->data.pair.car;
      parts = NULL;
    }
    if (head->type == AST_SYMBOL) {
      const char *name = head->data.variable;
      if (strcmp(name, "Sort") == 0 && parts != NULL) {
        lizard_ast_node_t *lvl = ((lizard_ast_list_node_t *)parts->head->next)->ast;
        if (lvl->type == AST_NUMBER) {
          return kt_sort(heap, (int)mpz_get_si(lvl->data.number));
        }
      }
      if (strcmp(name, "succ") == 0 && parts != NULL) {
        kterm_t *inner = sexp_to_kterm(heap,
            ((lizard_ast_list_node_t *)parts->head->next)->ast);
        return inner ? kt_succ(heap, inner) : NULL;
      }
      /* (List A) */
      if (strcmp(name, "List") == 0 && parts != NULL) {
        kterm_t *et = sexp_to_kterm(heap,
            ((lizard_ast_list_node_t *)parts->head->next)->ast);
        if (et) {
          kterm_t *l = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(l, 0, sizeof(*l));
          l->tag = KT_LIST;
          l->data.list.elem_type = et;
          return l;
        }
        return NULL;
      }
      /* (cons h t) */
      if (strcmp(name, "cons") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2;
        kterm_t *h, *t;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next;
        if (n2 == parts->nil) return NULL;
        h = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        t = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        if (h && t) {
          kterm_t *c = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(c, 0, sizeof(*c));
          c->tag = KT_CONS_K;
          c->data.cons_k.head = h;
          c->data.cons_k.tail = t;
          return c;
        }
        return NULL;
      }
      if (strcmp(name, "refl") == 0 && parts != NULL) {
        kterm_t *inner = sexp_to_kterm(heap,
            ((lizard_ast_list_node_t *)parts->head->next)->ast);
        return inner ? kt_refl(heap, inner) : NULL;
      }
      /* (Id A a b) */
      if (strcmp(name, "Id") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2, *n3;
        kterm_t *ta, *ka, *kb;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next;
        if (n2 == parts->nil) return NULL;
        n3 = n2->next;
        if (n3 == parts->nil) return NULL;
        ta = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        ka = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        kb = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n3)->ast);
        if (ta && ka && kb) return kt_id(heap, ta, ka, kb);
        return NULL;
      }
      /* (Pi (name domain) codomain) */
      if (strcmp(name, "Pi") == 0 && parts != NULL) {
        lz_list_node_t *binder_node = parts->head->next;
        lz_list_node_t *body_node;
        lizard_ast_node_t *binder;
        const char *pname = "_";
        kterm_t *domain, *codomain;
        if (binder_node == parts->nil) return NULL;
        body_node = binder_node->next;
        if (body_node == parts->nil) return NULL;
        binder = ((lizard_ast_list_node_t *)binder_node)->ast;
        if (binder->type == AST_APPLICATION && binder->data.application_arguments) {
          lz_list_node_t *bn = binder->data.application_arguments->head;
          if (bn != binder->data.application_arguments->nil) {
            lizard_ast_node_t *nm = ((lizard_ast_list_node_t *)bn)->ast;
            if (nm->type == AST_SYMBOL) pname = nm->data.variable;
            bn = bn->next;
            if (bn != binder->data.application_arguments->nil) {
              domain = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)bn)->ast);
            } else { return NULL; }
          } else { return NULL; }
        } else { return NULL; }
        codomain = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)body_node)->ast);
        if (domain && codomain) return kt_pi(heap, pname, domain, codomain);
        return NULL;
      }
      /* (Sigma (name type) body) — same binder parsing as Pi. */
      if (strcmp(name, "Sigma") == 0 && parts != NULL) {
        lz_list_node_t *binder_node = parts->head->next;
        lz_list_node_t *body_node;
        lizard_ast_node_t *binder;
        const char *pname = "_";
        kterm_t *fst_type = NULL, *snd_type;
        if (binder_node == parts->nil) return NULL;
        body_node = binder_node->next;
        if (body_node == parts->nil) return NULL;
        binder = ((lizard_ast_list_node_t *)binder_node)->ast;
        if (binder->type == AST_APPLICATION && binder->data.application_arguments) {
          lz_list_node_t *bn = binder->data.application_arguments->head;
          if (bn != binder->data.application_arguments->nil) {
            lizard_ast_node_t *nm = ((lizard_ast_list_node_t *)bn)->ast;
            if (nm->type == AST_SYMBOL) pname = nm->data.variable;
            bn = bn->next;
            if (bn != binder->data.application_arguments->nil) {
              fst_type = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)bn)->ast);
            }
          }
        }
        if (fst_type == NULL) return NULL;
        snd_type = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)body_node)->ast);
        if (snd_type == NULL) return NULL;
        {
          kterm_t *sig = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(sig, 0, sizeof(*sig));
          sig->tag = KT_SIGMA;
          sig->data.sigma.name = pname;
          sig->data.sigma.fst_type = fst_type;
          sig->data.sigma.snd_type = snd_type;
          return sig;
        }
      }
      /* (pair a b) */
      if (strcmp(name, "pair") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2;
        kterm_t *a, *b;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next;
        if (n2 == parts->nil) return NULL;
        a = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        b = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        if (a && b) {
          kterm_t *p = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(p, 0, sizeof(*p));
          p->tag = KT_PAIR;
          p->data.pair.fst = a;
          p->data.pair.snd = b;
          return p;
        }
        return NULL;
      }
      /* (fst e) */
      if (strcmp(name, "fst") == 0 && parts != NULL) {
        kterm_t *inner = sexp_to_kterm(heap,
            ((lizard_ast_list_node_t *)parts->head->next)->ast);
        if (inner) {
          kterm_t *p = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(p, 0, sizeof(*p));
          p->tag = KT_PROJ1;
          p->data.proj.target = inner;
          return p;
        }
        return NULL;
      }
      /* (snd e) */
      if (strcmp(name, "snd") == 0 && parts != NULL) {
        kterm_t *inner = sexp_to_kterm(heap,
            ((lizard_ast_list_node_t *)parts->head->next)->ast);
        if (inner) {
          kterm_t *p = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(p, 0, sizeof(*p));
          p->tag = KT_PROJ2;
          p->data.proj.target = inner;
          return p;
        }
        return NULL;
      }
      /* (lam (x A) body) */
      if (strcmp(name, "lam") == 0 && parts != NULL) {
        lz_list_node_t *binder_node = parts->head->next;
        lz_list_node_t *body_node;
        lizard_ast_node_t *binder;
        const char *pname = "_";
        kterm_t *domain = NULL, *body;
        if (binder_node == parts->nil) return NULL;
        body_node = binder_node->next;
        if (body_node == parts->nil) return NULL;
        binder = ((lizard_ast_list_node_t *)binder_node)->ast;
        if (binder->type == AST_APPLICATION && binder->data.application_arguments) {
          lz_list_node_t *bn = binder->data.application_arguments->head;
          if (bn != binder->data.application_arguments->nil) {
            lizard_ast_node_t *nm = ((lizard_ast_list_node_t *)bn)->ast;
            if (nm->type == AST_SYMBOL) pname = nm->data.variable;
            bn = bn->next;
            if (bn != binder->data.application_arguments->nil) {
              domain = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)bn)->ast);
            }
          }
        }
        if (domain == NULL) return NULL;
        body = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)body_node)->ast);
        if (body == NULL) return NULL;
        return kt_lam(heap, pname, domain, body);
      }
      /* (app f a) — explicit application. */
      if (strcmp(name, "app") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2;
        kterm_t *f, *a;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next;
        if (n2 == parts->nil) return NULL;
        f = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        a = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        if (f && a) return kt_app(heap, f, a);
        return NULL;
      }
      /* (natrec motive zero-case succ-case scrutinee) */
      if (strcmp(name, "natrec") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2, *n3, *n4;
        kterm_t *mot, *zc, *sc, *scrut;
        kterm_t *nr;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next; if (n2 == parts->nil) return NULL;
        n3 = n2->next; if (n3 == parts->nil) return NULL;
        n4 = n3->next; if (n4 == parts->nil) return NULL;
        mot   = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        zc    = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        sc    = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n3)->ast);
        scrut = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n4)->ast);
        if (!mot || !zc || !sc || !scrut) return NULL;
        nr = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(nr, 0, sizeof(*nr));
        nr->tag = KT_NAT_REC;
        nr->data.nat_rec.motive = mot;
        nr->data.nat_rec.zero_case = zc;
        nr->data.nat_rec.succ_case = sc;
        nr->data.nat_rec.scrutinee = scrut;
        return nr;
      }
      /* (Maybe A) */
      if (strcmp(name, "Maybe") == 0 && parts != NULL) {
        kterm_t *et = sexp_to_kterm(heap,
            ((lizard_ast_list_node_t *)parts->head->next)->ast);
        if (et) {
          kterm_t *m = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(m, 0, sizeof(*m));
          m->tag = KT_MAYBE;
          m->data.maybe.elem_type = et;
          return m;
        }
        return NULL;
      }
      /* (just v) */
      if (strcmp(name, "just") == 0 && parts != NULL) {
        kterm_t *v = sexp_to_kterm(heap,
            ((lizard_ast_list_node_t *)parts->head->next)->ast);
        if (v) {
          kterm_t *j = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
          memset(j, 0, sizeof(*j));
          j->tag = KT_JUST;
          j->data.just.value = v;
          return j;
        }
        return NULL;
      }
      /* (Sum A B) */
      if (strcmp(name, "Sum") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2;
        kterm_t *a, *b, *s;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next; if (n2 == parts->nil) return NULL;
        a = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        b = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        if (!a || !b) return NULL;
        s = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(s, 0, sizeof(*s));
        s->tag = KT_SUM_K;
        s->data.sum_k.left_type = a;
        s->data.sum_k.right_type = b;
        return s;
      }
      /* (inl v B) — left injection */
      if (strcmp(name, "inl") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2;
        kterm_t *v, *rt, *s;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next;
        v = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        rt = (n2 != parts->nil) ? sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast) : NULL;
        if (!v) return NULL;
        s = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(s, 0, sizeof(*s));
        s->tag = KT_INL;
        s->data.inl.value = v;
        s->data.inl.right_type = rt;
        return s;
      }
      /* (inr v A) — right injection */
      if (strcmp(name, "inr") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2;
        kterm_t *v, *lt, *s;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next;
        v = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        lt = (n2 != parts->nil) ? sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast) : NULL;
        if (!v) return NULL;
        s = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(s, 0, sizeof(*s));
        s->tag = KT_INR;
        s->data.inr.value = v;
        s->data.inr.left_type = lt;
        return s;
      }
      /* (if scrutinee true-case false-case) — Bool eliminator. */
      if (strcmp(name, "if") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2, *n3;
        kterm_t *scrut, *tc, *fc, *br;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next; if (n2 == parts->nil) return NULL;
        n3 = n2->next; if (n3 == parts->nil) return NULL;
        scrut = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        tc    = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        fc    = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n3)->ast);
        if (!scrut || !tc || !fc) return NULL;
        br = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(br, 0, sizeof(*br));
        br->tag = KT_BOOL_REC;
        br->data.bool_rec.motive = NULL;
        br->data.bool_rec.true_case = tc;
        br->data.bool_rec.false_case = fc;
        br->data.bool_rec.scrutinee = scrut;
        return br;
      }
      /* (J motive base-case type a b proof) */
      if (strcmp(name, "J") == 0 && parts != NULL) {
        lz_list_node_t *n1 = parts->head->next;
        lz_list_node_t *n2, *n3, *n4, *n5, *n6;
        kterm_t *mot, *bc, *ty, *a, *b, *pf;
        kterm_t *j;
        if (n1 == parts->nil) return NULL;
        n2 = n1->next; if (n2 == parts->nil) return NULL;
        n3 = n2->next; if (n3 == parts->nil) return NULL;
        n4 = n3->next; if (n4 == parts->nil) return NULL;
        n5 = n4->next; if (n5 == parts->nil) return NULL;
        n6 = n5->next; if (n6 == parts->nil) return NULL;
        mot = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n1)->ast);
        bc  = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n2)->ast);
        ty  = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n3)->ast);
        a   = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n4)->ast);
        b   = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n5)->ast);
        pf  = sexp_to_kterm(heap, ((lizard_ast_list_node_t *)n6)->ast);
        if (!mot || !bc || !ty || !a || !b || !pf) return NULL;
        j = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
        memset(j, 0, sizeof(*j));
        j->tag = KT_J;
        j->data.j.motive = mot;
        j->data.j.base_case = bc;
        j->data.j.type = ty;
        j->data.j.a = a;
        j->data.j.b = b;
        j->data.j.proof = pf;
        return j;
      }
    }
  }
  return NULL;
}

/* (kernel-infer expr) — parse S-exp as kernel term, infer type. */
lizard_ast_node_t *lizard_primitive_kernel_infer(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *expr, *result;
  kterm_t *term, *type;
  kctx_t *ctx;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  term = sexp_to_kterm(heap, expr);
  if (term == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  ctx = kctx_create(heap);
  type = kt_infer(heap, ctx, term);
  if (type == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  /* Print the inferred type to a string. */
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  {
    /* Use tmpfile since fmemopen is not C89. */
    char buf[256];
    FILE *fp = tmpfile();
    long pos;
    size_t got;
    if (fp != NULL) {
      kt_fprint(fp, type);
      fflush(fp);
      pos = ftell(fp);
      if (pos > 0 && (size_t)pos < sizeof(buf)) {
        rewind(fp);
        got = fread(buf, 1, (size_t)pos, fp);
        buf[got] = '\0';
      } else {
        buf[0] = '\0';
      }
      fclose(fp);
      {
        char *sbuf = (char *)lizard_heap_alloc(strlen(buf) + 1);
        strcpy(sbuf, buf);
        result->data.string = sbuf;
      }
    } else {
      result->data.string = "<type>";
    }
  }
  return result;
}

/* (kernel-check expr type-expr) — check term has the given type. */
lizard_ast_node_t *lizard_primitive_kernel_check(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *term_expr, *type_expr;
  kterm_t *term, *type;
  kctx_t *ctx;
  kernel_result_t result;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  term_expr = ((lizard_ast_list_node_t *)args->head)->ast;
  type_expr = ((lizard_ast_list_node_t *)args->head->next)->ast;
  term = sexp_to_kterm(heap, term_expr);
  type = sexp_to_kterm(heap, type_expr);
  if (term == NULL || type == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  ctx = kctx_create(heap);
  result = kt_check(heap, ctx, term, type);
  return lizard_make_bool(heap, result == KERNEL_OK);
}

/* ============================================================
 * Track K: Tactic primitives.
 * ============================================================ */
#include "tactics.h"

static proof_state_t *current_proof = NULL;

lizard_ast_node_t *lizard_primitive_begin_proof(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *type_expr;
  kterm_t *goal_type;
  kctx_t *ctx;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  type_expr = ((lizard_ast_list_node_t *)args->head)->ast;
  goal_type = sexp_to_kterm(heap, type_expr);
  if (goal_type == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  ctx = kctx_create(heap);
  current_proof = proof_begin(heap, ctx, goal_type);
  printf("Proof started.\n");
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}

lizard_ast_node_t *lizard_primitive_proof_state(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  (void)args; (void)env;
  if (current_proof == NULL) { printf("No active proof.\n"); }
  else { proof_state_fprint(stdout, current_proof); }
  return lizard_make_nil(heap);
}

lizard_ast_node_t *lizard_primitive_tactic_intro(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *name_node;
  const char *name;
  int r;
  (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  name_node = ((lizard_ast_list_node_t *)args->head)->ast;
  name = (name_node->type == AST_STRING) ? name_node->data.string
       : (name_node->type == AST_SYMBOL) ? name_node->data.variable : "x";
  r = tactic_intro(current_proof, name);
  if (r < 0) { printf("tactic-intro: goal is not a Pi type.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}

lizard_ast_node_t *lizard_primitive_tactic_exact(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  kterm_t *term;
  int r;
  (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  term = sexp_to_kterm(heap, expr);
  if (term == NULL) { printf("tactic-exact: cannot parse term.\n"); return lizard_make_bool(heap, 0); }
  r = tactic_exact(current_proof, term);
  if (r < 0) { printf("tactic-exact: type mismatch.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}

lizard_ast_node_t *lizard_primitive_tactic_refl(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_reflexivity(current_proof);
  if (r < 0) { printf("tactic-refl: goal is not Id a a.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}

lizard_ast_node_t *lizard_primitive_qed(lz_list_t *args,
                                         lizard_env_t *env,
                                         lizard_heap_t *heap) {
  kterm_t *proof;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  proof = proof_qed(current_proof);
  if (proof == NULL) {
    printf("qed: %d goal(s) remaining.\n", proof_open_goals(current_proof));
    return lizard_make_bool(heap, 0);
  }
  printf("Qed! Proof term: ");
  kt_fprint(stdout, proof);
  printf("\n");
  current_proof = NULL;
  return lizard_make_bool(heap, 1);
}

/* ============================================================
 * Track C.4: Atoms — mutable reference cells.
 *
 * Atoms are cons cells with car = sentinel symbol "%atom"
 * and cdr = the current value. swap! mutates cdr in place.
 * ============================================================ */

static int is_atom(lizard_ast_node_t *node) {
  return node != NULL && node->type == AST_PAIR &&
         node->data.pair.car != NULL &&
         node->data.pair.car->type == AST_SYMBOL &&
         strcmp(node->data.pair.car->data.variable, "%atom") == 0;
}

/* (atom val) — create a mutable atom. */
lizard_ast_node_t *lizard_primitive_atom(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *val, *tag, *cell;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  val = ((lizard_ast_list_node_t *)args->head)->ast;
  tag = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  tag->type = AST_SYMBOL;
  tag->data.variable = "%atom";
  cell = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  cell->type = AST_PAIR;
  cell->data.pair.car = tag;
  cell->data.pair.cdr = val;
  return cell;
}

/* (deref a) — read the atom's current value. */
lizard_ast_node_t *lizard_primitive_deref(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *a;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  a = ((lizard_ast_list_node_t *)args->head)->ast;
  if (!is_atom(a))
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return a->data.pair.cdr;
}

/* (swap! a f) — apply f to the current value, store result. */
lizard_ast_node_t *lizard_primitive_swap(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *a, *fn, *new_val;
  lz_list_t *call_args;
  lizard_ast_list_node_t *n;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  a  = ((lizard_ast_list_node_t *)args->head)->ast;
  fn = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (!is_atom(a))
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  call_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  n = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
  n->ast = a->data.pair.cdr;
  list_append(call_args, &n->node);
  new_val = lizard_apply(fn, call_args, env, heap, lizard_identity_cont);
  if (new_val && new_val->type == AST_ERROR) return new_val;
  a->data.pair.cdr = new_val;
  return new_val;
}

/* (reset! a val) — set the atom to a new value. */
lizard_ast_node_t *lizard_primitive_reset(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *a, *val;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  a   = ((lizard_ast_list_node_t *)args->head)->ast;
  val = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (!is_atom(a))
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  a->data.pair.cdr = val;
  return val;
}

/* (atom? x) — predicate. */
lizard_ast_node_t *lizard_primitive_atomp(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  return lizard_make_bool(heap,
      is_atom(((lizard_ast_list_node_t *)args->head)->ast));
}

/* ============================================================
 * Exceptions: raise / guard
 *
 * (guard handler-fn body) — evaluate body; if it returns an error,
 * invoke (handler-fn error) instead.
 * ============================================================ */

/* (guard handler-fn expr) — evaluate expr; if error, call handler. */
lizard_ast_node_t *lizard_primitive_guard(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *handler, *body, *result;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  handler = ((lizard_ast_list_node_t *)args->head)->ast;
  body    = ((lizard_ast_list_node_t *)args->head->next)->ast;
  result = lizard_eval(body, env, heap, lizard_identity_cont);
  /* If result is an error, invoke handler. */
  if (result != NULL && result->type == AST_ERROR) {
    lz_list_t *call_args;
    lizard_ast_list_node_t *n;
    call_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    n = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    n->ast = result;
    list_append(call_args, &n->node);
    return lizard_apply(handler, call_args, env, heap, lizard_identity_cont);
  }
  return result;
}

/* ============================================================
 * More string operations
 * ============================================================ */

/* (string-ref s i) — character at index as a 1-char string. */
lizard_ast_node_t *lizard_primitive_string_ref(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *s, *idx, *result;
  long i;
  char *buf;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s   = ((lizard_ast_list_node_t *)args->head)->ast;
  idx = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (s->type != AST_STRING || idx->type != AST_NUMBER)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  i = mpz_get_si(idx->data.number);
  if (i < 0 || (size_t)i >= strlen(s->data.string))
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  buf = (char *)lizard_heap_alloc(2);
  buf[0] = s->data.string[i];
  buf[1] = '\0';
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = buf;
  return result;
}

/* (string-contains? s sub) — #t if sub is found in s. */
lizard_ast_node_t *lizard_primitive_string_contains(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *s, *sub;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s   = ((lizard_ast_list_node_t *)args->head)->ast;
  sub = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (s->type != AST_STRING || sub->type != AST_STRING)
    return lizard_make_bool(heap, 0);
  return lizard_make_bool(heap, strstr(s->data.string, sub->data.string) != NULL);
}

/* (string-upcase s) — uppercase copy. */
lizard_ast_node_t *lizard_primitive_string_upcase(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *s, *result;
  char *buf;
  size_t i, len;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s = ((lizard_ast_list_node_t *)args->head)->ast;
  if (s->type != AST_STRING)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  len = strlen(s->data.string);
  buf = (char *)lizard_heap_alloc(len + 1);
  for (i = 0; i < len; i++) {
    char c = s->data.string[i];
    buf[i] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
  }
  buf[len] = '\0';
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = buf;
  return result;
}

/* (string-downcase s) — lowercase copy. */
lizard_ast_node_t *lizard_primitive_string_downcase(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *s, *result;
  char *buf;
  size_t i, len;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s = ((lizard_ast_list_node_t *)args->head)->ast;
  if (s->type != AST_STRING)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  len = strlen(s->data.string);
  buf = (char *)lizard_heap_alloc(len + 1);
  for (i = 0; i < len; i++) {
    char c = s->data.string[i];
    buf[i] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
  }
  buf[len] = '\0';
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = buf;
  return result;
}

/* (string-split s delim) — split string into list of strings. */
lizard_ast_node_t *lizard_primitive_string_split(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *s_node, *d_node, *result, *node, *str;
  const char *s, *d, *start, *found;
  size_t dlen;
  char *buf;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s_node = ((lizard_ast_list_node_t *)args->head)->ast;
  d_node = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (s_node->type != AST_STRING || d_node->type != AST_STRING)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  s = s_node->data.string;
  d = d_node->data.string;
  dlen = strlen(d);
  if (dlen == 0) return lizard_make_nil(heap);
  result = lizard_make_nil(heap);
  /* Build in reverse, then reverse. */
  start = s;
  while ((found = strstr(start, d)) != NULL) {
    size_t len = (size_t)(found - start);
    buf = (char *)lizard_heap_alloc(len + 1);
    memcpy(buf, start, len);
    buf[len] = '\0';
    str = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    str->type = AST_STRING;
    str->data.string = buf;
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = str;
    node->data.pair.cdr = result;
    result = node;
    start = found + dlen;
  }
  /* Last segment. */
  {
    size_t len = strlen(start);
    buf = (char *)lizard_heap_alloc(len + 1);
    memcpy(buf, start, len);
    buf[len] = '\0';
    str = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    str->type = AST_STRING;
    str->data.string = buf;
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = str;
    node->data.pair.cdr = result;
    result = node;
  }
  /* Reverse to get correct order. */
  {
    lizard_ast_node_t *rev = lizard_make_nil(heap);
    while (result->type == AST_PAIR) {
      node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      node->type = AST_PAIR;
      node->data.pair.car = result->data.pair.car;
      node->data.pair.cdr = rev;
      rev = node;
      result = result->data.pair.cdr;
    }
    result = rev;
  }
  return result;
}

/* (string-join lst delim) — join list of strings with delimiter. */
lizard_ast_node_t *lizard_primitive_string_join(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *lst, *d_node, *result;
  const char *d;
  size_t total, dlen;
  char *buf, *p;
  int first;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  lst    = ((lizard_ast_list_node_t *)args->head)->ast;
  d_node = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (d_node->type != AST_STRING)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  d = d_node->data.string;
  dlen = strlen(d);
  /* Calculate total length. */
  total = 0;
  first = 1;
  {
    lizard_ast_node_t *cur = lst;
    while (cur != NULL && cur->type == AST_PAIR) {
      if (cur->data.pair.car->type == AST_STRING) {
        if (!first) total += dlen;
        total += strlen(cur->data.pair.car->data.string);
        first = 0;
      }
      cur = cur->data.pair.cdr;
    }
  }
  buf = (char *)lizard_heap_alloc(total + 1);
  p = buf;
  first = 1;
  {
    lizard_ast_node_t *cur = lst;
    while (cur != NULL && cur->type == AST_PAIR) {
      if (cur->data.pair.car->type == AST_STRING) {
        size_t slen;
        if (!first) { memcpy(p, d, dlen); p += dlen; }
        slen = strlen(cur->data.pair.car->data.string);
        memcpy(p, cur->data.pair.car->data.string, slen);
        p += slen;
        first = 0;
      }
      cur = cur->data.pair.cdr;
    }
  }
  *p = '\0';
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = buf;
  return result;
}

/* ============================================================
 * Lazy evaluation: delay/force already in lizard.c.
 * promise? is the only new addition.
 * ============================================================ */

/* (promise? x) — predicate. */
lizard_ast_node_t *lizard_primitive_promisep(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  return lizard_make_bool(heap,
      ((lizard_ast_list_node_t *)args->head)->ast->type == AST_PROMISE);
}

/* ============================================================
 * Kernel reduction + equality primitives
 * ============================================================ */

/* Helper: format kernel term to a heap-allocated string. */
static const char *kt_to_string(lizard_heap_t *heap, kterm_t *t) {
  char buf[512];
  FILE *fp = tmpfile();
  long pos;
  size_t got;
  char *sbuf;
  if (fp == NULL) return "<kterm>";
  kt_fprint(fp, t);
  fflush(fp);
  pos = ftell(fp);
  if (pos <= 0 || (size_t)pos >= sizeof(buf)) {
    fclose(fp);
    return "<kterm>";
  }
  rewind(fp);
  got = fread(buf, 1, (size_t)pos, fp);
  buf[got] = '\0';
  fclose(fp);
  sbuf = (char *)lizard_heap_alloc(strlen(buf) + 1);
  strcpy(sbuf, buf);
  return sbuf;
}

/* (kernel-reduce expr) — normalize a kernel term. */
lizard_ast_node_t *lizard_primitive_kernel_reduce(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *expr, *result;
  kterm_t *term, *reduced;
  kctx_t *ctx;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  term = sexp_to_kterm(heap, expr);
  if (term == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  ctx = kctx_create(heap);
  reduced = kt_whnf(heap, ctx, term);
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = kt_to_string(heap, reduced);
  return result;
}

/* (kernel-equal? a b) — check definitional equality of two terms. */
lizard_ast_node_t *lizard_primitive_kernel_equalp(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *ea, *eb;
  kterm_t *a, *b;
  kctx_t *ctx;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  ea = ((lizard_ast_list_node_t *)args->head)->ast;
  eb = ((lizard_ast_list_node_t *)args->head->next)->ast;
  a = sexp_to_kterm(heap, ea);
  b = sexp_to_kterm(heap, eb);
  if (a == NULL || b == NULL)
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  ctx = kctx_create(heap);
  return lizard_make_bool(heap, kt_equal(heap, ctx, a, b));
}

/* (tactic-assumption) — search context for a matching hypothesis. */
lizard_ast_node_t *lizard_primitive_tactic_assumption(lz_list_t *args,
                                                       lizard_env_t *env,
                                                       lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_assumption(current_proof);
  if (r < 0) {
    printf("tactic-assumption: no matching hypothesis found.\n");
    return lizard_make_bool(heap, 0);
  }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}

/* (tactic-simpl) — reduce goal to WHNF. */
lizard_ast_node_t *lizard_primitive_tactic_simpl(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_simpl(current_proof);
  if (r < 0) { printf("tactic-simpl: no goal.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}

/* (tactic-split) — split a Sigma goal into two subgoals. */
lizard_ast_node_t *lizard_primitive_tactic_split(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_split(current_proof);
  if (r < 0) { printf("tactic-split: goal is not a Sigma type.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}

/* (tactic-left) — choose left for Sum goal. */
lizard_ast_node_t *lizard_primitive_tactic_left(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_left(current_proof);
  if (r < 0) { printf("tactic-left: goal is not a Sum type.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}

/* (tactic-right) — choose right for Sum goal. */
lizard_ast_node_t *lizard_primitive_tactic_right(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_right(current_proof);
  if (r < 0) { printf("tactic-right: goal is not a Sum type.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof);
  return lizard_make_bool(heap, 1);
}

/* (phash-values m) — list of values. */

/* (phash-entries m) — list of (key . value) pairs. */

/* ============================================================
 * Metavariable / hole primitives.
 * ============================================================ */

static meta_ctx_t *global_meta_ctx = NULL;

static meta_ctx_t *get_meta_ctx(lizard_heap_t *heap) {
  if (global_meta_ctx == NULL)
    global_meta_ctx = meta_ctx_create(heap);
  return global_meta_ctx;
}

lizard_ast_node_t *lizard_primitive_kernel_hole(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *type_expr, *result;
  kterm_t *type, *hole;
  meta_ctx_t *mctx;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  type_expr = ((lizard_ast_list_node_t *)args->head)->ast;
  type = sexp_to_kterm(heap, type_expr);
  if (type == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  mctx = get_meta_ctx(heap);
  hole = meta_fresh(heap, mctx, type);
  printf("Created hole ?%d : ", hole->data.meta.id);
  kt_fprint(stdout, type);
  printf("\n");
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init_set_si(result->data.number, hole->data.meta.id);
  return result;
}

lizard_ast_node_t *lizard_primitive_kernel_solve(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *id_node, *term_expr;
  kterm_t *term;
  meta_ctx_t *mctx;
  int id, r;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  id_node   = ((lizard_ast_list_node_t *)args->head)->ast;
  term_expr = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (id_node->type != AST_NUMBER)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  id = (int)mpz_get_si(id_node->data.number);
  term = sexp_to_kterm(heap, term_expr);
  if (term == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  mctx = get_meta_ctx(heap);
  r = meta_solve(mctx, id, term);
  if (r < 0) {
    printf("kernel-solve: failed\n");
    return lizard_make_bool(heap, 0);
  }
  printf("Solved ?%d = ", id);
  kt_fprint(stdout, term);
  printf("\n");
  return lizard_make_bool(heap, 1);
}

lizard_ast_node_t *lizard_primitive_kernel_zonk(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *expr, *result;
  kterm_t *term, *zonked;
  meta_ctx_t *mctx;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  term = sexp_to_kterm(heap, expr);
  if (term == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  mctx = get_meta_ctx(heap);
  zonked = meta_zonk(heap, mctx, term);
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = kt_to_string(heap, zonked);
  return result;
}

lizard_ast_node_t *lizard_primitive_kernel_meta_state(lz_list_t *args,
                                                       lizard_env_t *env,
                                                       lizard_heap_t *heap) {
  meta_ctx_t *mctx;
  lizard_ast_node_t *r;
  (void)args; (void)env;
  mctx = get_meta_ctx(heap);
  meta_ctx_fprint(stdout, mctx);
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, (long)meta_unsolved_count(mctx));
  return r;
}

/* (kernel-unify a b) — try to unify two terms by solving metas. */
lizard_ast_node_t *lizard_primitive_kernel_unify(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *ea, *eb;
  kterm_t *a, *b;
  kctx_t *ctx;
  meta_ctx_t *mctx;
  int result;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  ea = ((lizard_ast_list_node_t *)args->head)->ast;
  eb = ((lizard_ast_list_node_t *)args->head->next)->ast;
  a = sexp_to_kterm(heap, ea);
  b = sexp_to_kterm(heap, eb);
  if (a == NULL || b == NULL)
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  ctx = kctx_create(heap);
  mctx = get_meta_ctx(heap);
  result = kt_unify(heap, ctx, mctx, a, b);
  if (result) {
    printf("Unified successfully.\n");
    meta_ctx_fprint(stdout, mctx);
  } else {
    printf("Unification failed.\n");
  }
  return lizard_make_bool(heap, result);
}

/* Global kernel definition context. */
static kdef_ctx_t *global_kdef_ctx = NULL;

static kdef_ctx_t *get_kdef_ctx(lizard_heap_t *heap) {
  if (global_kdef_ctx == NULL)
    global_kdef_ctx = kdef_ctx_create(heap);
  return global_kdef_ctx;
}

/* (kernel-define name term-sexp type-sexp) — store a named definition. */
lizard_ast_node_t *lizard_primitive_kernel_define(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *name_node, *term_expr, *type_expr;
  const char *name;
  kterm_t *term, *type;
  kdef_ctx_t *dctx;
  kctx_t *ctx;
  kernel_result_t r;
  (void)env;
  if (!three_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  name_node = ((lizard_ast_list_node_t *)args->head)->ast;
  term_expr = ((lizard_ast_list_node_t *)args->head->next)->ast;
  type_expr = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
  name = (name_node->type == AST_SYMBOL) ? name_node->data.variable
       : (name_node->type == AST_STRING) ? name_node->data.string : "?";
  term = sexp_to_kterm(heap, term_expr);
  type = sexp_to_kterm(heap, type_expr);
  if (term == NULL || type == NULL)
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  /* Type-check the definition. */
  ctx = kctx_create(heap);
  r = kt_check(heap, ctx, term, type);
  if (r != KERNEL_OK) {
    printf("kernel-define: type error for '%s'\n", name);
    return lizard_make_bool(heap, 0);
  }
  dctx = get_kdef_ctx(heap);
  kdef_add(heap, dctx, name, type, term);
  printf("Defined %s : ", name);
  kt_fprint(stdout, type);
  printf("\n");
  return lizard_make_bool(heap, 1);
}

/* (kernel-lookup name) — look up a kernel definition. */
lizard_ast_node_t *lizard_primitive_kernel_lookup(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *name_node, *result;
  const char *name;
  kdef_ctx_t *dctx;
  kdef_t *def;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  name_node = ((lizard_ast_list_node_t *)args->head)->ast;
  name = (name_node->type == AST_SYMBOL) ? name_node->data.variable
       : (name_node->type == AST_STRING) ? name_node->data.string : "?";
  dctx = get_kdef_ctx(heap);
  def = kdef_lookup(dctx, name);
  if (def == NULL) return lizard_make_bool(heap, 0);
  /* Return the value as a string. */
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = kt_to_string(heap, def->value);
  return result;
}

/* ============================================================
 * Core Scheme predicates + math
 * ============================================================ */

#define NUM_PRED(name, test) \
lizard_ast_node_t *name(lz_list_t *args, lizard_env_t *env, \
                         lizard_heap_t *heap) { \
  lizard_ast_node_t *a; \
  (void)env; \
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC); \
  a = ((lizard_ast_list_node_t *)args->head)->ast; \
  if (a->type != AST_NUMBER) return lizard_make_bool(heap, 0); \
  return lizard_make_bool(heap, test); \
}

NUM_PRED(lizard_primitive_zerop,     mpz_sgn(a->data.number) == 0)
NUM_PRED(lizard_primitive_positivep, mpz_sgn(a->data.number) > 0)
NUM_PRED(lizard_primitive_negativep, mpz_sgn(a->data.number) < 0)
NUM_PRED(lizard_primitive_evenp,     mpz_even_p(a->data.number))
NUM_PRED(lizard_primitive_oddp,      mpz_odd_p(a->data.number))

lizard_ast_node_t *lizard_primitive_min(lz_list_t *args,
                                         lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *a, *b;
  (void)env;
  if (!two_args(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  a = ((lizard_ast_list_node_t *)args->head)->ast;
  b = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (a->type != AST_NUMBER || b->type != AST_NUMBER)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return (mpz_cmp(a->data.number, b->data.number) <= 0) ? a : b;
}

lizard_ast_node_t *lizard_primitive_max(lz_list_t *args,
                                         lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *a, *b;
  (void)env;
  if (!two_args(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  a = ((lizard_ast_list_node_t *)args->head)->ast;
  b = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (a->type != AST_NUMBER || b->type != AST_NUMBER)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return (mpz_cmp(a->data.number, b->data.number) >= 0) ? a : b;
}

/* ============================================================
 * Character + string conversion
 * ============================================================ */

/* (char->integer "c") — ASCII value of first char. */
lizard_ast_node_t *lizard_primitive_char_to_int(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *s, *r;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s = ((lizard_ast_list_node_t *)args->head)->ast;
  if (s->type != AST_STRING || strlen(s->data.string) == 0)
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, (long)(unsigned char)s->data.string[0]);
  return r;
}

/* (integer->char n) — char from ASCII value. */
lizard_ast_node_t *lizard_primitive_int_to_char(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *n, *r;
  char *buf;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  n = ((lizard_ast_list_node_t *)args->head)->ast;
  if (n->type != AST_NUMBER) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  buf = (char *)lizard_heap_alloc(2);
  buf[0] = (char)mpz_get_si(n->data.number);
  buf[1] = '\0';
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_STRING;
  r->data.string = buf;
  return r;
}

/* (string->list "abc") — list of 1-char strings. */
lizard_ast_node_t *lizard_primitive_string_to_list(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *s, *result, *node, *ch;
  size_t len;
  int i;
  char *buf;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s = ((lizard_ast_list_node_t *)args->head)->ast;
  if (s->type != AST_STRING) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  len = strlen(s->data.string);
  result = lizard_make_nil(heap);
  for (i = (int)len - 1; i >= 0; i--) {
    buf = (char *)lizard_heap_alloc(2);
    buf[0] = s->data.string[i];
    buf[1] = '\0';
    ch = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    ch->type = AST_STRING;
    ch->data.string = buf;
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = ch;
    node->data.pair.cdr = result;
    result = node;
  }
  return result;
}

/* (list->string chars) — concatenate list of 1-char strings. */
lizard_ast_node_t *lizard_primitive_list_to_string(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *lst, *result;
  size_t len;
  char *buf, *p;
  lizard_ast_node_t *cur;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  lst = ((lizard_ast_list_node_t *)args->head)->ast;
  len = 0;
  for (cur = lst; cur != NULL && cur->type == AST_PAIR; cur = cur->data.pair.cdr) {
    if (cur->data.pair.car->type == AST_STRING)
      len += strlen(cur->data.pair.car->data.string);
  }
  buf = (char *)lizard_heap_alloc(len + 1);
  p = buf;
  for (cur = lst; cur != NULL && cur->type == AST_PAIR; cur = cur->data.pair.cdr) {
    if (cur->data.pair.car->type == AST_STRING) {
      size_t slen = strlen(cur->data.pair.car->data.string);
      memcpy(p, cur->data.pair.car->data.string, slen);
      p += slen;
    }
  }
  *p = '\0';
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = buf;
  return result;
}

/* (string-reverse s) */
lizard_ast_node_t *lizard_primitive_string_reverse(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *s, *result;
  size_t len, i;
  char *buf;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  s = ((lizard_ast_list_node_t *)args->head)->ast;
  if (s->type != AST_STRING) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  len = strlen(s->data.string);
  buf = (char *)lizard_heap_alloc(len + 1);
  for (i = 0; i < len; i++) buf[i] = s->data.string[len - 1 - i];
  buf[len] = '\0';
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = buf;
  return result;
}

/* ============================================================
 * Track C.3: Transients — mutable batch ops on persistent data.
 *
 * (transient! pvec) → mutable vector
 * (conj! vec val) → append to mutable vector in place
 * (persistent! vec) → freeze back to immutable pvec
 * ============================================================ */

/* (transient! coll) — create a mutable copy. */
lizard_ast_node_t *lizard_primitive_transient(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *coll, *result;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  coll = ((lizard_ast_list_node_t *)args->head)->ast;
  if (coll->type == AST_PVEC) {
    /* Convert persistent vector to mutable vector. */
    int count = coll->data.pvec.count;
    int i;
    result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    result->type = AST_VECTOR;
    result->data.vector.elements = (lizard_ast_node_t **)
        lizard_heap_alloc((size_t)(count + 16) * sizeof(lizard_ast_node_t *));
    result->data.vector.size = (size_t)count;
    for (i = 0; i < count; i++) {
      result->data.vector.elements[i] = coll->data.pvec.entries[i];
    }
    return result;
  }
  if (coll->type == AST_HAMT) {
    /* Convert persistent hash map to mutable list of pairs. */
    hamt_flat_t *h = (hamt_flat_t *)coll->data.hamt.root;
    lizard_ast_node_t *lst = lizard_make_nil(heap);
    int i;
    for (i = h->count - 1; i >= 0; i--) {
      lizard_ast_node_t *pair, *node;
      pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      pair->type = AST_PAIR;
      pair->data.pair.car = h->entries[i].key;
      pair->data.pair.cdr = h->entries[i].val;
      node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      node->type = AST_PAIR;
      node->data.pair.car = pair;
      node->data.pair.cdr = lst;
      lst = node;
    }
    return lst;
  }
  return coll;
}

/* (conj! vec val) — append to mutable vector in place. */
lizard_ast_node_t *lizard_primitive_conj_mut(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *vec, *val;
  (void)env;
  if (!two_args(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  vec = ((lizard_ast_list_node_t *)args->head)->ast;
  val = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (vec->type == AST_VECTOR) {
    int len = (int)vec->data.vector.size;
    /* Grow if needed — simple realloc-style. */
    {
      lizard_ast_node_t **new_elems;
      new_elems = (lizard_ast_node_t **)
          lizard_heap_alloc((size_t)(len + 1) * sizeof(lizard_ast_node_t *));
      if (len > 0)
        memcpy(new_elems, vec->data.vector.elements,
               (size_t)len * sizeof(lizard_ast_node_t *));
      new_elems[len] = val;
      vec->data.vector.elements = new_elems;
      vec->data.vector.size = (size_t)(len + 1);
    }
    return vec;
  }
  /* For lists, cons onto front. */
  if (vec->type == AST_PAIR || vec->type == AST_NIL) {
    lizard_ast_node_t *node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = val;
    node->data.pair.cdr = vec;
    return node;
  }
  return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
}

/* (persistent! vec) — freeze mutable vector back to immutable pvec. */
lizard_ast_node_t *lizard_primitive_persistent(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *vec, *result;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  vec = ((lizard_ast_list_node_t *)args->head)->ast;
  if (vec->type == AST_VECTOR) {
    int count = (int)vec->data.vector.size;
    int i;
    result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    result->type = AST_PVEC;
    result->data.pvec.count = count;
    result->data.pvec.capacity = count;
    result->data.pvec.entries = (lizard_ast_node_t **)
        lizard_heap_alloc((size_t)count * sizeof(lizard_ast_node_t *));
    for (i = 0; i < count; i++) {
      result->data.pvec.entries[i] = vec->data.vector.elements[i];
    }
    return result;
  }
  return vec;
}

static void install_one(lizard_heap_t *heap, lizard_env_t *env,
                        const char *name, lizard_primitive_func_t func) {
  lizard_env_define(heap, env, name, lizard_make_primitive(heap, func));
}

/* ---- Fast bignum primitives ----
 *
 * Idiomatic Scheme code often expresses big computations via repeated
 * doubling, addition, or division — patterns that allocate O(N) fresh
 * bignums under lizard's grow-only arena. The primitives below let the
 * user defer to GMP's optimised routines which complete the same
 * computation in O(log N) or O(1) GMP calls, avoiding the allocator
 * blow-up entirely. For example:
 *
 *     (arithmetic-shift 1 1000000)   ; 2^1000000 in one mpz_mul_2exp
 *     (expt 3 1000)                  ; 3^1000 via mpz_pow_ui
 *     (gcd huge-a huge-b)            ; one mpz_gcd
 */

static lizard_ast_node_t *unary_number(lz_list_t *args, lizard_heap_t *heap,
                                       lizard_ast_node_t **out) {
  lizard_ast_list_node_t *node;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  node = (lizard_ast_list_node_t *)args->head;
  if (node->ast->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  *out = node->ast;
  return NULL;
}

static int binary_numbers(lz_list_t *args, lizard_heap_t *heap,
                          lizard_ast_node_t **a, lizard_ast_node_t **b,
                          lizard_ast_node_t **err) {
  lizard_ast_list_node_t *na, *nb;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next != args->nil) {
    *err = lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
    return 0;
  }
  na = (lizard_ast_list_node_t *)args->head;
  nb = (lizard_ast_list_node_t *)args->head->next;
  if (na->ast->type != AST_NUMBER || nb->ast->type != AST_NUMBER) {
    *err = lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
    return 0;
  }
  *a = na->ast;
  *b = nb->ast;
  return 1;
}

/* (arithmetic-shift x n) — shift x left by n bits if n>0, right if n<0.
 * Uses mpz_mul_2exp / mpz_fdiv_q_2exp: one GMP call. */
lizard_ast_node_t *lizard_primitive_arith_shift(lz_list_t *args, lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *x, *n, *err = NULL;
  lizard_ast_node_t *result;
  long shift;
  (void)env;
  if (!binary_numbers(args, heap, &x, &n, &err)) return err;
  if (!mpz_fits_slong_p(n->data.number)) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  shift = mpz_get_si(n->data.number);
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init(result->data.number);
  if (shift >= 0) {
    mpz_mul_2exp(result->data.number, x->data.number, (unsigned long)shift);
  } else {
    mpz_fdiv_q_2exp(result->data.number, x->data.number,
                    (unsigned long)(-shift));
  }
  return result;
}

/* (expt base exp) — base^exp where exp is a non-negative fixnum.
 * Uses mpz_pow_ui: GMP's optimised power routine, O(log exp). */
lizard_ast_node_t *lizard_primitive_expt(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *b, *e, *err = NULL;
  lizard_ast_node_t *result;
  unsigned long exp;
  (void)env;
  if (!binary_numbers(args, heap, &b, &e, &err)) return err;
  if (mpz_sgn(e->data.number) < 0 || !mpz_fits_ulong_p(e->data.number)) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  exp = mpz_get_ui(e->data.number);
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init(result->data.number);
  mpz_pow_ui(result->data.number, b->data.number, exp);
  return result;
}

/* (gcd a b) — greatest common divisor via mpz_gcd. */
lizard_ast_node_t *lizard_primitive_gcd(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_node_t *a, *b, *err = NULL;
  lizard_ast_node_t *result;
  (void)env;
  if (!binary_numbers(args, heap, &a, &b, &err)) return err;
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init(result->data.number);
  mpz_gcd(result->data.number, a->data.number, b->data.number);
  return result;
}

/* (lcm a b) — least common multiple via mpz_lcm. */
lizard_ast_node_t *lizard_primitive_lcm(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_node_t *a, *b, *err = NULL;
  lizard_ast_node_t *result;
  (void)env;
  if (!binary_numbers(args, heap, &a, &b, &err)) return err;
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init(result->data.number);
  mpz_lcm(result->data.number, a->data.number, b->data.number);
  return result;
}

/* (quotient a b) — truncating integer division. */
lizard_ast_node_t *lizard_primitive_quotient(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *a, *b, *err = NULL;
  lizard_ast_node_t *result;
  (void)env;
  if (!binary_numbers(args, heap, &a, &b, &err)) return err;
  if (mpz_sgn(b->data.number) == 0) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init(result->data.number);
  mpz_tdiv_q(result->data.number, a->data.number, b->data.number);
  return result;
}

/* (remainder a b) — truncating-division remainder (same sign as a). */
lizard_ast_node_t *lizard_primitive_remainder(lz_list_t *args, lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *a, *b, *err = NULL;
  lizard_ast_node_t *result;
  (void)env;
  if (!binary_numbers(args, heap, &a, &b, &err)) return err;
  if (mpz_sgn(b->data.number) == 0) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init(result->data.number);
  mpz_tdiv_r(result->data.number, a->data.number, b->data.number);
  return result;
}

/* (abs n). */
lizard_ast_node_t *lizard_primitive_abs(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_node_t *x = NULL, *err;
  lizard_ast_node_t *result;
  (void)env;
  err = unary_number(args, heap, &x);
  if (err) return err;
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init(result->data.number);
  mpz_abs(result->data.number, x->data.number);
  return result;
}

/* (square n) — one mpz_mul instead of (* n n) which would force-and-copy. */
lizard_ast_node_t *lizard_primitive_square(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *x = NULL, *err;
  lizard_ast_node_t *result;
  (void)env;
  err = unary_number(args, heap, &x);
  if (err) return err;
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init(result->data.number);
  mpz_mul(result->data.number, x->data.number, x->data.number);
  return result;
}

/* (modular-expt base exp mod) — modular exponentiation, O(log exp) and
 * keeps intermediates bounded by mod. */
lizard_ast_node_t *lizard_primitive_modexpt(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_list_node_t *na, *nb, *nc;
  lizard_ast_node_t *result;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next == args->nil ||
      args->head->next->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  na = (lizard_ast_list_node_t *)args->head;
  nb = (lizard_ast_list_node_t *)args->head->next;
  nc = (lizard_ast_list_node_t *)args->head->next->next;
  if (na->ast->type != AST_NUMBER || nb->ast->type != AST_NUMBER ||
      nc->ast->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init(result->data.number);
  mpz_powm(result->data.number, na->ast->data.number, nb->ast->data.number,
           nc->ast->data.number);
  return result;
}

/* ---------------------------------------------------------------------
 * Reflection + type primitives.
 * ------------------------------------------------------------------- */

static lizard_ast_node_t *make_symbol(lizard_heap_t *heap, const char *name) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  size_t len = strlen(name);
  char *buf = lizard_heap_alloc(len + 1);
  memcpy(buf, name, len + 1);
  n->type = AST_SYMBOL;
  n->data.variable = buf;
  return n;
}

static lizard_ast_node_t *make_string(lizard_heap_t *heap, const char *src,
                                      size_t len) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  char *buf = lizard_heap_alloc(len + 1);
  if (len > 0) memcpy(buf, src, len);
  buf[len] = '\0';
  n->type = AST_STRING;
  n->data.string = buf;
  return n;
}

/* (type-of x) -> a symbol naming x's runtime type. */
lizard_ast_node_t *lizard_primitive_type_of(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  const char *name;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  node = (lizard_ast_list_node_t *)args->head;
  switch (node->ast->type) {
  case AST_NUMBER:       name = "number";       break;
  case AST_STRING:       name = "string";       break;
  case AST_SYMBOL:       name = "symbol";       break;
  case AST_BOOL:         name = "boolean";      break;
  case AST_NIL:          name = "nil";          break;
  case AST_PAIR:         name = "pair";         break;
  case AST_APPLICATION:  name = "list";         break;
  case AST_LAMBDA:       name = "procedure";    break;
  case AST_PRIMITIVE:    name = "primitive";    break;
  case AST_MACRO:        name = "macro";        break;
  case AST_QUOTE:        name = "quote";        break;
  case AST_QUASIQUOTE:   name = "quasiquote";   break;
  case AST_PROMISE:      name = "promise";      break;
  case AST_ERROR:        name = "error";        break;
  case AST_CONTINUATION: name = "continuation"; break;
  case AST_VECTOR:       name = "vector";       break;
  case AST_HASH:         name = "hash";         break;
  case AST_SYNTAX_RULES: name = "syntax-rules"; break;
  case AST_TT_PI:          name = "Pi";          break;
  case AST_TT_SIGMA:       name = "Sigma";       break;
  case AST_TT_PI_FRESH:    name = "pi-fresh";    break;
  case AST_TT_SIGMA_FRESH: name = "sigma-fresh"; break;
  case AST_TT_CO_PI_FRESH:    name = "co-pi-fresh";    break;
  case AST_TT_CO_SIGMA_FRESH: name = "co-sigma-fresh"; break;
  case AST_TT_BOX:            name = "Box";            break;
  case AST_TT_DIAMOND:        name = "Diamond";        break;
  case AST_TT_BOX_INTRO:      name = "box";            break;
  case AST_TT_BOX_ELIM:       name = "unbox";          break;
  case AST_TT_DIAMOND_INTRO:  name = "diamond";        break;
  case AST_TT_DIAMOND_ELIM:   name = "let-diamond";    break;
  case AST_TT_BOX_APP:        name = "box-app";        break;
  case AST_TT_DIAMOND_BIND:   name = "diamond-bind";   break;
  case AST_TT_DIAMOND_INTRO_SYM: name = "dia";         break;
  case AST_TT_POSS_COERCE:    name = "poss-coerce";    break;
  case AST_TT_APP:         name = "@";           break;
  case AST_TT_SUM:         name = "Sum";         break;
  case AST_TT_UNIVERSE:    name = "U";           break;
  case AST_TT_COUNIVERSE:  name = "Uco";         break;
  case AST_TT_UNIVERSE_SET: name = "U-set";       break;
  case AST_TT_COUNIVERSE_SET: name = "Co-set";    break;
  case AST_TT_CO_MAX:       name = "Co-max";      break;
  case AST_TT_CO_MIN:       name = "Co-min";      break;
  case AST_TT_HIT_DECL:     name = "HIT";         break;
  case AST_TT_HIT_CONSTRUCTOR: name = "HIT-constructor"; break;
  case AST_TT_HIT_PATH:     name = "HIT-path";    break;
  case AST_TT_HIT_REF:      name = "HIT-ref";     break;
  case AST_TT_HIT_APP:      name = "HIT-app";     break;
  case AST_TT_ID:          name = "Id";          break;
  case AST_TT_REFL:        name = "refl";        break;
  case AST_TT_INDUCTIVE:   name = "Inductive";   break;
  case AST_TT_COINDUCTIVE: name = "Coinductive"; break;
  case AST_TT_ANNOT:       name = "annot";       break;
  case AST_TT_VARIABLE:     name = "variable";     break;
  case AST_TT_CONTEXT:      name = "context";      break;
  case AST_TT_SUBSTITUTION: name = "substitution"; break;
  case AST_TT_JUDGMENT:     name = "judgment";     break;
  case AST_TT_EQUIV:        name = "equivalence";  break;
  case AST_TT_TRANSPORT:    name = "transport";    break;
  case AST_TT_ID_SYM:       name = "Id-sym";       break;
  case AST_TT_ID_TRANS:     name = "Id-trans";     break;
  case AST_TT_LAMBDA:       name = "Lambda";       break;
  case AST_TT_AP:           name = "ap";           break;
  case AST_TT_PAIR:         name = "Pair";         break;
  case AST_TT_FST:          name = "fst";          break;
  case AST_TT_SND:          name = "snd";          break;
  case AST_TT_INL:          name = "inl";          break;
  case AST_TT_INR:          name = "inr";          break;
  case AST_TT_CASE:         name = "Case";         break;
  case AST_TT_UNIT:         name = "Unit";         break;
  case AST_TT_TT:           name = "tt";           break;
  case AST_TT_BOT:          name = "Bot";          break;
  case AST_TT_J:            name = "J";            break;
  case AST_TT_XPORT:        name = "xport";        break;
  case AST_TT_U_VAR:        name = "U-var";        break;
  case AST_TT_U_SUC:        name = "U-suc";        break;
  case AST_TT_U_MAX:        name = "U-max";        break;
  case AST_TT_U_MIN:        name = "U-min";        break;
  case AST_TT_INTERVAL:     name = "I";            break;
  case AST_TT_I0:           name = "i0";           break;
  case AST_TT_I1:           name = "i1";           break;
  case AST_TT_I_VAR:        name = "I-var";        break;
  case AST_TT_I_AND:        name = "I-and";        break;
  case AST_TT_I_OR:         name = "I-or";         break;
  case AST_TT_I_NEG:        name = "I-neg";        break;
  case AST_TT_PATH:         name = "Path";         break;
  case AST_TT_PATH_ABS:     name = "path-abs";     break;
  case AST_TT_PATH_APP:     name = "path-app";     break;
  case AST_TT_F0:           name = "F0";           break;
  case AST_TT_F1:           name = "F1";           break;
  case AST_TT_F_EQ:         name = "F-eq";         break;
  case AST_TT_F_AND:        name = "F-and";        break;
  case AST_TT_F_OR:         name = "F-or";         break;
  case AST_TT_PARTIAL:      name = "Partial";      break;
  case AST_TT_SUB:          name = "Sub";          break;
  case AST_TT_COMP:         name = "comp";         break;
  case AST_TT_HCOMP:        name = "hcomp";        break;
  case AST_TT_FILL:         name = "fill";         break;
  case AST_TT_EQUIV_TYPE:   name = "Equiv";        break;
  case AST_TT_ID_EQUIV:     name = "id-equiv";     break;
  case AST_TT_EQUIV_FUN:    name = "equiv-fun";    break;
  case AST_TT_EQUIV_INV:    name = "equiv-inv";    break;
  case AST_TT_GLUE:         name = "Glue";         break;
  case AST_TT_GLUE_INTRO:   name = "glue-intro";   break;
  case AST_TT_UNGLUE:       name = "unglue";       break;
  case AST_TT_UA:           name = "ua";           break;
  case AST_TT_SYSTEM_NIL:   name = "system-nil";   break;
  case AST_TT_SYSTEM_CONS:  name = "system-cons";  break;
  default:               name = "unknown";      break;
  }
  return make_symbol(heap, name);
}

/* (env-keys) -> list of bound symbol names in the current environment. */
lizard_ast_node_t *lizard_primitive_env_keys(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_env_entry_t *entry;
  lizard_env_t *frame;
  lizard_ast_node_t *result;
  lizard_ast_node_t *prev;
  (void)args;
  /* Build a list of symbols by walking entries from the innermost
     frame outwards. Duplicates are kept — outer shadows show up too. */
  result = lizard_make_nil(heap);
  for (frame = env; frame != NULL; frame = frame->parent) {
    for (entry = frame->entries; entry != NULL; entry = entry->next) {
      prev = result;
      result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      result->type = AST_PAIR;
      result->data.pair.car = make_symbol(heap, entry->symbol);
      result->data.pair.cdr = prev;
    }
  }
  return result;
}

/* (defined? 'sym) -> #t if sym is bound in current env, #f otherwise. */
lizard_ast_node_t *lizard_primitive_definedp(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  node = (lizard_ast_list_node_t *)args->head;
  if (node->ast->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  if (lizard_env_lookup(env, node->ast->data.variable)) {
    return lizard_make_bool(heap, true);
  }
  return lizard_make_bool(heap, false);
}

/* (procedure-arity f) -> number of formal parameters, or 'variadic. */
lizard_ast_node_t *lizard_primitive_proc_arity(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  lizard_ast_node_t *fn;
  lizard_ast_node_t *param_list;
  lizard_ast_node_t *result;
  lz_list_node_t *p;
  long count;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  node = (lizard_ast_list_node_t *)args->head;
  fn = node->ast;
  if (fn->type == AST_PRIMITIVE) {
    /* C primitives are variadic from the Lisp side. */
    return make_symbol(heap, "variadic");
  }
  if (fn->type != AST_LAMBDA) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  param_list = ((lizard_ast_list_node_t *)fn->data.lambda.parameters->head)
                   ->ast;
  if (param_list->type == AST_NIL) {
    count = 0;
  } else if (param_list->type == AST_APPLICATION) {
    count = 0;
    for (p = param_list->data.application_arguments->head;
         p != param_list->data.application_arguments->nil; p = p->next) {
      count++;
    }
  } else {
    return lizard_make_error(heap, LIZARD_ERROR_LAMBDA_PARAMS);
  }
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init_set_si(result->data.number, count);
  return result;
}

/* ---------------------------------------------------------------------
 * String primitives.
 * ------------------------------------------------------------------- */

static int get_string(lz_list_node_t *node, const char **out, size_t *len) {
  lizard_ast_node_t *ast = ((lizard_ast_list_node_t *)node)->ast;
  if (ast->type != AST_STRING) return 0;
  *out = ast->data.string;
  *len = strlen(ast->data.string);
  return 1;
}

/* (string-length s) */
lizard_ast_node_t *lizard_primitive_str_length(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  const char *s;
  size_t len;
  lizard_ast_node_t *result;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  if (!get_string(args->head, &s, &len)) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init_set_ui(result->data.number, (unsigned long)len);
  return result;
}

/* (string-append s ...) — concatenate any number of strings. */
lizard_ast_node_t *lizard_primitive_str_append(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lz_list_node_t *iter;
  size_t total;
  char *buf;
  size_t off;
  lizard_ast_node_t *result;
  (void)env;
  total = 0;
  for (iter = args->head; iter != args->nil; iter = iter->next) {
    lizard_ast_node_t *a = ((lizard_ast_list_node_t *)iter)->ast;
    if (a->type != AST_STRING) {
      return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
    }
    total += strlen(a->data.string);
  }
  buf = lizard_heap_alloc(total + 1);
  off = 0;
  for (iter = args->head; iter != args->nil; iter = iter->next) {
    const char *s = ((lizard_ast_list_node_t *)iter)->ast->data.string;
    size_t l = strlen(s);
    memcpy(buf + off, s, l);
    off += l;
  }
  buf[total] = '\0';
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_STRING;
  result->data.string = buf;
  return result;
}

/* (substring s start [end]) */
lizard_ast_node_t *lizard_primitive_substring(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  const char *s;
  size_t len;
  lizard_ast_node_t *s_ast, *start_ast, *end_ast;
  long start, end;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  s_ast = ((lizard_ast_list_node_t *)args->head)->ast;
  start_ast = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (s_ast->type != AST_STRING || start_ast->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  s = s_ast->data.string;
  len = strlen(s);
  start = mpz_get_si(start_ast->data.number);
  if (args->head->next->next != args->nil) {
    end_ast = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
    if (end_ast->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
    }
    end = mpz_get_si(end_ast->data.number);
  } else {
    end = (long)len;
  }
  if (start < 0 || end < start || (size_t)end > len) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  return make_string(heap, s + start, (size_t)(end - start));
}

/* (string=? a b) */
lizard_ast_node_t *lizard_primitive_str_eq(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *a, *b;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  a = ((lizard_ast_list_node_t *)args->head)->ast;
  b = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (a->type != AST_STRING || b->type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  return lizard_make_bool(heap, strcmp(a->data.string, b->data.string) == 0);
}

/* (number->string n) */
lizard_ast_node_t *lizard_primitive_num_to_str(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  char *buf;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = ((lizard_ast_list_node_t *)args->head)->ast;
  if (n->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  /* mpz_get_str(NULL, 10, n) allocates via GMP — which routes through
     our heap, so it's safe. */
  buf = mpz_get_str(NULL, 10, n->data.number);
  {
    lizard_ast_node_t *r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    r->type = AST_STRING;
    r->data.string = buf;
    return r;
  }
}

/* (string->number s) — returns #f if s isn't a parseable integer. */
lizard_ast_node_t *lizard_primitive_str_to_num(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *s, *r;
  int ok;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  s = ((lizard_ast_list_node_t *)args->head)->ast;
  if (s->type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init(r->data.number);
  ok = (mpz_set_str(r->data.number, s->data.string, 10) == 0);
  if (!ok) {
    return lizard_make_bool(heap, false);
  }
  return r;
}

/* (symbol->string s) */
lizard_ast_node_t *lizard_primitive_sym_to_str(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *s;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  s = ((lizard_ast_list_node_t *)args->head)->ast;
  if (s->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  return make_string(heap, s->data.variable, strlen(s->data.variable));
}

/* (string->symbol s) */
lizard_ast_node_t *lizard_primitive_str_to_sym(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *s;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  s = ((lizard_ast_list_node_t *)args->head)->ast;
  if (s->type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  return make_symbol(heap, s->data.string);
}

/* ---------------------------------------------------------------------
 * Exception handling — (raise value), (try thunk handler),
 *                      (error-object? x), (error-value x).
 * ------------------------------------------------------------------- */

/* Build an AST_ERROR carrying a user-supplied payload. The payload
 * lives as the sole element of the error's `data` list. */
static lizard_ast_node_t *make_user_error(lizard_heap_t *heap,
                                          lizard_ast_node_t *value) {
  lizard_ast_node_t *err;
  lz_list_t *data;
  lizard_ast_list_node_t *node;
  err = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  err->type = AST_ERROR;
  err->data.error.code = LIZARD_ERROR_USER;
  data = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  node = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
  node->ast = value;
  list_append(data, &node->node);
  err->data.error.data = data;
  return err;
}

/* (raise value) — propagate an error carrying `value`. */
lizard_ast_node_t *lizard_primitive_raise(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  return make_user_error(heap,
                         ((lizard_ast_list_node_t *)args->head)->ast);
}

/* (try thunk handler) — call (thunk); if it errors, call (handler err).
 * Otherwise return thunk's result. */
lizard_ast_node_t *lizard_primitive_try(lz_list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_node_t *thunk, *handler, *result;
  lz_list_t *empty, *one;
  lizard_ast_list_node_t *wrap;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  thunk = ((lizard_ast_list_node_t *)args->head)->ast;
  handler = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if ((thunk->type != AST_LAMBDA && thunk->type != AST_PRIMITIVE) ||
      (handler->type != AST_LAMBDA && handler->type != AST_PRIMITIVE)) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  empty = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  result = lizard_apply(thunk, empty, env, heap, lizard_identity_cont);
  if (result && result->type == AST_ERROR) {
    one = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    wrap = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    wrap->ast = result;
    list_append(one, &wrap->node);
    return lizard_apply(handler, one, env, heap, lizard_identity_cont);
  }
  return result;
}

/* (error-object? x) */
lizard_ast_node_t *lizard_primitive_error_objp(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = ((lizard_ast_list_node_t *)args->head)->ast;
  return lizard_make_bool(heap, x && x->type == AST_ERROR);
}

/* (error-value err) — return the payload of an error.
 * For user-raised errors this is whatever was passed to (raise).
 * For system errors it's the message string. */
lizard_ast_node_t *lizard_primitive_error_value(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = ((lizard_ast_list_node_t *)args->head)->ast;
  if (!x || x->type != AST_ERROR) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  if (x->data.error.data &&
      x->data.error.data->head != x->data.error.data->nil) {
    return ((lizard_ast_list_node_t *)x->data.error.data->head)->ast;
  }
  return lizard_make_nil(heap);
}

/* (gensym [prefix]) — produce a fresh unique symbol on each call.
 * Useful for hand-written hygienic macros until we have syntax-rules.
 *
 * Phase 0: counter lives in the runtime (heap->runtime->gensym_counter).
 * Falls back to a static counter for standalone heaps without a runtime. */
static unsigned long lizard_gensym_counter_fallback = 0;

lizard_ast_node_t *lizard_primitive_gensym(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  const char *prefix = "g";
  char counter_buf[32];
  char *buf;
  size_t len;
  unsigned long *counter;
  (void)env;
  counter = (heap->runtime != NULL)
              ? &heap->runtime->gensym_counter
              : &lizard_gensym_counter_fallback;
  if (args->head != args->nil) {
    lizard_ast_node_t *p = ((lizard_ast_list_node_t *)args->head)->ast;
    if (p->type == AST_STRING) {
      prefix = p->data.string;
    } else if (p->type == AST_SYMBOL) {
      prefix = p->data.variable;
    } else {
      return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
    }
  }
  (*counter)++;
  sprintf(counter_buf, "%lu", *counter);
  len = strlen(prefix) + strlen(counter_buf) + 1U;
  buf = lizard_heap_alloc(len);
  strcpy(buf, prefix);
  strcat(buf, counter_buf);
  return make_symbol(heap, buf);
}

/* Vector and hash primitives live in prims_collections.c. */

/* Shared arg-count helpers retained for older primitive groups in this file.
 * Type-theory primitives have their own static copies in prims_tt.c until
 * all primitive groups are migrated to prims_common.h. */
static int single_arg(lz_list_t *args) {
  return args->head != args->nil && args->head->next == args->nil;
}
static int two_args(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next == args->nil;
}
static int three_args(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next != args->nil &&
         args->head->next->next->next == args->nil;
}

/* Type-theory notation and logic primitives live in prims_tt.c. */

void lizard_install_primitives(lizard_heap_t *heap, lizard_env_t *env) {
  install_one(heap, env, "null?",   lizard_primitive_nullp);
  install_one(heap, env, "pair?",   lizard_primitive_pairp);
  install_one(heap, env, "string?", lizard_primitive_stringp);
  install_one(heap, env, "bool?",   lizard_primitive_boolp);
  install_one(heap, env, "number?", lizard_primitive_numberp);
  install_one(heap, env, "symbol?", lizard_primitive_symbolp);
  install_one(heap, env, "procedure?", lizard_primitive_procedurep);
  install_one(heap, env, "+",       lizard_primitive_plus);
  install_one(heap, env, "-",       lizard_primitive_minus);
  install_one(heap, env, "*",       lizard_primitive_multiply);
  install_one(heap, env, "/",       lizard_primitive_divide);
  install_one(heap, env, "=",       lizard_primitive_equal);
  install_one(heap, env, "^",       lizard_primitive_pow);
  install_one(heap, env, "%",       lizard_primitive_mod);
  install_one(heap, env, "<",       lizard_primitive_lt);
  install_one(heap, env, "<=",      lizard_primitive_le);
  install_one(heap, env, ">",       lizard_primitive_gt);
  install_one(heap, env, ">=",      lizard_primitive_ge);
  install_one(heap, env, "cons",    lizard_primitive_cons);
  install_one(heap, env, "car",     lizard_primitive_car);
  install_one(heap, env, "cdr",     lizard_primitive_cdr);
  install_one(heap, env, "list",    lizard_primitive_list);
  install_one(heap, env, "eval",    lizard_primitive_eval);
  install_one(heap, env, "unquote", lizard_primitive_unquote);
  install_one(heap, env, "tokens",  lizard_primitive_tokens);
  install_one(heap, env, "ast",     lizard_primitive_ast);
  install_one(heap, env, "and",     lizard_primitive_and);
  install_one(heap, env, "or",      lizard_primitive_or);
  install_one(heap, env, "not",     lizard_primitive_not);
  install_one(heap, env, "xor",     lizard_primitive_xor);
  install_one(heap, env, "nand",    lizard_primitive_nand);
  install_one(heap, env, "nor",     lizard_primitive_nor);
  install_one(heap, env, "xnor",    lizard_primitive_xnor);
  install_one(heap, env, "display", lizard_primitive_display);
  install_one(heap, env, "write",   lizard_primitive_write);
  install_one(heap, env, "newline", lizard_primitive_newline);
  install_one(heap, env, "load",    lizard_primitive_load);
  /* Phase C: module loader. */
  install_one(heap, env, "import",            lizard_primitive_import);
  install_one(heap, env, "module-loaded?",    lizard_primitive_module_loadedp);
  install_one(heap, env, "module-search-path",lizard_primitive_module_search_path);
  install_one(heap, env, "add-module-path!",  lizard_primitive_add_module_path);
  /* Phase D: GC. */
  install_one(heap, env, "gc-stats",          lizard_primitive_gc_stats);
  install_one(heap, env, "gc",               lizard_primitive_gc);
  /* Phase E: bytecode VM. */
  install_one(heap, env, "vm-eval",          lizard_primitive_vm_eval);
  install_one(heap, env, "disassemble",      lizard_primitive_disassemble);
  install_one(heap, env, "vm-time",          lizard_primitive_vm_time);
  install_one(heap, env, "time-eval",        lizard_primitive_time_eval);
  install_one(heap, env, "profile",          lizard_primitive_profile);
  install_one(heap, env, "error-location",   lizard_primitive_error_location);
  /* List operations. */
  install_one(heap, env, "length",           lizard_primitive_length);
  install_one(heap, env, "append",           lizard_primitive_append);
  install_one(heap, env, "reverse",          lizard_primitive_reverse);
  install_one(heap, env, "list?",            lizard_primitive_listp);
  install_one(heap, env, "apply",            lizard_primitive_apply);
  install_one(heap, env, "map",              lizard_primitive_map);
  install_one(heap, env, "filter",           lizard_primitive_filter);
  install_one(heap, env, "for-each",         lizard_primitive_for_each);
  /* More list utilities. */
  install_one(heap, env, "fold-left",        lizard_primitive_fold_left);
  install_one(heap, env, "assoc",            lizard_primitive_assoc);
  install_one(heap, env, "member",           lizard_primitive_member);
  install_one(heap, env, "list-ref",         lizard_primitive_list_ref);
  install_one(heap, env, "iota",             lizard_primitive_iota);
  /* Track R: syntax objects. */
  install_one(heap, env, "datum->syntax",    lizard_primitive_datum_to_syntax);
  install_one(heap, env, "syntax->datum",    lizard_primitive_syntax_to_datum);
  install_one(heap, env, "syntax-e",         lizard_primitive_syntax_to_datum);
  install_one(heap, env, "syntax-source",    lizard_primitive_syntax_source);
  install_one(heap, env, "syntax?",          lizard_primitive_syntaxp);
  /* Track C: persistent vectors. */
  install_one(heap, env, "pvec",             lizard_primitive_pvec);
  install_one(heap, env, "pvec-ref",         lizard_primitive_pvec_ref);
  install_one(heap, env, "pvec-set",         lizard_primitive_pvec_set);
  install_one(heap, env, "pvec-push",        lizard_primitive_pvec_push);
  install_one(heap, env, "pvec-count",       lizard_primitive_pvec_count);
  install_one(heap, env, "pvec->list",       lizard_primitive_pvec_to_list);
  install_one(heap, env, "pvec?",            lizard_primitive_pvecp);
  /* Track C: persistent hash maps. */
  install_one(heap, env, "phash-map",        lizard_primitive_phash_map);
  install_one(heap, env, "phash-get",        lizard_primitive_phash_get);
  install_one(heap, env, "phash-set",        lizard_primitive_phash_set);
  install_one(heap, env, "phash-keys",       lizard_primitive_phash_keys);
  install_one(heap, env, "phash-count",      lizard_primitive_phash_count);
  install_one(heap, env, "phash-map?",       lizard_primitive_phash_mapp);
  /* Track K: kernel primitives. */
  install_one(heap, env, "kernel-check",     lizard_primitive_kernel_check);
  install_one(heap, env, "kernel-infer",     lizard_primitive_kernel_infer);
  /* Track K: tactics. */
  install_one(heap, env, "begin-proof",      lizard_primitive_begin_proof);
  install_one(heap, env, "proof-state",      lizard_primitive_proof_state);
  install_one(heap, env, "tactic-intro",     lizard_primitive_tactic_intro);
  install_one(heap, env, "tactic-exact",     lizard_primitive_tactic_exact);
  install_one(heap, env, "tactic-refl",      lizard_primitive_tactic_refl);
  install_one(heap, env, "qed",              lizard_primitive_qed);
  install_one(heap, env, "tactic-assumption",lizard_primitive_tactic_assumption);
  install_one(heap, env, "tactic-simpl",     lizard_primitive_tactic_simpl);
  install_one(heap, env, "tactic-split",     lizard_primitive_tactic_split);
  install_one(heap, env, "tactic-left",      lizard_primitive_tactic_left);
  install_one(heap, env, "tactic-right",     lizard_primitive_tactic_right);
  /* Track C: hash map utilities. */
  install_one(heap, env, "phash-values",     lizard_primitive_phash_values);
  install_one(heap, env, "phash-entries",    lizard_primitive_phash_entries);
  /* Track C.4: atoms. */
  install_one(heap, env, "atom",             lizard_primitive_atom);
  install_one(heap, env, "deref",            lizard_primitive_deref);
  install_one(heap, env, "swap!",            lizard_primitive_swap);
  install_one(heap, env, "reset!",           lizard_primitive_reset);
  install_one(heap, env, "atom?",            lizard_primitive_atomp);
  /* Exceptions — raise/try already registered below; guard is new. */
  install_one(heap, env, "guard",            lizard_primitive_guard);
  /* String operations. */
  install_one(heap, env, "string-ref",       lizard_primitive_string_ref);
  install_one(heap, env, "string-contains?", lizard_primitive_string_contains);
  install_one(heap, env, "string-upcase",    lizard_primitive_string_upcase);
  install_one(heap, env, "string-downcase",  lizard_primitive_string_downcase);
  install_one(heap, env, "string-split",     lizard_primitive_string_split);
  install_one(heap, env, "string-join",      lizard_primitive_string_join);
  /* Lazy evaluation — delay/force from lizard.c, promise? is new. */
  install_one(heap, env, "delay",            lizard_primitive_delay);
  install_one(heap, env, "force",            lizard_primitive_force);
  install_one(heap, env, "promise?",         lizard_primitive_promisep);
  /* Kernel reduction. */
  install_one(heap, env, "kernel-reduce",    lizard_primitive_kernel_reduce);
  install_one(heap, env, "kernel-equal?",    lizard_primitive_kernel_equalp);
  /* Metavariables. */
  install_one(heap, env, "kernel-hole",      lizard_primitive_kernel_hole);
  install_one(heap, env, "kernel-solve",     lizard_primitive_kernel_solve);
  install_one(heap, env, "kernel-zonk",      lizard_primitive_kernel_zonk);
  install_one(heap, env, "kernel-meta-state",lizard_primitive_kernel_meta_state);
  install_one(heap, env, "kernel-unify",     lizard_primitive_kernel_unify);
  install_one(heap, env, "kernel-define",    lizard_primitive_kernel_define);
  install_one(heap, env, "kernel-lookup",    lizard_primitive_kernel_lookup);
  /* Core Scheme predicates + math. */
  install_one(heap, env, "zero?",            lizard_primitive_zerop);
  install_one(heap, env, "positive?",        lizard_primitive_positivep);
  install_one(heap, env, "negative?",        lizard_primitive_negativep);
  install_one(heap, env, "even?",            lizard_primitive_evenp);
  install_one(heap, env, "odd?",             lizard_primitive_oddp);
  install_one(heap, env, "min",              lizard_primitive_min);
  install_one(heap, env, "max",              lizard_primitive_max);
  /* Character + string conversion. */
  install_one(heap, env, "char->integer",    lizard_primitive_char_to_int);
  install_one(heap, env, "integer->char",    lizard_primitive_int_to_char);
  install_one(heap, env, "string->list",     lizard_primitive_string_to_list);
  install_one(heap, env, "list->string",     lizard_primitive_list_to_string);
  install_one(heap, env, "string-reverse",   lizard_primitive_string_reverse);
  /* Track C.3: Transients. */
  install_one(heap, env, "transient!",       lizard_primitive_transient);
  install_one(heap, env, "conj!",            lizard_primitive_conj_mut);
  install_one(heap, env, "persistent!",      lizard_primitive_persistent);
  install_one(heap, env, "arithmetic-shift", lizard_primitive_arith_shift);
  install_one(heap, env, "expt",            lizard_primitive_expt);
  install_one(heap, env, "gcd",             lizard_primitive_gcd);
  install_one(heap, env, "lcm",             lizard_primitive_lcm);
  install_one(heap, env, "quotient",        lizard_primitive_quotient);
  install_one(heap, env, "remainder",       lizard_primitive_remainder);
  install_one(heap, env, "abs",             lizard_primitive_abs);
  install_one(heap, env, "square",          lizard_primitive_square);
  install_one(heap, env, "modular-expt",    lizard_primitive_modexpt);
  /* Reflection. */
  install_one(heap, env, "type-of",         lizard_primitive_type_of);
  install_one(heap, env, "env-keys",        lizard_primitive_env_keys);
  install_one(heap, env, "defined?",        lizard_primitive_definedp);
  install_one(heap, env, "procedure-arity", lizard_primitive_proc_arity);
  /* String ops. */
  install_one(heap, env, "string-length",   lizard_primitive_str_length);
  install_one(heap, env, "string-append",   lizard_primitive_str_append);
  install_one(heap, env, "substring",       lizard_primitive_substring);
  install_one(heap, env, "string=?",        lizard_primitive_str_eq);
  install_one(heap, env, "number->string",  lizard_primitive_num_to_str);
  install_one(heap, env, "string->number",  lizard_primitive_str_to_num);
  install_one(heap, env, "symbol->string",  lizard_primitive_sym_to_str);
  install_one(heap, env, "string->symbol",  lizard_primitive_str_to_sym);
  /* Exceptions + gensym. */
  install_one(heap, env, "raise",           lizard_primitive_raise);
  install_one(heap, env, "try",             lizard_primitive_try);
  install_one(heap, env, "error-object?",   lizard_primitive_error_objp);
  install_one(heap, env, "error-value",     lizard_primitive_error_value);
  install_one(heap, env, "gensym",          lizard_primitive_gensym);
  /* Vectors. */
  install_one(heap, env, "make-vector",     lizard_primitive_make_vector);
  install_one(heap, env, "vector",          lizard_primitive_vector);
  install_one(heap, env, "vector?",         lizard_primitive_vectorp);
  install_one(heap, env, "vector-length",   lizard_primitive_vec_length);
  install_one(heap, env, "vector-ref",      lizard_primitive_vec_ref);
  install_one(heap, env, "vector-set!",     lizard_primitive_vec_set);
  install_one(heap, env, "vector->list",    lizard_primitive_vec_to_list);
  install_one(heap, env, "list->vector",    lizard_primitive_list_to_vec);
  /* Hash tables. */
  install_one(heap, env, "make-hash-table", lizard_primitive_make_hash);
  install_one(heap, env, "hash?",           lizard_primitive_hashp);
  install_one(heap, env, "hash-set!",       lizard_primitive_hash_set);
  install_one(heap, env, "hash-ref",        lizard_primitive_hash_ref);
  install_one(heap, env, "hash-has-key?",   lizard_primitive_hash_has);
  install_one(heap, env, "hash-size",       lizard_primitive_hash_size);
  install_one(heap, env, "hash-keys",       lizard_primitive_hash_keys);
  install_one(heap, env, "hash-remove!",    lizard_primitive_hash_remove);
  /* ----- Type-theory notation. NO CHECKING happens here; these are
   * opaque carriers for designing the surface of a foundational
   * system. See the comment above the primitives for the full caveat. */
  install_one(heap, env, "Pi",            lizard_primitive_tt_pi);
  install_one(heap, env, "Sigma",         lizard_primitive_tt_sigma);
  install_one(heap, env, "pi-fresh",      lizard_primitive_tt_pi_fresh);
  install_one(heap, env, "pi-fresh?",     lizard_primitive_tt_pi_freshp);
  install_one(heap, env, "sigma-fresh",   lizard_primitive_tt_sigma_fresh);
  install_one(heap, env, "sigma-fresh?",  lizard_primitive_tt_sigma_freshp);
  install_one(heap, env, "co-pi-fresh",     lizard_primitive_tt_co_pi_fresh);
  install_one(heap, env, "co-pi-fresh?",    lizard_primitive_tt_co_pi_freshp);
  install_one(heap, env, "co-sigma-fresh",  lizard_primitive_tt_co_sigma_fresh);
  install_one(heap, env, "co-sigma-fresh?", lizard_primitive_tt_co_sigma_freshp);
  /* Phase M.5.1 — modal type constructors. */
  install_one(heap, env, "Box",           lizard_primitive_tt_box);
  install_one(heap, env, "Box?",          lizard_primitive_tt_boxp);
  install_one(heap, env, "Box-arg",       lizard_primitive_tt_box_arg);
  install_one(heap, env, "Diamond",       lizard_primitive_tt_diamond);
  install_one(heap, env, "Diamond?",      lizard_primitive_tt_diamondp);
  install_one(heap, env, "Diamond-arg",   lizard_primitive_tt_diamond_arg);
  /* Phase M.5.2 — Box intro and elim. */
  install_one(heap, env, "box",            lizard_primitive_tt_box_intro);
  install_one(heap, env, "box?",           lizard_primitive_tt_box_introp);
  install_one(heap, env, "box-body",       lizard_primitive_tt_box_intro_body);
  install_one(heap, env, "unbox",          lizard_primitive_tt_box_elim);
  install_one(heap, env, "unbox?",         lizard_primitive_tt_box_elimp);
  install_one(heap, env, "unbox-binder",   lizard_primitive_tt_box_elim_binder);
  install_one(heap, env, "unbox-scrutinee",lizard_primitive_tt_box_elim_scrutinee);
  install_one(heap, env, "unbox-body",     lizard_primitive_tt_box_elim_body);
  /* Phase M.5.5 — Diamond intro and elim. */
  install_one(heap, env, "diamond",        lizard_primitive_tt_diamond_intro);
  install_one(heap, env, "diamond?",       lizard_primitive_tt_diamond_introp);
  install_one(heap, env, "diamond-body",   lizard_primitive_tt_diamond_intro_body);
  install_one(heap, env, "let-diamond",    lizard_primitive_tt_diamond_elim);
  install_one(heap, env, "let-diamond?",   lizard_primitive_tt_diamond_elimp);
  install_one(heap, env, "let-diamond-binder",    lizard_primitive_tt_diamond_elim_binder);
  install_one(heap, env, "let-diamond-scrutinee", lizard_primitive_tt_diamond_elim_scrutinee);
  install_one(heap, env, "let-diamond-body",      lizard_primitive_tt_diamond_elim_body);
  /* Phase M.5.6 — K-axiom application. */
  install_one(heap, env, "box-app",   lizard_primitive_tt_box_app);
  install_one(heap, env, "box-app?",  lizard_primitive_tt_box_appp);
  install_one(heap, env, "box-app-fun", lizard_primitive_tt_box_app_fun);
  install_one(heap, env, "box-app-arg", lizard_primitive_tt_box_app_arg);
  /* Phase M.5.8 — Diamond Kleisli composition. */
  install_one(heap, env, "diamond-bind",      lizard_primitive_tt_diamond_bind);
  install_one(heap, env, "diamond-bind?",     lizard_primitive_tt_diamond_bindp);
  install_one(heap, env, "diamond-bind-fun",  lizard_primitive_tt_diamond_bind_fun);
  install_one(heap, env, "diamond-bind-arg",  lizard_primitive_tt_diamond_bind_arg);
  /* Phase M.5.9 — symmetric S5 forms. */
  install_one(heap, env, "dia",               lizard_primitive_tt_diamond_intro_sym);
  install_one(heap, env, "dia?",              lizard_primitive_tt_diamond_intro_symp);
  install_one(heap, env, "dia-body",          lizard_primitive_tt_diamond_intro_sym_body);
  install_one(heap, env, "poss-coerce",       lizard_primitive_tt_poss_coerce);
  install_one(heap, env, "poss-coerce?",      lizard_primitive_tt_poss_coercep);
  install_one(heap, env, "poss-coerce-body",  lizard_primitive_tt_poss_coerce_body);
  install_one(heap, env, "@",             lizard_primitive_tt_at);
  install_one(heap, env, "Sum",           lizard_primitive_tt_sum);
  install_one(heap, env, "U",             lizard_primitive_tt_universe);
  install_one(heap, env, "Uco",           lizard_primitive_tt_couniverse);
  install_one(heap, env, "U-set",         lizard_primitive_tt_universe_set);
  install_one(heap, env, "U-set?",        lizard_primitive_tt_universe_setp);
  install_one(heap, env, "Co-set",        lizard_primitive_tt_couniverse_set);
  install_one(heap, env, "Co-set?",       lizard_primitive_tt_couniverse_setp);
  install_one(heap, env, "Co-max",        lizard_primitive_tt_co_max);
  install_one(heap, env, "Co-max?",       lizard_primitive_tt_co_maxp);
  install_one(heap, env, "Co-min",        lizard_primitive_tt_co_min);
  install_one(heap, env, "Co-min?",       lizard_primitive_tt_co_minp);
  /* Phase H.1 — HIT scaffolding. */
  install_one(heap, env, "HIT",                  lizard_primitive_tt_hit_decl);
  install_one(heap, env, "HIT?",                 lizard_primitive_tt_hit_declp);
  install_one(heap, env, "HIT-constructor",      lizard_primitive_tt_hit_constructor);
  install_one(heap, env, "HIT-constructor?",     lizard_primitive_tt_hit_constructorp);
  install_one(heap, env, "HIT-path",             lizard_primitive_tt_hit_path);
  install_one(heap, env, "HIT-path?",            lizard_primitive_tt_hit_pathp);
  install_one(heap, env, "HIT-ref",              lizard_primitive_tt_hit_ref);
  install_one(heap, env, "HIT-ref?",             lizard_primitive_tt_hit_refp);
  install_one(heap, env, "HIT-app",              lizard_primitive_tt_hit_app);
  install_one(heap, env, "HIT-app?",             lizard_primitive_tt_hit_appp);
  install_one(heap, env, "HIT-lookup",           lizard_primitive_tt_hit_lookup);
  /* Phase M.1 — logic-rule configuration. */
  install_one(heap, env, "logic-rule-register",  lizard_primitive_logic_rule_register);
  install_one(heap, env, "logic-rule-enable",    lizard_primitive_logic_rule_enable);
  install_one(heap, env, "logic-rule-disable",   lizard_primitive_logic_rule_disable);
  install_one(heap, env, "logic-rule-enabled?",  lizard_primitive_logic_rule_enabledp);
  install_one(heap, env, "logic-config",         lizard_primitive_logic_config);
  install_one(heap, env, "logic-config-size",    lizard_primitive_logic_config_size);
  install_one(heap, env, "logic-config-reset",   lizard_primitive_logic_config_reset);
  /* Phase M.3 — named logic bundles. */
  install_one(heap, env, "set-logic",            lizard_primitive_set_logic);
  install_one(heap, env, "current-logic",        lizard_primitive_current_logic);
  install_one(heap, env, "list-logics",          lizard_primitive_list_logics);
  install_one(heap, env, "Id",            lizard_primitive_tt_id);
  install_one(heap, env, "refl",          lizard_primitive_tt_refl);
  install_one(heap, env, "Inductive",     lizard_primitive_tt_inductive);
  install_one(heap, env, "Coinductive",   lizard_primitive_tt_coinductive);
  install_one(heap, env, "annot",         lizard_primitive_tt_annot);
  /* TT predicates */
  install_one(heap, env, "Pi?",           lizard_primitive_tt_pip);
  install_one(heap, env, "Sigma?",        lizard_primitive_tt_sigmap);
  install_one(heap, env, "@?",            lizard_primitive_tt_appp);
  install_one(heap, env, "app?",          lizard_primitive_tt_appp);
  install_one(heap, env, "Sum?",          lizard_primitive_tt_sump);
  install_one(heap, env, "U?",            lizard_primitive_tt_universep);
  install_one(heap, env, "Uco?",          lizard_primitive_tt_couniversep);
  install_one(heap, env, "Id?",           lizard_primitive_tt_idp);
  install_one(heap, env, "refl?",         lizard_primitive_tt_reflp);
  install_one(heap, env, "Inductive?",    lizard_primitive_tt_inductivep);
  install_one(heap, env, "Coinductive?",  lizard_primitive_tt_coinductivep);
  install_one(heap, env, "annot?",        lizard_primitive_tt_annotp);
  /* TT accessors */
  install_one(heap, env, "Pi-binder",     lizard_primitive_tt_pi_binder);
  install_one(heap, env, "Pi-domain",     lizard_primitive_tt_pi_domain);
  install_one(heap, env, "Pi-codomain",   lizard_primitive_tt_pi_codomain);
  install_one(heap, env, "Sigma-binder",  lizard_primitive_tt_sigma_binder);
  install_one(heap, env, "Sigma-domain",  lizard_primitive_tt_sigma_domain);
  install_one(heap, env, "Sigma-codomain",lizard_primitive_tt_sigma_codomain);
  install_one(heap, env, "@-fun",         lizard_primitive_tt_app_fun);
  install_one(heap, env, "@-arg",         lizard_primitive_tt_app_arg);
  install_one(heap, env, "app-fun",       lizard_primitive_tt_app_fun);
  install_one(heap, env, "app-arg",       lizard_primitive_tt_app_arg);
  install_one(heap, env, "Sum-left",      lizard_primitive_tt_sum_left);
  install_one(heap, env, "Sum-right",     lizard_primitive_tt_sum_right);
  install_one(heap, env, "U-level",       lizard_primitive_tt_universe_level);
  install_one(heap, env, "Uco-level",     lizard_primitive_tt_couniverse_level);
  install_one(heap, env, "Id-domain",     lizard_primitive_tt_id_domain);
  install_one(heap, env, "Id-a",          lizard_primitive_tt_id_a);
  install_one(heap, env, "Id-b",          lizard_primitive_tt_id_b);
  install_one(heap, env, "refl-value",    lizard_primitive_tt_refl_value);
  install_one(heap, env, "Inductive-name",
              lizard_primitive_tt_inductive_name);
  install_one(heap, env, "Inductive-constructors",
              lizard_primitive_tt_inductive_ctors);
  install_one(heap, env, "Coinductive-name",
              lizard_primitive_tt_coinductive_name);
  install_one(heap, env, "Coinductive-destructors",
              lizard_primitive_tt_coinductive_dtors);
  install_one(heap, env, "annot-term",    lizard_primitive_tt_annot_term);
  install_one(heap, env, "annot-type",    lizard_primitive_tt_annot_type);
  /* Context layer — couniverse-stratified bindings, contexts,
     substitutions, judgments. NO checking. */
  install_one(heap, env, "variable",      lizard_primitive_tt_variable);
  install_one(heap, env, "context",       lizard_primitive_tt_context);
  install_one(heap, env, "empty-context", lizard_primitive_tt_empty_ctx);
  install_one(heap, env, "context-extend",lizard_primitive_tt_ctx_extend);
  install_one(heap, env, "context-lookup",lizard_primitive_tt_ctx_lookup);
  install_one(heap, env, "context-bindings", lizard_primitive_tt_ctx_bindings);
  install_one(heap, env, "context-length",lizard_primitive_tt_ctx_length);
  install_one(heap, env, "substitution",  lizard_primitive_tt_substitution);
  install_one(heap, env, "judgment",      lizard_primitive_tt_judgment);
  install_one(heap, env, "uco-level",     lizard_primitive_tt_uco_level);
  /* Context layer predicates */
  install_one(heap, env, "variable?",     lizard_primitive_tt_variablep);
  install_one(heap, env, "context?",      lizard_primitive_tt_contextp);
  install_one(heap, env, "substitution?", lizard_primitive_tt_substitutionp);
  install_one(heap, env, "judgment?",     lizard_primitive_tt_judgmentp);
  /* Context layer accessors */
  install_one(heap, env, "variable-name", lizard_primitive_tt_var_name);
  install_one(heap, env, "variable-type", lizard_primitive_tt_var_type);
  install_one(heap, env, "substitution-source", lizard_primitive_tt_subst_source);
  install_one(heap, env, "substitution-target", lizard_primitive_tt_subst_target);
  install_one(heap, env, "judgment-context", lizard_primitive_tt_judg_context);
  install_one(heap, env, "judgment-term", lizard_primitive_tt_judg_term);
  install_one(heap, env, "judgment-type", lizard_primitive_tt_judg_type);
  /* Identity manipulation + equivalence (notation only). */
  install_one(heap, env, "equivalence",   lizard_primitive_tt_equiv);
  install_one(heap, env, "transport",     lizard_primitive_tt_transport);
  install_one(heap, env, "Id-sym",        lizard_primitive_tt_id_sym);
  install_one(heap, env, "Id-trans",      lizard_primitive_tt_id_trans);
  install_one(heap, env, "equivalence?",  lizard_primitive_tt_equivp);
  install_one(heap, env, "transport?",    lizard_primitive_tt_transportp);
  install_one(heap, env, "Id-sym?",       lizard_primitive_tt_id_symp);
  install_one(heap, env, "Id-trans?",     lizard_primitive_tt_id_transp);
  install_one(heap, env, "equivalence-left",  lizard_primitive_tt_equiv_left);
  install_one(heap, env, "equivalence-right", lizard_primitive_tt_equiv_right);
  install_one(heap, env, "equivalence-fwd",   lizard_primitive_tt_equiv_fwd);
  install_one(heap, env, "equivalence-bwd",   lizard_primitive_tt_equiv_bwd);
  install_one(heap, env, "transport-path",    lizard_primitive_tt_transport_path);
  install_one(heap, env, "transport-value",   lizard_primitive_tt_transport_value);
  install_one(heap, env, "Id-sym-path",       lizard_primitive_tt_id_sym_path);
  install_one(heap, env, "Id-trans-p",        lizard_primitive_tt_id_trans_p);
  install_one(heap, env, "Id-trans-q",        lizard_primitive_tt_id_trans_q);
  /* TT-level lambda for pi-beta reduction. */
  install_one(heap, env, "Lambda",      lizard_primitive_tt_lambda);
  install_one(heap, env, "Lambda?",     lizard_primitive_tt_lambdap);
  install_one(heap, env, "Lambda-binder", lizard_primitive_tt_lambda_binder);
  install_one(heap, env, "Lambda-body",   lizard_primitive_tt_lambda_body);
  /* HOTT-fragment: ap is congruence of identity along a function. */
  install_one(heap, env, "ap",         lizard_primitive_tt_ap);
  install_one(heap, env, "ap?",        lizard_primitive_tt_app_p_hott);
  install_one(heap, env, "ap-fn",      lizard_primitive_tt_ap_fn);
  install_one(heap, env, "ap-path",    lizard_primitive_tt_ap_path);
  /* HOTT-fragment: Sigma intro/elim, Sum intro/elim, Unit, Bot, J.
   * The Sigma-intro/elim names are capitalized (Pair, Pair?) to avoid
   * collision with Scheme's cons-pair? predicate. */
  install_one(heap, env, "Pair",       lizard_primitive_tt_pair);
  install_one(heap, env, "Pair?",      lizard_primitive_tt_pairp);
  install_one(heap, env, "fst",        lizard_primitive_tt_fst);
  install_one(heap, env, "fst?",       lizard_primitive_tt_fstp);
  install_one(heap, env, "fst-target", lizard_primitive_tt_fst_target);
  install_one(heap, env, "snd",        lizard_primitive_tt_snd);
  install_one(heap, env, "snd?",       lizard_primitive_tt_sndp);
  install_one(heap, env, "snd-target", lizard_primitive_tt_snd_target);
  install_one(heap, env, "Pair-first", lizard_primitive_tt_pair_first);
  install_one(heap, env, "Pair-second",lizard_primitive_tt_pair_second);
  install_one(heap, env, "inl",        lizard_primitive_tt_inl);
  install_one(heap, env, "inl?",       lizard_primitive_tt_inlp);
  install_one(heap, env, "inl-value",  lizard_primitive_tt_inl_value);
  install_one(heap, env, "inr",        lizard_primitive_tt_inr);
  install_one(heap, env, "inr?",       lizard_primitive_tt_inrp);
  install_one(heap, env, "inr-value",  lizard_primitive_tt_inr_value);
  install_one(heap, env, "Case",       lizard_primitive_tt_case);
  install_one(heap, env, "Case?",      lizard_primitive_tt_casep);
  install_one(heap, env, "Case-scrutinee", lizard_primitive_tt_case_scrutinee);
  install_one(heap, env, "Case-left",  lizard_primitive_tt_case_left);
  install_one(heap, env, "Case-right", lizard_primitive_tt_case_right);
  install_one(heap, env, "Unit",       lizard_primitive_tt_unit);
  install_one(heap, env, "Unit?",      lizard_primitive_tt_unitp);
  install_one(heap, env, "tt",         lizard_primitive_tt_tt);
  install_one(heap, env, "tt?",        lizard_primitive_tt_ttp);
  install_one(heap, env, "Bot",        lizard_primitive_tt_bot);
  install_one(heap, env, "Bot?",       lizard_primitive_tt_botp);
  install_one(heap, env, "J",          lizard_primitive_tt_j);
  install_one(heap, env, "J?",         lizard_primitive_tt_jp);
  install_one(heap, env, "J-motive",   lizard_primitive_tt_j_motive);
  install_one(heap, env, "J-refl-case",lizard_primitive_tt_j_refl_case);
  install_one(heap, env, "J-path",     lizard_primitive_tt_j_path);
  install_one(heap, env, "xport",      lizard_primitive_tt_xport);
  install_one(heap, env, "xport?",     lizard_primitive_tt_xportp);
  install_one(heap, env, "xport-motive", lizard_primitive_tt_xport_motive);
  install_one(heap, env, "xport-path",   lizard_primitive_tt_xport_path);
  install_one(heap, env, "xport-value",  lizard_primitive_tt_xport_value);
  /* Universe-expression layer for universe polymorphism + cumulativity. */
  install_one(heap, env, "U-var",      lizard_primitive_tt_u_var);
  install_one(heap, env, "U-var?",     lizard_primitive_tt_u_varp);
  install_one(heap, env, "U-suc",      lizard_primitive_tt_u_suc);
  install_one(heap, env, "U-suc?",     lizard_primitive_tt_u_sucp);
  install_one(heap, env, "U-suc-operand", lizard_primitive_tt_u_suc_operand);
  install_one(heap, env, "U-max",      lizard_primitive_tt_u_max);
  install_one(heap, env, "U-max?",     lizard_primitive_tt_u_maxp);
  install_one(heap, env, "U-min",      lizard_primitive_tt_u_min);
  install_one(heap, env, "U-min?",     lizard_primitive_tt_u_minp);
  install_one(heap, env, "U-min-left", lizard_primitive_tt_u_min_left);
  install_one(heap, env, "U-min-right",lizard_primitive_tt_u_min_right);
  install_one(heap, env, "U-max-left", lizard_primitive_tt_u_max_left);
  install_one(heap, env, "U-max-right",lizard_primitive_tt_u_max_right);
  /* Cubical layer: interval, paths, and connection operations.
   * Foundation for cubical computational univalence. */
  install_one(heap, env, "I",          lizard_primitive_tt_interval);
  install_one(heap, env, "I?",         lizard_primitive_tt_intervalp);
  install_one(heap, env, "i0",         lizard_primitive_tt_i0);
  install_one(heap, env, "i0?",        lizard_primitive_tt_i0p);
  install_one(heap, env, "i1",         lizard_primitive_tt_i1);
  install_one(heap, env, "i1?",        lizard_primitive_tt_i1p);
  install_one(heap, env, "I-var",      lizard_primitive_tt_i_var);
  install_one(heap, env, "I-var?",     lizard_primitive_tt_i_varp);
  install_one(heap, env, "I-and",      lizard_primitive_tt_i_and);
  install_one(heap, env, "I-and?",     lizard_primitive_tt_i_andp);
  install_one(heap, env, "I-or",       lizard_primitive_tt_i_or);
  install_one(heap, env, "I-or?",      lizard_primitive_tt_i_orp);
  install_one(heap, env, "I-neg",      lizard_primitive_tt_i_neg);
  install_one(heap, env, "I-neg?",     lizard_primitive_tt_i_negp);
  install_one(heap, env, "Path",       lizard_primitive_tt_path);
  install_one(heap, env, "Path?",      lizard_primitive_tt_pathp);
  install_one(heap, env, "Path-domain",lizard_primitive_tt_path_domain);
  install_one(heap, env, "Path-a",     lizard_primitive_tt_path_a);
  install_one(heap, env, "Path-b",     lizard_primitive_tt_path_b);
  install_one(heap, env, "path-abs",   lizard_primitive_tt_path_abs);
  install_one(heap, env, "path-abs?",  lizard_primitive_tt_path_absp);
  install_one(heap, env, "path-abs-binder", lizard_primitive_tt_path_abs_binder);
  install_one(heap, env, "path-abs-body",   lizard_primitive_tt_path_abs_body);
  install_one(heap, env, "path-app",   lizard_primitive_tt_path_app);
  install_one(heap, env, "path-app?",  lizard_primitive_tt_path_appp);
  install_one(heap, env, "path-app-path",  lizard_primitive_tt_path_app_path);
  install_one(heap, env, "path-app-point", lizard_primitive_tt_path_app_point);
  /* Face formulas and partial elements (Turn 7). */
  install_one(heap, env, "F0",        lizard_primitive_tt_f0);
  install_one(heap, env, "F0?",       lizard_primitive_tt_f0p);
  install_one(heap, env, "F1",        lizard_primitive_tt_f1);
  install_one(heap, env, "F1?",       lizard_primitive_tt_f1p);
  install_one(heap, env, "F-eq",      lizard_primitive_tt_f_eq);
  install_one(heap, env, "F-eq?",     lizard_primitive_tt_f_eqp);
  install_one(heap, env, "F-eq-left", lizard_primitive_tt_f_eq_left);
  install_one(heap, env, "F-eq-right",lizard_primitive_tt_f_eq_right);
  install_one(heap, env, "F-and",     lizard_primitive_tt_f_and);
  install_one(heap, env, "F-and?",    lizard_primitive_tt_f_andp);
  install_one(heap, env, "F-or",      lizard_primitive_tt_f_or);
  install_one(heap, env, "F-or?",     lizard_primitive_tt_f_orp);
  install_one(heap, env, "Partial",   lizard_primitive_tt_partial);
  install_one(heap, env, "Partial?",  lizard_primitive_tt_partialp);
  install_one(heap, env, "Partial-face", lizard_primitive_tt_partial_face);
  install_one(heap, env, "Partial-type", lizard_primitive_tt_partial_type);
  install_one(heap, env, "Sub",       lizard_primitive_tt_sub);
  install_one(heap, env, "Sub?",      lizard_primitive_tt_subp);
  install_one(heap, env, "Sub-type",  lizard_primitive_tt_sub_type);
  install_one(heap, env, "Sub-face",  lizard_primitive_tt_sub_face);
  install_one(heap, env, "Sub-partial", lizard_primitive_tt_sub_partial);
  /* Kan composition (Turn 8). */
  install_one(heap, env, "comp",       lizard_primitive_tt_comp);
  install_one(heap, env, "comp?",      lizard_primitive_tt_compp);
  install_one(heap, env, "hcomp",      lizard_primitive_tt_hcomp);
  install_one(heap, env, "hcomp?",     lizard_primitive_tt_hcompp);
  install_one(heap, env, "fill",       lizard_primitive_tt_fill);
  install_one(heap, env, "fill?",      lizard_primitive_tt_fillp);
  install_one(heap, env, "comp-type-family", lizard_primitive_tt_comp_type_family);
  install_one(heap, env, "comp-face",     lizard_primitive_tt_comp_face);
  install_one(heap, env, "comp-partial",  lizard_primitive_tt_comp_partial);
  install_one(heap, env, "comp-base",     lizard_primitive_tt_comp_base);
  /* Equivalences, Glue, ua (Turns 9 & 10). */
  install_one(heap, env, "Equiv",         lizard_primitive_tt_equiv_type);
  install_one(heap, env, "Equiv?",        lizard_primitive_tt_equiv_typep);
  install_one(heap, env, "Equiv-domain",  lizard_primitive_tt_equiv_domain);
  install_one(heap, env, "Equiv-codomain",lizard_primitive_tt_equiv_codomain);
  install_one(heap, env, "id-equiv",      lizard_primitive_tt_id_equiv);
  install_one(heap, env, "id-equiv?",     lizard_primitive_tt_id_equivp);
  install_one(heap, env, "equiv-fun",     lizard_primitive_tt_equiv_fun);
  install_one(heap, env, "equiv-fun?",    lizard_primitive_tt_equiv_funp);
  install_one(heap, env, "equiv-inv",     lizard_primitive_tt_equiv_inv);
  install_one(heap, env, "equiv-inv?",    lizard_primitive_tt_equiv_invp);
  install_one(heap, env, "Glue",          lizard_primitive_tt_glue);
  install_one(heap, env, "Glue?",         lizard_primitive_tt_gluep);
  install_one(heap, env, "Glue-base",     lizard_primitive_tt_glue_base);
  install_one(heap, env, "Glue-face",     lizard_primitive_tt_glue_face);
  install_one(heap, env, "Glue-t",        lizard_primitive_tt_glue_t);
  install_one(heap, env, "Glue-equiv",    lizard_primitive_tt_glue_equiv);
  install_one(heap, env, "glue-intro",    lizard_primitive_tt_glue_intro);
  install_one(heap, env, "glue-intro?",   lizard_primitive_tt_glue_introp);
  install_one(heap, env, "unglue",        lizard_primitive_tt_unglue);
  install_one(heap, env, "unglue?",       lizard_primitive_tt_unglue_p);
  install_one(heap, env, "ua",            lizard_primitive_tt_ua);
  install_one(heap, env, "ua?",           lizard_primitive_tt_uap);
  /* Systems (Turn 11). */
  install_one(heap, env, "system-nil",    lizard_primitive_tt_system_nil);
  install_one(heap, env, "system-nil?",   lizard_primitive_tt_system_nilp);
  install_one(heap, env, "system-cons",   lizard_primitive_tt_system_cons);
  install_one(heap, env, "system-cons?",  lizard_primitive_tt_system_consp);
  install_one(heap, env, "system-face",   lizard_primitive_tt_system_face);
  install_one(heap, env, "system-value",  lizard_primitive_tt_system_value);
  install_one(heap, env, "system-next",   lizard_primitive_tt_system_next);
  install_one(heap, env, "system-lookup", lizard_primitive_tt_system_lookup);
  /* Optional proof-theory scaffolds. */
  install_one(heap, env, "S1", lizard_primitive_tt_s1);
  install_one(heap, env, "S1?", lizard_primitive_tt_s1p);
  install_one(heap, env, "base", lizard_primitive_tt_s1_base);
  install_one(heap, env, "base?", lizard_primitive_tt_s1_basep);
  install_one(heap, env, "loop", lizard_primitive_tt_s1_loop);
  install_one(heap, env, "loop?", lizard_primitive_tt_s1_loopp);
  install_one(heap, env, "Trunc", lizard_primitive_tt_trunc);
  install_one(heap, env, "Trunc?", lizard_primitive_tt_truncp);
  install_one(heap, env, "Trunc-level", lizard_primitive_tt_trunc_level);
  install_one(heap, env, "Trunc-type", lizard_primitive_tt_trunc_type);
  install_one(heap, env, "trunc", lizard_primitive_tt_trunc_intro);
  install_one(heap, env, "trunc?", lizard_primitive_tt_trunc_introp);
  install_one(heap, env, "trunc-value", lizard_primitive_tt_trunc_value);
  install_one(heap, env, "trunc-elim", lizard_primitive_tt_trunc_elim);
  install_one(heap, env, "trunc-elim?", lizard_primitive_tt_trunc_elimp);
  install_one(heap, env, "trunc-elim-motive", lizard_primitive_tt_trunc_elim_motive);
  install_one(heap, env, "trunc-elim-handler", lizard_primitive_tt_trunc_elim_handler);
  install_one(heap, env, "trunc-elim-value", lizard_primitive_tt_trunc_elim_value);
  install_one(heap, env, "trunc-elim-prop", lizard_primitive_tt_trunc_elim_prop);
  install_one(heap, env, "theory-extension", lizard_primitive_tt_extension);
  install_one(heap, env, "theory-extension?", lizard_primitive_tt_extensionp);
  install_one(heap, env, "theory-extension-name", lizard_primitive_tt_extension_name);
  install_one(heap, env, "theory-extension-args", lizard_primitive_tt_extension_args);
  install_one(heap, env, "universe-leq?", lizard_primitive_tt_universe_leq);
  install_one(heap, env, "couniverse-leq?", lizard_primitive_tt_couniverse_leq);
  install_one(heap, env, "face-entails?", lizard_primitive_tt_face_entails);
  /* Bidirectional type checker for the λΠ fragment.
   *   (infer Γ t)    returns the inferred type of t in context Γ.
   *   (check Γ t T)  returns #t if t checks against T in Γ.
   * Currently covers: variables, Pi, Lambda, application, U, annotation. */
  install_one(heap, env, "infer",      lizard_primitive_tt_infer);
  install_one(heap, env, "check",      lizard_primitive_tt_check);
  /* M.5.2 Turn 2 — two-context inference. */
  install_one(heap, env, "infer-modal", lizard_primitive_tt_infer_modal);
  install_one(heap, env, "check-modal", lizard_primitive_tt_check_modal);
  /* Judgmental equality engine + flag system. The engine implements
   * a small confluent rewriting system for the identity-type
   * fragment. Each rule is gated by a flag, all #t by default. */
  lizard_tt_flags_init();
  install_one(heap, env, "reduce",        lizard_primitive_tt_reduce);
  install_one(heap, env, "equal?",        lizard_primitive_tt_equal);
  install_one(heap, env, "flag-set!",     lizard_primitive_flag_set);
  install_one(heap, env, "flag-get",      lizard_primitive_flag_get);
  install_one(heap, env, "flag-list",     lizard_primitive_flag_list);
}
