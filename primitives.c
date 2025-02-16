#include "primitives.h"
#include "lizard.h"
#include "mem.h"
#include "tokenizer.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
lizard_ast_node_t *lizard_primitive_plus(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  list_node_t *node;
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *arg;
  lizard_ast_node_t *result;
  mpz_t sum;
  mpz_init(sum);

  for (node = args->head; node != args->nil; node = node->next) {
    arg_node = CAST(node, lizard_ast_list_node_t);
    arg = &arg_node->ast;
    if (arg->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_PLUS);
    }
    mpz_add(sum, sum, arg->data.number);
  }

  result = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  result->type = AST_NUMBER;
  mpz_init(result->data.number);
  mpz_set(result->data.number, sum);
  mpz_clear(sum);
  return result;
}

lizard_ast_node_t *lizard_primitive_minus(list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  list_node_t *node;
  lizard_ast_list_node_t *first_node, *arg_node;
  lizard_ast_node_t *arg, *ret;
  mpz_t result;
  if (args->head == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_MINUS_ARGC);
  }
  node = args->head;
  first_node = CAST(node, lizard_ast_list_node_t);
  if (first_node->ast.type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_MINUS_ARGT);
  }
  mpz_init(result);
  mpz_set(result, first_node->ast.data.number);
  node = node->next;
  if (node == args->nil) {
    mpz_neg(result, result);
  } else {
    for (; node != args->nil; node = node->next) {
      arg_node = CAST(node, lizard_ast_list_node_t);
      arg = &arg_node->ast;
      if (arg->type != AST_NUMBER) {
        return lizard_make_error(heap, LIZARD_ERROR_MINUS_ARGT_2);
      }
      mpz_sub(result, result, arg->data.number);
    }
  }
  ret = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  ret->type = AST_NUMBER;
  mpz_init(ret->data.number);
  mpz_set(ret->data.number, result);
  mpz_clear(result);
  return ret;
}

lizard_ast_node_t *lizard_primitive_multiply(list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap) {
  list_node_t *node;
  lizard_ast_list_node_t *arg_node;
  lizard_ast_node_t *arg, *ret;

  mpz_t product;
  mpz_init_set_ui(product, 1);
  for (node = args->head; node != args->nil; node = node->next) {
    arg_node = CAST(node, lizard_ast_list_node_t);
    arg = &arg_node->ast;
    if (arg->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_MUL);
    }
    mpz_mul(product, product, arg->data.number);
  }
  ret = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  ret->type = AST_NUMBER;
  mpz_init(ret->data.number);
  mpz_set(ret->data.number, product);
  mpz_clear(product);
  return ret;
}

lizard_ast_node_t *lizard_primitive_divide(list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap) {
  mpz_t quotient;
  list_node_t *node;
  lizard_ast_list_node_t *first_node, *arg_node;
  lizard_ast_node_t *arg, *ret;

  if (args->head == args->nil || args->head->next == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_DIV_ARGC);
  }
  node = args->head;
  first_node = CAST(node, lizard_ast_list_node_t);
  if (first_node->ast.type != AST_NUMBER) {
    return lizard_make_error(heap, LIZARD_ERROR_DIV_ARGT);
  }
  mpz_init(quotient);
  mpz_set(quotient, first_node->ast.data.number);
  for (node = node->next; node != args->nil; node = node->next) {
    arg_node = CAST(node, lizard_ast_list_node_t);
    arg = &arg_node->ast;
    if (arg->type != AST_NUMBER) {
      return lizard_make_error(heap, LIZARD_ERROR_DIV_ARGT_2);
    }
    if (mpz_cmp_ui(arg->data.number, 0) == 0) {
      return lizard_make_error(heap, LIZARD_ERROR_DIV_ZERO);
    }
    mpz_tdiv_q(quotient, quotient, arg->data.number);
  }
  ret = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  ret->type = AST_NUMBER;
  mpz_init(ret->data.number);
  mpz_set(ret->data.number, quotient);
  mpz_clear(quotient);
  return ret;
}
lizard_ast_node_t *lizard_primitive_equal(list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap) {
  list_node_t *node;
  lizard_ast_list_node_t *first_node, *other_node;
  lizard_ast_node_t *first, *other, *result;
  int eq;
  if (args->head == args->nil || args->head->next == args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_EQ_ARGC);
  }
  first_node = (lizard_ast_list_node_t *)args->head;
  first = &first_node->ast;
  eq = 1;
  for (node = args->head->next; node != args->nil; node = node->next) {
    other_node = (lizard_ast_list_node_t *)node;
    other = &other_node->ast;
    if (first->type != other->type) {
      eq = 0;
      break;
    }
    switch (first->type) {
    case AST_NUMBER:
      if (mpz_cmp(first->data.number, other->data.number) != 0)
        eq = 0;
      break;
    case AST_SYMBOL:
      if (strcmp(first->data.variable, other->data.variable) != 0)
        eq = 0;
      break;
    case AST_BOOL:
      if (first->data.boolean != other->data.boolean)
        eq = 0;
      break;
    case AST_NIL:
      break;
    default:
      /* comparing pointers */
      if (first != other)
        eq = 0;
    }
    if (!eq)
      break;
  }
  result = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  result->type = AST_BOOL;
  result->data.boolean = (eq != 0);
  return result;
}

