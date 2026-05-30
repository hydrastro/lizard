/* tests/test_helpers.c
 *
 * Façade that gives unit tests a single line to spin up an interpreter
 * and another to evaluate a string.
 */
#include "test_helpers.h"

#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lizard_test_env_init(lizard_test_env_t *e) {
  mp_set_memory_functions(lizard_heap_alloc, lizard_heap_realloc,
                          lizard_heap_free_wrapper);
  e->heap = lizard_heap_create(1024 * 1024, 16 * 1024 * 1024);
  heap = e->heap; /* publish to the library globals */
  e->env = lizard_env_create(e->heap, NULL);
  lizard_install_primitives(e->heap, e->env);
}

void lizard_test_env_destroy(lizard_test_env_t *e) {
  if (e && e->heap) {
    lizard_heap_destroy(e->heap);
    e->heap = NULL;
    heap = NULL;
  }
}

lizard_ast_node_t *lizard_test_eval(lizard_test_env_t *e, const char *src) {
  lz_list_t *tokens;
  lz_list_t *asts;
  lz_list_node_t *iter;
  lizard_ast_node_t *result = NULL;

  tokens = lizard_tokenize(src);
  asts = lizard_parse(tokens, e->heap);

  for (iter = asts->head; iter != asts->nil; iter = iter->next) {
    lizard_ast_node_t *expanded;
    expanded = lizard_expand_macros(((lizard_ast_list_node_t *)iter)->ast,
                                    e->env, e->heap);
    result = lizard_eval(expanded, e->env, e->heap, lizard_identity_cont);
    if (result && result->type == AST_ERROR) {
      return result;
    }
  }
  return result;
}

/* Format a value using the public pretty printer into a static buffer.
   For really large output we just truncate — tests typically check
   short, predictable shapes. */
static char lizard_test_buf[2048];

const char *lizard_test_format(lizard_ast_node_t *node) {
  FILE *fp;
  long pos;
  size_t n;
  size_t got;

  lizard_test_buf[0] = '\0';
  fp = tmpfile();
  if (!fp) {
    return lizard_test_buf;
  }
  lizard_fprint_value(fp, node);
  fflush(fp);
  pos = ftell(fp);
  if (pos < 0) {
    fclose(fp);
    return lizard_test_buf;
  }
  n = (size_t)pos;
  if (n >= sizeof(lizard_test_buf)) {
    n = sizeof(lizard_test_buf) - 1;
  }
  rewind(fp);
  got = fread(lizard_test_buf, 1, n, fp);
  lizard_test_buf[got] = '\0';
  fclose(fp);
  return lizard_test_buf;
}

int lizard_test_is_int(lizard_ast_node_t *node, long n) {
  if (!node || node->type != AST_NUMBER) {
    return 0;
  }
  return mpz_cmp_si(node->data.number, n) == 0;
}

int lizard_test_is_symbol(lizard_ast_node_t *node, const char *name) {
  if (!node || node->type != AST_SYMBOL) {
    return 0;
  }
  return strcmp(node->data.variable, name) == 0;
}

int lizard_test_is_true(lizard_ast_node_t *node) {
  return node && node->type == AST_BOOL && node->data.boolean;
}

int lizard_test_is_false(lizard_ast_node_t *node) {
  return node && node->type == AST_BOOL && !node->data.boolean;
}

int lizard_test_is_error(lizard_ast_node_t *node) {
  return node && node->type == AST_ERROR;
}
