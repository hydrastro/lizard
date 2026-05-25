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

<<<<<<< HEAD
=======
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

>>>>>>> refs/remotes/origin/master
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
<<<<<<< HEAD
  char c;
=======
  int c;
  char input_char;
  ssize_t bytes_read;
>>>>>>> refs/remotes/origin/master
  buffer = (char *)malloc(REPL_BUFFER_SIZE);
  if (buffer == NULL) {
    return NULL;
  }
  position = 0;
  length = 0;
  buffer[0] = '\0';
  if (!isatty(STDIN_FILENO)) {
<<<<<<< HEAD
    /* Non-interactive: read one line, no raw-mode editor, no echo. */
    size_t n;
    if (!fgets(buffer, REPL_BUFFER_SIZE, stdin)) {
      free(buffer);
      return NULL;
=======
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
>>>>>>> refs/remotes/origin/master
    }
    n = strlen(buffer);
    if (n > 0 && buffer[n - 1] == '\n') {
      buffer[n - 1] = '\0';
    }
    return buffer;
  }
  (void)!system("stty raw -echo");
  for (;;) {
    c = (char)getchar();
    if (c == 3 || c == 4) {
<<<<<<< HEAD
      (void)!system("stty cooked echo");
=======
      if (isatty(STDIN_FILENO)) {
        int stty_status;
        stty_status = system("stty cooked echo");
        (void)stty_status;
      }
>>>>>>> refs/remotes/origin/master
      if (c == 3) {
        printf("\n^C\n");
      } else {
        printf("\n^D\n");
      }
      free(buffer);
      exit(0);
    }
    if (c == '\033') {
<<<<<<< HEAD
      getchar();
      c = (char)getchar();
=======
      (void)getchar();
      c = getchar();
      if (c == EOF) {
        free(buffer);
        exit(0);
      }
>>>>>>> refs/remotes/origin/master
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
<<<<<<< HEAD
      (void)!system("stty cooked echo");
=======
      if (isatty(STDIN_FILENO)) {
        int stty_status;
        stty_status = system("stty cooked echo");
        (void)stty_status;
      }
>>>>>>> refs/remotes/origin/master
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
<<<<<<< HEAD
      buffer[position] = c;
=======
      buffer[position] = (char)c;
>>>>>>> refs/remotes/origin/master
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
    /* Use newline as the joiner so ; comments terminate at the
       end of their original source line, not the end of the whole
       multi-line expression. */
    strcat(buffer, "\n");
    strcat(buffer, next_line);
    free(next_line);
  }
  add_to_history(buffer);
  return buffer;
}

static void lizard_define_primitive(lizard_heap_t *heap, lizard_env_t *global_env,
                             const char *name, lizard_primitive_func_t func) {
  lizard_env_define(heap, global_env, name, lizard_make_primitive(heap, func));
}

static void define_primitives(lizard_heap_t *heap, lizard_env_t *global_env) {
  lizard_define_primitive(heap, global_env, "null?", lizard_primitive_nullp);
  lizard_define_primitive(heap, global_env, "pair?", lizard_primitive_pairp);
  lizard_define_primitive(heap, global_env, "string?",
                          lizard_primitive_stringp);
  lizard_define_primitive(heap, global_env, "bool?", lizard_primitive_boolp);
  lizard_define_primitive(heap, global_env, "+", lizard_primitive_plus);
  lizard_define_primitive(heap, global_env, "-", lizard_primitive_minus);
  lizard_define_primitive(heap, global_env, "*", lizard_primitive_multiply);
  lizard_define_primitive(heap, global_env, "/", lizard_primitive_divide);
  lizard_define_primitive(heap, global_env, "=", lizard_primitive_equal);
  lizard_define_primitive(heap, global_env, "^", lizard_primitive_pow);
  lizard_define_primitive(heap, global_env, "%", lizard_primitive_mod);
  lizard_define_primitive(heap, global_env, "<", lizard_primitive_lt);
  lizard_define_primitive(heap, global_env, "<=", lizard_primitive_le);
  lizard_define_primitive(heap, global_env, ">", lizard_primitive_gt);
  lizard_define_primitive(heap, global_env, ">=", lizard_primitive_ge);
  lizard_define_primitive(heap, global_env, "cons", lizard_primitive_cons);
  lizard_define_primitive(heap, global_env, "car", lizard_primitive_car);
  lizard_define_primitive(heap, global_env, "cdr", lizard_primitive_cdr);
  lizard_define_primitive(heap, global_env, "list", lizard_primitive_list);
  lizard_define_primitive(heap, global_env, "eval", lizard_primitive_eval);
  lizard_define_primitive(heap, global_env, "unquote",
                          lizard_primitive_unquote);
  lizard_define_primitive(heap, global_env, "tokens", lizard_primitive_tokens);
  lizard_define_primitive(heap, global_env, "ast", lizard_primitive_ast);
  lizard_define_primitive(heap, global_env, "and", lizard_primitive_and);
  lizard_define_primitive(heap, global_env, "or", lizard_primitive_or);
  lizard_define_primitive(heap, global_env, "not", lizard_primitive_not);
<<<<<<< HEAD
  lizard_define_primitive(heap, global_env, "display",
                          lizard_primitive_display);
  lizard_define_primitive(heap, global_env, "write", lizard_primitive_write);
  lizard_define_primitive(heap, global_env, "newline",
                          lizard_primitive_newline);
=======
  lizard_define_primitive(heap, global_env, "xor", lizard_primitive_xor);
  lizard_define_primitive(heap, global_env, "nand", lizard_primitive_nand);
  lizard_define_primitive(heap, global_env, "nor", lizard_primitive_nor);
  lizard_define_primitive(heap, global_env, "xnor", lizard_primitive_xnor);
>>>>>>> refs/remotes/origin/master
}

int main(void) {
  char *input;
  int i;
  ds_list_t *tokens;
  ds_list_t *ast_list;
  ds_list_node_t *node;
  lizard_ast_list_node_t *expr_node;
  lizard_ast_node_t *expanded_ast;
  lizard_ast_node_t *result;
  mp_set_memory_functions(lizard_heap_alloc, lizard_heap_realloc,
                          lizard_heap_free_wrapper);
  heap = lizard_heap_create(1024 * 1024, 16 * 1024 * 1024);
  {
    lizard_env_t *global_env;
    global_env = lizard_env_create(heap, NULL);

    define_primitives(heap, global_env);

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
<<<<<<< HEAD
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
=======
        /* lizard_force_all(result->data.application_arguments, heap); */
        /* result = lizard_force(result, heap); */
        printf("=> ");
        lizard_print_ast(result, 0);
>>>>>>> refs/remotes/origin/master
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
