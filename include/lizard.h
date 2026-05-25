#ifndef LIZARD_H
#define LIZARD_H

#include <ds.h>
#include <gmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

/* lizard uses its own typedefs anchored on the struct *tags*
   `linked_list` and `list_node` rather than ds's typedef names.
   Both the old and the new ds expose these structs under the same
   tags (ds typedef names: `list_t` in older releases, `ds_list_t`
   in newer ones), so this compiles regardless of which ds version
   is installed and interoperates with either set of ds function
   signatures. */
struct linked_list;
struct list_node;
typedef struct linked_list lz_list_t;
typedef struct list_node lz_list_node_t;

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
  AST_PAIR,
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

typedef lizard_ast_node_t *(*lizard_primitive_func_t)(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap);
typedef lizard_ast_node_t *(*lizard_callcc_func_t)(
    lz_list_t *args, lizard_env_t *env, lizard_heap_t *heap,
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
      lz_list_t *parameters;
      lizard_env_t *closure_env;
    } lambda;
    lz_list_t *begin_expressions;
    lz_list_t *cond_clauses;
    lz_list_t *application_arguments;
    struct {
      lizard_ast_node_t *car;
      lizard_ast_node_t *cdr;
    } pair;
    lizard_primitive_func_t primitive;
    struct {
      lizard_ast_node_t *expr;
      lizard_env_t *env;
      lizard_ast_node_t *value;
      bool forced;
    } promise;
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
      lz_list_t *data;
    } error;
  } data;
};

typedef struct lizard_ast_list_node {
  lz_list_node_t node;
  lizard_ast_node_t *ast;
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

typedef lizard_ast_node_t *(*lizard_continuation_t)(lizard_ast_node_t *result,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap);

lizard_ast_node_t *lizard_convert_list_literal(lizard_ast_node_t *node,
                                               lizard_heap_t *heap);
lizard_ast_node_t *lizard_make_promise(lizard_heap_t *heap,
                                       lizard_ast_node_t *expr,
                                       lizard_env_t *env);
lizard_ast_node_t *lizard_force(lizard_ast_node_t *node, lizard_heap_t *heap);

lizard_ast_node_t *lizard_primitive_delay(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_force(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);

lizard_ast_node_t *lizard_eval(lizard_ast_node_t *node, lizard_env_t *env,
                               lizard_heap_t *heap, lizard_continuation_t cont);
lizard_ast_node_t *lizard_apply(lizard_ast_node_t *func, lz_list_t *args,
                                lizard_env_t *env, lizard_heap_t *heap,
                                lizard_continuation_t cont);
lizard_ast_node_t *lizard_expand_macros(lizard_ast_node_t *node,
                                        lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_expand_quasiquote(lizard_ast_node_t *node,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap);

#endif /* LIZARD_H */
