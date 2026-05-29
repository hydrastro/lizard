#ifndef LIZARD_TOKENIZER_H
#define LIZARD_TOKENIZER_H

#include "lizard_api.h"
#include "lizard_internal.h"

typedef enum {
  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,
  TOKEN_SYMBOL,
  TOKEN_NUMBER,
  TOKEN_STRING
} lizard_token_type_t;

typedef struct lizard_token {
  lizard_token_type_t type;
  int line;
  int column;
  int offset;
  union {
    char *string;
    char *symbol;
    mpz_t number;
  } data;
} lizard_token_t;

typedef struct lizard_token_list_node {
  lz_list_node_t node;
  lizard_token_t token;
} lizard_token_list_node_t;

bool lizard_is_digit(const char *input, int i);
void lizard_source_position(const char *input, int offset, int *line, int *column);
void lizard_add_token(lz_list_t *list, lizard_token_type_t token_type,
                      char *data, int line, int column, int offset);
lz_list_t *lizard_tokenize(const char *input);
const lizard_diagnostic_t *lizard_tokenizer_last_diagnostic(void);
void lizard_free_tokens(lz_list_t *token_list);
lz_list_t *lizard_tokenize_source(const char *source,
                                  const char *filename,
                                  lizard_diagnostic_t *diagnostic);

#endif /* LIZARD_TOKENIZER_H */
