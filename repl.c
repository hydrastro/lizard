#include "env.h"
#include "lang.h"
#include "lizard.h"
#include "mem.h"
#include "parser.h"
#include "primitives.h"
#include "tokenizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REPL_BUFFER_SIZE 1024

#define HISTORY_SIZE 100
#define KEY_UP 'A'
#define KEY_DOWN 'B'

char *history[HISTORY_SIZE] = {NULL};
int history_count = 0;
int history_index = 0;

lizard_heap_t *heap;

void add_to_history(const char *line) {
  if (line[0] == '\0')
    return;

  if (history_count > 0 && strcmp(history[history_count - 1], line) == 0) {
    return;
  }

  free(history[history_count % HISTORY_SIZE]);
  history[history_count % HISTORY_SIZE] = strdup(line);
  history_count++;
  history_index = history_count;
}

char *read_line(void) {
  char *buffer = malloc(REPL_BUFFER_SIZE);
  int position = 0;
  int length = 0;
  buffer[0] = '\0';

  system("stty raw -echo");
  while (1) {
    char c = getchar();

    if (c == 3 || c == 4) {
      system("stty cooked echo");
      if (c == 3) {
        printf("\n^C\n");
      } else if (c == 4) {
        printf("\n^D\n");
      }
      free(buffer);
      exit(0);
    }

    if (c == '\033') {
      getchar();
      c = getchar();

      if (c == KEY_UP && history_count > 0) {
        if (history_index > 0) {
          history_index--;
          while (position > 0) {
            printf("\b \b");
            position--;
          }
          strcpy(buffer, history[history_index % HISTORY_SIZE]);
          printf("%s", buffer);
          position = strlen(buffer);
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
          position = strlen(buffer);
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
      system("stty cooked echo");
      printf("\n");
      buffer[length] = '\0';
      add_to_history(buffer);
      return buffer;
    } else if (c == 127 || c == '\b') {
      if (position > 0) {
        position--;
        length--;
        printf("\b \b");
        memmove(&buffer[position], &buffer[position + 1], length - position);
      }
    } else if (position < REPL_BUFFER_SIZE - 1) {
      memmove(&buffer[position + 1], &buffer[position], length - position);
      buffer[position++] = c;
      buffer[++length] = '\0';
      printf("\033[K");
      printf("%s", &buffer[position - 1]);
      for (int i = position; i < length; i++) {
        printf("\b");
      }
    }
  }
}

int main(void) {
  mp_set_memory_functions(lizard_heap_alloc, NULL, lizard_heap_free_wrapper);
  heap = lizard_heap_create(1024 * 1024, 16 * 1024 * 1024);
  lizard_env_t *global_env = lizard_env_create(heap, NULL);

  lizard_env_define(heap, global_env, "+",
                    lizard_make_primitive(heap, lizard_primitive_plus));
  lizard_env_define(heap, global_env, "-",
                    lizard_make_primitive(heap, lizard_primitive_minus));
  lizard_env_define(heap, global_env, "*",
                    lizard_make_primitive(heap, lizard_primitive_multiply));
  lizard_env_define(heap, global_env, "/",
                    lizard_make_primitive(heap, lizard_primitive_divide));
  lizard_env_define(heap, global_env, "=",
                    lizard_make_primitive(heap, lizard_primitive_equal));
  lizard_env_define(heap, global_env, "cons",
                    lizard_make_primitive(heap, lizard_primitive_cons));
  lizard_env_define(heap, global_env, "car",
                    lizard_make_primitive(heap, lizard_primitive_car));
  lizard_env_define(heap, global_env, "cdr",
                    lizard_make_primitive(heap, lizard_primitive_cdr));
  lizard_env_define(heap, global_env, "list",
                    lizard_make_primitive(heap, lizard_primitive_list));
  lizard_env_define(heap, global_env, "tokens",
                    lizard_make_primitive(heap, lizard_primitive_tokens));
  lizard_env_define(heap, global_env, "eval",
                    lizard_make_primitive(heap, lizard_primitive_eval));
  lizard_env_define(heap, global_env, "unquote",
                    lizard_make_primitive(heap, lizard_primitive_unquote));

  while (1) {
    printf("lizard> ");
    fflush(stdout);
    char *input = read_line();
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

    list_t *tokens = lizard_tokenize(input);
    list_t *ast_list = lizard_parse(tokens, heap);
    for (list_node_t *node = ast_list->head; node != ast_list->nil;
         node = node->next) {
      lizard_ast_list_node_t *expr_node = (lizard_ast_list_node_t *)node;
      lizard_ast_node_t *result =
          lizard_eval(&expr_node->ast, global_env, heap);
      printf("=> ");
      print_ast(result, 0);
    }

    /*lizard_free_tokens(tokens);*/
    free(input);
  }

  for (int i = 0; i < HISTORY_SIZE; i++) {
    free(history[i]);
  }
  lizard_heap_destroy(heap);

  return 0;
}
