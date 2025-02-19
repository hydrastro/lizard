#ifndef LIZARD_H
#define LIZARD_H
#include "errors.h"
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
  AST_STRING,
  AST_NUMBER,
  AST_SYMBOL,
  AST_BOOL,
  AST_NIL,
  AST_QUOTE,
  AST_QUASIQUOTE,
  AST_UNQUOTE,
  AST_UNQUOTE_SPLICING,
  AST_ASSIGNMENT,
  AST_DEFINITION,
  AST_IF,
  AST_LAMBDA,
  AST_BEGIN,
  AST_COND,
  AST_APPLICATION,
  AST_PRIMITIVE,
  AST_MACRO,
  AST_PROMISE,
  AST_CONTINUATION,
  AST_CALLCC,
  AST_ERROR
} lizard_ast_node_type_t;

typedef struct lizard_ast_node lizard_ast_node_t;
typedef struct lizard_env lizard_env_t;
typedef struct lizard_heap lizard_heap_t;
typedef lizard_ast_node_t *(*lizard_primitive_func_t)(list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap);
typedef lizard_ast_node_t *(*lizard_callcc_func_t)(
    list_t *args, lizard_env_t *env, lizard_heap_t *heap,
    lizard_ast_node_t *(*current_cont)(lizard_ast_node_t *, lizard_env_t *,
                                       lizard_heap_t *));

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
    lizard_callcc_func_t callcc;
    struct {
      lizard_ast_node_t *variable;
      lizard_ast_node_t *transformer;
    } macro_def;
    struct {
      lizard_ast_node_t *(*captured_cont)(lizard_ast_node_t *result,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap);
    } continuation;
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

typedef struct lizard_heap_segment {
  char *start;
  char *top;
  char *end;
  struct lizard_heap_segment *next;
} lizard_heap_segment_t;

struct lizard_heap {
  lizard_heap_segment_t *head;
  lizard_heap_segment_t *current;
  size_t initial_size;
  size_t max_segment_size;
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

lizard_ast_node_t *make_continuation(
    lizard_ast_node_t *(*current_cont)(lizard_ast_node_t *, lizard_env_t *,
                                       lizard_heap_t *),
    lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_callcc(
    list_t *args, lizard_env_t *env, lizard_heap_t *heap,
    lizard_ast_node_t *(*current_cont)(lizard_ast_node_t *, lizard_env_t *,
                                       lizard_heap_t *));

lizard_ast_node_t *identity_cont(lizard_ast_node_t *result, lizard_env_t *env,
                                 lizard_heap_t *heap);
typedef lizard_ast_node_t *(*continuation_t)(lizard_ast_node_t *result,
                                             lizard_env_t *env,
                                             lizard_heap_t *heap);

void print_ast(lizard_ast_node_t *node, int depth);
lizard_ast_node_t *lizard_eval(lizard_ast_node_t *node, lizard_env_t *env,
                               lizard_heap_t *heap, continuation_t cont);

lizard_ast_node_t *lizard_apply(lizard_ast_node_t *func, list_t *args,
                                lizard_env_t *env, lizard_heap_t *heap,
                                lizard_ast_node_t *(*cont)(lizard_ast_node_t *,
                                                           lizard_env_t *,
                                                           lizard_heap_t *));
lizard_ast_node_t *lizard_expand_macros(lizard_ast_node_t *node,
                                        lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_expand_quasiquote(lizard_ast_node_t *node,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap);
#endif /* LIZARD_H */