#include "env.h"
#include "lang.h"
#include "lizard_internal.h"
#include "mem.h"
#include "parser.h"
#include "primitives.h"
#include "printer.h"
#include "tokenizer.h"
#include "syntax_expansion_report.h"
#include "report_schema.h"
#include <limits.h>
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
  printf("usage: %s [--help] [--version] [--trace-expansion] "
         "[--print-expansion-trace] [--trace-expansion-file PATH] "
         "[--expand-only] [--expand-only-format text|json] [--list-report-schemas] [--list-report-schemas-format text|json] [--require-report-schema TYPE:VERSION:FORMAT] [--eval EXPR] [file]\n", argv0);
  printf("Run Lizard interactively, evaluate EXPR, or evaluate a file/stdin in script mode.\n");
  printf("  --trace-expansion        enable traced SurfaceTerm macro expansion\n");
  printf("  --print-expansion-trace  print an owned expansion trace report after each evaluation\n");
  printf("  --trace-expansion-file PATH\n");
  printf("                           write line-oriented expansion trace reports to PATH\n");
  printf("  --expand-only             expand input and print a syntax expansion report; do not evaluate\n");
  printf("  --expand-only-format FMT  choose expand-only output: text or json\n");
  printf("  --list-report-schemas    list supported tooling report schemas and exit\n");
  printf("  --list-report-schemas-format FMT\n");
  printf("                           choose schema-list output: text or json\n");
  printf("  --require-report-schema TYPE:VERSION:FORMAT\n");
  printf("                           require a supported report schema, format any|text|json\n");
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

static void print_expansion_trace_report(lizard_context_t *context,
                                         FILE *fp) {
  lizard_expansion_trace_report_t *report;
  unsigned long count;
  unsigned long i;
  char line[1024];

  if (context == NULL || fp == NULL) {
    return;
  }

  report = lizard_context_expansion_trace_report(context);
  if (report == NULL) {
    fprintf(fp, "Expansion trace: <allocation failed>\n");
    return;
  }

  count = lizard_expansion_trace_report_count(report);
  fprintf(fp, "Expansion trace: %lu event(s)\n", count);
  for (i = 0UL; i < count; i++) {
    line[0] = '\0';
    if (lizard_expansion_trace_report_event_string(report, i, line,
                                                   sizeof(line))) {
      fprintf(fp, "  [%lu] %s\n", i, line);
    }
  }
  lizard_expansion_trace_report_destroy(report);
}

static int write_expansion_trace_report(lizard_context_t *context,
                                        FILE *fp) {
  lizard_expansion_trace_report_t *report;
  int ok;

  if (context == NULL || fp == NULL) {
    return 1;
  }

  report = lizard_context_expansion_trace_report(context);
  if (report == NULL) {
    return 0;
  }
  ok = lizard_expansion_trace_report_fprint(fp, report);
  lizard_expansion_trace_report_destroy(report);
  if (fflush(fp) != 0) {
    ok = 0;
  }
  return ok;
}

static int maybe_emit_expansion_trace(lizard_context_t *context,
                                      int print_trace, FILE *trace_file) {
  int ok;

  ok = 1;
  if (print_trace) {
    print_expansion_trace_report(context, stderr);
  }
  if (trace_file != NULL) {
    ok = write_expansion_trace_report(context, trace_file);
    if (!ok) {
      fprintf(stderr, "failed to write expansion trace report\n");
    }
  }
  return ok;
}


static int expand_source_only(lizard_context_t *context, const char *source,
                              FILE *trace_file, int json_output) {
  lizard_syntax_expansion_report_t *report;
  const lizard_diagnostic_t *diagnostic;
  lizard_status_t status;
  int ok;

  if (context == NULL || source == NULL) {
    return 1;
  }
  report = lizard_context_syntax_expansion_report(context, source,
                                                  "<string>");
  if (report == NULL) {
    fprintf(stderr, "failed to create syntax expansion report\n");
    return 1;
  }
  status = lizard_syntax_expansion_report_status(report);
  if (status != LIZARD_STATUS_OK) {
    diagnostic = lizard_syntax_expansion_report_diagnostic(report);
    if (diagnostic != NULL && diagnostic->message[0] != '\0') {
      if (diagnostic->span.start_line > 0) {
        fprintf(stderr, "error at %d:%d: %s\n",
                diagnostic->span.start_line,
                diagnostic->span.start_column,
                diagnostic->message);
      } else {
        fprintf(stderr, "error: %s\n", diagnostic->message);
      }
    }
    lizard_syntax_expansion_report_destroy(report);
    return 1;
  }

  ok = json_output ? lizard_syntax_expansion_report_fprint_json(stdout, report)
                   : lizard_syntax_expansion_report_fprint(stdout, report);
  if (trace_file != NULL) {
    ok = (json_output ? lizard_syntax_expansion_report_fprint_json(trace_file, report)
                      : lizard_syntax_expansion_report_fprint(trace_file, report)) && ok;
    if (fflush(trace_file) != 0) {
      ok = 0;
    }
  }
  lizard_syntax_expansion_report_destroy(report);
  return ok ? 0 : 1;
}

