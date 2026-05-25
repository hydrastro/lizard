#include "env.h"
#include "lang.h"
#include "lizard.h"
#include "mem.h"
#include "parser.h"
#include "primitives.h"
#include "printer.h"
#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define REPL_BUFFER_SIZE 1024
#define HISTORY_SIZE 100
#define KEY_UP 'A'
#define KEY_DOWN 'B'

static char *history[HISTORY_SIZE] = {0};
static int history_count = 0;
static int history_index = 0;
lizard_heap_t *heap;

static char *lizard_repl_strdup(const char *s) {
  size_t n;
  char *copy;
  n = strlen(s) + 1U;
  copy = (char *)malloc(n);
  if (copy != NULL) {
    memcpy(copy, s, n);
  }
  return copy;
}

static void add_to_history(const char *line) {
  if (line[0] == '\0') {
    return;
  }
  if (history_count > 0 && strcmp(history[history_count - 1], line) == 0) {
    return;
  }
  free(history[history_count % HISTORY_SIZE]);
  history[history_count % HISTORY_SIZE] = lizard_repl_strdup(line);
  history_count++;
  history_index = history_count;
}

static char *read_line(void) {
  char *buffer;
  int position, length;
  int c;
  char input_char;
  ssize_t bytes_read;
  buffer = (char *)malloc(REPL_BUFFER_SIZE);
  if (buffer == NULL) {
    return NULL;
  }
  position = 0;
  length = 0;
  buffer[0] = '\0';
  if (!isatty(STDIN_FILENO)) {
    if (fgets(buffer, REPL_BUFFER_SIZE, stdin) == NULL) {
      free(buffer);
      return NULL;
    }
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return buffer;
  }
  if (isatty(STDIN_FILENO)) {
    int stty_status;
    stty_status = system("stty raw -echo");
    (void)stty_status;
  }
  for (;;) {
    if (isatty(STDIN_FILENO)) {
      c = getchar();
      if (c == EOF) {
        free(buffer);
        exit(0);
      }
    } else {
      bytes_read = read(STDIN_FILENO, &input_char, 1);
      if (bytes_read == 0) {
        printf("EOF\n");
        exit(0);
      } else if (bytes_read < 0) {
        perror("read error");
        exit(1);
      }
      c = (unsigned char)input_char;
    }
    if (c == 3 || c == 4) {
      if (isatty(STDIN_FILENO)) {
        int stty_status;
        stty_status = system("stty cooked echo");
        (void)stty_status;
      }
      if (c == 3) {
        printf("\n^C\n");
      } else {
        printf("\n^D\n");
      }
      free(buffer);
      exit(0);
    }
    if (c == '\033') {
      (void)getchar();
      c = getchar();
      if (c == EOF) {
        free(buffer);
        exit(0);
      }
      if (c == KEY_UP && history_count > 0) {
        if (history_index > 0) {
          history_index--;
          while (position > 0) {
            printf("\b \b");
            position--;
          }
          strcpy(buffer, history[history_index % HISTORY_SIZE]);
          printf("%s", buffer);
          position = (int)strlen(buffer);
          length = position;
        }
        continue;
      } else if (c == KEY_DOWN && history_count > 0) {
        if (history_index < history_count) {
          history_index++;
          while (position > 0) {
            printf("\b \b");
            position--;
          }
          if (history_index == history_count) {
            buffer[0] = '\0';
          } else {
            strcpy(buffer, history[history_index % HISTORY_SIZE]);
          }
          printf("%s", buffer);
          position = (int)strlen(buffer);
          length = position;
        }
        continue;
      } else if (c == 'D') {
        if (position > 0) {
          position--;
          printf("\b");
        }
        continue;
      } else if (c == 'C') {
        if (position < length) {
          printf("%c", buffer[position]);
          position++;
        }
        continue;
      }
    }
    if (c == '\r' || c == '\n') {
      if (isatty(STDIN_FILENO)) {
        int stty_status;
        stty_status = system("stty cooked echo");
        (void)stty_status;
      }
      printf("\n");
      buffer[length] = '\0';
      return buffer;
    } else if (c == 127 || c == '\b') {
      if (position > 0) {
        position--;
        length--;
        printf("\b \b");
        memmove(&buffer[position], &buffer[position + 1], (size_t)(length - position));
      }
    } else if (position < REPL_BUFFER_SIZE - 1) {
      memmove(&buffer[position + 1], &buffer[position], (size_t)(length - position));
      buffer[position] = (char)c;
      position++;
      length++;
      buffer[length] = '\0';
      printf("\033[K");
      printf("%s", &buffer[position - 1]);
      {
        int i;
        for (i = position; i < length; i++) {
          printf("\b");
        }
      }
    }
  }
}

