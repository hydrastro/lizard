#include "primitives.h"
#include "env.h"
#include "errors.h"
#include "lizard.h"
#include "mem.h"
#include "parser.h"
#include "printer.h"
#include "tokenizer.h"
#include <setjmp.h>
#include <stdint.h>
extern jmp_buf callcc_buf;
extern int callcc_active;
extern lizard_ast_node_t *callcc_value;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

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
  if (!callcc_active) {
    callcc_active = 1;
    if (setjmp(callcc_buf) != 0) {
      callcc_active = 0;
      return callcc_value;
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
  switch (node->type) {
  case AST_NUMBER:
    mpz_init(copy->data.number);
    mpz_set(copy->data.number, node->data.number);
    break;
  case AST_STRING:
    copy->data.string = strdup(node->data.string);
    break;
  case AST_SYMBOL:
    copy->data.variable = strdup(node->data.variable);
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

/* ---- primitive registration ----
   Install every built-in primitive into the given environment.
   Called by both the REPL and the test harness so the set of
   primitives stays in one place. */

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
  case AST_TT_APP:         name = "@";           break;
  case AST_TT_SUM:         name = "Sum";         break;
  case AST_TT_UNIVERSE:    name = "U";           break;
  case AST_TT_COUNIVERSE:  name = "Uco";         break;
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
 * Useful for hand-written hygienic macros until we have syntax-rules. */
static unsigned long lizard_gensym_counter = 0;

lizard_ast_node_t *lizard_primitive_gensym(lz_list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  const char *prefix = "g";
  char buf[64];
  (void)env;
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
  lizard_gensym_counter++;
  snprintf(buf, sizeof(buf), "%s%lu", prefix, lizard_gensym_counter);
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
TT_PREDICATE(tt_appp,         AST_TT_APP)
TT_PREDICATE(tt_sump,         AST_TT_SUM)
TT_PREDICATE(tt_universep,    AST_TT_UNIVERSE)
TT_PREDICATE(tt_couniversep,  AST_TT_COUNIVERSE)
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
  install_one(heap, env, "@",             lizard_primitive_tt_at);
  install_one(heap, env, "Sum",           lizard_primitive_tt_sum);
  install_one(heap, env, "U",             lizard_primitive_tt_universe);
  install_one(heap, env, "Uco",           lizard_primitive_tt_couniverse);
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
  install_one(heap, env, "universe-leq?", lizard_primitive_tt_universe_leq);
  install_one(heap, env, "face-entails?", lizard_primitive_tt_face_entails);
  /* Bidirectional type checker for the λΠ fragment.
   *   (infer Γ t)    returns the inferred type of t in context Γ.
   *   (check Γ t T)  returns #t if t checks against T in Γ.
   * Currently covers: variables, Pi, Lambda, application, U, annotation. */
  install_one(heap, env, "infer",      lizard_primitive_tt_infer);
  install_one(heap, env, "check",      lizard_primitive_tt_check);
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
