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
static int no_args(lz_list_t *args);
static int single_arg(lz_list_t *args);
static int two_args(lz_list_t *args);
static int three_args(lz_list_t *args);
static int four_args(lz_list_t *args);

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
  tokens = lizard_tokenize(input);
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
  tokens = lizard_tokenize(input);
  ast_list = lizard_parse(tokens, heap);
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
lizard_ast_node_t *lizard_primitive_load(lz_list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg;
  const char *path;
  FILE *fp;
  long file_size;
  char *source;
  size_t got;
  lz_list_t *tokens;
  lz_list_t *ast_list;
  lz_list_node_t *iter;
  lizard_ast_node_t *result;

  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_ARGC);
  }
  arg = (lizard_ast_list_node_t *)args->head;
  if (arg->ast->type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_ARGT);
  }
  path = arg->ast->data.string;
  fp = fopen(path, "rb");
  if (!fp) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_OPEN);
  }
  if (fseek(fp, 0L, SEEK_END) != 0) {
    fclose(fp);
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_READ);
  }
  file_size = ftell(fp);
  if (file_size < 0) {
    fclose(fp);
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_READ);
  }
  rewind(fp);
  source = (char *)lizard_heap_alloc((size_t)file_size + 1);
  got = fread(source, 1, (size_t)file_size, fp);
  fclose(fp);
  if (got != (size_t)file_size) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_READ);
  }
  source[file_size] = '\0';

  tokens = lizard_tokenize(source);
  ast_list = lizard_parse(tokens, heap);
  result = lizard_make_nil(heap);
  for (iter = ast_list->head; iter != ast_list->nil; iter = iter->next) {
    lizard_ast_node_t *expanded = lizard_expand_macros(
        ((lizard_ast_list_node_t *)iter)->ast, env, heap);
    result = lizard_eval(expanded, env, heap, lizard_identity_cont);
    if (result && result->type == AST_ERROR) {
      return result;
    }
  }
  return result;
}

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
static FILE *resolve_module_file(const char *path, lizard_heap_t *heap,
                                 char **out_resolved) {
  FILE *fp;
  lizard_search_path_t *sp;
  /* Try the raw path first. */
  fp = fopen(path, "rb");
  if (fp != NULL) {
    size_t len = strlen(path) + 1;
    *out_resolved = (char *)lizard_heap_alloc(len);
    strcpy(*out_resolved, path);
    return fp;
  }
  /* Try each search path directory. */
  if (heap->runtime != NULL) {
    for (sp = heap->runtime->search_path_head; sp != NULL; sp = sp->next) {
      size_t dir_len = strlen(sp->directory);
      size_t path_len = strlen(path);
      char *full = (char *)lizard_heap_alloc(dir_len + path_len + 1);
      strcpy(full, sp->directory);
      strcat(full, path);
      fp = fopen(full, "rb");
      if (fp != NULL) {
        *out_resolved = full;
        return fp;
      }
    }
  }
  *out_resolved = NULL;
  return NULL;
}

static lizard_module_entry_t *find_cached_module(lizard_heap_t *heap,
                                                  const char *path) {
  lizard_module_entry_t *e;
  if (heap->runtime == NULL) return NULL;
  for (e = heap->runtime->modules_head; e != NULL; e = e->next) {
    if (strcmp(e->canonical_path, path) == 0) return e;
  }
  return NULL;
}

static void cache_module(lizard_heap_t *heap, const char *path,
                         lizard_ast_node_t *result) {
  lizard_module_entry_t *e;
  if (heap->runtime == NULL) return;
  e = (lizard_module_entry_t *)malloc(sizeof(lizard_module_entry_t));
  if (e == NULL) return;
  e->canonical_path = (char *)malloc(strlen(path) + 1);
  if (e->canonical_path != NULL) {
    strcpy(e->canonical_path, path);
  }
  e->result = result;
  e->next = heap->runtime->modules_head;
  heap->runtime->modules_head = e;
}

/* (import "path.lisp")
 * Like load, but:
 *   1. Resolves via the module search path
 *   2. Caches the result — second import of the same path is a no-op
 *   3. Returns the cached result of the last expression */
lizard_ast_node_t *lizard_primitive_import(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg;
  const char *path;
  char *resolved;
  FILE *fp;
  long file_size;
  char *source;
  size_t got;
  lz_list_t *tokens;
  lz_list_t *ast_list;
  lz_list_node_t *iter;
  lizard_ast_node_t *result;
  lizard_module_entry_t *cached;

  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_ARGC);
  }
  arg = (lizard_ast_list_node_t *)args->head;
  if (arg->ast->type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_ARGT);
  }
  path = arg->ast->data.string;

  /* Check cache first. */
  cached = find_cached_module(heap, path);
  if (cached != NULL) {
    return cached->result;
  }

  /* Resolve via search path. */
  fp = resolve_module_file(path, heap, &resolved);
  if (fp == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_OPEN);
  }
  /* Also check cache under the resolved path. */
  if (resolved != NULL && strcmp(resolved, path) != 0) {
    cached = find_cached_module(heap, resolved);
    if (cached != NULL) {
      fclose(fp);
      return cached->result;
    }
  }

  /* Read file. */
  if (fseek(fp, 0L, SEEK_END) != 0) {
    fclose(fp);
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_READ);
  }
  file_size = ftell(fp);
  if (file_size < 0) {
    fclose(fp);
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_READ);
  }
  rewind(fp);
  source = (char *)lizard_heap_alloc((size_t)file_size + 1);
  got = fread(source, 1, (size_t)file_size, fp);
  fclose(fp);
  if (got != (size_t)file_size) {
    return lizard_make_error(heap, LIZARD_ERROR_LOAD_READ);
  }
  source[file_size] = '\0';

  /* Evaluate in the current environment (like load). */
  tokens = lizard_tokenize(source);
  ast_list = lizard_parse(tokens, heap);
  result = lizard_make_nil(heap);
  for (iter = ast_list->head; iter != ast_list->nil; iter = iter->next) {
    lizard_ast_node_t *expanded = lizard_expand_macros(
        ((lizard_ast_list_node_t *)iter)->ast, env, heap);
    result = lizard_eval(expanded, env, heap, lizard_identity_cont);
    if (result && result->type == AST_ERROR) {
      return result;
    }
  }

  /* Cache under both the original and resolved paths. */
  cache_module(heap, path, result);
  if (resolved != NULL && strcmp(resolved, path) != 0) {
    cache_module(heap, resolved, result);
  }
  return result;
}

/* (module-loaded? "path") — #t if already imported, #f otherwise. */
lizard_ast_node_t *lizard_primitive_module_loadedp(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  arg = (lizard_ast_list_node_t *)args->head;
  if (arg->ast->type != AST_STRING) {
    return lizard_make_bool(heap, 0);
  }
  return lizard_make_bool(heap, find_cached_module(heap, arg->ast->data.string) != NULL);
}

/* (module-search-path) — returns the search path as a list of strings. */
lizard_ast_node_t *lizard_primitive_module_search_path(lz_list_t *args,
                                                        lizard_env_t *env,
                                                        lizard_heap_t *heap) {
  lizard_ast_node_t *result;
  lizard_search_path_t *sp;
  (void)env;
  (void)args;
  result = lizard_make_nil(heap);
  if (heap->runtime == NULL) return result;
  /* Build list in reverse so the result is in path order. */
  {
    lizard_search_path_t *entries[64];
    int count = 0;
    int i;
    for (sp = heap->runtime->search_path_head; sp != NULL && count < 64; sp = sp->next) {
      entries[count++] = sp;
    }
    for (i = count - 1; i >= 0; i--) {
      lizard_ast_node_t *pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      lizard_ast_node_t *str = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      str->type = AST_STRING;
      str->data.string = entries[i]->directory;
      pair->type = AST_PAIR;
      pair->data.pair.car = str;
      pair->data.pair.cdr = result;
      result = pair;
    }
  }
  return result;
}

