#ifndef LIZARD_TOKENIZER_H
#define LIZARD_TOKENIZER_H

#include "lizard.h"

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

bool lizard_is_digit(const char *input, int i);
void lizard_add_token(list_t *list, lizard_token_type_t token_type, char *data);
list_t *lizard_tokenize(const char *input);
void lizard_free_tokens(list_t *token_list);

#endif /* LIZARD_TOKENIZER_H */