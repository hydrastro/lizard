/* tests/test_helpers.h
 *
 * Small façade over the interpreter so individual tests don't have
 * to manually wire up a heap, install primitives, tokenize/parse/eval.
 *
 * Usage:
 *   lizard_test_env_t e;
 *   lizard_test_env_init(&e);
 *   lizard_ast_node_t *r = lizard_test_eval(&e, "(+ 1 2)");
 *   ...assertions on r...
 *   lizard_test_env_destroy(&e);
 */
#ifndef LIZARD_TEST_HELPERS_H
#define LIZARD_TEST_HELPERS_H

#include "env.h"
#include "lang.h"
#include "lizard_internal.h"
#include "mem.h"
#include "parser.h"
#include "primitives.h"
#include "printer.h"
#include "tokenizer.h"

typedef struct {
  lizard_heap_t *heap;
  lizard_env_t *env;
} lizard_test_env_t;

/* Forward decls for primitives the test harness installs. We don't
   re-declare every primitive — repl.c does that — but we need a
   single registration routine. The test_helpers.c file owns it. */
void lizard_test_env_init(lizard_test_env_t *e);
void lizard_test_env_destroy(lizard_test_env_t *e);

/* Tokenize → parse → expand_macros → eval the source string, return
   the value of the LAST top-level expression. */
lizard_ast_node_t *lizard_test_eval(lizard_test_env_t *e, const char *src);

/* Format the given value into a static buffer (truncated if needed)
   and return it. Each call overwrites the buffer; not thread-safe. */
const char *lizard_test_format(lizard_ast_node_t *node);

/* True if the value is an AST_NUMBER equal to n. */
int lizard_test_is_int(lizard_ast_node_t *node, long n);

/* True if the value is the given symbol name. */
int lizard_test_is_symbol(lizard_ast_node_t *node, const char *name);

/* True if the value is the boolean #t (resp. #f). */
int lizard_test_is_true(lizard_ast_node_t *node);
int lizard_test_is_false(lizard_ast_node_t *node);

/* True if the value is an error with the given numeric code. */
int lizard_test_is_error(lizard_ast_node_t *node);

#endif /* LIZARD_TEST_HELPERS_H */
