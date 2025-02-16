#ifndef LIZARD_H
#define LIZARD_H
#include <ds.h>
#include <gmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

/*
___                     .-*''*-.
 '.* *'.        .|     *       _*
  _)*_*_\__   \.`',/  * EVAL .'  *
 / _______ \  = ,. =  *.___.'    *
 \_)|@_@|(_/   // \   '.   APPLY.'
   ))\_/((    //        *._  _.*
  (((\V/)))  //            ||
 //-\\^//-\\--            /__\
*/

typedef enum {
  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,
  TOKEN_SYMBOL,
  TOKEN_NUMBER,
  TOKEN_STRING
} lizard_token_type_t;

typedef struct lizard_token {
  lizard_token_type_t type;
  union {
    char *string;
    char *symbol;
    mpz_t number;
  } data;
} lizard_token_t;

typedef struct lizard_token_list_node {
  list_node_t node;
  lizard_token_t token;
} lizard_token_list_node_t;

#define LIZARD_ERROR_LIST                                                      \
  X(LIZARD_ERROR_NONE)                                                         \
  X(LIZARD_ERROR_UNBOUND_SYMBOL)                                               \
  X(LIZARD_ERROR_INVALID_FUNCTION_DEF)                                         \
  X(LIZARD_ERROR_INVALID_FUNCTION_NAME)                                        \
  X(LIZARD_ERROR_INVALID_PARAMETER)                                            \
  X(LIZARD_ERROR_INVALID_DEF)                                                  \
  X(LIZARD_ERROR_ASSIGNMENT)                                                   \
  X(LIZARD_ERROR_ASSIGNMENT_UNBOUND)                                           \
  X(LIZARD_ERROR_INVALID_MACRO_NAME)                                           \
  X(LIZARD_ERROR_NODE_TYPE)                                                    \
  X(LIZARD_ERROR_LAMBDA_PARAMS)                                                \
  X(LIZARD_ERROR_VARIADIC_UNFOLLOWED)                                          \
  X(LIZARD_ERROR_VARIADIC_SYMBOL)                                              \
  X(LIZARD_ERROR_LAMBDA_ARITY_LESS)                                            \
  X(LIZARD_ERROR_LAMBDA_PARAMETER)                                             \
  X(LIZARD_ERROR_LAMBDA_ARITY_MORE)                                            \
  X(LIZARD_ERROR_INVALID_APPLY)                                                \
  X(LIZARD_ERROR_PLUS)                                                         \
  X(LIZARD_ERROR_MINUS_ARGC)                                                   \
  X(LIZARD_ERROR_MINUS_ARGT)                                                   \
  X(LIZARD_ERROR_MINUS_ARGT_2)                                                 \
  X(LIZARD_ERROR_MUL)                                                          \
  X(LIZARD_ERROR_DIV_ARGC)                                                     \
  X(LIZARD_ERROR_DIV_ARGT)                                                     \
  X(LIZARD_ERROR_DIV_ARGT_2)                                                   \
  X(LIZARD_ERROR_DIV_ZERO)                                                     \
  X(LIZARD_ERROR_EQ_ARGC)                                                      \
  X(LIZARD_ERROR_CONS_ARGC)                                                    \
  X(LIZARD_ERROR_CAR_ARGC)                                                     \
  X(LIZARD_ERROR_CAR_ARGT)                                                     \
  X(LIZARD_ERROR_CAR_NIL)                                                      \
  X(LIZARD_ERROR_CDR_ARGC)                                                     \
  X(LIZARD_ERROR_CDR_ARGT)                                                     \
  X(LIZARD_ERROR_CDR_NIL)                                                      \
  X(LIZARD_ERROR_TOKENS_ARGC)                                                  \
  X(LIZARD_ERROR_TOKENS_ARGT)                                                  \
  X(LIZARD_ERROR_EVAL_ARGC)

typedef enum lizard_error_code {
#define X(err) err,
  LIZARD_ERROR_LIST
#undef X
      LIZARD_ERROR_COUNT
} lizard_error_code_t;

typedef enum {
  AST_STRING,
  AST_NUMBER,
  AST_SYMBOL,
  AST_BOOL,
  AST_NIL,
  AST_QUOTED,
  AST_ASSIGNMENT,
  AST_DEFINITION,
  AST_IF,
  AST_LAMBDA,
  AST_BEGIN,
  AST_COND,
  AST_APPLICATION,
  AST_PRIMITIVE,
  AST_MACRO,
  AST_ERROR
} lizard_ast_node_type_t;

typedef struct lizard_ast_node lizard_ast_node_t;
typedef struct lizard_env lizard_env_t;
typedef struct lizard_heap lizard_heap_t;
typedef lizard_ast_node_t *(*lizard_primitive_func_t)(list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap);

struct lizard_ast_node {
  lizard_ast_node_type_t type;
  union {
    bool boolean;
    const char *string;
    mpz_t number;
    const char *variable;
    struct lizard_ast_node *quoted;
    struct {
      struct lizard_ast_node *variable;
      struct lizard_ast_node *value;
    } assignment;
    struct {
      struct lizard_ast_node *variable;
      struct lizard_ast_node *value;
    } definition;
    struct {
      struct lizard_ast_node *pred;
      struct lizard_ast_node *cons;
      struct lizard_ast_node *alt;
    } if_clause;
    struct {
      list_t *parameters;
      lizard_env_t *closure_env;
    } lambda;
    list_t *begin_expressions;
    list_t *cond_clauses;
    list_t *application_arguments;
    struct {
      lizard_ast_node_t *car;
      lizard_ast_node_t *cdr;
    } pair;
    lizard_primitive_func_t primitive;
    struct {
      lizard_ast_node_t *variable;
      lizard_ast_node_t *transformer;
    } macro_def;
    struct {
      int code;
      list_t *data;
    } error;
  } data;
};

typedef struct lizard_ast_list_node {
  list_node_t node;
  lizard_ast_node_t ast;
} lizard_ast_list_node_t;

struct lizard_heap {
  size_t *start;
  size_t *top;
  size_t *end;
  size_t reserved;
};

typedef struct lizard_env_entry {
  const char *symbol;
  lizard_ast_node_t *value;
  struct lizard_env_entry *next;
} lizard_env_entry_t;

struct lizard_env {
  lizard_env_entry_t *entries;
  struct lizard_env *parent;
};

void print_ast(lizard_ast_node_t *node, int depth);
lizard_ast_node_t *lizard_eval(lizard_ast_node_t *node, lizard_env_t *env,
                               lizard_heap_t *heap);
lizard_ast_node_t *lizard_apply(lizard_ast_node_t *func, list_t *args,
                                lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_expand_macros(lizard_ast_node_t *node,
                                        lizard_env_t *env, lizard_heap_t *heap);
#endif /* LIZARD_H */