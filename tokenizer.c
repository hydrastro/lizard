#include "tokenizer.h"
#include "lizard.h"

bool lizard_is_digit(const char *input, int i) {
  if (input[i] >= '0' && input[i] <= '9') {
    return true;
  }
  if ((input[i] == '-' || input[i] == '+') && input[i + 1] >= '0' &&
      input[i + 1] <= '9') {
    return true;
  }
  return false;
}

void lizard_add_token(list_t *list, lizard_token_type_t token_type,
                      char *data) {

  lizard_token_list_node_t *node;
  node = malloc(sizeof(lizard_token_list_node_t));
  node->token.type = token_type;
  switch (token_type) {
  case TOKEN_LEFT_PAREN:
  case TOKEN_RIGHT_PAREN:
    node->token.data.string = NULL;
    break;
  case TOKEN_SYMBOL:
    node->token.data.symbol = data;
    break;
  case TOKEN_NUMBER:
    mpz_init(node->token.data.number);
    mpz_set_str(node->token.data.number, data, 10);
    free(data);
    break;
  case TOKEN_STRING:
    node->token.data.string = data;
    break;
  }
  list_append(list, &node->node);
}

list_t *lizard_tokenize(const char *input) {
  list_t *list = list_create();
  int i, j, k;
  char *buffer;
  i = 0;
  while (input[i] != '\0') {
    if (input[i] == ' ') {
      i++;
      continue;
    }
    if (input[i] == '(') {
      lizard_add_token(list, TOKEN_LEFT_PAREN, NULL);
      i++;
    } else if (input[i] == ')') {
      lizard_add_token(list, TOKEN_RIGHT_PAREN, NULL);
      i++;
    } else if (input[i] == '"') {
      j = i;
      i++;
      while (input[i] != '"' && input[i] != '\0') {
        i++;
      }
      if (input[i] == '\0') {
        fprintf(stderr, "Error: unterminated string.\n");
        exit(1);
      }
      buffer = malloc(sizeof(char) * (long unsigned int)(i - j));
      for (k = 0, j++; j < i; j++, k++) {
        buffer[k] = input[j];
      }
      buffer[k] = '\0';
      lizard_add_token(list, TOKEN_STRING, buffer);
      i++;

    } else if (lizard_is_digit(input, i)) {
      j = i;
      while (lizard_is_digit(input, i)) {
        i++;
      }
      buffer = malloc(sizeof(char) * (long unsigned int)(i - j + 1));
      for (k = 0; j < i; j++, k++) {
        buffer[k] = input[j];
      }
      buffer[k] = '\0';
      lizard_add_token(list, TOKEN_NUMBER, buffer);

    } else {
      j = i;
      if (input[i] == '\'' || input[i] == '`' || input[i] == ',' ||
          input[i] == '!' || input[i] == '@' || input[i] == '#' ||
          input[i] == '$' || input[i] == '%' || input[i] == '^' ||
          input[i] == '&' || input[i] == '[' || input[i] == ']' ||
          (input[i] == ',' && input[i + 1] == '@')) {
        if (input[i] == ',' && input[i + 1] == '@') {
          buffer = malloc(sizeof(char) * 3);
          buffer[0] = input[i];
          buffer[1] = input[i + 1];
          buffer[2] = '\0';
          lizard_add_token(list, TOKEN_SYMBOL, buffer);
          i += 2;
        } else {

          buffer = malloc(sizeof(char) * 2);
          buffer[0] = input[i];
          buffer[1] = '\0';
          lizard_add_token(list, TOKEN_SYMBOL, buffer);
          i++;
        }
      } else {
        while (input[i] != ' ' && input[i] != '\0' && input[i] != '(' &&
               input[i] != ')' && input[i] != '\'' && input[i] != '`' &&
               input[i] != ',') {
          i++;
        }
        buffer = malloc(sizeof(char) * (long unsigned int)(i - j + 1));
        for (k = 0; j < i; j++, k++) {
          buffer[k] = input[j];
        }
        buffer[k] = '\0';
        lizard_add_token(list, TOKEN_SYMBOL, buffer);
      }
    }
  }

  return list;
}

void lizard_destroy_token(list_node_t *node) {
  lizard_token_list_node_t *token_node = CAST(node, lizard_token_list_node_t);
  lizard_token_t *token = &token_node->token;

  switch (token->type) {
  case TOKEN_SYMBOL:
  case TOKEN_STRING:
    free(token->data.string);
    break;
  case TOKEN_NUMBER:
    mpz_clear(token->data.number);
    break;
  default:
    break;
  }

  free(token_node);
}

void lizard_free_tokens(list_t *token_list) {
  list_destroy(token_list, lizard_destroy_token);
}