static int eval_source(lizard_context_t *context, const char *source,
                       int interactive, int print_trace, FILE *trace_file) {
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
    (void)maybe_emit_expansion_trace(context, print_trace, trace_file);
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
  if (!maybe_emit_expansion_trace(context, print_trace, trace_file)) {
    return 1;
  }
  return 0;
}

static int list_report_schemas(int json_output) {
  int ok;

  ok = json_output ? lizard_report_schema_list_fprint_json(stdout)
                   : lizard_report_schema_list_fprint(stdout);
  if (!ok) {
    fprintf(stderr, "failed to write report schema list\n");
    return 1;
  }
  return 0;
}


static int parse_nonnegative_version(const char *text, int *out_value) {
  char *end;
  long value;

  if (text == NULL || text[0] == '\0' || out_value == NULL) {
    return 0;
  }
  value = strtol(text, &end, 10);
  if (end == text || *end != '\0' || value < 0L || value > (long)INT_MAX) {
    return 0;
  }
  *out_value = (int)value;
  return 1;
}

static int copy_schema_requirement_part(char *dst, size_t dst_size,
                                        const char *start,
                                        const char *end) {
  size_t len;

  if (dst == NULL || dst_size == 0U || start == NULL || end == NULL ||
      end < start) {
    return 0;
  }
  len = (size_t)(end - start);
  if (len == 0U || len >= dst_size) {
    return 0;
  }
  memcpy(dst, start, len);
  dst[len] = '\0';
  return 1;
}

static int require_report_schema_spec(const char *spec) {
  const char *first;
  const char *second;
  const char *extra;
  char type[128];
  char version_text[32];
  char format[16];
  int min_version;
  lizard_report_schema_info_t info;

  if (spec == NULL) {
    fprintf(stderr, "invalid --require-report-schema: expected TYPE:VERSION:FORMAT\n");
    return 2;
  }
  first = strchr(spec, ':');
  if (first == NULL) {
    fprintf(stderr, "invalid --require-report-schema: expected TYPE:VERSION:FORMAT\n");
    return 2;
  }
  second = strchr(first + 1, ':');
  if (second == NULL) {
    fprintf(stderr, "invalid --require-report-schema: expected TYPE:VERSION:FORMAT\n");
    return 2;
  }
  extra = strchr(second + 1, ':');
  if (extra != NULL ||
      !copy_schema_requirement_part(type, sizeof(type), spec, first) ||
      !copy_schema_requirement_part(version_text, sizeof(version_text),
                                    first + 1, second) ||
      !copy_schema_requirement_part(format, sizeof(format), second + 1,
                                    spec + strlen(spec))) {
    fprintf(stderr, "invalid --require-report-schema: expected TYPE:VERSION:FORMAT\n");
    return 2;
  }
  if (!parse_nonnegative_version(version_text, &min_version)) {
    fprintf(stderr, "invalid --require-report-schema version: %s\n",
            version_text);
    return 2;
  }
  if (strcmp(format, "any") != 0 && strcmp(format, "text") != 0 &&
      strcmp(format, "json") != 0) {
    fprintf(stderr, "invalid --require-report-schema format: %s\n", format);
    return 2;
  }
  if (!lizard_report_schema_require(type, min_version, format, &info)) {
    fprintf(stderr,
            "required report schema not supported: type=%s min-version=%d format=%s\n",
            type, min_version, format);
    return 1;
  }
  return 0;
}


