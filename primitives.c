#include "primitives.h"
#include "lizard.h"
#include "mem.h"
#include "parser.h"
#include "printer.h"
#include "tokenizer.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

lizard_ast_node_t *lizard_primitive_plus(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  list_node_t *node;
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

lizard_ast_node_t *lizard_primitive_minus(list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  list_node_t *node;
  lizard_ast_list_node_t *first_arg_node, *arg_node;
  lizard_ast_node_t *acc, *arg;

  node = args->head;
  if (node == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_MINUS_ARGC);
  }
  first_arg_node = (lizard_ast_list_node_t *)node;
  acc = first_arg_node->ast;
  if (acc->type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_MINUS_ARGT);
  }
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

lizard_ast_node_t *lizard_primitive_multiply(list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  list_node_t *node;
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

lizard_ast_node_t *lizard_primitive_divide(list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  list_node_t *node;
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
    list_t *args = node->data.application_arguments;
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
    list_t *params_a = a->data.lambda.parameters;
    list_t *params_b = b->data.lambda.parameters;
    list_node_t *pa = params_a->head, *pb = params_b->head;
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
    list_t *exprs_a = a->data.begin_expressions;
    list_t *exprs_b = b->data.begin_expressions;
    list_node_t *na = exprs_a->head, *nb = exprs_b->head;
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
    list_t *args_a = a->data.application_arguments;
    list_t *args_b = b->data.application_arguments;
    list_node_t *na = args_a->head, *nb = args_b->head;
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

lizard_ast_node_t *lizard_primitive_equal(list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  list_node_t *node;
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

lizard_ast_node_t *lizard_primitive_pow(list_t *args, lizard_env_t *env,
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

lizard_ast_node_t *lizard_primitive_lt(list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *prev, *current;
  list_node_t *node;
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

lizard_ast_node_t *lizard_primitive_le(list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *prev, *current;
  list_node_t *node;
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

lizard_ast_node_t *lizard_primitive_gt(list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *prev, *current;
  list_node_t *node;
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

lizard_ast_node_t *lizard_primitive_ge(list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap) {
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *prev, *current;
  list_node_t *node;
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

lizard_ast_node_t *lizard_primitive_mod(list_t *args, lizard_env_t *env,
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

  ret = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  ret->type = AST_NUMBER;
  mpz_init(ret->data.number);
  mpz_mod(ret->data.number, dividend->data.number, divisor->data.number);

  return ret;
}

lizard_ast_node_t *lizard_primitive_cons(list_t *args, lizard_env_t *env,
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

lizard_ast_node_t *lizard_primitive_car(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CAR_ARGC);
  }
  node = (lizard_ast_list_node_t *)args->head;
  if (node->ast->type != AST_PAIR) {
    return lizard_make_error(heap, LIZARD_ERROR_CAR_ARGT);
  }
  return node->ast->data.pair.car;
}

lizard_ast_node_t *lizard_primitive_cdr(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CDR_ARGC);
  }
  node = (lizard_ast_list_node_t *)args->head;
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

lizard_ast_node_t *lizard_primitive_list(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *first, *rest;
  list_t *rest_args, *cons_args;
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

lizard_ast_node_t *lizard_primitive_tokens(list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  const char *input;
  list_t *tokens;
  list_node_t *token_node;
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

lizard_ast_node_t *lizard_primitive_ast(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  const char *input;
  list_t *tokens, *ast_list;
  list_node_t *current_node;

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

lizard_ast_node_t *lizard_primitive_eval(list_t *args, lizard_env_t *env,
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

lizard_ast_node_t *lizard_primitive_unquote(list_t *args, lizard_env_t *env,
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
    list_t *args, lizard_env_t *env, lizard_heap_t *heap,
    lizard_ast_node_t *(*current_cont)(lizard_ast_node_t *, lizard_env_t *,
                                       lizard_heap_t *)) {
  lizard_ast_list_node_t *arg0;
  lizard_ast_node_t *proc;
  lizard_ast_node_t *cont_obj;
  list_t *arg_list;
  lizard_ast_list_node_t *node_arg;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CALLCC_ARGC);
  }
  arg0 = (lizard_ast_list_node_t *)args->head;
  proc = arg0->ast;
  if (proc->type != AST_LAMBDA && proc->type != AST_PRIMITIVE) {
    return lizard_make_error(heap, LIZARD_ERROR_CALLCC_APPLY);
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

lizard_ast_node_t *lizard_primitive_nullp(list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_list_node_t *node_arg;
  (void)env; /* unused */
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_NULLP_ARGC);
  }
  node_arg = (lizard_ast_list_node_t *)args->head;
  if (node_arg->ast->type == AST_NIL)
    return lizard_make_bool(heap, true);
  else
    return lizard_make_bool(heap, false);
}

lizard_ast_node_t *lizard_primitive_pairp(list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_list_node_t *node_arg;
  (void)env;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_PAIRP_ARGC);
  }
  node_arg = (lizard_ast_list_node_t *)args->head;
  if (node_arg->ast->type == AST_APPLICATION)
    return lizard_make_bool(heap, true);
  else
    return lizard_make_bool(heap, false);
}

lizard_ast_node_t *lizard_primitive_stringp(list_t *args, lizard_env_t *env,
                                            lizard_heap_t *heap) {
  lizard_ast_list_node_t *node_arg;
  (void)env;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_STRINGP_ARGC);
  }
  node_arg = (lizard_ast_list_node_t *)args->head;
  if (node_arg->ast->type == AST_STRING)
    return lizard_make_bool(heap, true);
  else
    return lizard_make_bool(heap, false);
}

lizard_ast_node_t *lizard_primitive_boolp(list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  lizard_ast_list_node_t *node_arg;
  (void)env;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_BOOLP_ARGC);
  }
  node_arg = (lizard_ast_list_node_t *)args->head;
  if (node_arg->ast->type == AST_BOOL)
    return lizard_make_bool(heap, true);
  else
    return lizard_make_bool(heap, false);
}

lizard_ast_node_t *lizard_primitive_and(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  list_node_t *iter;
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

lizard_ast_node_t *lizard_primitive_or(list_t *args, lizard_env_t *env,
                                       lizard_heap_t *heap) {
  list_node_t *iter;
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

lizard_ast_node_t *lizard_primitive_not(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_list_node_t *node_arg;
  (void)env;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_NOT_ARGC);
  }
  node_arg = (lizard_ast_list_node_t *)args->head;
  if (lizard_is_false(node_arg->ast))
    return lizard_make_bool(heap, true);
  else
    return lizard_make_bool(heap, false);
}

lizard_ast_node_t *lizard_primitive_xor(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  list_node_t *iter;
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

lizard_ast_node_t *lizard_primitive_nand(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *and_result = lizard_primitive_and(args, env, heap);
  return lizard_make_bool(heap, lizard_is_false(and_result));
}

lizard_ast_node_t *lizard_primitive_nor(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_node_t *or_result = lizard_primitive_or(args, env, heap);
  return lizard_make_bool(heap, lizard_is_false(or_result));
}

lizard_ast_node_t *lizard_primitive_xnor(list_t *args, lizard_env_t *env,
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
      list_node_t *iter;
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
