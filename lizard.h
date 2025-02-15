#include <ctype.h>
#include <ds.h>
#include <gmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    const char *string;
    const char *symbol;
    mpz_t number;
  } data;
} lizard_token_t;

typedef struct lizard_token_list_node {
  list_node_t node;
  lizard_token_t token;
} lizard_token_list_node_t;

/* Define the error codes via an X‐macro list.
   (This list is independent of language.) */
#define ERROR_LIST                                                             \
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

/* Create the enum of errors */
typedef enum lizard_error_code {
#define X(err) err,
  ERROR_LIST
#undef X
      LIZARD_ERROR_COUNT /* Handy count of errors */
} lizard_error_code_t;

#define ERROR_MESSAGES_EN                                                      \
  X(LIZARD_ERROR_NONE, "Everything's good.")                                   \
  X(LIZARD_ERROR_UNBOUND_SYMBOL, "Error: unbound symbol.")                     \
  X(LIZARD_ERROR_INVALID_FUNCTION_DEF,                                         \
    "Error: invalid function definition syntax.")                              \
  X(LIZARD_ERROR_INVALID_FUNCTION_NAME,                                        \
    "Error: function name must be a symbol.")                                  \
  X(LIZARD_ERROR_INVALID_PARAMETER, "Error: parameter is not a symbol.")       \
  X(LIZARD_ERROR_INVALID_DEF, "Error: invalid definition syntax.")             \
  X(LIZARD_ERROR_ASSIGNMENT, "Error: set! expects a symbol as variable.")      \
  X(LIZARD_ERROR_ASSIGNMENT_UNBOUND, "Error: unbound symbol in set.")          \
  X(LIZARD_ERROR_INVALID_MACRO_NAME, "Error: macro name must be a symbol.")    \
  X(LIZARD_ERROR_NODE_TYPE, "Error: unknown AST node type in eval.")           \
  X(LIZARD_ERROR_LAMBDA_PARAMS,                                                \
    "Error: lambda parameters are not in application form.")                   \
  X(LIZARD_ERROR_VARIADIC_UNFOLLOWED,                                          \
    "Error: variadic marker must be followed by a parameter name.")            \
  X(LIZARD_ERROR_VARIADIC_SYMBOL,                                              \
    "Error: variadic parameter must be a symbol.")                             \
  X(LIZARD_ERROR_LAMBDA_ARITY_LESS,                                            \
    "Error: lambda arity mismatch (not enough arguments).")                    \
  X(LIZARD_ERROR_LAMBDA_PARAMETER, "Error: lambda parameter is not a symbol.") \
  X(LIZARD_ERROR_LAMBDA_ARITY_MORE,                                            \
    "Error: lambda arity mismatch (too many arguments).")                      \
  X(LIZARD_ERROR_INVALID_APPLY, "Error: Attempt to apply a non-function.")     \
  X(LIZARD_ERROR_PLUS, "Error: + expects numbers.")                            \
  X(LIZARD_ERROR_MINUS_ARGC, "Error: '-' expects at least one argument.")      \
  X(LIZARD_ERROR_MINUS_ARGT, "Error: '-' expects number arguments.")           \
  X(LIZARD_ERROR_MINUS_ARGT_2, "Error: '-' expects number arguments.")         \
  X(LIZARD_ERROR_MUL, "Error: '*' expects number arguments.")                  \
  X(LIZARD_ERROR_DIV_ARGC, "Error: '/' expects at least two arguments.")       \
  X(LIZARD_ERROR_DIV_ARGT, "Error: '/' expects number arguments.")             \
  X(LIZARD_ERROR_DIV_ARGT_2, "Error: '/' expects number arguments.")           \
  X(LIZARD_ERROR_DIV_ZERO, "Error: division by zero.")                         \
  X(LIZARD_ERROR_EQ_ARGC, "Error: '=' expects at least two arguments.")        \
  X(LIZARD_ERROR_CONS_ARGC, "Error: 'cons' expects exactly two arguments.")    \
  X(LIZARD_ERROR_CAR_ARGC, "Error: 'car' expects exactly one argument.")       \
  X(LIZARD_ERROR_CAR_ARGT, "Error: 'car' expects a list.")                     \
  X(LIZARD_ERROR_CAR_NIL, "Error: 'car' called on an empty list.")             \
  X(LIZARD_ERROR_CDR_ARGC, "Error: 'cdr' expects exactly one argument.")       \
  X(LIZARD_ERROR_CDR_ARGT, "Error: 'cdr' expects a list.")                     \
  X(LIZARD_ERROR_CDR_NIL, "Error: 'cdr' called on an empty list.")             \
  X(LIZARD_ERROR_TOKENS_ARGC,                                                  \
    "Error: 'print-tokens' expects exactly one argument.")                     \
  X(LIZARD_ERROR_TOKENS_ARGT,                                                  \
    "Error: 'print-tokens' expects a string argument.")                        \
  X(LIZARD_ERROR_EVAL_ARGC, "Error: eval expects exactly one argument.")

