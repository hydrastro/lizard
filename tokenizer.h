#ifndef LIZARD_TOKENIZER_H
#define LIZARD_TOKENIZER_H

#include "lizard.h"

bool lizard_is_digit(const char *input, int i);
void lizard_add_token(list_t *list, lizard_token_type_t token_type, char *data);
list_t *lizard_tokenize(const char *input);
void lizard_free_tokens(list_t *token_list);

#endif /* LIZARD_TOKENIZER_H */