int main(int argc, char **argv) {
  char *input;
  int i;
  int argi;
  int interactive;
  int exit_code;
  int trace_expansion;
  int print_expansion_trace;
  int expand_only;
  int expand_only_json;
  int list_schemas;
  int list_schemas_json;
  int require_schema_count;
  const char *require_schema_specs[32];
  const char *trace_expansion_file_path;
  FILE *trace_expansion_file;
  const char *eval_expr;
  const char *file_path;

  eval_expr = NULL;
  file_path = NULL;
  trace_expansion_file_path = NULL;
  trace_expansion_file = NULL;
  exit_code = 0;
  trace_expansion = 0;
  print_expansion_trace = 0;
  expand_only = 0;
  expand_only_json = 0;
  list_schemas = 0;
  list_schemas_json = 0;
  require_schema_count = 0;

  for (argi = 1; argi < argc; argi++) {
    if (strcmp(argv[argi], "--help") == 0 || strcmp(argv[argi], "-h") == 0) {
      print_usage(argv[0]);
      return 0;
    }
    if (strcmp(argv[argi], "--version") == 0) {
      printf("lizard %s\n", LIZARD_VERSION);
      return 0;
    }
    if (strcmp(argv[argi], "--trace-expansion") == 0) {
      trace_expansion = 1;
      continue;
    }
    if (strcmp(argv[argi], "--print-expansion-trace") == 0) {
      trace_expansion = 1;
      print_expansion_trace = 1;
      continue;
    }
    if (strcmp(argv[argi], "--trace-expansion-file") == 0) {
      argi++;
      if (argi >= argc) {
        fprintf(stderr, "--trace-expansion-file expects a path\n");
        return 2;
      }
      trace_expansion = 1;
      trace_expansion_file_path = argv[argi];
      continue;
    }
    if (strcmp(argv[argi], "--expand-only") == 0) {
      trace_expansion = 1;
      expand_only = 1;
      continue;
    }
    if (strcmp(argv[argi], "--expand-only-format") == 0) {
      argi++;
      if (argi >= argc) {
        fprintf(stderr, "--expand-only-format expects text or json\n");
        return 2;
      }
      if (strcmp(argv[argi], "text") == 0) {
        expand_only_json = 0;
      } else if (strcmp(argv[argi], "json") == 0) {
        expand_only_json = 1;
      } else {
        fprintf(stderr, "invalid --expand-only-format: %s\n", argv[argi]);
        return 2;
      }
      continue;
    }
    if (strcmp(argv[argi], "--list-report-schemas") == 0) {
      list_schemas = 1;
      continue;
    }
    if (strcmp(argv[argi], "--list-report-schemas-format") == 0) {
      argi++;
      if (argi >= argc) {
        fprintf(stderr, "--list-report-schemas-format expects text or json\n");
        return 2;
      }
      if (strcmp(argv[argi], "text") == 0) {
        list_schemas_json = 0;
      } else if (strcmp(argv[argi], "json") == 0) {
        list_schemas_json = 1;
      } else {
        fprintf(stderr, "invalid --list-report-schemas-format: %s\n",
                argv[argi]);
        return 2;
      }
      continue;
    }
    if (strcmp(argv[argi], "--require-report-schema") == 0) {
      argi++;
      if (argi >= argc) {
        fprintf(stderr, "--require-report-schema expects TYPE:VERSION:FORMAT\n");
        return 2;
      }
      if (require_schema_count >= 32) {
        fprintf(stderr, "too many --require-report-schema options\n");
        return 2;
      }
      require_schema_specs[require_schema_count] = argv[argi];
      require_schema_count++;
      continue;
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
    if (file_path == NULL) {
      file_path = argv[argi];
    } else {
      print_usage(argv[0]);
      return 2;
    }
  }
  if (list_schemas) {
    if (eval_expr != NULL || file_path != NULL || expand_only ||
        trace_expansion_file_path != NULL || print_expansion_trace ||
        trace_expansion || require_schema_count > 0) {
      fprintf(stderr, "--list-report-schemas cannot be combined with evaluation, tracing, or requirement options\n");
      return 2;
    }
    return list_report_schemas(list_schemas_json);
  }
  if (require_schema_count > 0) {
    int r;
    int status;
    if (eval_expr != NULL || file_path != NULL || expand_only ||
        trace_expansion_file_path != NULL || print_expansion_trace ||
        trace_expansion) {
      fprintf(stderr, "--require-report-schema cannot be combined with evaluation or tracing options\n");
      return 2;
    }
    for (r = 0; r < require_schema_count; r++) {
      status = require_report_schema_spec(require_schema_specs[r]);
      if (status != 0) {
        return status;
      }
    }
    return 0;
  }
  if (eval_expr != NULL && file_path != NULL) {
    fprintf(stderr, "--eval and file input are mutually exclusive\n");
    return 2;
  }
  if (trace_expansion_file_path != NULL) {
    trace_expansion_file = fopen(trace_expansion_file_path, "w");
    if (trace_expansion_file == NULL) {
      perror(trace_expansion_file_path);
      return 1;
    }
  }
  if (file_path != NULL && freopen(file_path, "r", stdin) == NULL) {
    if (trace_expansion_file != NULL) {
      fclose(trace_expansion_file);
    }
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
      if (trace_expansion_file != NULL) {
        fclose(trace_expansion_file);
      }
      return 1;
    }
    context = lizard_context_create(runtime);
    if (context == NULL) {
      fprintf(stderr, "failed to create lizard context\n");
      lizard_runtime_destroy(runtime);
      if (trace_expansion_file != NULL) {
        fclose(trace_expansion_file);
      }
      return 1;
    }

    lizard_context_set_trace_expansion(context, trace_expansion);

    if (eval_expr != NULL) {
      if (expand_only) {
        exit_code = expand_source_only(context, eval_expr,
                                       trace_expansion_file,
                                       expand_only_json);
      } else {
        exit_code = eval_source(context, eval_expr, 0, print_expansion_trace,
                                trace_expansion_file);
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
        if (expand_only) {
          if (expand_source_only(context, input, trace_expansion_file,
                                 expand_only_json) != 0) {
            exit_code = 1;
          }
        } else if (eval_source(context, input, interactive, print_expansion_trace,
                               trace_expansion_file) != 0) {
          exit_code = 1;
        }
        free(input);
      }
    }
    lizard_context_destroy(context);
    lizard_runtime_destroy(runtime);
  }
  if (trace_expansion_file != NULL) {
    fclose(trace_expansion_file);
  }
  for (i = 0; i < HISTORY_SIZE; i++) {
    free(history[i]);
  }
  return exit_code;
}
