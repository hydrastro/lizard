#include "env.h"
#include "lang.h"
#include "lizard_internal.h"
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
#define LIZARD_VERSION "0.1.0-dev"

static void print_usage(const char *argv0) {
  printf("usage: %s [--help] [--version] [--eval EXPR] [file]\n", argv0);
  printf("Run Lizard interactively, evaluate EXPR, or evaluate a file/stdin in script mode.\n");
}

static char *history[HISTORY_SIZE] = {0};
static int history_count = 0;
static int history_index = 0;

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
  int last_idx;
  if (line[0] == '\0') {
    return;
  }
  if (history_count > 0) {
    last_idx = (history_count - 1) % HISTORY_SIZE;
    if (history[last_idx] != NULL && strcmp(history[last_idx], line) == 0) {
      return;
    }
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

static int eval_source(lizard_context_t *context, const char *source,
                       int interactive) {
  lizard_value_t *result;
  lizard_status_t status;

  result = NULL;
  status = lizard_context_eval_string(context, source, &result);
  if (status != LIZARD_STATUS_OK) {
    if (result != NULL && result->type == AST_ERROR) {
      /* Evaluation error: print the error value (carries its own location). */
      lizard_print_value(result);
      printf("\n");
    } else {
      /* Parse/tokenize error: present the recorded diagnostic with location.
       * The parser records this as data and no longer prints it itself. */
      const lizard_diagnostic_t *d;
      d = lizard_context_last_diagnostic(context);
      if (d != NULL && d->message[0] != '\0') {
        if (d->span.start_line > 0) {
          fprintf(stderr, "error at %d:%d: %s\n", d->span.start_line,
                  d->span.start_column, d->message);
        } else {
          fprintf(stderr, "error: %s\n", d->message);
        }
      }
    }
    return 1;
  }
  if (interactive) {
    printf("=> ");
    lizard_print_value(result);
    printf("\n");
  } else if (result != NULL && result->type != AST_NIL) {
    printf("=> ");
    lizard_print_value(result);
    printf("\n");
  }
  return 0;
}

int main(int argc, char **argv) {
  char *input;
  int i;
  int argi;
  int interactive;
  int exit_code;
  const char *eval_expr;
  const char *file_path;
  int opt_trace_expansion;
  int opt_print_trace;
  const char *opt_trace_file;
  int opt_expand_only;
  const char *opt_expand_format;
  int opt_list_schemas;
  const char *opt_list_format;
  const char *opt_require_schema;

  eval_expr = NULL;
  file_path = NULL;
  exit_code = 0;
  opt_trace_expansion = 0;
  opt_print_trace = 0;
  opt_trace_file = NULL;
  opt_expand_only = 0;
  opt_expand_format = "text";
  opt_list_schemas = 0;
  opt_list_format = "text";
  opt_require_schema = NULL;

  for (argi = 1; argi < argc; argi++) {
    if (strcmp(argv[argi], "--help") == 0 || strcmp(argv[argi], "-h") == 0) {
      print_usage(argv[0]);
      return 0;
    }
    if (strcmp(argv[argi], "--version") == 0) {
      printf("lizard %s\n", LIZARD_VERSION);
      return 0;
    }
    if (strcmp(argv[argi], "--eval") == 0 || strcmp(argv[argi], "-e") == 0) {
      argi++;
      if (argi >= argc) {
        fprintf(stderr, "--eval expects an expression\n");
        return 2;
      }
      eval_expr = argv[argi];
      continue;
    }
    if (strcmp(argv[argi], "--trace-expansion") == 0) {
      opt_trace_expansion = 1;
      continue;
    }
    if (strcmp(argv[argi], "--print-expansion-trace") == 0) {
      opt_trace_expansion = 1;
      opt_print_trace = 1;
      continue;
    }
    if (strcmp(argv[argi], "--trace-expansion-file") == 0) {
      argi++;
      if (argi >= argc) {
        fprintf(stderr, "--trace-expansion-file expects a path\n");
        return 2;
      }
      opt_trace_expansion = 1;
      opt_trace_file = argv[argi];
      continue;
    }
    if (strcmp(argv[argi], "--expand-only") == 0) {
      opt_expand_only = 1;
      continue;
    }
    if (strcmp(argv[argi], "--expand-only-format") == 0) {
      argi++;
      if (argi >= argc) {
        fprintf(stderr, "--expand-only-format expects a format\n");
        return 2;
      }
      opt_expand_format = argv[argi];
      continue;
    }
    if (strcmp(argv[argi], "--list-report-schemas") == 0) {
      opt_list_schemas = 1;
      continue;
    }
    if (strcmp(argv[argi], "--list-report-schemas-format") == 0) {
      argi++;
      if (argi >= argc) {
        fprintf(stderr, "--list-report-schemas-format expects a format\n");
        return 2;
      }
      opt_list_format = argv[argi];
      continue;
    }
    if (strcmp(argv[argi], "--require-report-schema") == 0) {
      argi++;
      if (argi >= argc) {
        fprintf(stderr, "--require-report-schema expects type:version:format\n");
        return 2;
      }
      opt_require_schema = argv[argi];
      continue;
    }
    if (file_path == NULL) {
      file_path = argv[argi];
    } else {
      print_usage(argv[0]);
      return 2;
    }
  }
  if (opt_list_schemas) {
    if (eval_expr != NULL || file_path != NULL) {
      fprintf(stderr, "--list-report-schemas cannot be combined with evaluation\n");
      return 2;
    }
    if (strcmp(opt_list_format, "text") != 0 &&
        strcmp(opt_list_format, "json") != 0) {
      fprintf(stderr, "invalid --list-report-schemas-format: %s\n",
              opt_list_format);
      return 2;
    }
    if (strcmp(opt_list_format, "json") == 0) {
      lizard_report_schema_list_fprint_json(stdout);
    } else {
      lizard_report_schema_list_fprint(stdout);
    }
    printf("\n");
    return 0;
  }
  if (opt_require_schema != NULL) {
    char spec[256];
    char *colon1;
    char *colon2;
    const char *rtype;
    const char *rfmt;
    int rver;
    lizard_report_schema_info_t rinfo;
    if (eval_expr != NULL || file_path != NULL) {
      fprintf(stderr, "--require-report-schema cannot be combined with evaluation\n");
      return 2;
    }
    strncpy(spec, opt_require_schema, sizeof(spec) - 1U);
    spec[sizeof(spec) - 1U] = '\0';
    colon1 = strchr(spec, ':');
    colon2 = (colon1 != NULL) ? strchr(colon1 + 1, ':') : NULL;
    if (colon1 == NULL || colon2 == NULL) {
      fprintf(stderr, "invalid --require-report-schema format: %s\n",
              opt_require_schema);
      return 2;
    }
    *colon1 = '\0';
    *colon2 = '\0';
    rtype = spec;
    rver = atoi(colon1 + 1);
    rfmt = colon2 + 1;
    if (strcmp(rfmt, "text") != 0 && strcmp(rfmt, "json") != 0) {
      fprintf(stderr, "invalid --require-report-schema format: %s\n",
              opt_require_schema);
      return 2;
    }
    if (!lizard_report_schema_require(rtype, rver, rfmt, &rinfo)) {
      fprintf(stderr, "required report schema not supported: %s\n",
              opt_require_schema);
      return 2;
    }
    return 0;
  }
  if (eval_expr != NULL && file_path != NULL) {
    fprintf(stderr, "--eval and file input are mutually exclusive\n");
    return 2;
  }
  if (file_path != NULL && freopen(file_path, "r", stdin) == NULL) {
    perror(file_path);
    return 1;
  }

  {
    lizard_runtime_t *runtime;
    lizard_context_t *context;

    /* lizard_runtime_create installs the GMP allocators, creates the heap,
     * and sets up all formerly-global state (modules, search path, flags). */
    runtime = lizard_runtime_create(NULL);
    if (runtime == NULL) {
      fprintf(stderr, "failed to create lizard runtime\n");
      return 1;
    }
    context = lizard_context_create(runtime);
    if (context == NULL) {
      fprintf(stderr, "failed to create lizard context\n");
      lizard_runtime_destroy(runtime);
      return 1;
    }

    if (opt_expand_only) {
      if (strcmp(opt_expand_format, "text") != 0 &&
          strcmp(opt_expand_format, "json") != 0) {
        fprintf(stderr, "invalid --expand-only-format: %s\n", opt_expand_format);
        exit_code = 2;
      } else if (eval_expr == NULL) {
        fprintf(stderr, "--expand-only requires --eval\n");
        exit_code = 2;
      } else {
        lizard_syntax_expansion_report_t *rep;
        rep = lizard_context_syntax_expansion_report(context, eval_expr,
                                                     "<eval>");
        if (rep == NULL) {
          fprintf(stderr, "failed to build expansion report\n");
          exit_code = 1;
        } else {
          if (strcmp(opt_expand_format, "json") == 0) {
            lizard_syntax_expansion_report_fprint_json(stdout, rep);
            printf("\n");
          } else {
            unsigned long n;
            unsigned long ti;
            lizard_expansion_trace_event_t ev;
            lizard_syntax_expansion_report_fprint(stdout, rep);
            /* The text header does not embed the trace; emit the recorded
             * stages so tooling and humans can see macro-expand events. */
            n = lizard_syntax_expansion_report_trace_count(rep);
            for (ti = 0; ti < n; ti++) {
              if (lizard_syntax_expansion_report_trace_event(rep, ti, &ev)) {
                printf("event\tindex=%lu\tstage=%s\n", ti,
                       ev.stage != NULL ? ev.stage : "");
              }
            }
          }
          lizard_syntax_expansion_report_destroy(rep);
        }
      }
    } else if (eval_expr != NULL) {
      if (opt_trace_expansion) {
        lizard_context_set_trace_expansion(context, 1);
      }
      exit_code = eval_source(context, eval_expr, 0);
      if (opt_trace_expansion) {
        lizard_expansion_trace_report_t *tr;
        tr = lizard_context_expansion_trace_report(context);
        if (tr != NULL) {
          if (opt_trace_file != NULL) {
            FILE *tf;
            tf = fopen(opt_trace_file, "wb");
            if (tf != NULL) {
              lizard_expansion_trace_report_fprint(tf, tr);
              fclose(tf);
            } else {
              fprintf(stderr, "unable to open trace file: %s\n", opt_trace_file);
            }
          }
          if (opt_print_trace) {
            printf("Expansion trace:\n");
            lizard_expansion_trace_report_fprint(stdout, tr);
          }
          lizard_expansion_trace_report_destroy(tr);
        }
      }
    } else {
      while (1) {
        interactive = isatty(STDIN_FILENO);
        if (interactive) {
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
        if (eval_source(context, input, interactive) != 0) {
          exit_code = 1;
        }
        free(input);
      }
    }
    lizard_context_destroy(context);
    lizard_runtime_destroy(runtime);
  }
  for (i = 0; i < HISTORY_SIZE; i++) {
    free(history[i]);
  }
  return exit_code;
}
