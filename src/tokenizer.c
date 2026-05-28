#include "tokenizer.h"
#include "lizard_internal.h"
#include "mem.h"

static lizard_diagnostic_t lizard_tokenizer_diag;
static int lizard_tokenizer_failed = 0;

static void lizard_tokenizer_clear_diagnostic(void) {
  lizard_tokenizer_failed = 0;
  lizard_tokenizer_diag.status = LIZARD_STATUS_OK;
  lizard_tokenizer_diag.span.filename = NULL;
  lizard_tokenizer_diag.span.start_line = 0;
  lizard_tokenizer_diag.span.start_column = 0;
  lizard_tokenizer_diag.span.end_line = 0;
  lizard_tokenizer_diag.span.end_column = 0;
  lizard_tokenizer_diag.span.start_offset = 0;
  lizard_tokenizer_diag.span.end_offset = 0;
  lizard_tokenizer_diag.message[0] = '\0';
}

static void lizard_tokenizer_set_diagnostic(lizard_diagnostic_t *diagnostic,
                                            const char *filename,
                                            int line, int column, int offset,
                                            const char *message) {
  if (diagnostic == NULL) {
    return;
  }
  diagnostic->status = LIZARD_STATUS_PARSE_ERROR;
  diagnostic->span.filename = filename;
  diagnostic->span.start_line = line;
  diagnostic->span.start_column = column;
  diagnostic->span.end_line = line;
  diagnostic->span.end_column = column;
  diagnostic->span.start_offset = offset;
  diagnostic->span.end_offset = offset;
  if (message == NULL) {
    message = "";
  }
  strncpy(diagnostic->message, message, sizeof(diagnostic->message) - 1U);
  diagnostic->message[sizeof(diagnostic->message) - 1U] = '\0';
}

const lizard_diagnostic_t *lizard_tokenizer_last_diagnostic(void) {
  return lizard_tokenizer_failed ? &lizard_tokenizer_diag : NULL;
}

static const char *lizard_copy_filename(const char *filename) {
  char *copy;
  size_t len;
  if (filename == NULL) {
    return NULL;
  }
  if (strcmp(filename, "<string>") == 0) {
    return "<string>";
  }
  len = strlen(filename) + 1U;
  copy = (char *)lizard_heap_alloc(len);
  memcpy(copy, filename, len);
  return copy;
}

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

void lizard_source_position(const char *input, int offset, int *line,
                            int *column) {
  int i;
  int l;
  int c;
  l = 1;
  c = 1;
  for (i = 0; input != NULL && input[i] != '\0' && i < offset; i++) {
    if (input[i] == '\n') {
      l++;
      c = 1;
    } else {
      c++;
    }
  }
  if (line != NULL) {
    *line = l;
  }
  if (column != NULL) {
    *column = c;
  }
}

void lizard_add_token_source(lz_list_t *list, lizard_token_type_t token_type,
                             char *data, const char *filename, int line,
                             int column, int offset) {

  lizard_token_list_node_t *node;
  node = lizard_heap_alloc(sizeof(lizard_token_list_node_t));
  node->token.type = token_type;
  node->token.filename = filename;
  node->token.line = line;
  node->token.column = column;
  node->token.offset = offset;
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
    lizard_heap_free(data);
    break;
  case TOKEN_STRING:
    node->token.data.string = data;
    break;
  }
  list_append(list, &node->node);
}

void lizard_add_token(lz_list_t *list, lizard_token_type_t token_type,
                      char *data, int line, int column, int offset) {
  lizard_add_token_source(list, token_type, data, NULL, line, column, offset);
}

static void lizard_add_token_at(lz_list_t *list, const char *input,
                                const char *filename, int offset,
                                lizard_token_type_t token_type, char *data) {
  int line;
  int column;
  lizard_source_position(input, offset, &line, &column);
  lizard_add_token_source(list, token_type, data, filename, line, column,
                          offset);
}

