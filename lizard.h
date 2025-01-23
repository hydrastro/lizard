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
    char *string;
    char *symbol;
    mpz_t number;
  } data;
} lizard_token_t;

typedef struct lizard_token_list_node {
  list_node_t node;
  lizard_token_t token;
} lizard_token_list_node_t;

typedef enum {
  AST_STRING,
  AST_NUMBER,
  AST_SYMBOL,
  AST_QUOTED,
  AST_ASSIGNMENT,
  AST_DEFINITION,
  AST_IF,
  AST_LAMBDA,
  AST_BEGIN,
  AST_COND,
  AST_APPLICATION
} lizard_ast_node_type_t;

typedef struct lizard_ast_node {
  lizard_ast_node_type_t type;
  union {
    char *string;
    mpz_t number;
    char *variable;
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
    list_t *lambda_parameters;
    list_t *begin_expressions;
    list_t *cond_clauses;
    list_t *application_arguments;
  } data;
} lizard_ast_node_t;

typedef struct lizard_ast_list_node {
  list_node_t node;
  lizard_ast_node_t ast;
} lizard_ast_list_node_t;

bool lizard_is_digit(char *input, int i);
void lizard_add_token(list_t *list, lizard_token_type_t token_type, char *data);
list_t *lizard_tokenize(char *input);
lizard_ast_node_t *lizard_parse_expression(list_t *token_list,
                                           list_node_t **current_node_pointer,
                                           int *depth);
list_t *lizard_parse(list_t *token_list);
void print_ast(lizard_ast_node_t *node, int depth);