#define ERROR_MESSAGES                                                         \
  X(LIZARD_ERROR_MESSAGES_EN, lizard_error_messages_lang_en)

typedef enum { LIZARD_LANG_EN, LIZARD_LANG_COUNT } lizard_lang_t;

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

bool lizard_is_digit(const char *input, int i);
void lizard_add_token(list_t *list, lizard_token_type_t token_type, char *data);
list_t *lizard_tokenize(const char *input);
lizard_ast_node_t *lizard_parse_expression(list_t *token_list,
                                           list_node_t **current_node_pointer,
                                           int *depth, lizard_heap_t *);
list_t *lizard_parse(list_t *token_list, lizard_heap_t *);
void print_ast(lizard_ast_node_t *node, int depth);

lizard_heap_t *lizard_heap_create(size_t initial_size, size_t reserved_size);
void *lizard_heap_alloc(lizard_heap_t *heap, size_t size);

lizard_env_t *lizard_env_create(lizard_heap_t *heap, lizard_env_t *parent);
void lizard_env_define(lizard_heap_t *heap, lizard_env_t *env,
                       const char *symbol, lizard_ast_node_t *value);
lizard_ast_node_t *lizard_env_lookup(lizard_env_t *env, const char *symbol);
int lizard_env_set(lizard_env_t *env, const char *symbol,
                   lizard_ast_node_t *value);

lizard_ast_node_t *lizard_eval(lizard_ast_node_t *node, lizard_env_t *env,
                               lizard_heap_t *heap);

lizard_ast_node_t *lizard_apply(lizard_ast_node_t *func, list_t *args,
                                lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_plus(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_minus(list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_multiply(list_t *args, lizard_env_t *env,
                                             lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_divide(list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_equal(list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);

lizard_ast_node_t *lizard_primitive_cons(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_car(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_cdr(list_t *args, lizard_env_t *env,
                                        lizard_heap_t *heap);

lizard_ast_node_t *lizard_make_primitive(lizard_heap_t *heap,
                                         lizard_primitive_func_t func);

lizard_ast_node_t *lizard_make_bool(lizard_heap_t *heap, bool value);

lizard_ast_node_t *lizard_make_nil(lizard_heap_t *heap);

lizard_ast_node_t *lizard_make_macro_def(lizard_heap_t *heap,
                                         lizard_ast_node_t *name,
                                         lizard_ast_node_t *transformer);

lizard_ast_node_t *lizard_expand_macros(lizard_ast_node_t *node,
                                        lizard_env_t *env, lizard_heap_t *heap);

lizard_ast_node_t *lizard_primitive_list(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);

lizard_ast_node_t *lizard_primitive_tokens(list_t *args, lizard_env_t *env,
                                           lizard_heap_t *heap);

lizard_ast_node_t *lizard_primitive_eval(list_t *args, lizard_env_t *env,
                                         lizard_heap_t *heap);

lizard_ast_node_t *lizard_make_error(lizard_heap_t *heap, int error_code);