lz_list_t *lizard_tokenize_source(const char *input, const char *filename,
                                  lizard_diagnostic_t *diagnostic) {
  lz_list_t *list;
  int i, j, k;
  int token_start;
  int line;
  int column;
  const char *owned_filename;
  char *buffer;

  lizard_tokenizer_clear_diagnostic();
  if (diagnostic != NULL) {
    diagnostic->status = LIZARD_STATUS_OK;
    diagnostic->span.filename = NULL;
    diagnostic->span.start_line = 0;
    diagnostic->span.start_column = 0;
    diagnostic->span.end_line = 0;
    diagnostic->span.end_column = 0;
    diagnostic->span.start_offset = 0;
    diagnostic->span.end_offset = 0;
    diagnostic->message[0] = '\0';
  }
  if (input == NULL) {
    lizard_tokenizer_failed = 1;
    lizard_tokenizer_set_diagnostic(&lizard_tokenizer_diag, filename, 0, 0, 0,
                                    "null input");
    if (diagnostic != NULL) {
      *diagnostic = lizard_tokenizer_diag;
    }
    return NULL;
  }

  owned_filename = lizard_copy_filename(filename);
  list = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  i = 0;
  while (input[i] != '\0') {
    if (input[i] == ' ' || input[i] == '\t' || input[i] == '\n' ||
        input[i] == '\r') {
      i++;
      continue;
    }
    if (input[i] == ';') {
      /* Scheme line comment */
      while (input[i] != '\0' && input[i] != '\n') {
        i++;
      }
      continue;
    }
    if (input[i] == '(') {
      lizard_add_token_at(list, input, owned_filename, i, TOKEN_LEFT_PAREN,
                          NULL);
      i++;
    } else if (input[i] == ')') {
      lizard_add_token_at(list, input, owned_filename, i, TOKEN_RIGHT_PAREN,
                          NULL);
      i++;
    } else if (input[i] == '"') {
      token_start = i;
      j = i;
      i++;
      while (input[i] != '"' && input[i] != '\0') {
        /* Skip over an escaped character so \" doesn't close the
           string. The actual escape decoding happens during copy. */
        if (input[i] == '\\' && input[i + 1] != '\0') {
          i += 2;
        } else {
          i++;
        }
      }
      if (input[i] == '\0') {
        lizard_source_position(input, token_start, &line, &column);
        lizard_tokenizer_failed = 1;
        lizard_tokenizer_set_diagnostic(&lizard_tokenizer_diag,
                                        owned_filename, line, column,
                                        token_start, "unterminated string");
        if (diagnostic != NULL) {
          *diagnostic = lizard_tokenizer_diag;
        }
        return NULL;
      }
      /* Decoded length is at most (i - j - 1); we'll \0-terminate. */
      buffer = lizard_heap_alloc(sizeof(char) * (long unsigned int)(i - j));
      k = 0;
      j++; /* step over the opening " */
      while (j < i) {
        char c = input[j];
        if (c == '\\' && j + 1 < i) {
          char esc = input[j + 1];
          switch (esc) {
          case 'n':  buffer[k++] = '\n'; break;
          case 't':  buffer[k++] = '\t'; break;
          case 'r':  buffer[k++] = '\r'; break;
          case '\\': buffer[k++] = '\\'; break;
          case '"':  buffer[k++] = '"';  break;
          case '0':  buffer[k++] = '\0'; break;
          default:
            /* unknown escape — keep the literal character */
            buffer[k++] = esc;
            break;
          }
          j += 2;
        } else {
          buffer[k++] = c;
          j++;
        }
      }
      buffer[k] = '\0';
      lizard_add_token_at(list, input, owned_filename, token_start,
                          TOKEN_STRING, buffer);
      i++;

    } else if (lizard_is_digit(input, i)) {
      token_start = i;
      j = i;
      while (lizard_is_digit(input, i)) {
        i++;
      }
      buffer = lizard_heap_alloc(sizeof(char) * (long unsigned int)(i - j + 1));
      for (k = 0; j < i; j++, k++) {
        buffer[k] = input[j];
      }
      buffer[k] = '\0';
      lizard_add_token_at(list, input, owned_filename, token_start,
                          TOKEN_NUMBER, buffer);

    } else {
      token_start = i;
      j = i;
      if (input[i] == '\'' || input[i] == '`' || input[i] == ',' ||
          input[i] == '!' || input[i] == '@' || input[i] == '#' ||
          input[i] == '$' || input[i] == '%' || input[i] == '^' ||
          input[i] == '&' || input[i] == '[' || input[i] == ']' ||
          (input[i] == ',' && input[i + 1] == '@')) {
        if (input[i] == ',' && input[i + 1] == '@') {
          buffer = lizard_heap_alloc(sizeof(char) * 3);
          buffer[0] = input[i];
          buffer[1] = input[i + 1];
          buffer[2] = '\0';
          lizard_add_token_at(list, input, owned_filename, token_start,
                              TOKEN_SYMBOL, buffer);
          i += 2;
        } else {

          buffer = lizard_heap_alloc(sizeof(char) * 2);
          buffer[0] = input[i];
          buffer[1] = '\0';
          lizard_add_token_at(list, input, owned_filename, token_start,
                              TOKEN_SYMBOL, buffer);
          i++;
        }
      } else {
        while (input[i] != ' ' && input[i] != '\t' && input[i] != '\n' &&
               input[i] != '\r' && input[i] != '\0' && input[i] != '(' &&
               input[i] != ')' && input[i] != '\'' && input[i] != '`' &&
               input[i] != ',' && input[i] != ';') {
          i++;
        }
        buffer =
            lizard_heap_alloc(sizeof(char) * (long unsigned int)(i - j + 1));
        for (k = 0; j < i; j++, k++) {
          buffer[k] = input[j];
        }
        buffer[k] = '\0';
        lizard_add_token_at(list, input, owned_filename, token_start,
                            TOKEN_SYMBOL, buffer);
      }
    }
  }

  return list;
}

lz_list_t *lizard_tokenize(const char *input) {
  return lizard_tokenize_source(input, NULL, NULL);
}

static void lizard_destroy_token(lz_list_node_t *node) {
  lizard_token_list_node_t *token_node = CAST(node, lizard_token_list_node_t);
  lizard_token_t *token = &token_node->token;

  switch (token->type) {
  case TOKEN_SYMBOL:
  case TOKEN_STRING:
    lizard_heap_free(token->data.string);
    break;
  case TOKEN_NUMBER:
    mpz_clear(token->data.number);
    break;
  default:
    break;
  }

  lizard_heap_free(token_node);
}

void lizard_free_tokens(lz_list_t *token_list) {
  list_destroy(token_list, lizard_destroy_token);
}