static int paren_balance(const char *s) {
  int balance;
  balance = 0;
  for (; *s; s++) {
    if (*s == '(') {
      balance++;
    } else if (*s == ')') {
      balance--;
    }
  }
  return balance;
}

static char *read_input(void) {
  size_t total_size;
  char *buffer;
  char *line;
  int tty;
  tty = isatty(STDIN_FILENO);
  total_size = REPL_BUFFER_SIZE;
  buffer = (char *)malloc(total_size);
  buffer[0] = '\0';
  line = read_line();
  if (!line) {
    free(buffer);
    return NULL;
  }
  strcpy(buffer, line);
  free(line);
  while (paren_balance(buffer) > 0) {
    char *next_line;
    size_t needed;
    int pb;
    if (tty) {
      for (pb = paren_balance(buffer); pb > 0; pb--) {
        printf(".");
      }
      printf("> ");
      fflush(stdout);
    }
    next_line = read_line();
    if (!next_line) {
      if (tty) {
        printf("Incomplete expression discarded.\n");
      }
      free(buffer);
      return lizard_repl_strdup("");
    }
    needed = strlen(buffer) + strlen(next_line) + 2;
    if (needed > total_size) {
      total_size = needed;
      buffer = (char *)realloc(buffer, total_size);
    }
    /* Join with newline so ; comments terminate at the original
       line boundary instead of swallowing the rest of the form. */
    strcat(buffer, "\n");
    strcat(buffer, next_line);
    free(next_line);
  }
  add_to_history(buffer);
  return buffer;
}

/* primitive registration is now provided by liblizard so tests can use it */

int main(void) {
  char *input;
  int i;
  lz_list_t *tokens;
  lz_list_t *ast_list;
  lz_list_node_t *node;
  lizard_ast_list_node_t *expr_node;
  lizard_ast_node_t *expanded_ast;
  lizard_ast_node_t *result;
  mp_set_memory_functions(lizard_heap_alloc, lizard_heap_realloc,
                          lizard_heap_free_wrapper);
  heap = lizard_heap_create(1024 * 1024, 16 * 1024 * 1024);
  {
    lizard_env_t *global_env;
    global_env = lizard_env_create(heap, NULL);

    lizard_install_primitives(heap, global_env);

    while (1) {
      if (isatty(STDIN_FILENO)) {
        printf("lizard> ");
        fflush(stdout);
      }
      input = read_input();
      if (input == NULL) {
        break;
      }
      if (strcmp(input, "quit") == 0 || feof(stdin)) {
        free(input);
        break;
      }
      if (input[0] == '\0') {
        free(input);
        continue;
      }
      tokens = lizard_tokenize(input);
      ast_list = lizard_parse(tokens, heap);
      node = ast_list->head;
      while (node != ast_list->nil) {
        expr_node = (lizard_ast_list_node_t *)node;
        expanded_ast = lizard_expand_macros(expr_node->ast, global_env, heap);
        result =
            lizard_eval(expanded_ast, global_env, heap, lizard_identity_cont);
        if (isatty(STDIN_FILENO)) {
          /* Interactive REPL: echo every result. */
          printf("=> ");
          lizard_print_value(result);
          printf("\n");
        } else if (result && result->type == AST_ERROR) {
          /* Script mode: stay silent except for errors. */
          lizard_print_value(result);
          printf("\n");
        }
        node = node->next;
      }
      free(input);
    }
  }
  for (i = 0; i < HISTORY_SIZE; i++) {
    free(history[i]);
  }
  lizard_heap_destroy(heap);
  return 0;
}