lizard_ast_node_t *lizard_primitive_cons(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_node_t *node;
  lizard_ast_list_node_t *car_node, *new_car_node, *cdr_node, *new_cdr_node;
  if (args->head == args->nil || args->head->next == args->nil ||
      args->head->next->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CONS_ARGC);
  }
  node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  node->type = AST_APPLICATION;
  node->data.application_arguments = list_create();

  car_node = CAST(args->head, lizard_ast_list_node_t);
  new_car_node = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
  new_car_node->ast = car_node->ast; /* shallow copy */
  list_append(node->data.application_arguments, &new_car_node->node);

  cdr_node = CAST(args->head->next, lizard_ast_list_node_t);
  new_cdr_node = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
  new_cdr_node->ast = cdr_node->ast; /* shallow copy */
  list_append(node->data.application_arguments, &new_cdr_node->node);

  return node;
}

lizard_ast_node_t *lizard_primitive_car(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  list_t *app_args;
  lizard_ast_list_node_t *first_arg;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CAR_ARGC);
  }
  node = CAST(args->head, lizard_ast_list_node_t);
  if (node->ast.type != AST_APPLICATION) {
    return lizard_make_error(heap, LIZARD_ERROR_CAR_ARGT);
  }
  app_args = node->ast.data.application_arguments;
  if (app_args->head == app_args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CAR_NIL);
  }
  first_arg = CAST(app_args->head, lizard_ast_list_node_t);
  return &first_arg->ast;
}

lizard_ast_node_t *lizard_primitive_cdr(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap) {
  lizard_ast_list_node_t *node, *copy;
  list_t *app_args;
  lizard_ast_node_t *cdr_result;
  list_node_t *iter;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CDR_ARGC);
  }
  node = CAST(args->head, lizard_ast_list_node_t);
  if (node->ast.type != AST_APPLICATION) {
    return lizard_make_error(heap, LIZARD_ERROR_CDR_ARGT);
  }
  app_args = node->ast.data.application_arguments;
  if (app_args->head == app_args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_CDR_NIL);
  }
  cdr_result = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
  cdr_result->type = AST_APPLICATION;
  cdr_result->data.application_arguments = list_create();

  for (iter = app_args->head->next; iter != app_args->nil; iter = iter->next) {
    copy = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
    copy->ast = ((lizard_ast_list_node_t *)iter)->ast; /* shallow copy */
    list_append(cdr_result->data.application_arguments, &copy->node);
  }
  return cdr_result;
}

lizard_ast_node_t *lizard_make_primitive(lizard_heap_t *heap,
                                         lizard_primitive_func_t func) {
  lizard_ast_node_t *node = lizard_heap_alloc(heap, sizeof(lizard_ast_node_t));
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

  first = &CAST(args->head, lizard_ast_list_node_t)->ast;
  rest_args = list_create();
  rest_args->head = args->head->next;
  rest_args->nil = args->nil;
  rest = lizard_primitive_list(rest_args, env, heap);

  cons_args = list_create();
  first_node = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
  first_node->ast = *first;
  list_append(cons_args, &first_node->node);
  rest_node = lizard_heap_alloc(heap, sizeof(lizard_ast_list_node_t));
  rest_node->ast = *rest;
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
  if (node->ast.type != AST_STRING) {
    return lizard_make_error(heap, LIZARD_ERROR_TOKENS_ARGT);
  }

  input = node->ast.data.string;
  tokens = lizard_tokenize(input);

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

lizard_ast_node_t *lizard_primitive_eval(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap) {
  lizard_ast_list_node_t *node;
  lizard_ast_node_t *expr;
  if (args->head == args->nil || args->head->next != args->nil) {
    return lizard_make_error(heap, LIZARD_ERROR_EVAL_ARGC);
  }

  node = CAST(args->head, lizard_ast_list_node_t);
  expr = &node->ast;
  return lizard_eval(expr, env, heap);
}
#pragma GCC diagnostic pop