/* (add-module-path! "dir") — prepend a directory to the search path. */
lizard_ast_node_t *lizard_primitive_add_module_path(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg;
  const char *dir;
  lizard_search_path_t *entry;
  size_t len;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  arg = (lizard_ast_list_node_t *)args->head;
  if (arg->ast->type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  if (heap->runtime == NULL) {
    return lizard_make_bool(heap, 0);
  }
  dir = arg->ast->data.string;
  len = strlen(dir);
  entry = (lizard_search_path_t *)malloc(sizeof(lizard_search_path_t));
  if (entry == NULL) {
    return lizard_make_bool(heap, 0);
  }
  /* Ensure trailing slash. */
  if (len > 0 && dir[len - 1] == '/') {
    entry->directory = (char *)malloc(len + 1);
    if (entry->directory) strcpy(entry->directory, dir);
  } else {
    entry->directory = (char *)malloc(len + 2);
    if (entry->directory) {
      strcpy(entry->directory, dir);
      strcat(entry->directory, "/");
    }
  }
  entry->next = heap->runtime->search_path_head;
  heap->runtime->search_path_head = entry;
  return lizard_make_bool(heap, 1);
}

/* ---- primitive registration ----
   Install every built-in primitive into the given environment.
   Called by both the REPL and the test harness so the set of
   primitives stays in one place. */

/* Phase D: GC stats primitive. */
#include "gc.h"

/* Helper: build an alist pair (key . value) where value is an unsigned long. */
static lizard_ast_node_t *gc_make_stat(lizard_heap_t *heap,
                                        const char *key,
                                        unsigned long val) {
  lizard_ast_node_t *k = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  lizard_ast_node_t *v = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  lizard_ast_node_t *p = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  k->type = AST_SYMBOL;
  k->data.variable = key;
  v->type = AST_NUMBER;
  mpz_init_set_ui(v->data.number, val);
  p->type = AST_PAIR;
  p->data.pair.car = k;
  p->data.pair.cdr = v;
  return p;
}

static lizard_ast_node_t *gc_cons(lizard_heap_t *heap,
                                   lizard_ast_node_t *car,
                                   lizard_ast_node_t *cdr) {
  lizard_ast_node_t *c = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  c->type = AST_PAIR;
  c->data.pair.car = car;
  c->data.pair.cdr = cdr;
  return c;
}

lizard_ast_node_t *lizard_primitive_gc_stats(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_gc_stats_t stats;
  lizard_ast_node_t *result;
  (void)args;

  lizard_gc_collect_stats(heap, env, 1, &stats);

  /* Build alist: ((segments . N) (bytes . N) (total . N) (live . N) (garbage . N)) */
  result = lizard_make_nil(heap);
  result = gc_cons(heap, gc_make_stat(heap, "garbage",  (unsigned long)stats.nodes_garbage), result);
  result = gc_cons(heap, gc_make_stat(heap, "live",     (unsigned long)stats.nodes_marked),  result);
  result = gc_cons(heap, gc_make_stat(heap, "total",    (unsigned long)stats.nodes_total),   result);
  result = gc_cons(heap, gc_make_stat(heap, "bytes",    (unsigned long)stats.total_bytes_allocated), result);
  result = gc_cons(heap, gc_make_stat(heap, "segments", (unsigned long)stats.total_segments), result);
  return result;
}

/* (gc) — run a full GC cycle: mark from roots, free dead segments.
 * Returns an alist with before/after stats and bytes freed. */
lizard_ast_node_t *lizard_primitive_gc(lz_list_t *args,
                                        lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_gc_stats_t before, after;
  size_t freed;
  lizard_ast_node_t *result;
  (void)args;

  /* Stats before collection. */
  lizard_gc_collect_stats(heap, env, 1, &before);

  /* Collect — frees dead segments. */
  freed = lizard_gc_collect(heap, env);

  /* Stats after. */
  lizard_gc_collect_stats(heap, env, 1, &after);

  /* Build result alist. */
  result = lizard_make_nil(heap);
  result = gc_cons(heap, gc_make_stat(heap, "freed-bytes",      (unsigned long)freed),                result);
  result = gc_cons(heap, gc_make_stat(heap, "segments-after",   (unsigned long)after.total_segments), result);
  result = gc_cons(heap, gc_make_stat(heap, "segments-before",  (unsigned long)before.total_segments),result);
  result = gc_cons(heap, gc_make_stat(heap, "garbage",          (unsigned long)before.nodes_garbage), result);
  result = gc_cons(heap, gc_make_stat(heap, "live",             (unsigned long)before.nodes_marked),  result);
  return result;
}

/* Phase E: bytecode VM primitives. */
#include "bytecode.h"

static const char *opcode_name(lizard_opcode_t op) {
  switch (op) {
  case OP_CONST: return "CONST";
  case OP_NIL: return "NIL";
  case OP_TRUE: return "TRUE";
  case OP_FALSE: return "FALSE";
  case OP_LOAD: return "LOAD";
  case OP_STORE: return "STORE";
  case OP_SET: return "SET";
  case OP_POP: return "POP";
  case OP_ADD: return "ADD";
  case OP_SUB: return "SUB";
  case OP_MUL: return "MUL";
  case OP_DIV: return "DIV";
  case OP_MOD: return "MOD";
  case OP_EQ: return "EQ";
  case OP_LT: return "LT";
  case OP_GT: return "GT";
  case OP_NOT: return "NOT";
  case OP_CONS: return "CONS";
  case OP_CAR: return "CAR";
  case OP_CDR: return "CDR";
  case OP_JUMP: return "JUMP";
  case OP_JUMP_IF_FALSE: return "JIF";
  case OP_CALL: return "CALL";
  case OP_TAIL_CALL: return "TCALL";
  case OP_RETURN: return "RET";
  case OP_CLOSURE: return "CLOSURE";
  case OP_DISPLAY: return "DISPLAY";
  case OP_NEWLINE: return "NEWLINE";
  case OP_HALT: return "HALT";
  }
  return "???";
}

/* (vm-eval expr) — compile expr to bytecode and execute on the VM. */
lizard_ast_node_t *lizard_primitive_vm_eval(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  lizard_bc_chunk_t *chunk;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  /* Strict bytecode: the VM only executes the runtime fragment.  Type-theory
   * terms (Pi/Sigma/U/Id/... through extensions) are not runtime values and
   * must be rejected rather than silently compiled. */
  if (expr != NULL && expr->type >= AST_TT_PI &&
      expr->type <= AST_TT_EXTENSION) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  chunk = lizard_compile(expr, heap);
  if (chunk == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  return lizard_vm_exec(chunk, env, heap);
}

/* (disassemble expr) — compile expr and print the bytecode. */
lizard_ast_node_t *lizard_primitive_disassemble(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  lizard_bc_chunk_t *chunk;
  int i;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  chunk = lizard_compile(expr, heap);
  if (chunk == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  printf("--- bytecode: %d instructions, %d constants ---\n",
         chunk->code_count, chunk->const_count);
  for (i = 0; i < chunk->code_count; i++) {
    printf("  %04d  %-8s %d", i, opcode_name(chunk->code[i].op),
           chunk->code[i].arg);
    /* Show constant value for CONST/LOAD/STORE instructions. */
    if ((chunk->code[i].op == OP_CONST ||
         chunk->code[i].op == OP_LOAD ||
         chunk->code[i].op == OP_STORE) &&
        chunk->code[i].arg < chunk->const_count) {
      printf("  ; ");
      lizard_fprint_value(stdout, chunk->constants[chunk->code[i].arg]);
    }
    printf("\n");
  }
  printf("---\n");
  return lizard_make_bool(heap, 1);
}

#include <time.h>

/* (vm-time expr) — compile + execute via bytecode VM, print elapsed time. */
lizard_ast_node_t *lizard_primitive_vm_time(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  lizard_ast_node_t *result;
  lizard_bc_chunk_t *chunk;
  clock_t start, end;
  double elapsed;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  chunk = lizard_compile(expr, heap);
  if (chunk == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  start = clock();
  result = lizard_vm_exec(chunk, env, heap);
  end = clock();
  elapsed = (double)(end - start) / (double)CLOCKS_PER_SEC;
  printf("; vm-time: %.6f s\n", elapsed);
  return result;
}

/* (time-eval expr) — evaluate via tree-walker, print elapsed time. */
lizard_ast_node_t *lizard_primitive_time_eval(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  lizard_ast_node_t *result;
  clock_t start, end;
  double elapsed;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  start = clock();
  result = lizard_eval(expr, env, heap, lizard_identity_cont);
  end = clock();
  elapsed = (double)(end - start) / (double)CLOCKS_PER_SEC;
  printf("; time-eval: %.6f s\n", elapsed);
  return result;
}

/* (profile expr) — compile + execute with full instruction profiling. */
lizard_ast_node_t *lizard_primitive_profile(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  lizard_ast_node_t *result;
  lizard_bc_chunk_t *chunk;
  lizard_vm_profile_t prof;
  clock_t start, end;
  int i;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  chunk = lizard_compile(expr, heap);
  if (chunk == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  memset(&prof, 0, sizeof(prof));
  start = clock();
  result = lizard_vm_exec_profiled(chunk, env, heap, &prof);
  end = clock();
  prof.elapsed_seconds = (double)(end - start) / (double)CLOCKS_PER_SEC;

  printf("--- profile ---\n");
  printf("  elapsed:      %.6f s\n", prof.elapsed_seconds);
  printf("  instructions: %lu\n", (unsigned long)prof.total_instructions);
  printf("  calls:        %lu\n", (unsigned long)prof.total_calls);
  printf("  tail calls:   %lu\n", (unsigned long)prof.tail_calls);
  if (prof.total_instructions > 0) {
    printf("  MIPS:         %.1f\n",
           (double)prof.total_instructions / prof.elapsed_seconds / 1e6);
  }
  printf("  top opcodes:\n");
  for (i = 0; i < LIZARD_OPCODE_COUNT; i++) {
    if (prof.opcode_counts[i] > 0) {
      printf("    %-8s %lu\n", opcode_name((lizard_opcode_t)i),
             (unsigned long)prof.opcode_counts[i]);
    }
  }
  printf("---\n");
  return result;
}

/* (error-location err) — return source location of an error as an alist,
 * or '() if no location is available. */
lizard_ast_node_t *lizard_primitive_error_location(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *err;
  lizard_ast_node_t *result;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  err = ((lizard_ast_list_node_t *)args->head)->ast;
  if (err == NULL || err->type != AST_ERROR) {
    return lizard_make_nil(heap);
  }
  if (err->span.start_line == 0) {
    return lizard_make_nil(heap);
  }
  /* Build alist: ((line . N) (column . N)) */
  result = lizard_make_nil(heap);
  result = gc_cons(heap, gc_make_stat(heap, "column", (unsigned long)err->span.start_column), result);
  result = gc_cons(heap, gc_make_stat(heap, "line",   (unsigned long)err->span.start_line),   result);
  return result;
}

/* ============================================================
 * Practical list operations — essential R5RS subset.
 * ============================================================ */

/* (length lst) — number of elements in a proper list. */
lizard_ast_node_t *lizard_primitive_length(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *lst;
  lizard_ast_node_t *r;
  long count = 0;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  lst = ((lizard_ast_list_node_t *)args->head)->ast;
  while (lst != NULL && lst->type == AST_PAIR) {
    count++;
    lst = lst->data.pair.cdr;
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, count);
  return r;
}

/* (append lst1 lst2) — concatenate two lists. */
lizard_ast_node_t *lizard_primitive_append(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *a, *b, *result, *tail, *node;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  a = ((lizard_ast_list_node_t *)args->head)->ast;
  b = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (a == NULL || a->type == AST_NIL) return b;
  /* Copy the first list. */
  result = NULL;
  tail = NULL;
  while (a != NULL && a->type == AST_PAIR) {
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = a->data.pair.car;
    node->data.pair.cdr = lizard_make_nil(heap);
    if (tail != NULL) {
      tail->data.pair.cdr = node;
    } else {
      result = node;
    }
    tail = node;
    a = a->data.pair.cdr;
  }
  if (tail != NULL) {
    tail->data.pair.cdr = b;
  } else {
    result = b;
  }
  return result;
}

/* (reverse lst) — reverse a list. */
lizard_ast_node_t *lizard_primitive_reverse(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *lst, *result, *node;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  lst = ((lizard_ast_list_node_t *)args->head)->ast;
  result = lizard_make_nil(heap);
  while (lst != NULL && lst->type == AST_PAIR) {
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = lst->data.pair.car;
    node->data.pair.cdr = result;
    result = node;
    lst = lst->data.pair.cdr;
  }
  return result;
}

/* (list? x) — #t if x is a proper list (nil-terminated chain of pairs). */
lizard_ast_node_t *lizard_primitive_listp(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = ((lizard_ast_list_node_t *)args->head)->ast;
  while (x != NULL && x->type == AST_PAIR) {
    x = x->data.pair.cdr;
  }
  return lizard_make_bool(heap, x != NULL && x->type == AST_NIL);
}

/* (apply f args-list) — apply function to a list of arguments. */
lizard_ast_node_t *lizard_primitive_apply(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *fn, *arg_list, *cur;
  lz_list_t *call_args;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  fn = ((lizard_ast_list_node_t *)args->head)->ast;
  arg_list = ((lizard_ast_list_node_t *)args->head->next)->ast;

  /* Build the lz_list_t from the Scheme list. */
  call_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  cur = arg_list;
  while (cur != NULL && cur->type == AST_PAIR) {
    lizard_ast_list_node_t *n;
    n = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    n->ast = cur->data.pair.car;
    list_append(call_args, &n->node);
    cur = cur->data.pair.cdr;
  }

  return lizard_apply(fn, call_args, env, heap, lizard_identity_cont);
}

/* (map f lst) — apply f to each element, return list of results. */
lizard_ast_node_t *lizard_primitive_map(lz_list_t *args,
                                         lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *fn, *lst, *result, *tail;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  fn = ((lizard_ast_list_node_t *)args->head)->ast;
  lst = ((lizard_ast_list_node_t *)args->head->next)->ast;
  result = lizard_make_nil(heap);
  tail = NULL;
  while (lst != NULL && lst->type == AST_PAIR) {
    lizard_ast_node_t *val, *node;
    lz_list_t *one_arg;
    lizard_ast_list_node_t *n;
    one_arg = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    n = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    n->ast = lst->data.pair.car;
    list_append(one_arg, &n->node);
    val = lizard_apply(fn, one_arg, env, heap, lizard_identity_cont);
    if (val && val->type == AST_ERROR) return val;
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = val;
    node->data.pair.cdr = lizard_make_nil(heap);
    if (tail != NULL) {
      tail->data.pair.cdr = node;
    } else {
      result = node;
    }
    tail = node;
    lst = lst->data.pair.cdr;
  }
  return result;
}

/* (filter pred lst) — keep elements where (pred elem) is truthy. */
lizard_ast_node_t *lizard_primitive_filter(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *fn, *lst, *result, *tail;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  fn = ((lizard_ast_list_node_t *)args->head)->ast;
  lst = ((lizard_ast_list_node_t *)args->head->next)->ast;
  result = lizard_make_nil(heap);
  tail = NULL;
  while (lst != NULL && lst->type == AST_PAIR) {
    lizard_ast_node_t *test, *node;
    lz_list_t *one_arg;
    lizard_ast_list_node_t *n;
    one_arg = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    n = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    n->ast = lst->data.pair.car;
    list_append(one_arg, &n->node);
    test = lizard_apply(fn, one_arg, env, heap, lizard_identity_cont);
    if (test && test->type == AST_ERROR) return test;
    /* Keep if truthy (not #f and not nil). */
    if (!(test->type == AST_BOOL && !test->data.boolean) &&
        test->type != AST_NIL) {
      node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      node->type = AST_PAIR;
      node->data.pair.car = lst->data.pair.car;
      node->data.pair.cdr = lizard_make_nil(heap);
      if (tail != NULL) {
        tail->data.pair.cdr = node;
      } else {
        result = node;
      }
      tail = node;
    }
    lst = lst->data.pair.cdr;
  }
  return result;
}

/* (for-each f lst) — apply f to each element for side effects. */
lizard_ast_node_t *lizard_primitive_for_each(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *fn, *lst, *val;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  fn = ((lizard_ast_list_node_t *)args->head)->ast;
  lst = ((lizard_ast_list_node_t *)args->head->next)->ast;
  while (lst != NULL && lst->type == AST_PAIR) {
    lz_list_t *one_arg;
    lizard_ast_list_node_t *n;
    one_arg = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    n = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    n->ast = lst->data.pair.car;
    list_append(one_arg, &n->node);
    val = lizard_apply(fn, one_arg, env, heap, lizard_identity_cont);
    if (val && val->type == AST_ERROR) return val;
    lst = lst->data.pair.cdr;
  }
  return lizard_make_nil(heap);
}

/* (fold-left f init lst) — left fold: (f (f (f init e0) e1) e2) ... */
lizard_ast_node_t *lizard_primitive_fold_left(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *fn, *acc, *lst;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  fn  = ((lizard_ast_list_node_t *)args->head)->ast;
  acc = ((lizard_ast_list_node_t *)args->head->next)->ast;
  lst = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
  while (lst != NULL && lst->type == AST_PAIR) {
    lz_list_t *call_args;
    lizard_ast_list_node_t *n1, *n2;
    call_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
    n1 = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    n1->ast = acc;
    list_append(call_args, &n1->node);
    n2 = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    n2->ast = lst->data.pair.car;
    list_append(call_args, &n2->node);
    acc = lizard_apply(fn, call_args, env, heap, lizard_identity_cont);
    if (acc && acc->type == AST_ERROR) return acc;
    lst = lst->data.pair.cdr;
  }
  return acc;
}

/* (assoc key alist) — find pair with matching car (using equal?). */
lizard_ast_node_t *lizard_primitive_assoc(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *key, *alist, *pair;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  key   = ((lizard_ast_list_node_t *)args->head)->ast;
  alist = ((lizard_ast_list_node_t *)args->head->next)->ast;
  while (alist != NULL && alist->type == AST_PAIR) {
    pair = alist->data.pair.car;
    if (pair != NULL && pair->type == AST_PAIR) {
      lizard_ast_node_t *k = pair->data.pair.car;
      /* Simple equality: same type + same value. */
      if (k->type == key->type) {
        int eq = 0;
        if (k->type == AST_NUMBER) {
          eq = (mpz_cmp(k->data.number, key->data.number) == 0);
        } else if (k->type == AST_SYMBOL) {
          eq = (strcmp(k->data.variable, key->data.variable) == 0);
        } else if (k->type == AST_STRING) {
          eq = (strcmp(k->data.string, key->data.string) == 0);
        } else if (k->type == AST_BOOL) {
          eq = (k->data.boolean == key->data.boolean);
        } else if (k->type == AST_NIL) {
          eq = 1;
        }
        if (eq) return pair;
      }
    }
    alist = alist->data.pair.cdr;
  }
  return lizard_make_bool(heap, 0);  /* #f if not found */
}

/* (member x lst) — find first sublist starting with x, or #f. */
lizard_ast_node_t *lizard_primitive_member(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *key, *lst, *elem;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  key = ((lizard_ast_list_node_t *)args->head)->ast;
  lst = ((lizard_ast_list_node_t *)args->head->next)->ast;
  while (lst != NULL && lst->type == AST_PAIR) {
    elem = lst->data.pair.car;
    if (elem->type == key->type) {
      int eq = 0;
      if (elem->type == AST_NUMBER) {
        eq = (mpz_cmp(elem->data.number, key->data.number) == 0);
      } else if (elem->type == AST_SYMBOL) {
        eq = (strcmp(elem->data.variable, key->data.variable) == 0);
      } else if (elem->type == AST_STRING) {
        eq = (strcmp(elem->data.string, key->data.string) == 0);
      } else if (elem->type == AST_BOOL) {
        eq = (elem->data.boolean == key->data.boolean);
      } else if (elem->type == AST_NIL) {
        eq = 1;
      }
      if (eq) return lst;
    }
    lst = lst->data.pair.cdr;
  }
  return lizard_make_bool(heap, 0);
}

/* (list-ref lst n) — 0-indexed element access. */
lizard_ast_node_t *lizard_primitive_list_ref(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *lst, *idx;
  long n, i;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  lst = ((lizard_ast_list_node_t *)args->head)->ast;
  idx = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (idx->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = mpz_get_si(idx->data.number);
  i = 0;
  while (lst != NULL && lst->type == AST_PAIR) {
    if (i == n) return lst->data.pair.car;
    i++;
    lst = lst->data.pair.cdr;
  }
  return lizard_make_error(heap, LIZARD_ERROR_USER);
}

/* (iota n) — list of integers 0..n-1. (iota n start) — start..start+n-1. */
lizard_ast_node_t *lizard_primitive_iota(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *count_node, *start_node;
  lizard_ast_node_t *result, *node;
  long count, start, i;
  (void)env;
  count_node = ((lizard_ast_list_node_t *)args->head)->ast;
  if (count_node->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  count = mpz_get_si(count_node->data.number);
  start = 0;
  if (args->head->next != args->nil) {
    start_node = ((lizard_ast_list_node_t *)args->head->next)->ast;
    if (start_node->type == AST_NUMBER) {
      start = mpz_get_si(start_node->data.number);
    }
  }
  result = lizard_make_nil(heap);
  for (i = count - 1 + start; i >= start; i--) {
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.cdr = result;
    node->data.pair.car = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->data.pair.car->type = AST_NUMBER;
    mpz_init_set_si(node->data.pair.car->data.number, i);
    result = node;
  }
  return result;
}

/* ============================================================
 * Track R: Syntax object primitives.
 * ============================================================ */

/* (datum->syntax context datum) — wrap datum with lexical context. */
lizard_ast_node_t *lizard_primitive_datum_to_syntax(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *ctx_node, *datum, *stx;
  lizard_env_t *ctx;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  ctx_node = ((lizard_ast_list_node_t *)args->head)->ast;
  datum = ((lizard_ast_list_node_t *)args->head->next)->ast;
  /* If context is a syntax object, use its context; otherwise use current env. */
  ctx = (ctx_node->type == AST_SYNTAX) ? ctx_node->data.syntax.context : env;
  stx = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  stx->type = AST_SYNTAX;
  stx->data.syntax.datum = datum;
  stx->data.syntax.context = ctx;
  stx->data.syntax.source = datum->span;
  stx->data.syntax.phase = 0;
  stx->data.syntax.scopes = NULL;
  stx->data.syntax.scope_count = 0;
  return stx;
}

/* (syntax->datum stx) / (syntax-e stx) — unwrap syntax object. */
lizard_ast_node_t *lizard_primitive_syntax_to_datum(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *stx;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  stx = ((lizard_ast_list_node_t *)args->head)->ast;
  if (stx->type != AST_SYNTAX) {
    return stx;  /* non-syntax passes through */
  }
  return stx->data.syntax.datum;
}

/* (syntax-source stx) — return source location as alist. */
lizard_ast_node_t *lizard_primitive_syntax_source(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *stx, *result;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  stx = ((lizard_ast_list_node_t *)args->head)->ast;
  if (stx->type != AST_SYNTAX || stx->data.syntax.source.start_line == 0) {
    return lizard_make_nil(heap);
  }
  result = lizard_make_nil(heap);
  result = gc_cons(heap, gc_make_stat(heap, "column", (unsigned long)stx->data.syntax.source.start_column), result);
  result = gc_cons(heap, gc_make_stat(heap, "line",   (unsigned long)stx->data.syntax.source.start_line),   result);
  return result;
}

/* (syntax? x) — predicate. */
lizard_ast_node_t *lizard_primitive_syntaxp(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  return lizard_make_bool(heap,
      ((lizard_ast_list_node_t *)args->head)->ast->type == AST_SYNTAX);
}

/* ============================================================
 * Track C: Persistent vector primitives.
 *
 * Initial implementation: copy-on-write flat array. Will be
 * upgraded to a 32-way trie in Track C Phase 2 for O(log32 n)
 * updates on large vectors.
 * ============================================================ */

/* (pvec e1 e2 ...) — create a persistent vector from elements. */
lizard_ast_node_t *lizard_primitive_pvec(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *v;
  lz_list_node_t *iter;
  int count, i;
  (void)env;
  count = 0;
  for (iter = args->head; iter != args->nil; iter = iter->next) count++;
  v = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  v->type = AST_PVEC;
  v->data.pvec.count = count;
  v->data.pvec.capacity = count > 0 ? count : 4;
  v->data.pvec.entries = (lizard_ast_node_t **)lizard_heap_alloc(
      (size_t)v->data.pvec.capacity * sizeof(lizard_ast_node_t *));
  i = 0;
  for (iter = args->head; iter != args->nil; iter = iter->next) {
    v->data.pvec.entries[i++] = ((lizard_ast_list_node_t *)iter)->ast;
  }
  return v;
}

/* (pvec-ref v i) — 0-indexed access. */
lizard_ast_node_t *lizard_primitive_pvec_ref(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *v, *idx;
  long i;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  idx = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (v->type != AST_PVEC || idx->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  i = mpz_get_si(idx->data.number);
  if (i < 0 || i >= v->data.pvec.count) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  return v->data.pvec.entries[i];
}

/* (pvec-set v i val) — functional update, returns new vector. */
lizard_ast_node_t *lizard_primitive_pvec_set(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *v, *idx, *val, *nv;
  long i;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v   = ((lizard_ast_list_node_t *)args->head)->ast;
  idx = ((lizard_ast_list_node_t *)args->head->next)->ast;
  val = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
  if (v->type != AST_PVEC || idx->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  i = mpz_get_si(idx->data.number);
  if (i < 0 || i >= v->data.pvec.count) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  /* Copy-on-write: allocate a new vector with the updated element. */
  nv = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  nv->type = AST_PVEC;
  nv->data.pvec.count = v->data.pvec.count;
  nv->data.pvec.capacity = v->data.pvec.count;
  nv->data.pvec.entries = (lizard_ast_node_t **)lizard_heap_alloc(
      (size_t)nv->data.pvec.count * sizeof(lizard_ast_node_t *));
  memcpy(nv->data.pvec.entries, v->data.pvec.entries,
         (size_t)nv->data.pvec.count * sizeof(lizard_ast_node_t *));
  nv->data.pvec.entries[i] = val;
  return nv;
}

/* (pvec-push v val) — append, returns new vector. */
lizard_ast_node_t *lizard_primitive_pvec_push(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *v, *val, *nv;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v   = ((lizard_ast_list_node_t *)args->head)->ast;
  val = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (v->type != AST_PVEC) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  nv = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  nv->type = AST_PVEC;
  nv->data.pvec.count = v->data.pvec.count + 1;
  nv->data.pvec.capacity = nv->data.pvec.count;
  nv->data.pvec.entries = (lizard_ast_node_t **)lizard_heap_alloc(
      (size_t)nv->data.pvec.count * sizeof(lizard_ast_node_t *));
  if (v->data.pvec.count > 0) {
    memcpy(nv->data.pvec.entries, v->data.pvec.entries,
           (size_t)v->data.pvec.count * sizeof(lizard_ast_node_t *));
  }
  nv->data.pvec.entries[v->data.pvec.count] = val;
  return nv;
}

/* (pvec-count v) — number of elements. */
lizard_ast_node_t *lizard_primitive_pvec_count(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *v, *r;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  if (v->type != AST_PVEC) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, v->data.pvec.count);
  return r;
}

/* (pvec->list v) — convert to a Scheme list. */
lizard_ast_node_t *lizard_primitive_pvec_to_list(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *v, *result, *node;
  int i;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  if (v->type != AST_PVEC) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_make_nil(heap);
  for (i = v->data.pvec.count - 1; i >= 0; i--) {
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = v->data.pvec.entries[i];
    node->data.pair.cdr = result;
    result = node;
  }
  return result;
}

/* (pvec? x) — predicate. */
lizard_ast_node_t *lizard_primitive_pvecp(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  return lizard_make_bool(heap,
      ((lizard_ast_list_node_t *)args->head)->ast->type == AST_PVEC);
}

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

static int hamt_key_equal(lizard_ast_node_t *a, lizard_ast_node_t *b) {
  if (a->type != b->type) return 0;
  switch (a->type) {
  case AST_NUMBER:  return mpz_cmp(a->data.number, b->data.number) == 0;
  case AST_SYMBOL:  return strcmp(a->data.variable, b->data.variable) == 0;
  case AST_STRING:  return strcmp(a->data.string, b->data.string) == 0;
  case AST_BOOL:    return a->data.boolean == b->data.boolean;
  case AST_NIL:     return 1;
  default:          return a == b;
  }
}

/* (hash-map k1 v1 k2 v2 ...) — create from alternating key-value pairs. */
lizard_ast_node_t *lizard_primitive_phash_map(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *node;
  hamt_flat_t *h;
  lz_list_node_t *iter;
  int count;
  (void)env;
  count = 0;
  for (iter = args->head; iter != args->nil; iter = iter->next) count++;
  h = (hamt_flat_t *)lizard_heap_alloc(sizeof(hamt_flat_t));
  h->count = 0;
  h->capacity = (count / 2) + 4;
  h->entries = (hamt_entry_t *)lizard_heap_alloc(
      (size_t)h->capacity * sizeof(hamt_entry_t));
  /* Parse alternating key-value pairs. */
  iter = args->head;
  while (iter != args->nil && iter->next != args->nil) {
    h->entries[h->count].key = ((lizard_ast_list_node_t *)iter)->ast;
    iter = iter->next;
    h->entries[h->count].val = ((lizard_ast_list_node_t *)iter)->ast;
    iter = iter->next;
    h->count++;
  }
  node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  node->type = AST_HAMT;
  node->data.hamt.root = h;
  node->data.hamt.count = h->count;
  return node;
}

/* (hash-get m key [default]) — lookup key, return value or default/#f. */
lizard_ast_node_t *lizard_primitive_phash_get(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *key, *dflt;
  hamt_flat_t *h;
  int i;
  (void)env;
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  key = ((lizard_ast_list_node_t *)args->head->next)->ast;
  dflt = (args->head->next->next != args->nil)
           ? ((lizard_ast_list_node_t *)args->head->next->next)->ast
           : lizard_make_bool(heap, 0);
  if (map_node->type != AST_HAMT) return dflt;
  h = (hamt_flat_t *)map_node->data.hamt.root;
  for (i = 0; i < h->count; i++) {
    if (hamt_key_equal(h->entries[i].key, key)) {
      return h->entries[i].val;
    }
  }
  return dflt;
}

/* (hash-set m key val) — functional update, returns new map. */
lizard_ast_node_t *lizard_primitive_phash_set(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *key, *val, *result;
  hamt_flat_t *h, *nh;
  int i, found;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  key = ((lizard_ast_list_node_t *)args->head->next)->ast;
  val = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
  if (map_node->type != AST_HAMT) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  h = (hamt_flat_t *)map_node->data.hamt.root;
  /* Copy-on-write. */
  nh = (hamt_flat_t *)lizard_heap_alloc(sizeof(hamt_flat_t));
  nh->capacity = h->count + 2;
  nh->entries = (hamt_entry_t *)lizard_heap_alloc(
      (size_t)nh->capacity * sizeof(hamt_entry_t));
  found = 0;
  for (i = 0; i < h->count; i++) {
    if (hamt_key_equal(h->entries[i].key, key)) {
      nh->entries[i].key = key;
      nh->entries[i].val = val;
      found = 1;
    } else {
      nh->entries[i] = h->entries[i];
    }
  }
  if (!found) {
    nh->entries[h->count].key = key;
    nh->entries[h->count].val = val;
    nh->count = h->count + 1;
  } else {
    nh->count = h->count;
  }
  result = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  result->type = AST_HAMT;
  result->data.hamt.root = nh;
  result->data.hamt.count = nh->count;
  return result;
}

/* (hash-keys m) — list of keys. */
lizard_ast_node_t *lizard_primitive_phash_keys(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *result, *node;
  hamt_flat_t *h;
  int i;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  if (map_node->type != AST_HAMT) return lizard_make_nil(heap);
  h = (hamt_flat_t *)map_node->data.hamt.root;
  result = lizard_make_nil(heap);
  for (i = h->count - 1; i >= 0; i--) {
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = h->entries[i].key;
    node->data.pair.cdr = result;
    result = node;
  }
  return result;
}

/* (hash-count m) — number of entries. */
lizard_ast_node_t *lizard_primitive_phash_count(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *r;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number,
      (map_node->type == AST_HAMT) ? map_node->data.hamt.count : 0);
  return r;
}

/* (hash-map? x) — predicate. */
lizard_ast_node_t *lizard_primitive_phash_mapp(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  return lizard_make_bool(heap,
      ((lizard_ast_list_node_t *)args->head)->ast->type == AST_HAMT);
}

/* ============================================================
 * Track K: Kernel type-checker primitives.
 * ============================================================ */
#include "kernel.h"

/* Helper: convert Lisp S-expression to kernel term. */
static kterm_t *sexp_to_kterm(lizard_heap_t *heap, lizard_ast_node_t *e) {
  if (e == NULL) return NULL;
  if (e->type == AST_PAIR) {
    /* Quoted compound data arrives as pairs; re-parse into an application so
     * the constructor branches below (which expect an argument list) fire.
     * Mirrors lizard_kernel_sexp_to_kterm in kernel_sexp.c. */
    lizard_ast_node_t *reparsed;
    reparsed = lizard_reparse_datum(e, heap);
    if (reparsed != NULL && reparsed != e) {
      return sexp_to_kterm(heap, reparsed);
    }
  }
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
lizard_ast_node_t *lizard_primitive_phash_values(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *result, *node;
  hamt_flat_t *h;
  int i;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  if (map_node->type != AST_HAMT) return lizard_make_nil(heap);
  h = (hamt_flat_t *)map_node->data.hamt.root;
  result = lizard_make_nil(heap);
  for (i = h->count - 1; i >= 0; i--) {
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = h->entries[i].val;
    node->data.pair.cdr = result;
    result = node;
  }
  return result;
}

/* (phash-entries m) — list of (key . value) pairs. */
lizard_ast_node_t *lizard_primitive_phash_entries(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *map_node, *result, *node, *pair;
  hamt_flat_t *h;
  int i;
  (void)env;
  if (!single_arg(args))
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  map_node = ((lizard_ast_list_node_t *)args->head)->ast;
  if (map_node->type != AST_HAMT) return lizard_make_nil(heap);
  h = (hamt_flat_t *)map_node->data.hamt.root;
  result = lizard_make_nil(heap);
  for (i = h->count - 1; i >= 0; i--) {
    pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    pair->type = AST_PAIR;
    pair->data.pair.car = h->entries[i].key;
    pair->data.pair.cdr = h->entries[i].val;
    node = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    node->type = AST_PAIR;
    node->data.pair.car = pair;
    node->data.pair.cdr = result;
    result = node;
  }
  return result;
}

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

/* ---------------------------------------------------------------------
 * Vectors — fixed-size, mutable, O(1) indexed access.
 * ------------------------------------------------------------------- */

static lizard_ast_node_t *make_vector(lizard_heap_t *heap, size_t n,
                                      lizard_ast_node_t *fill) {
  lizard_ast_node_t *v;
  size_t i;
  v = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  v->type = AST_VECTOR;
  v->data.vector.size = n;
  v->data.vector.elements =
      lizard_heap_alloc(n * sizeof(lizard_ast_node_t *));
  for (i = 0; i < n; i++) v->data.vector.elements[i] = fill;
  return v;
}

/* (make-vector n [fill]) */
lizard_ast_node_t *lizard_primitive_make_vector(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n_ast, *fill;
  long n;
  (void)env;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n_ast = ((lizard_ast_list_node_t *)args->head)->ast;
  if (n_ast->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = mpz_get_si(n_ast->data.number);
  if (n < 0) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  fill = (args->head->next != args->nil)
             ? ((lizard_ast_list_node_t *)args->head->next)->ast
             : lizard_make_nil(heap);
  return make_vector(heap, (size_t)n, fill);
}

/* (vector v1 v2 v3 ...) — list-literal style. */
lizard_ast_node_t *lizard_primitive_vector(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  size_t n;
  lz_list_node_t *iter;
  lizard_ast_node_t *v;
  size_t i;
  (void)env;
  n = 0;
  for (iter = args->head; iter != args->nil; iter = iter->next) n++;
  v = make_vector(heap, n, lizard_make_nil(heap));
  i = 0;
  for (iter = args->head; iter != args->nil; iter = iter->next) {
    v->data.vector.elements[i++] = ((lizard_ast_list_node_t *)iter)->ast;
  }
  return v;
}

/* (vector? x) */
lizard_ast_node_t *lizard_primitive_vectorp(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = ((lizard_ast_list_node_t *)args->head)->ast;
  return lizard_make_bool(heap, x && x->type == AST_VECTOR);
}

/* (vector-length v) */
lizard_ast_node_t *lizard_primitive_vec_length(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *v, *r;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  if (v->type != AST_VECTOR) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_ui(r->data.number, (unsigned long)v->data.vector.size);
  return r;
}

/* (vector-ref v i) */
lizard_ast_node_t *lizard_primitive_vec_ref(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *v, *idx;
  long i;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  idx = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (v->type != AST_VECTOR || idx->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  i = mpz_get_si(idx->data.number);
  if (i < 0 || (size_t)i >= v->data.vector.size) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  return v->data.vector.elements[i];
}

/* (vector-set! v i x) — mutates and returns nil. */
lizard_ast_node_t *lizard_primitive_vec_set(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *v, *idx, *val;
  long i;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next == args->nil ||
      args->head->next->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  idx = ((lizard_ast_list_node_t *)args->head->next)->ast;
  val = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
  if (v->type != AST_VECTOR || idx->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  i = mpz_get_si(idx->data.number);
  if (i < 0 || (size_t)i >= v->data.vector.size) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  v->data.vector.elements[i] = val;
  return lizard_make_nil(heap);
}

/* (vector->list v) */
lizard_ast_node_t *lizard_primitive_vec_to_list(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *v, *result, *pair;
  long i;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  v = ((lizard_ast_list_node_t *)args->head)->ast;
  if (v->type != AST_VECTOR) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_make_nil(heap);
  for (i = (long)v->data.vector.size - 1; i >= 0; i--) {
    pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    pair->type = AST_PAIR;
    pair->data.pair.car = v->data.vector.elements[i];
    pair->data.pair.cdr = result;
    result = pair;
  }
  return result;
}

/* (list->vector xs) */
lizard_ast_node_t *lizard_primitive_list_to_vec(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *xs, *cur, *v;
  size_t n, i;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  xs = ((lizard_ast_list_node_t *)args->head)->ast;
  /* count length */
  n = 0;
  cur = xs;
  while (cur && cur->type == AST_PAIR) {
    n++;
    cur = cur->data.pair.cdr;
  }
  if (cur && cur->type != AST_NIL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  v = make_vector(heap, n, lizard_make_nil(heap));
  cur = xs;
  for (i = 0; i < n; i++) {
    v->data.vector.elements[i] = cur->data.pair.car;
    cur = cur->data.pair.cdr;
  }
  return v;
}

/* ---------------------------------------------------------------------
 * Hash tables — open-addressed with linear probing.
 * Equality + hashing supported for numbers, strings, symbols, booleans.
 * ------------------------------------------------------------------- */

#define LIZARD_HASH_INIT_CAP 8

static unsigned long lizard_hash_value(lizard_ast_node_t *k) {
  unsigned long h;
  const char *s;
  switch (k->type) {
  case AST_NUMBER:
    /* mpz hashed by mod-2^64 of its first limb. */
    return mpz_get_ui(k->data.number) * 2654435761UL;
  case AST_STRING:
    s = k->data.string;
    h = 2166136261UL;
    while (*s) {
      h ^= (unsigned char)*s++;
      h *= 16777619UL;
    }
    return h;
  case AST_SYMBOL:
    s = k->data.variable;
    h = 14695981039346656037UL;
    while (*s) {
      h ^= (unsigned char)*s++;
      h *= 1099511628211UL;
    }
    return h;
  case AST_BOOL:
    return k->data.boolean ? 1u : 0u;
  case AST_NIL:
    return 2u;
  default:
    return (unsigned long)(uintptr_t)k;
  }
}

static int lizard_keys_eq(lizard_ast_node_t *a, lizard_ast_node_t *b) {
  if (!a || !b || a->type != b->type) return 0;
  switch (a->type) {
  case AST_NUMBER: return mpz_cmp(a->data.number, b->data.number) == 0;
  case AST_STRING: return strcmp(a->data.string, b->data.string) == 0;
  case AST_SYMBOL: return strcmp(a->data.variable, b->data.variable) == 0;
  case AST_BOOL:   return a->data.boolean == b->data.boolean;
  case AST_NIL:    return 1;
  default:         return a == b;
  }
}

static void hash_grow(lizard_heap_t *heap, lizard_ast_node_t *h) {
  size_t old_cap = h->data.hash.cap;
  lizard_ast_node_t **old_k = h->data.hash.keys;
  lizard_ast_node_t **old_v = h->data.hash.values;
  size_t new_cap = old_cap * 2;
  size_t i, idx;
  h->data.hash.cap = new_cap;
  h->data.hash.keys = lizard_heap_alloc(new_cap * sizeof(lizard_ast_node_t *));
  h->data.hash.values = lizard_heap_alloc(new_cap * sizeof(lizard_ast_node_t *));
  for (i = 0; i < new_cap; i++) {
    h->data.hash.keys[i] = NULL;
    h->data.hash.values[i] = NULL;
  }
  for (i = 0; i < old_cap; i++) {
    if (old_k[i] != NULL) {
      idx = lizard_hash_value(old_k[i]) & (new_cap - 1);
      while (h->data.hash.keys[idx] != NULL) {
        idx = (idx + 1) & (new_cap - 1);
      }
      h->data.hash.keys[idx] = old_k[i];
      h->data.hash.values[idx] = old_v[i];
    }
  }
}

/* (make-hash-table) */
lizard_ast_node_t *lizard_primitive_make_hash(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *h;
  size_t i;
  (void)env;
  (void)args;
  h = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  h->type = AST_HASH;
  h->data.hash.size = 0;
  h->data.hash.cap = LIZARD_HASH_INIT_CAP;
  h->data.hash.keys =
      lizard_heap_alloc(LIZARD_HASH_INIT_CAP * sizeof(lizard_ast_node_t *));
  h->data.hash.values =
      lizard_heap_alloc(LIZARD_HASH_INIT_CAP * sizeof(lizard_ast_node_t *));
  for (i = 0; i < LIZARD_HASH_INIT_CAP; i++) {
    h->data.hash.keys[i] = NULL;
    h->data.hash.values[i] = NULL;
  }
  return h;
}

/* (hash? x) */
lizard_ast_node_t *lizard_primitive_hashp(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = ((lizard_ast_list_node_t *)args->head)->ast;
  return lizard_make_bool(heap, x && x->type == AST_HASH);
}

static size_t hash_find_slot(lizard_ast_node_t *h, lizard_ast_node_t *k,
                             int *found) {
  size_t idx = lizard_hash_value(k) & (h->data.hash.cap - 1);
  while (h->data.hash.keys[idx] != NULL) {
    if (lizard_keys_eq(h->data.hash.keys[idx], k)) {
      *found = 1;
      return idx;
    }
    idx = (idx + 1) & (h->data.hash.cap - 1);
  }
  *found = 0;
  return idx;
}

/* (hash-set! h k v) */
lizard_ast_node_t *lizard_primitive_hash_set(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *h, *k, *v;
  size_t idx;
  int found;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next == args->nil ||
      args->head->next->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  h = ((lizard_ast_list_node_t *)args->head)->ast;
  k = ((lizard_ast_list_node_t *)args->head->next)->ast;
  v = ((lizard_ast_list_node_t *)args->head->next->next)->ast;
  if (h->type != AST_HASH) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  /* Grow if load factor would exceed 0.75. */
  if (h->data.hash.size * 4 >= h->data.hash.cap * 3) {
    hash_grow(heap, h);
  }
  idx = hash_find_slot(h, k, &found);
  if (!found) {
    h->data.hash.size++;
    h->data.hash.keys[idx] = k;
  }
  h->data.hash.values[idx] = v;
  return lizard_make_nil(heap);
}

/* (hash-ref h k [default]) */
lizard_ast_node_t *lizard_primitive_hash_ref(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *h, *k, *deflt;
  size_t idx;
  int found;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  h = ((lizard_ast_list_node_t *)args->head)->ast;
  k = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (h->type != AST_HASH) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  deflt = (args->head->next->next != args->nil)
              ? ((lizard_ast_list_node_t *)args->head->next->next)->ast
              : lizard_make_bool(heap, false);
  idx = hash_find_slot(h, k, &found);
  if (found) return h->data.hash.values[idx];
  return deflt;
}

/* (hash-has-key? h k) */
lizard_ast_node_t *lizard_primitive_hash_has(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *h, *k;
  int found;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  h = ((lizard_ast_list_node_t *)args->head)->ast;
  k = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (h->type != AST_HASH) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  hash_find_slot(h, k, &found);
  return lizard_make_bool(heap, found ? true : false);
}

/* (hash-size h) */
lizard_ast_node_t *lizard_primitive_hash_size(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *h, *r;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  h = ((lizard_ast_list_node_t *)args->head)->ast;
  if (h->type != AST_HASH) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_ui(r->data.number, (unsigned long)h->data.hash.size);
  return r;
}

/* (hash-keys h) -> list */
lizard_ast_node_t *lizard_primitive_hash_keys(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *h, *result, *pair;
  size_t i;
  (void)env;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  h = ((lizard_ast_list_node_t *)args->head)->ast;
  if (h->type != AST_HASH) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_make_nil(heap);
  for (i = 0; i < h->data.hash.cap; i++) {
    if (h->data.hash.keys[i] != NULL) {
      pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      pair->type = AST_PAIR;
      pair->data.pair.car = h->data.hash.keys[i];
      pair->data.pair.cdr = result;
      result = pair;
    }
  }
  return result;
}

/* (hash-remove! h k) */
lizard_ast_node_t *lizard_primitive_hash_remove(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *h, *k;
  size_t idx, next;
  int found;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  h = ((lizard_ast_list_node_t *)args->head)->ast;
  k = ((lizard_ast_list_node_t *)args->head->next)->ast;
  if (h->type != AST_HASH) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  idx = hash_find_slot(h, k, &found);
  if (!found) return lizard_make_bool(heap, false);
  /* Linear-probing deletion: clear slot then re-insert any cluster
   * elements that may now have been displaced. */
  h->data.hash.keys[idx] = NULL;
  h->data.hash.values[idx] = NULL;
  h->data.hash.size--;
  next = (idx + 1) & (h->data.hash.cap - 1);
  while (h->data.hash.keys[next] != NULL) {
    lizard_ast_node_t *rk = h->data.hash.keys[next];
    lizard_ast_node_t *rv = h->data.hash.values[next];
    h->data.hash.keys[next] = NULL;
    h->data.hash.values[next] = NULL;
    h->data.hash.size--;
    /* re-insert */
    {
      lz_list_t *fake_args =
          list_create_alloc(lizard_heap_alloc, lizard_heap_free);
      lizard_ast_list_node_t *a, *b, *c;
      a = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
      b = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
      c = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
      a->ast = h; b->ast = rk; c->ast = rv;
      list_append(fake_args, &a->node);
      list_append(fake_args, &b->node);
      list_append(fake_args, &c->node);
      lizard_primitive_hash_set(fake_args, env, heap);
    }
    next = (next + 1) & (h->data.hash.cap - 1);
  }
  return lizard_make_bool(heap, true);
}

/* ---------------------------------------------------------------------
 * Type-theory notation primitives.
 *
 * These primitives let users build, recognize, and inspect type-theory
 * forms (Pi, Sigma, @, Sum, U, Uco, Id, refl, Inductive, Coinductive)
 * as first-class values. NOTHING IS CHECKED — these are opaque
 * carriers for designing the surface of a proposed type theory, not
 * an implementation of one. A `(Pi (x Nat) Bool)` and a
 * `(Pi (x Nat) garbage)` both build and print fine.
 *
 * The naming convention: type-theoretic constructors are capitalized
 * (Pi, Sigma, U, Uco, Id, Sum, Inductive, Coinductive) to mirror the
 * mathematical notation; predicates/accessors use lowercase.
 * ------------------------------------------------------------------- */

static int no_args(lz_list_t *args) {
  return args->head == args->nil;
}
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
static int four_args(lz_list_t *args) {
  return args->head != args->nil && args->head->next != args->nil &&
         args->head->next->next != args->nil &&
         args->head->next->next->next != args->nil &&
         args->head->next->next->next->next == args->nil;
}
static lizard_ast_node_t *nth_arg(lz_list_t *args, int n) {
  lz_list_node_t *it = args->head;
  while (n-- > 0 && it != args->nil) it = it->next;
  if (it == args->nil) return NULL;
  return ((lizard_ast_list_node_t *)it)->ast;
}

/* (Pi binder domain codomain) -- if binder is nil, this is non-dep -> */
lizard_ast_node_t *lizard_primitive_tt_pi(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PI;
  n->data.tt_pi.binder = nth_arg(args, 0);
  n->data.tt_pi.domain = nth_arg(args, 1);
  n->data.tt_pi.codomain = nth_arg(args, 2);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_sigma(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SIGMA;
  n->data.tt_sigma.binder = nth_arg(args, 0);
  n->data.tt_sigma.domain = nth_arg(args, 1);
  n->data.tt_sigma.codomain = nth_arg(args, 2);
  return n;
}

/* (pi-fresh 'x A B) — Phase L.3 dimension-creating Pi.
 * Same term shape as Pi; differs only in typing. */
lizard_ast_node_t *lizard_primitive_tt_pi_fresh(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PI_FRESH;
  n->data.tt_pi_fresh.binder = nth_arg(args, 0);
  n->data.tt_pi_fresh.domain = nth_arg(args, 1);
  n->data.tt_pi_fresh.codomain = nth_arg(args, 2);
  return n;
}

/* (sigma-fresh 'x A B) — Phase L.3 dimension-creating Sigma. */
lizard_ast_node_t *lizard_primitive_tt_sigma_fresh(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SIGMA_FRESH;
  n->data.tt_sigma_fresh.binder = nth_arg(args, 0);
  n->data.tt_sigma_fresh.domain = nth_arg(args, 1);
  n->data.tt_sigma_fresh.codomain = nth_arg(args, 2);
  return n;
}

/* (co-pi-fresh 'x A B) — Phase L.5 dual dimension-creating Pi.
 * Same term shape; typing rule produces a couniverse-set. */
lizard_ast_node_t *lizard_primitive_tt_co_pi_fresh(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CO_PI_FRESH;
  n->data.tt_co_pi_fresh.binder = nth_arg(args, 0);
  n->data.tt_co_pi_fresh.domain = nth_arg(args, 1);
  n->data.tt_co_pi_fresh.codomain = nth_arg(args, 2);
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_co_sigma_fresh(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CO_SIGMA_FRESH;
  n->data.tt_co_sigma_fresh.binder = nth_arg(args, 0);
  n->data.tt_co_sigma_fresh.domain = nth_arg(args, 1);
  n->data.tt_co_sigma_fresh.codomain = nth_arg(args, 2);
  return n;
}

/* (Box A) — Phase M.5.1 necessity modality type constructor. */
lizard_ast_node_t *lizard_primitive_tt_box(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_BOX;
  n->data.tt_box.argument = nth_arg(args, 0);
  return n;
}

/* (Diamond A) — Phase M.5.1 possibility modality type constructor. */
lizard_ast_node_t *lizard_primitive_tt_diamond(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_DIAMOND;
  n->data.tt_diamond.argument = nth_arg(args, 0);
  return n;
}

/* (box e) — Phase M.5.2 Box introduction. */
lizard_ast_node_t *lizard_primitive_tt_box_intro(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_BOX_INTRO;
  n->data.tt_box_intro.body = nth_arg(args, 0);
  return n;
}

/* (unbox x b body) — Phase M.5.2 Box elimination. */
lizard_ast_node_t *lizard_primitive_tt_box_elim(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_BOX_ELIM;
  n->data.tt_box_elim.binder = nth_arg(args, 0);
  n->data.tt_box_elim.scrutinee = nth_arg(args, 1);
  n->data.tt_box_elim.body = nth_arg(args, 2);
  return n;
}

/* (diamond e) — Phase M.5.5 Diamond introduction. */
lizard_ast_node_t *lizard_primitive_tt_diamond_intro(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_DIAMOND_INTRO;
  n->data.tt_diamond_intro.body = nth_arg(args, 0);
  return n;
}

/* (let-diamond x b body) — Phase M.5.5 Diamond elimination. */
lizard_ast_node_t *lizard_primitive_tt_diamond_elim(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_DIAMOND_ELIM;
  n->data.tt_diamond_elim.binder = nth_arg(args, 0);
  n->data.tt_diamond_elim.scrutinee = nth_arg(args, 1);
  n->data.tt_diamond_elim.body = nth_arg(args, 2);
  return n;
}

/* (box-app f a) — Phase M.5.6 K-axiom application. */
lizard_ast_node_t *lizard_primitive_tt_box_app(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_BOX_APP;
  n->data.tt_box_app.fun = nth_arg(args, 0);
  n->data.tt_box_app.arg = nth_arg(args, 1);
  return n;
}

/* (diamond-bind f d) — Phase M.5.8 Diamond Kleisli composition. */
lizard_ast_node_t *lizard_primitive_tt_diamond_bind(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_DIAMOND_BIND;
  n->data.tt_diamond_bind.fun = nth_arg(args, 0);
  n->data.tt_diamond_bind.arg = nth_arg(args, 1);
  return n;
}

/* (dia e) — Phase M.5.9 symmetric Diamond introduction. */
lizard_ast_node_t *lizard_primitive_tt_diamond_intro_sym(lz_list_t *args,
                                                          lizard_env_t *env,
                                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_DIAMOND_INTRO_SYM;
  n->data.tt_diamond_intro_sym.body = nth_arg(args, 0);
  return n;
}

/* (poss-coerce e) — Phase M.5.9 shift from true to poss. */
lizard_ast_node_t *lizard_primitive_tt_poss_coerce(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_POSS_COERCE;
  n->data.tt_poss_coerce.body = nth_arg(args, 0);
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_at(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_APP;
  n->data.tt_app.fun = nth_arg(args, 0);
  n->data.tt_app.arg = nth_arg(args, 1);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_sum(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SUM;
  n->data.tt_sum.left = nth_arg(args, 0);
  n->data.tt_sum.right = nth_arg(args, 1);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_universe(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n, *lvl;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  lvl = nth_arg(args, 0);
  if (lvl->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UNIVERSE;
  n->data.tt_universe.level = mpz_get_si(lvl->data.number);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_couniverse(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *n, *lvl;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  lvl = nth_arg(args, 0);
  if (lvl->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_COUNIVERSE;
  n->data.tt_couniverse.level = mpz_get_si(lvl->data.number);
  return n;
}

/* Helper for the *-set variadic constructors: parse the args as a
 * list of naturals, sort, dedup. Returns a pair (raw, count) by
 * out-parameter. Returns NULL on type error (negative or non-numeric). */
static long *parse_dim_args(lz_list_t *args, lizard_heap_t *heap,
                            long *count_out) {
  long *raw, *deduped;
  long count, i, j, unique;
  lz_list_node_t *cur;
  lizard_ast_node_t *arg;
  count = 0;
  cur = args->head;
  while (cur != args->nil) { count++; cur = cur->next; }
  if (count == 0) { *count_out = 0; return NULL; }
  raw = lizard_heap_alloc(sizeof(long) * (size_t)count);
  for (i = 0; i < count; i++) {
    arg = nth_arg(args, (int)i);
    if (arg == NULL || arg->type != AST_NUMBER) {
      *count_out = -1; return NULL;  /* sentinel for error */
    }
    raw[i] = mpz_get_si(arg->data.number);
    if (raw[i] < 0) {
      *count_out = -1; return NULL;
    }
  }
  /* Insertion sort. */
  for (i = 1; i < count; i++) {
    long key = raw[i];
    j = i - 1;
    while (j >= 0 && raw[j] > key) {
      raw[j+1] = raw[j];
      j--;
    }
    raw[j+1] = key;
  }
  /* Dedup in place. */
  unique = 1;
  for (i = 1; i < count; i++) {
    if (raw[i] != raw[unique-1]) {
      raw[unique++] = raw[i];
    }
  }
  if (unique == 0) { *count_out = 0; return NULL; }
  deduped = lizard_heap_alloc(sizeof(long) * (size_t)unique);
  for (i = 0; i < unique; i++) deduped[i] = raw[i];
  *count_out = unique;
  return deduped;
}

/* (U-set d1 d2 ...) — multi-dimensional universe. Phase L.2. */
lizard_ast_node_t *lizard_primitive_tt_universe_set(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  long *dims, count;
  (void)env;
  dims = parse_dim_args(args, heap, &count);
  if (count < 0) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UNIVERSE_SET;
  n->data.tt_universe_set.dims = dims;
  n->data.tt_universe_set.count = count;
  return n;
}

/* (Co-set d1 d2 ...) — multi-dimensional COUNIVERSE. Phase L.4.
 *
 * Same shape as U-set but a separate AST type. Lives in its own
 * lattice — no auto-conversion to/from U-set. The dim space is
 * shared with U-set (both index by natural numbers) but the *kind*
 * is distinguished by the AST tag.
 */
lizard_ast_node_t *lizard_primitive_tt_couniverse_set(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  long *dims, count;
  (void)env;
  dims = parse_dim_args(args, heap, &count);
  if (count < 0) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_COUNIVERSE_SET;
  n->data.tt_couniverse_set.dims = dims;
  n->data.tt_couniverse_set.count = count;
  return n;
}

/* (Co-max c1 c2) — join in the couniverse lattice. Set union, dual
 * to U-max. Phase L.4. */
lizard_ast_node_t *lizard_primitive_tt_co_max(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CO_MAX;
  n->data.tt_co_max.left = nth_arg(args, 0);
  n->data.tt_co_max.right = nth_arg(args, 1);
  return n;
}

/* (Co-min c1 c2) — meet in the couniverse lattice. Set intersection. */
lizard_ast_node_t *lizard_primitive_tt_co_min(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CO_MIN;
  n->data.tt_co_min.left = nth_arg(args, 0);
  n->data.tt_co_min.right = nth_arg(args, 1);
  return n;
}

/* ===== Phase H.1 — HIT constructors =====
 *
 * (HIT 'name [HIT-constructor or HIT-path records ...])
 *   Build a HIT declaration. The variadic tail can be any mix of
 *   HIT_CONSTRUCTOR and HIT_PATH records; we sort them into two
 *   lists by node type. As a side effect, registers the declaration
 *   in the per-process HIT registry so it can be looked up by name.
 *
 * (HIT-constructor 'name arg-type-1 arg-type-2 ...)
 *   Build a point-constructor record. The first arg is the constructor
 *   name; the remaining args are the types of its arguments.
 *
 * (HIT-path 'name source target)
 *   Build a path-constructor record. Three args.
 *
 * (HIT-ref 'name) — use a registered HIT by name as a type.
 * (HIT-app 'cname arg1 arg2 ...) — apply a constructor.
 * (HIT-lookup 'name) — query the registry; returns the declaration or nil.
 */
lizard_ast_node_t *lizard_primitive_tt_hit_constructor(lz_list_t *args,
                                                       lizard_env_t *env,
                                                       lizard_heap_t *heap) {
  lizard_ast_node_t *n, *name_arg;
  lz_list_t *arg_types;
  lz_list_node_t *cur;
  int i, n_args;
  (void)env;
  /* Need at least the name; arg-types may be empty. */
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  /* Count args. */
  n_args = 0;
  cur = args->head;
  while (cur != args->nil) { n_args++; cur = cur->next; }
  /* Collect arg-types into a fresh list (everything after the name). */
  arg_types = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (i = 1; i < n_args; i++) {
    lizard_ast_list_node_t *cell = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    cell->ast = nth_arg(args, i);
    list_append(arg_types, &cell->node);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_HIT_CONSTRUCTOR;
  n->data.tt_hit_constructor.name = name_arg;
  n->data.tt_hit_constructor.arg_types = arg_types;
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_hit_path(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_HIT_PATH;
  n->data.tt_hit_path.name = nth_arg(args, 0);
  n->data.tt_hit_path.source = nth_arg(args, 1);
  n->data.tt_hit_path.target = nth_arg(args, 2);
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_hit_decl(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n, *name_arg, *child;
  lz_list_t *ctors, *paths;
  lz_list_node_t *cur;
  int i, n_args;
  (void)env;
  /* Need at least the name. */
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  ctors = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  paths = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  /* Count, then sort each tail arg by its AST type. */
  n_args = 0;
  cur = args->head;
  while (cur != args->nil) { n_args++; cur = cur->next; }
  for (i = 1; i < n_args; i++) {
    lizard_ast_list_node_t *cell;
    child = nth_arg(args, i);
    if (child == NULL) continue;
    cell = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    cell->ast = child;
    if (child->type == AST_TT_HIT_CONSTRUCTOR) {
      list_append(ctors, &cell->node);
    } else if (child->type == AST_TT_HIT_PATH) {
      list_append(paths, &cell->node);
    } else {
      /* Anything else is an arg-type error. */
      return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
    }
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_HIT_DECL;
  n->data.tt_hit_decl.name = name_arg;
  n->data.tt_hit_decl.constructors = ctors;
  n->data.tt_hit_decl.paths = paths;
  /* Side effect: register so subsequent (HIT-ref 'name) and (HIT-lookup
   * 'name) can find it. Note that this is module-level state, which is
   * a deliberate simplification for H.1. */
  lizard_tt_hit_register(n);
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_hit_ref(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *n, *name_arg;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_HIT_REF;
  n->data.tt_hit_ref.name = name_arg;
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_hit_app(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *n, *name_arg;
  lz_list_t *app_args;
  lz_list_node_t *cur;
  int i, n_args;
  (void)env;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n_args = 0;
  cur = args->head;
  while (cur != args->nil) { n_args++; cur = cur->next; }
  app_args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (i = 1; i < n_args; i++) {
    lizard_ast_list_node_t *cell = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    cell->ast = nth_arg(args, i);
    list_append(app_args, &cell->node);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_HIT_APP;
  n->data.tt_hit_app.name = name_arg;
  n->data.tt_hit_app.args = app_args;
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_hit_lookup(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg, *result;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_tt_hit_lookup(name_arg->data.variable);
  if (result == NULL) {
    /* Return nil for "not found". */
    lizard_ast_node_t *nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    nil->type = AST_NIL;
    return nil;
  }
  return result;
}
lizard_ast_node_t *lizard_primitive_tt_id(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_ID;
  n->data.tt_id.domain = nth_arg(args, 0);
  n->data.tt_id.a = nth_arg(args, 1);
  n->data.tt_id.b = nth_arg(args, 2);
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_refl(lz_list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_REFL;
  n->data.tt_refl.value = nth_arg(args, 0);
  return n;
}
/* (Inductive 'Name 'ctor1 'ctor2 ...) — first arg is the type name,
 * the rest are constructor specs (left opaque; user can put whatever
 * they want — symbols, lists describing fields, whatever). */
lizard_ast_node_t *lizard_primitive_tt_inductive(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  lz_list_t *ctors;
  lz_list_node_t *it;
  (void)env;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  ctors = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (it = args->head->next; it != args->nil; it = it->next) {
    lizard_ast_list_node_t *w =
        lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    w->ast = ((lizard_ast_list_node_t *)it)->ast;
    list_append(ctors, &w->node);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_INDUCTIVE;
  n->data.tt_inductive.name = nth_arg(args, 0);
  n->data.tt_inductive.constructors = ctors;
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_coinductive(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  lz_list_t *dtors;
  lz_list_node_t *it;
  (void)env;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  dtors = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (it = args->head->next; it != args->nil; it = it->next) {
    lizard_ast_list_node_t *w =
        lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    w->ast = ((lizard_ast_list_node_t *)it)->ast;
    list_append(dtors, &w->node);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_COINDUCTIVE;
  n->data.tt_coinductive.name = nth_arg(args, 0);
  n->data.tt_coinductive.destructors = dtors;
  return n;
}
lizard_ast_node_t *lizard_primitive_tt_annot(lz_list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_ANNOT;
  n->data.tt_annot.term = nth_arg(args, 0);
  n->data.tt_annot.type = nth_arg(args, 1);
  return n;
}

/* Predicates. */
#define TT_PREDICATE(name, tag)                                                \
  lizard_ast_node_t *lizard_primitive_##name(lz_list_t *args,                  \
                                             lizard_env_t *env,                \
                                             lizard_heap_t *heap) {            \
    lizard_ast_node_t *x;                                                      \
    (void)env;                                                                 \
    if (!single_arg(args)) {                                                   \
      return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);             \
    }                                                                          \
    x = nth_arg(args, 0);                                                      \
    return lizard_make_bool(heap, x && x->type == tag);                        \
  }
TT_PREDICATE(tt_pip,          AST_TT_PI)
TT_PREDICATE(tt_sigmap,       AST_TT_SIGMA)
TT_PREDICATE(tt_pi_freshp,    AST_TT_PI_FRESH)
TT_PREDICATE(tt_sigma_freshp, AST_TT_SIGMA_FRESH)
TT_PREDICATE(tt_co_pi_freshp,    AST_TT_CO_PI_FRESH)
TT_PREDICATE(tt_co_sigma_freshp, AST_TT_CO_SIGMA_FRESH)
TT_PREDICATE(tt_boxp,     AST_TT_BOX)
TT_PREDICATE(tt_diamondp, AST_TT_DIAMOND)
TT_PREDICATE(tt_box_introp, AST_TT_BOX_INTRO)
TT_PREDICATE(tt_box_elimp,  AST_TT_BOX_ELIM)
TT_PREDICATE(tt_diamond_introp, AST_TT_DIAMOND_INTRO)
TT_PREDICATE(tt_diamond_elimp,  AST_TT_DIAMOND_ELIM)
TT_PREDICATE(tt_box_appp,       AST_TT_BOX_APP)
TT_PREDICATE(tt_diamond_bindp,  AST_TT_DIAMOND_BIND)
TT_PREDICATE(tt_diamond_intro_symp, AST_TT_DIAMOND_INTRO_SYM)
TT_PREDICATE(tt_poss_coercep,       AST_TT_POSS_COERCE)
TT_PREDICATE(tt_appp,         AST_TT_APP)
TT_PREDICATE(tt_sump,         AST_TT_SUM)
TT_PREDICATE(tt_universep,    AST_TT_UNIVERSE)
TT_PREDICATE(tt_couniversep,  AST_TT_COUNIVERSE)
TT_PREDICATE(tt_universe_setp, AST_TT_UNIVERSE_SET)
TT_PREDICATE(tt_couniverse_setp, AST_TT_COUNIVERSE_SET)
TT_PREDICATE(tt_co_maxp,       AST_TT_CO_MAX)
TT_PREDICATE(tt_co_minp,       AST_TT_CO_MIN)
TT_PREDICATE(tt_hit_declp,     AST_TT_HIT_DECL)
TT_PREDICATE(tt_hit_constructorp, AST_TT_HIT_CONSTRUCTOR)
TT_PREDICATE(tt_hit_pathp,     AST_TT_HIT_PATH)
TT_PREDICATE(tt_hit_refp,      AST_TT_HIT_REF)
TT_PREDICATE(tt_hit_appp,      AST_TT_HIT_APP)
TT_PREDICATE(tt_idp,          AST_TT_ID)
TT_PREDICATE(tt_reflp,        AST_TT_REFL)
TT_PREDICATE(tt_inductivep,   AST_TT_INDUCTIVE)
TT_PREDICATE(tt_coinductivep, AST_TT_COINDUCTIVE)
TT_PREDICATE(tt_annotp,       AST_TT_ANNOT)

/* Accessors — pull fields out of TT nodes. Return error on mismatch. */
#define TT_ACCESSOR(name, tag, expr)                                           \
  lizard_ast_node_t *lizard_primitive_##name(lz_list_t *args,                  \
                                             lizard_env_t *env,                \
                                             lizard_heap_t *heap) {            \
    lizard_ast_node_t *x;                                                      \
    lizard_ast_node_t *r;                                                      \
    (void)env;                                                                 \
    if (!single_arg(args)) {                                                   \
      return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);             \
    }                                                                          \
    x = nth_arg(args, 0);                                                      \
    if (!x || x->type != tag) {                                                \
      return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);                  \
    }                                                                          \
    r = (expr);                                                                \
    return r ? r : lizard_make_nil(heap);                                      \
  }
TT_ACCESSOR(tt_pi_binder,    AST_TT_PI,    x->data.tt_pi.binder)
TT_ACCESSOR(tt_pi_domain,    AST_TT_PI,    x->data.tt_pi.domain)
TT_ACCESSOR(tt_pi_codomain,  AST_TT_PI,    x->data.tt_pi.codomain)
TT_ACCESSOR(tt_sigma_binder, AST_TT_SIGMA, x->data.tt_sigma.binder)
TT_ACCESSOR(tt_sigma_domain, AST_TT_SIGMA, x->data.tt_sigma.domain)
TT_ACCESSOR(tt_sigma_codomain, AST_TT_SIGMA, x->data.tt_sigma.codomain)
TT_ACCESSOR(tt_app_fun,      AST_TT_APP,   x->data.tt_app.fun)
TT_ACCESSOR(tt_app_arg,      AST_TT_APP,   x->data.tt_app.arg)
TT_ACCESSOR(tt_sum_left,     AST_TT_SUM,   x->data.tt_sum.left)
TT_ACCESSOR(tt_sum_right,    AST_TT_SUM,   x->data.tt_sum.right)
TT_ACCESSOR(tt_box_arg,      AST_TT_BOX,     x->data.tt_box.argument)
TT_ACCESSOR(tt_diamond_arg,  AST_TT_DIAMOND, x->data.tt_diamond.argument)
TT_ACCESSOR(tt_box_intro_body, AST_TT_BOX_INTRO, x->data.tt_box_intro.body)
TT_ACCESSOR(tt_box_elim_binder,    AST_TT_BOX_ELIM, x->data.tt_box_elim.binder)
TT_ACCESSOR(tt_box_elim_scrutinee, AST_TT_BOX_ELIM, x->data.tt_box_elim.scrutinee)
TT_ACCESSOR(tt_box_elim_body,      AST_TT_BOX_ELIM, x->data.tt_box_elim.body)
TT_ACCESSOR(tt_diamond_intro_body,    AST_TT_DIAMOND_INTRO, x->data.tt_diamond_intro.body)
TT_ACCESSOR(tt_diamond_elim_binder,    AST_TT_DIAMOND_ELIM, x->data.tt_diamond_elim.binder)
TT_ACCESSOR(tt_diamond_elim_scrutinee, AST_TT_DIAMOND_ELIM, x->data.tt_diamond_elim.scrutinee)
TT_ACCESSOR(tt_diamond_elim_body,      AST_TT_DIAMOND_ELIM, x->data.tt_diamond_elim.body)
TT_ACCESSOR(tt_box_app_fun, AST_TT_BOX_APP, x->data.tt_box_app.fun)
TT_ACCESSOR(tt_box_app_arg, AST_TT_BOX_APP, x->data.tt_box_app.arg)
TT_ACCESSOR(tt_diamond_bind_fun, AST_TT_DIAMOND_BIND, x->data.tt_diamond_bind.fun)
TT_ACCESSOR(tt_diamond_bind_arg, AST_TT_DIAMOND_BIND, x->data.tt_diamond_bind.arg)
TT_ACCESSOR(tt_diamond_intro_sym_body, AST_TT_DIAMOND_INTRO_SYM, x->data.tt_diamond_intro_sym.body)
TT_ACCESSOR(tt_poss_coerce_body,       AST_TT_POSS_COERCE,       x->data.tt_poss_coerce.body)
TT_ACCESSOR(tt_id_domain,    AST_TT_ID,    x->data.tt_id.domain)
TT_ACCESSOR(tt_id_a,         AST_TT_ID,    x->data.tt_id.a)
TT_ACCESSOR(tt_id_b,         AST_TT_ID,    x->data.tt_id.b)
TT_ACCESSOR(tt_refl_value,   AST_TT_REFL,  x->data.tt_refl.value)
TT_ACCESSOR(tt_inductive_name, AST_TT_INDUCTIVE, x->data.tt_inductive.name)
TT_ACCESSOR(tt_coinductive_name, AST_TT_COINDUCTIVE,
            x->data.tt_coinductive.name)
TT_ACCESSOR(tt_annot_term,   AST_TT_ANNOT, x->data.tt_annot.term)
TT_ACCESSOR(tt_annot_type,   AST_TT_ANNOT, x->data.tt_annot.type)

/* Universe level — returns the integer. */
lizard_ast_node_t *lizard_primitive_tt_universe_level(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  lizard_ast_node_t *x, *r;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = nth_arg(args, 0);
  if (!x || x->type != AST_TT_UNIVERSE) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, x->data.tt_universe.level);
  return r;
}
lizard_ast_node_t *lizard_primitive_tt_couniverse_level(lz_list_t *args,
                                                        lizard_env_t *env,
                                                        lizard_heap_t *heap) {
  lizard_ast_node_t *x, *r;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = nth_arg(args, 0);
  if (!x || x->type != AST_TT_COUNIVERSE) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, x->data.tt_couniverse.level);
  return r;
}

/* Constructors / destructors for inductive/coinductive — return a list */
lizard_ast_node_t *lizard_primitive_tt_inductive_ctors(lz_list_t *args,
                                                       lizard_env_t *env,
                                                       lizard_heap_t *heap) {
  lizard_ast_node_t *x, *result;
  lz_list_node_t *it;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = nth_arg(args, 0);
  if (!x || x->type != AST_TT_INDUCTIVE) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_make_nil(heap);
  /* Build the list in reverse so the result preserves order. */
  for (it = x->data.tt_inductive.constructors->head;
       it != x->data.tt_inductive.constructors->nil; it = it->next) {
    /* skip — we'll build properly below */;
  }
  {
    /* easier: collect into an array first */
    size_t n = 0, i;
    lizard_ast_node_t **buf;
    for (it = x->data.tt_inductive.constructors->head;
         it != x->data.tt_inductive.constructors->nil; it = it->next) n++;
    buf = lizard_heap_alloc((n + 1) * sizeof(lizard_ast_node_t *));
    i = 0;
    for (it = x->data.tt_inductive.constructors->head;
         it != x->data.tt_inductive.constructors->nil; it = it->next) {
      buf[i++] = ((lizard_ast_list_node_t *)it)->ast;
    }
    for (i = n; i > 0; i--) {
      lizard_ast_node_t *p = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      p->type = AST_PAIR;
      p->data.pair.car = buf[i - 1];
      p->data.pair.cdr = result;
      result = p;
    }
  }
  return result;
}
lizard_ast_node_t *lizard_primitive_tt_coinductive_dtors(lz_list_t *args,
                                                         lizard_env_t *env,
                                                         lizard_heap_t *heap) {
  lizard_ast_node_t *x, *result;
  lz_list_node_t *it;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = nth_arg(args, 0);
  if (!x || x->type != AST_TT_COINDUCTIVE) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_make_nil(heap);
  {
    size_t n = 0, i;
    lizard_ast_node_t **buf;
    for (it = x->data.tt_coinductive.destructors->head;
         it != x->data.tt_coinductive.destructors->nil; it = it->next) n++;
    buf = lizard_heap_alloc((n + 1) * sizeof(lizard_ast_node_t *));
    i = 0;
    for (it = x->data.tt_coinductive.destructors->head;
         it != x->data.tt_coinductive.destructors->nil; it = it->next) {
      buf[i++] = ((lizard_ast_list_node_t *)it)->ast;
    }
    for (i = n; i > 0; i--) {
      lizard_ast_node_t *p = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      p->type = AST_PAIR;
      p->data.pair.car = buf[i - 1];
      p->data.pair.cdr = result;
      result = p;
    }
  }
  return result;
}

/* ---------------------------------------------------------------------
 * Context layer — couniverse-stratified.
 *
 * Per the thesis proposal:
 *   Uco -2 : variables / binding sites
 *   Uco -1 : contexts (lists of variables)
 *   Uco  0 : substitutions / context morphisms
 *
 * The `uco-level` primitive returns the couniverse level for any of
 * these. NONE of this checks well-formedness. A context can contain
 * "bindings" that aren't really variables, a substitution can list
 * mappings that don't match its source's domain, a judgment is just a
 * three-tuple that prints like an inference-rule conclusion. The
 * forms are here so you can sketch the surface and pattern-match on
 * the structure, not because anything is verified.
 * ------------------------------------------------------------------- */

/* (variable 'name type) — at Uco -2 */
lizard_ast_node_t *lizard_primitive_tt_variable(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_VARIABLE;
  n->data.tt_variable.name = nth_arg(args, 0);
  n->data.tt_variable.type = nth_arg(args, 1);
  return n;
}

/* (context binding1 binding2 ...) — at Uco -1.
 * Each binding is whatever you want; the convention is that they're
 * variable values, but we don't enforce that. */
lizard_ast_node_t *lizard_primitive_tt_context(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  lz_list_t *bindings;
  lz_list_node_t *it;
  (void)env;
  bindings = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (it = args->head; it != args->nil; it = it->next) {
    lizard_ast_list_node_t *w =
        lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    w->ast = ((lizard_ast_list_node_t *)it)->ast;
    list_append(bindings, &w->node);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CONTEXT;
  n->data.tt_context.bindings = bindings;
  return n;
}

/* (context-extend ctx variable) — return a new context with the
 * variable appended at the end. The original is not mutated. */
lizard_ast_node_t *lizard_primitive_tt_ctx_extend(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *ctx, *var, *n;
  lz_list_t *bindings;
  lz_list_node_t *it;
  lizard_ast_list_node_t *w;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  ctx = nth_arg(args, 0);
  var = nth_arg(args, 1);
  if (!ctx || ctx->type != AST_TT_CONTEXT) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  bindings = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (it = ctx->data.tt_context.bindings->head;
       it != ctx->data.tt_context.bindings->nil; it = it->next) {
    w = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    w->ast = ((lizard_ast_list_node_t *)it)->ast;
    list_append(bindings, &w->node);
  }
  w = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
  w->ast = var;
  list_append(bindings, &w->node);
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CONTEXT;
  n->data.tt_context.bindings = bindings;
  return n;
}

/* (context-lookup ctx 'name) — return the matching variable, or #f. */
lizard_ast_node_t *lizard_primitive_tt_ctx_lookup(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *ctx, *name;
  lz_list_node_t *it;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  ctx = nth_arg(args, 0);
  name = nth_arg(args, 1);
  if (!ctx || ctx->type != AST_TT_CONTEXT) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  if (!name || name->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  /* Walk right-to-left (innermost first) so shadowing works the
   * usual way. */
  {
    /* Build an array first so we can iterate backwards. */
    size_t n = 0, i;
    lizard_ast_node_t **buf;
    for (it = ctx->data.tt_context.bindings->head;
         it != ctx->data.tt_context.bindings->nil; it = it->next) n++;
    buf = lizard_heap_alloc((n + 1) * sizeof(lizard_ast_node_t *));
    i = 0;
    for (it = ctx->data.tt_context.bindings->head;
         it != ctx->data.tt_context.bindings->nil; it = it->next) {
      buf[i++] = ((lizard_ast_list_node_t *)it)->ast;
    }
    for (i = n; i > 0; i--) {
      lizard_ast_node_t *v = buf[i - 1];
      if (v && v->type == AST_TT_VARIABLE && v->data.tt_variable.name &&
          v->data.tt_variable.name->type == AST_SYMBOL &&
          strcmp(v->data.tt_variable.name->data.variable,
                 name->data.variable) == 0) {
        return v;
      }
    }
  }
  return lizard_make_bool(heap, false);
}

/* (context-bindings ctx) — return the list of variables in the
 * context, in declaration order. */
lizard_ast_node_t *lizard_primitive_tt_ctx_bindings(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *ctx, *result;
  lz_list_node_t *it;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  ctx = nth_arg(args, 0);
  if (!ctx || ctx->type != AST_TT_CONTEXT) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  result = lizard_make_nil(heap);
  {
    size_t n = 0, i;
    lizard_ast_node_t **buf;
    for (it = ctx->data.tt_context.bindings->head;
         it != ctx->data.tt_context.bindings->nil; it = it->next) n++;
    buf = lizard_heap_alloc((n + 1) * sizeof(lizard_ast_node_t *));
    i = 0;
    for (it = ctx->data.tt_context.bindings->head;
         it != ctx->data.tt_context.bindings->nil; it = it->next) {
      buf[i++] = ((lizard_ast_list_node_t *)it)->ast;
    }
    for (i = n; i > 0; i--) {
      lizard_ast_node_t *p = lizard_heap_alloc(sizeof(lizard_ast_node_t));
      p->type = AST_PAIR;
      p->data.pair.car = buf[i - 1];
      p->data.pair.cdr = result;
      result = p;
    }
  }
  return result;
}

/* (context-length ctx) */
lizard_ast_node_t *lizard_primitive_tt_ctx_length(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *ctx, *r;
  lz_list_node_t *it;
  long count = 0;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  ctx = nth_arg(args, 0);
  if (!ctx || ctx->type != AST_TT_CONTEXT) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  for (it = ctx->data.tt_context.bindings->head;
       it != ctx->data.tt_context.bindings->nil; it = it->next) count++;
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, count);
  return r;
}

/* (substitution from-ctx to-ctx mappings-list) — at Uco 0.
 * mappings-list is a list of (name . term) pairs (or anything else;
 * we don't enforce). */
lizard_ast_node_t *lizard_primitive_tt_substitution(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *src, *tgt, *maps_arg, *n;
  lz_list_t *mappings;
  lizard_ast_node_t *cur;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  src = nth_arg(args, 0);
  tgt = nth_arg(args, 1);
  maps_arg = nth_arg(args, 2);
  mappings = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (cur = maps_arg; cur && cur->type == AST_PAIR; cur = cur->data.pair.cdr) {
    lizard_ast_list_node_t *w =
        lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    w->ast = cur->data.pair.car;
    list_append(mappings, &w->node);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SUBSTITUTION;
  n->data.tt_substitution.source = src;
  n->data.tt_substitution.target = tgt;
  n->data.tt_substitution.mappings = mappings;
  return n;
}

/* (judgment ctx term type) — opaque triple. */
lizard_ast_node_t *lizard_primitive_tt_judgment(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_JUDGMENT;
  n->data.tt_judgment.context = nth_arg(args, 0);
  n->data.tt_judgment.term = nth_arg(args, 1);
  n->data.tt_judgment.type = nth_arg(args, 2);
  return n;
}

/* (empty-context) — convenient nullary constructor. */
lizard_ast_node_t *lizard_primitive_tt_empty_ctx(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CONTEXT;
  n->data.tt_context.bindings =
      list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  return n;
}

/* uco-level — the central operation. Returns the couniverse level
 * for any of the binding/context/substitution forms, or #f for other
 * values. This is the *only* place lizard hard-codes the
 * stratification: -2 for variables, -1 for contexts, 0 for
 * substitutions / judgments. */
lizard_ast_node_t *lizard_primitive_tt_uco_level(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *x, *r;
  long lvl;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  x = nth_arg(args, 0);
  if (!x) return lizard_make_bool(heap, false);
  switch (x->type) {
  case AST_TT_VARIABLE:     lvl = -2; break;
  case AST_TT_CONTEXT:      lvl = -1; break;
  case AST_TT_SUBSTITUTION: lvl =  0; break;
  case AST_TT_JUDGMENT:     lvl =  0; break;
  default:
    return lizard_make_bool(heap, false);
  }
  r = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  r->type = AST_NUMBER;
  mpz_init_set_si(r->data.number, lvl);
  return r;
}

/* Predicates and accessors. */
TT_PREDICATE(tt_variablep,     AST_TT_VARIABLE)
TT_PREDICATE(tt_contextp,      AST_TT_CONTEXT)
TT_PREDICATE(tt_substitutionp, AST_TT_SUBSTITUTION)
TT_PREDICATE(tt_judgmentp,     AST_TT_JUDGMENT)

TT_ACCESSOR(tt_var_name,      AST_TT_VARIABLE,     x->data.tt_variable.name)
TT_ACCESSOR(tt_var_type,      AST_TT_VARIABLE,     x->data.tt_variable.type)
TT_ACCESSOR(tt_subst_source,  AST_TT_SUBSTITUTION, x->data.tt_substitution.source)
TT_ACCESSOR(tt_subst_target,  AST_TT_SUBSTITUTION, x->data.tt_substitution.target)
TT_ACCESSOR(tt_judg_context,  AST_TT_JUDGMENT,     x->data.tt_judgment.context)
TT_ACCESSOR(tt_judg_term,     AST_TT_JUDGMENT,     x->data.tt_judgment.term)
TT_ACCESSOR(tt_judg_type,     AST_TT_JUDGMENT,     x->data.tt_judgment.type)

/* ---------------------------------------------------------------------
 * Identity manipulation + equivalence (NOTATION ONLY).
 *
 * These primitives let you sketch the basic HOTT-style identity
 * lemmas — symmetry, transitivity, transport, equivalence — as
 * structured values. Nothing about the validity of the constructed
 * forms is checked. (equivalence A B fwd bwd) does not verify that
 * fwd and bwd are actually inverse; (transport p x) does not verify
 * that p is an Id-proof relating two types; (Id-trans p q) does not
 * verify that p's endpoint matches q's startpoint.
 *
 * Why bother having these? They give a vocabulary for writing the
 * lemmas one would prove in a real proof assistant. With them you
 * can pattern-match on identity manipulations, define functions that
 * walk an identity term and dispatch on its head, and sketch the
 * normalisation rules a future checker might use.
 * ------------------------------------------------------------------- */

/* (equivalence A B fwd bwd) — claims A ≃ B via fwd : A -> B and
 * bwd : B -> A. */
lizard_ast_node_t *lizard_primitive_tt_equiv(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next == args->nil ||
      args->head->next->next->next == args->nil ||
      args->head->next->next->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EQUIV;
  n->data.tt_equiv.left  = nth_arg(args, 0);
  n->data.tt_equiv.right = nth_arg(args, 1);
  n->data.tt_equiv.fwd   = nth_arg(args, 2);
  n->data.tt_equiv.bwd   = nth_arg(args, 3);
  return n;
}

/* (transport path value) — claims to transport `value` along `path`. */
lizard_ast_node_t *lizard_primitive_tt_transport(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_TRANSPORT;
  n->data.tt_transport.path = nth_arg(args, 0);
  n->data.tt_transport.value = nth_arg(args, 1);
  return n;
}

/* (Id-sym p) — claimed symmetry of an Id-proof. */
lizard_ast_node_t *lizard_primitive_tt_id_sym(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_ID_SYM;
  n->data.tt_id_sym.path = nth_arg(args, 0);
  return n;
}

/* (Id-trans p q) — claimed transitivity / composition. */
lizard_ast_node_t *lizard_primitive_tt_id_trans(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_ID_TRANS;
  n->data.tt_id_trans.p = nth_arg(args, 0);
  n->data.tt_id_trans.q = nth_arg(args, 1);
  return n;
}

TT_PREDICATE(tt_equivp,     AST_TT_EQUIV)
TT_PREDICATE(tt_transportp, AST_TT_TRANSPORT)
TT_PREDICATE(tt_id_symp,    AST_TT_ID_SYM)
TT_PREDICATE(tt_id_transp,  AST_TT_ID_TRANS)

TT_ACCESSOR(tt_equiv_left,    AST_TT_EQUIV,     x->data.tt_equiv.left)
TT_ACCESSOR(tt_equiv_right,   AST_TT_EQUIV,     x->data.tt_equiv.right)
TT_ACCESSOR(tt_equiv_fwd,     AST_TT_EQUIV,     x->data.tt_equiv.fwd)
TT_ACCESSOR(tt_equiv_bwd,     AST_TT_EQUIV,     x->data.tt_equiv.bwd)
TT_ACCESSOR(tt_transport_path,  AST_TT_TRANSPORT, x->data.tt_transport.path)
TT_ACCESSOR(tt_transport_value, AST_TT_TRANSPORT, x->data.tt_transport.value)
TT_ACCESSOR(tt_id_sym_path,   AST_TT_ID_SYM,    x->data.tt_id_sym.path)
TT_ACCESSOR(tt_id_trans_p,    AST_TT_ID_TRANS,  x->data.tt_id_trans.p)
TT_ACCESSOR(tt_id_trans_q,    AST_TT_ID_TRANS,  x->data.tt_id_trans.q)

/* TT-level Lambda. */
lizard_ast_node_t *lizard_primitive_tt_lambda(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_LAMBDA;
  n->data.tt_lambda.binder = nth_arg(args, 0);
  n->data.tt_lambda.body   = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_lambdap, AST_TT_LAMBDA)
TT_ACCESSOR(tt_lambda_binder, AST_TT_LAMBDA, x->data.tt_lambda.binder)
TT_ACCESSOR(tt_lambda_body,   AST_TT_LAMBDA, x->data.tt_lambda.body)

/* ap — congruence of identity along a function. */
lizard_ast_node_t *lizard_primitive_tt_ap(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_AP;
  n->data.tt_ap.fn   = nth_arg(args, 0);
  n->data.tt_ap.path = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_app_p_hott, AST_TT_AP)
TT_ACCESSOR(tt_ap_fn,   AST_TT_AP, x->data.tt_ap.fn)
TT_ACCESSOR(tt_ap_path, AST_TT_AP, x->data.tt_ap.path)

/* pair / fst / snd */
lizard_ast_node_t *lizard_primitive_tt_pair(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PAIR;
  n->data.tt_pair.fst = nth_arg(args, 0);
  n->data.tt_pair.snd = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_pairp, AST_TT_PAIR)
TT_ACCESSOR(tt_pair_first,  AST_TT_PAIR, x->data.tt_pair.fst)
TT_ACCESSOR(tt_pair_second, AST_TT_PAIR, x->data.tt_pair.snd)

lizard_ast_node_t *lizard_primitive_tt_fst(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_FST;
  n->data.tt_proj.target = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_fstp, AST_TT_FST)
TT_ACCESSOR(tt_fst_target, AST_TT_FST, x->data.tt_proj.target)

lizard_ast_node_t *lizard_primitive_tt_snd(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SND;
  n->data.tt_proj.target = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_sndp, AST_TT_SND)
TT_ACCESSOR(tt_snd_target, AST_TT_SND, x->data.tt_proj.target)

/* inl / inr / case */
lizard_ast_node_t *lizard_primitive_tt_inl(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_INL;
  n->data.tt_inj.value = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_inlp, AST_TT_INL)
TT_ACCESSOR(tt_inl_value, AST_TT_INL, x->data.tt_inj.value)

lizard_ast_node_t *lizard_primitive_tt_inr(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_INR;
  n->data.tt_inj.value = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_inrp, AST_TT_INR)
TT_ACCESSOR(tt_inr_value, AST_TT_INR, x->data.tt_inj.value)

lizard_ast_node_t *lizard_primitive_tt_case(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_CASE;
  n->data.tt_case.scrutinee    = nth_arg(args, 0);
  n->data.tt_case.left_branch  = nth_arg(args, 1);
  n->data.tt_case.right_branch = nth_arg(args, 2);
  return n;
}
TT_PREDICATE(tt_casep, AST_TT_CASE)
TT_ACCESSOR(tt_case_scrutinee, AST_TT_CASE, x->data.tt_case.scrutinee)
TT_ACCESSOR(tt_case_left,  AST_TT_CASE, x->data.tt_case.left_branch)
TT_ACCESSOR(tt_case_right, AST_TT_CASE, x->data.tt_case.right_branch)

/* Unit type and its inhabitant; Bot. These are nullary — no args. */
lizard_ast_node_t *lizard_primitive_tt_unit(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UNIT;
  return n;
}
TT_PREDICATE(tt_unitp, AST_TT_UNIT)

lizard_ast_node_t *lizard_primitive_tt_tt(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_TT;
  return n;
}
TT_PREDICATE(tt_ttp, AST_TT_TT)

lizard_ast_node_t *lizard_primitive_tt_bot(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_BOT;
  return n;
}
TT_PREDICATE(tt_botp, AST_TT_BOT)

/* J — path induction. (J motive refl-case path). */
lizard_ast_node_t *lizard_primitive_tt_j(lz_list_t *args,
                                         lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_J;
  n->data.tt_j.motive    = nth_arg(args, 0);
  n->data.tt_j.refl_case = nth_arg(args, 1);
  n->data.tt_j.path      = nth_arg(args, 2);
  return n;
}
TT_PREDICATE(tt_jp, AST_TT_J)
TT_ACCESSOR(tt_j_motive,    AST_TT_J, x->data.tt_j.motive)
TT_ACCESSOR(tt_j_refl_case, AST_TT_J, x->data.tt_j.refl_case)
TT_ACCESSOR(tt_j_path,      AST_TT_J, x->data.tt_j.path)

/* xport — transport with explicit motive. The motive is a Lambda
 * (Lambda 'x T) whose body T tells the engine which per-type-former
 * rule to apply. (xport (Lambda 'x T) path value). */
lizard_ast_node_t *lizard_primitive_tt_xport(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_XPORT;
  n->data.tt_xport.motive = nth_arg(args, 0);
  n->data.tt_xport.path   = nth_arg(args, 1);
  n->data.tt_xport.value  = nth_arg(args, 2);
  return n;
}
TT_PREDICATE(tt_xportp, AST_TT_XPORT)
TT_ACCESSOR(tt_xport_motive, AST_TT_XPORT, x->data.tt_xport.motive)
TT_ACCESSOR(tt_xport_path,   AST_TT_XPORT, x->data.tt_xport.path)
TT_ACCESSOR(tt_xport_value,  AST_TT_XPORT, x->data.tt_xport.value)

/* Universe-expression constructors. (U-var 'i), (U-suc u), (U-max u v). */
lizard_ast_node_t *lizard_primitive_tt_u_var(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n, *name_arg;
  char *buf;
  size_t len;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (!name_arg || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_U_VAR;
  len = strlen(name_arg->data.variable);
  buf = lizard_heap_alloc(len + 1);
  strcpy(buf, name_arg->data.variable);
  n->data.tt_u_var.name = buf;
  return n;
}
TT_PREDICATE(tt_u_varp, AST_TT_U_VAR)

lizard_ast_node_t *lizard_primitive_tt_u_suc(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_U_SUC;
  n->data.tt_u_suc.operand = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_u_sucp, AST_TT_U_SUC)
TT_ACCESSOR(tt_u_suc_operand, AST_TT_U_SUC, x->data.tt_u_suc.operand)

lizard_ast_node_t *lizard_primitive_tt_u_max(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_U_MAX;
  n->data.tt_u_max.left = nth_arg(args, 0);
  n->data.tt_u_max.right = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_u_maxp, AST_TT_U_MAX)
TT_ACCESSOR(tt_u_max_left,  AST_TT_U_MAX, x->data.tt_u_max.left)
TT_ACCESSOR(tt_u_max_right, AST_TT_U_MAX, x->data.tt_u_max.right)

/* (U-min u v) — meet (greatest lower bound). Dual of U-max.
 * Phase L.1: with both join and meet, universes form a lattice. */
lizard_ast_node_t *lizard_primitive_tt_u_min(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_U_MIN;
  n->data.tt_u_min.left = nth_arg(args, 0);
  n->data.tt_u_min.right = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_u_minp, AST_TT_U_MIN)
TT_ACCESSOR(tt_u_min_left,  AST_TT_U_MIN, x->data.tt_u_min.left)
TT_ACCESSOR(tt_u_min_right, AST_TT_U_MIN, x->data.tt_u_min.right)

/* ===== Cubical layer ===== */

/* The interval pre-type itself and its endpoints. Nullary. */
lizard_ast_node_t *lizard_primitive_tt_interval(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_INTERVAL;
  return n;
}
TT_PREDICATE(tt_intervalp, AST_TT_INTERVAL)

lizard_ast_node_t *lizard_primitive_tt_i0(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I0;
  return n;
}
TT_PREDICATE(tt_i0p, AST_TT_I0)

lizard_ast_node_t *lizard_primitive_tt_i1(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I1;
  return n;
}
TT_PREDICATE(tt_i1p, AST_TT_I1)

/* (I-var 'i) — an interval variable */
lizard_ast_node_t *lizard_primitive_tt_i_var(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n, *name_arg;
  char *buf;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (!name_arg || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I_VAR;
  buf = lizard_heap_alloc(strlen(name_arg->data.variable) + 1);
  strcpy(buf, name_arg->data.variable);
  n->data.tt_i_var.name = buf;
  return n;
}
TT_PREDICATE(tt_i_varp, AST_TT_I_VAR)

/* (I-and i j) */
lizard_ast_node_t *lizard_primitive_tt_i_and(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I_AND;
  n->data.tt_i_binop.left = nth_arg(args, 0);
  n->data.tt_i_binop.right = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_i_andp, AST_TT_I_AND)

/* (I-or i j) */
lizard_ast_node_t *lizard_primitive_tt_i_or(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I_OR;
  n->data.tt_i_binop.left = nth_arg(args, 0);
  n->data.tt_i_binop.right = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_i_orp, AST_TT_I_OR)

/* (I-neg i) */
lizard_ast_node_t *lizard_primitive_tt_i_neg(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_I_NEG;
  n->data.tt_i_neg.operand = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_i_negp, AST_TT_I_NEG)

/* (Path A a b) — path type */
lizard_ast_node_t *lizard_primitive_tt_path(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PATH;
  n->data.tt_path.domain = nth_arg(args, 0);
  n->data.tt_path.a      = nth_arg(args, 1);
  n->data.tt_path.b      = nth_arg(args, 2);
  return n;
}
TT_PREDICATE(tt_pathp, AST_TT_PATH)
TT_ACCESSOR(tt_path_domain, AST_TT_PATH, x->data.tt_path.domain)
TT_ACCESSOR(tt_path_a,      AST_TT_PATH, x->data.tt_path.a)
TT_ACCESSOR(tt_path_b,      AST_TT_PATH, x->data.tt_path.b)

/* (path-abs 'i body) — path abstraction, like Lambda but for intervals */
lizard_ast_node_t *lizard_primitive_tt_path_abs(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n, *binder_arg;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  binder_arg = nth_arg(args, 0);
  if (!binder_arg || binder_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PATH_ABS;
  n->data.tt_path_abs.binder = binder_arg;
  n->data.tt_path_abs.body = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_path_absp, AST_TT_PATH_ABS)
TT_ACCESSOR(tt_path_abs_binder, AST_TT_PATH_ABS, x->data.tt_path_abs.binder)
TT_ACCESSOR(tt_path_abs_body,   AST_TT_PATH_ABS, x->data.tt_path_abs.body)

/* (path-app p i) — path application, written conceptually as (p @ i) */
lizard_ast_node_t *lizard_primitive_tt_path_app(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PATH_APP;
  n->data.tt_path_app.path  = nth_arg(args, 0);
  n->data.tt_path_app.point = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_path_appp, AST_TT_PATH_APP)
TT_ACCESSOR(tt_path_app_path,  AST_TT_PATH_APP, x->data.tt_path_app.path)
TT_ACCESSOR(tt_path_app_point, AST_TT_PATH_APP, x->data.tt_path_app.point)

/* ===== Faces and partial elements (Turn 7) ===== */

lizard_ast_node_t *lizard_primitive_tt_f0(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_F0;
  return n;
}
TT_PREDICATE(tt_f0p, AST_TT_F0)

lizard_ast_node_t *lizard_primitive_tt_f1(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_F1;
  return n;
}
TT_PREDICATE(tt_f1p, AST_TT_F1)

/* (F-eq i j) — face: interval term i equals interval term j. */
lizard_ast_node_t *lizard_primitive_tt_f_eq(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_F_EQ;
  n->data.tt_f_eq.left  = nth_arg(args, 0);
  n->data.tt_f_eq.right = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_f_eqp, AST_TT_F_EQ)
TT_ACCESSOR(tt_f_eq_left,  AST_TT_F_EQ, x->data.tt_f_eq.left)
TT_ACCESSOR(tt_f_eq_right, AST_TT_F_EQ, x->data.tt_f_eq.right)

lizard_ast_node_t *lizard_primitive_tt_f_and(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_F_AND;
  n->data.tt_f_binop.left  = nth_arg(args, 0);
  n->data.tt_f_binop.right = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_f_andp, AST_TT_F_AND)

lizard_ast_node_t *lizard_primitive_tt_f_or(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_F_OR;
  n->data.tt_f_binop.left  = nth_arg(args, 0);
  n->data.tt_f_binop.right = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_f_orp, AST_TT_F_OR)

/* (Partial φ A) */
lizard_ast_node_t *lizard_primitive_tt_partial(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_PARTIAL;
  n->data.tt_partial.face = nth_arg(args, 0);
  n->data.tt_partial.type = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_partialp, AST_TT_PARTIAL)
TT_ACCESSOR(tt_partial_face, AST_TT_PARTIAL, x->data.tt_partial.face)
TT_ACCESSOR(tt_partial_type, AST_TT_PARTIAL, x->data.tt_partial.type)

/* (Sub A φ u) */
lizard_ast_node_t *lizard_primitive_tt_sub(lz_list_t *args,
                                           lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SUB;
  n->data.tt_sub.type    = nth_arg(args, 0);
  n->data.tt_sub.face    = nth_arg(args, 1);
  n->data.tt_sub.partial = nth_arg(args, 2);
  return n;
}
TT_PREDICATE(tt_subp, AST_TT_SUB)
TT_ACCESSOR(tt_sub_type,    AST_TT_SUB, x->data.tt_sub.type)
TT_ACCESSOR(tt_sub_face,    AST_TT_SUB, x->data.tt_sub.face)
TT_ACCESSOR(tt_sub_partial, AST_TT_SUB, x->data.tt_sub.partial)

/* ===== Kan composition (Turn 8) =====
 *
 * comp, hcomp, fill all take the same 4 arguments:
 *   (op type_family face partial base)
 * and differ only in semantics. */

#define MAKE_COMP_FAMILY(NAME, AST_KIND)                                       \
  lizard_ast_node_t *lizard_primitive_tt_##NAME(lz_list_t *args,               \
                                                lizard_env_t *env,             \
                                                lizard_heap_t *heap) {         \
    lizard_ast_node_t *n;                                                      \
    (void)env;                                                                 \
    if (!four_args(args)) {                                                    \
      return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);             \
    }                                                                          \
    n = lizard_heap_alloc(sizeof(lizard_ast_node_t));                          \
    n->type = AST_KIND;                                                        \
    n->data.tt_comp.type_family = nth_arg(args, 0);                            \
    n->data.tt_comp.face        = nth_arg(args, 1);                            \
    n->data.tt_comp.partial     = nth_arg(args, 2);                            \
    n->data.tt_comp.base        = nth_arg(args, 3);                            \
    return n;                                                                  \
  }                                                                            \
  TT_PREDICATE(tt_##NAME##p, AST_KIND)

MAKE_COMP_FAMILY(comp,  AST_TT_COMP)
MAKE_COMP_FAMILY(hcomp, AST_TT_HCOMP)
MAKE_COMP_FAMILY(fill,  AST_TT_FILL)

TT_ACCESSOR(tt_comp_type_family, AST_TT_COMP, x->data.tt_comp.type_family)
TT_ACCESSOR(tt_comp_face,        AST_TT_COMP, x->data.tt_comp.face)
TT_ACCESSOR(tt_comp_partial,     AST_TT_COMP, x->data.tt_comp.partial)
TT_ACCESSOR(tt_comp_base,        AST_TT_COMP, x->data.tt_comp.base)

/* ===== Equivalences, Glue, and ua (Turns 9 & 10) ===== */

/* (Equiv A B) — type former. Note: distinct from `equivalence` which
 * is a notation-level 4-arg packaging. */
lizard_ast_node_t *lizard_primitive_tt_equiv_type(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EQUIV_TYPE;
  n->data.tt_equiv_type.domain   = nth_arg(args, 0);
  n->data.tt_equiv_type.codomain = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_equiv_typep, AST_TT_EQUIV_TYPE)
TT_ACCESSOR(tt_equiv_domain,   AST_TT_EQUIV_TYPE, x->data.tt_equiv_type.domain)
TT_ACCESSOR(tt_equiv_codomain, AST_TT_EQUIV_TYPE, x->data.tt_equiv_type.codomain)

/* (id-equiv A) — the identity equivalence on A */
lizard_ast_node_t *lizard_primitive_tt_id_equiv(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_ID_EQUIV;
  n->data.tt_equiv_op.operand = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_id_equivp, AST_TT_ID_EQUIV)

/* (equiv-fun e), (equiv-inv e) */
lizard_ast_node_t *lizard_primitive_tt_equiv_fun(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EQUIV_FUN;
  n->data.tt_equiv_op.operand = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_equiv_funp, AST_TT_EQUIV_FUN)

lizard_ast_node_t *lizard_primitive_tt_equiv_inv(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EQUIV_INV;
  n->data.tt_equiv_op.operand = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_equiv_invp, AST_TT_EQUIV_INV)

/* (Glue A φ T e) */
lizard_ast_node_t *lizard_primitive_tt_glue(lz_list_t *args,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!four_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_GLUE;
  n->data.tt_glue.base  = nth_arg(args, 0);
  n->data.tt_glue.face  = nth_arg(args, 1);
  n->data.tt_glue.t     = nth_arg(args, 2);
  n->data.tt_glue.equiv = nth_arg(args, 3);
  return n;
}
TT_PREDICATE(tt_gluep, AST_TT_GLUE)
TT_ACCESSOR(tt_glue_base,  AST_TT_GLUE, x->data.tt_glue.base)
TT_ACCESSOR(tt_glue_face,  AST_TT_GLUE, x->data.tt_glue.face)
TT_ACCESSOR(tt_glue_t,     AST_TT_GLUE, x->data.tt_glue.t)
TT_ACCESSOR(tt_glue_equiv, AST_TT_GLUE, x->data.tt_glue.equiv)

/* (glue-intro φ t a) — written conceptually `glue φ t a` */
lizard_ast_node_t *lizard_primitive_tt_glue_intro(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_GLUE_INTRO;
  n->data.tt_glue_intro.face = nth_arg(args, 0);
  n->data.tt_glue_intro.t    = nth_arg(args, 1);
  n->data.tt_glue_intro.a    = nth_arg(args, 2);
  return n;
}
TT_PREDICATE(tt_glue_introp, AST_TT_GLUE_INTRO)

/* (unglue e g) */
lizard_ast_node_t *lizard_primitive_tt_unglue(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UNGLUE;
  n->data.tt_unglue.equiv  = nth_arg(args, 0);
  n->data.tt_unglue.target = nth_arg(args, 1);
  return n;
}
TT_PREDICATE(tt_unglue_p, AST_TT_UNGLUE)

/* (ua e) */
lizard_ast_node_t *lizard_primitive_tt_ua(lz_list_t *args,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_UA;
  n->data.tt_ua.equiv = nth_arg(args, 0);
  return n;
}
TT_PREDICATE(tt_uap, AST_TT_UA)

/* ===== System: multi-clause partial element (Turn 11) ===== */

/* (system-nil) — the empty system, defined nowhere. */
lizard_ast_node_t *lizard_primitive_tt_system_nil(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SYSTEM_NIL;
  return n;
}
TT_PREDICATE(tt_system_nilp, AST_TT_SYSTEM_NIL)

/* (system-cons face value next-system) */
lizard_ast_node_t *lizard_primitive_tt_system_cons(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_SYSTEM_CONS;
  n->data.tt_system_cons.face  = nth_arg(args, 0);
  n->data.tt_system_cons.value = nth_arg(args, 1);
  n->data.tt_system_cons.next  = nth_arg(args, 2);
  return n;
}
TT_PREDICATE(tt_system_consp, AST_TT_SYSTEM_CONS)
TT_ACCESSOR(tt_system_face,  AST_TT_SYSTEM_CONS, x->data.tt_system_cons.face)
TT_ACCESSOR(tt_system_value, AST_TT_SYSTEM_CONS, x->data.tt_system_cons.value)
TT_ACCESSOR(tt_system_next,  AST_TT_SYSTEM_CONS, x->data.tt_system_cons.next)

/* ===== Phase M.1 — Lisp primitives for logic-rule configuration ===== */

/* (logic-rule-register 'name)
 * Register a new rule (default disabled) if not present. Returns #t. */
lizard_ast_node_t *lizard_primitive_logic_rule_register(lz_list_t *args,
                                                        lizard_env_t *env,
                                                        lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  lizard_logic_rule_register(name_arg->data.variable, 0);
  return lizard_make_bool(heap, 1);
}

/* (logic-rule-enable 'name)
 * Enable a rule, auto-registering if needed. */
lizard_ast_node_t *lizard_primitive_logic_rule_enable(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  lizard_logic_rule_enable(name_arg->data.variable);
  return lizard_make_bool(heap, 1);
}

/* (logic-rule-disable 'name) */
lizard_ast_node_t *lizard_primitive_logic_rule_disable(lz_list_t *args,
                                                       lizard_env_t *env,
                                                       lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  lizard_logic_rule_disable(name_arg->data.variable);
  return lizard_make_bool(heap, 1);
}

/* (logic-rule-enabled? 'name)
 * Returns #t / #f / 'unknown (a symbol). */
lizard_ast_node_t *lizard_primitive_logic_rule_enabledp(lz_list_t *args,
                                                        lizard_env_t *env,
                                                        lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg;
  int r;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_logic_rule_enabled(name_arg->data.variable);
  if (r == 1) return lizard_make_bool(heap, 1);
  if (r == 0) return lizard_make_bool(heap, 0);
  /* Unknown — return the symbol 'unknown. */
  {
    char *buf = lizard_heap_alloc(8);
    lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    strcpy(buf, "unknown");
    n->type = AST_SYMBOL;
    n->data.variable = buf;
    return n;
  }
}

/* (logic-config)
 * Returns a list of (name . enabled?) pairs — one cons per registered
 * rule. The order is registration order, reverse of internal prepend. */
typedef struct {
  lizard_heap_t *heap;
  lizard_ast_node_t *head;  /* growing list, prepended */
} logic_config_collect_t;

static int logic_config_collect_cb(const char *name, int enabled, void *ud) {
  logic_config_collect_t *c = (logic_config_collect_t *)ud;
  lizard_ast_node_t *pair, *sym, *val, *cons;
  char *namedup;
  size_t namelen;
  /* Build (name . enabled?) as a pair. */
  pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  pair->type = AST_PAIR;
  namelen = strlen(name) + 1;
  namedup = lizard_heap_alloc(namelen);
  memcpy(namedup, name, namelen);
  sym = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  sym->type = AST_SYMBOL;
  sym->data.variable = namedup;
  val = lizard_make_bool(c->heap, enabled);
  pair->data.pair.car = sym;
  pair->data.pair.cdr = val;
  /* Prepend to head. */
  cons = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  cons->type = AST_PAIR;
  cons->data.pair.car = pair;
  cons->data.pair.cdr = c->head;
  c->head = cons;
  return 0;
}

lizard_ast_node_t *lizard_primitive_logic_config(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  logic_config_collect_t c;
  lizard_ast_node_t *nil;
  (void)env; (void)args;
  /* Build the empty list (nil) as the initial tail. */
  nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  nil->type = AST_NIL;
  c.heap = heap;
  c.head = nil;
  lizard_logic_config_walk(logic_config_collect_cb, &c);
  return c.head;
}

/* (logic-config-size) — number of registered rules. */
lizard_ast_node_t *lizard_primitive_logic_config_size(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_NUMBER;
  mpz_init(n->data.number);
  mpz_set_si(n->data.number, lizard_logic_config_size());
  return n;
}

/* (logic-config-reset) — clear the registry. */
lizard_ast_node_t *lizard_primitive_logic_config_reset(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  (void)env; (void)args;
  lizard_logic_config_reset();
  return lizard_make_bool(heap, 1);
}


/* ===== Optional proof-theory scaffolds =====
 *
 * These constructors are intentionally opt-in. They are useful for designing
 * cubical S¹, truncations, and user-provided theory extensions, but they are
 * not trusted kernel terms yet.
 */

static int lizard_rule_on(const char *name) {
  return lizard_logic_rule_enabled(name) == 1;
}

static lizard_ast_node_t *lizard_make_nullary_tt(lizard_heap_t *heap,
                                                  lizard_ast_node_type_t type) {
  lizard_ast_node_t *n;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = type;
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_s1(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  (void)env;
  if (!no_args(args) || !lizard_rule_on("cubical-s1-enabled")) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  return lizard_make_nullary_tt(heap, AST_TT_S1);
}

lizard_ast_node_t *lizard_primitive_tt_s1p(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  return lizard_make_bool(heap, x != NULL && x->type == AST_TT_S1);
}

lizard_ast_node_t *lizard_primitive_tt_s1_base(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  (void)env;
  if (!no_args(args) || !lizard_rule_on("cubical-s1-enabled")) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  return lizard_make_nullary_tt(heap, AST_TT_S1_BASE);
}

lizard_ast_node_t *lizard_primitive_tt_s1_basep(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  return lizard_make_bool(heap, x != NULL && x->type == AST_TT_S1_BASE);
}

lizard_ast_node_t *lizard_primitive_tt_s1_loop(lz_list_t *args,
                                               lizard_env_t *env,
                                               lizard_heap_t *heap) {
  (void)env;
  if (!no_args(args) || !lizard_rule_on("cubical-s1-enabled")) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  return lizard_make_nullary_tt(heap, AST_TT_S1_LOOP);
}

lizard_ast_node_t *lizard_primitive_tt_s1_loopp(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  return lizard_make_bool(heap, x != NULL && x->type == AST_TT_S1_LOOP);
}

lizard_ast_node_t *lizard_primitive_tt_trunc(lz_list_t *args,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!two_args(args) || !lizard_rule_on("truncations-enabled")) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_TRUNC;
  n->data.tt_trunc.level = nth_arg(args, 0);
  n->data.tt_trunc.type = nth_arg(args, 1);
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_truncp(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  return lizard_make_bool(heap, x != NULL && x->type == AST_TT_TRUNC);
}

lizard_ast_node_t *lizard_primitive_tt_trunc_level(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_TRUNC) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return x->data.tt_trunc.level;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_type(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_TRUNC) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return x->data.tt_trunc.type;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_intro(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env;
  if (!single_arg(args) || !lizard_rule_on("truncations-enabled")) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_TRUNC_INTRO;
  n->data.tt_trunc_intro.value = nth_arg(args, 0);
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_introp(lz_list_t *args,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  return lizard_make_bool(heap, x != NULL && x->type == AST_TT_TRUNC_INTRO);
}

lizard_ast_node_t *lizard_primitive_tt_trunc_value(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_TRUNC_INTRO) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return x->data.tt_trunc_intro.value;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_elim(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  int has_prop;
  (void)env;
  /* Accept 3 args (scaffold, no prop witness) or 4 args (checked,
   * with prop witness for propositionality). */
  has_prop = four_args(args);
  if (!has_prop && !three_args(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  if (!lizard_rule_on("truncations-enabled")) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_TRUNC_ELIM;
  n->data.tt_trunc_elim.motive = nth_arg(args, 0);
  n->data.tt_trunc_elim.handler = nth_arg(args, 1);
  if (has_prop) {
    n->data.tt_trunc_elim.prop  = nth_arg(args, 2);
    n->data.tt_trunc_elim.value = nth_arg(args, 3);
  } else {
    n->data.tt_trunc_elim.prop  = NULL;
    n->data.tt_trunc_elim.value = nth_arg(args, 2);
  }
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_elimp(lz_list_t *args,
                                                   lizard_env_t *env,
                                                   lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  return lizard_make_bool(heap, x != NULL && x->type == AST_TT_TRUNC_ELIM);
}

lizard_ast_node_t *lizard_primitive_tt_trunc_elim_motive(lz_list_t *args,
                                                         lizard_env_t *env,
                                                         lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_TRUNC_ELIM) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return x->data.tt_trunc_elim.motive;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_elim_handler(lz_list_t *args,
                                                          lizard_env_t *env,
                                                          lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_TRUNC_ELIM) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return x->data.tt_trunc_elim.handler;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_elim_value(lz_list_t *args,
                                                        lizard_env_t *env,
                                                        lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_TRUNC_ELIM) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return x->data.tt_trunc_elim.value;
}

lizard_ast_node_t *lizard_primitive_tt_trunc_elim_prop(lz_list_t *args,
                                                       lizard_env_t *env,
                                                       lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_TRUNC_ELIM) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  if (x->data.tt_trunc_elim.prop == NULL) return lizard_make_nil(heap);
  return x->data.tt_trunc_elim.prop;
}

static lizard_ast_node_t *tt_list_to_lisp_list(lz_list_t *list,
                                               lizard_heap_t *heap) {
  lizard_ast_node_t *head;
  lizard_ast_node_t *tail;
  lz_list_node_t *iter;

  head = lizard_make_nil(heap);
  tail = NULL;
  if (list == NULL) return head;
  for (iter = list->head; iter != list->nil; iter = iter->next) {
    lizard_ast_node_t *cell;
    cell = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    cell->type = AST_PAIR;
    cell->data.pair.car = ((lizard_ast_list_node_t *)iter)->ast;
    cell->data.pair.cdr = lizard_make_nil(heap);
    if (tail == NULL) {
      head = cell;
    } else {
      tail->data.pair.cdr = cell;
    }
    tail = cell;
  }
  return head;
}

lizard_ast_node_t *lizard_primitive_tt_extension(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  lizard_ast_node_t *name;
  lz_list_t *rest;
  lz_list_node_t *iter;
  (void)env;
  if (args->head == args->nil || !lizard_rule_on("theory-extensions-enabled")) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  name = nth_arg(args, 0);
  if (name == NULL || name->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  rest = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  for (iter = args->head->next; iter != args->nil; iter = iter->next) {
    lizard_ast_list_node_t *copy;
    copy = lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    copy->ast = ((lizard_ast_list_node_t *)iter)->ast;
    list_append(rest, &copy->node);
  }
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_TT_EXTENSION;
  n->data.tt_extension.name = name;
  n->data.tt_extension.args = rest;
  return n;
}

lizard_ast_node_t *lizard_primitive_tt_extensionp(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  return lizard_make_bool(heap, x != NULL && x->type == AST_TT_EXTENSION);
}

lizard_ast_node_t *lizard_primitive_tt_extension_name(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_EXTENSION) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return x->data.tt_extension.name;
}

lizard_ast_node_t *lizard_primitive_tt_extension_args(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  lizard_ast_node_t *x;
  (void)env;
  if (!single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  x = nth_arg(args, 0);
  if (x == NULL || x->type != AST_TT_EXTENSION) return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  return tt_list_to_lisp_list(x->data.tt_extension.args, heap);
}

/* ===== Phase M.3 — Lisp primitives for logic bundles ===== */

/* (set-logic 'NAME)
 * Apply a named logic bundle. Returns #t on success, #f if unknown.
 * Unknown name does NOT raise an error — returns #f so user code can
 * test before assuming the logic is loaded. */
lizard_ast_node_t *lizard_primitive_set_logic(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg;
  int ok;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  ok = lizard_logic_set_bundle(name_arg->data.variable);
  return lizard_make_bool(heap, ok);
}

/* (current-logic)
 * Returns a symbol naming the current logic, or 'custom. */
lizard_ast_node_t *lizard_primitive_current_logic(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  const char *name;
  lizard_ast_node_t *n;
  char *buf;
  size_t len;
  (void)env; (void)args;
  name = lizard_logic_current_bundle();
  len = strlen(name) + 1;
  buf = lizard_heap_alloc(len);
  memcpy(buf, name, len);
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_SYMBOL;
  n->data.variable = buf;
  return n;
}

/* (list-logics)
 * Returns a list of symbols naming the predefined logic bundles. */
typedef struct {
  lizard_heap_t *heap;
  lizard_ast_node_t *head;
} list_logics_collect_t;

static int list_logics_collect_cb(const char *name, void *ud) {
  list_logics_collect_t *c = (list_logics_collect_t *)ud;
  lizard_ast_node_t *sym, *cons;
  char *buf;
  size_t len;
  len = strlen(name) + 1;
  buf = lizard_heap_alloc(len);
  memcpy(buf, name, len);
  sym = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  sym->type = AST_SYMBOL;
  sym->data.variable = buf;
  cons = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  cons->type = AST_PAIR;
  cons->data.pair.car = sym;
  cons->data.pair.cdr = c->head;
  c->head = cons;
  return 0;
}

lizard_ast_node_t *lizard_primitive_list_logics(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  list_logics_collect_t c;
  lizard_ast_node_t *nil;
  (void)env; (void)args;
  nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  nil->type = AST_NIL;
  c.heap = heap;
  c.head = nil;
  lizard_logic_bundles_walk(list_logics_collect_cb, &c);
  return c.head;
}

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
