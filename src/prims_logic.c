/* src/prims_logic.c -- named logic-bundle Lisp primitives.
 *
 * Split out of primitives.c as part of Recoverable Core Phase 1K.
 */
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "primitives.h"

#include <string.h>

static int single_arg(lz_list_t *args) {
  return args->head != args->nil && args->head->next == args->nil;
}

static lizard_ast_node_t *nth_arg(lz_list_t *args, int n) {
  lz_list_node_t *cur;
  int i;
  cur = args->head;
  for (i = 0; cur != args->nil && i < n; i++) {
    cur = cur->next;
  }
  return cur == args->nil ? NULL : ((lizard_ast_list_node_t *)cur)->ast;
}

/* ===== Phase M.3 — Lisp primitives for logic bundles ===== */

/* (set-logic 'NAME)
 * Apply a named logic bundle. Returns #t on success, #f if unknown.
 * Unknown name does NOT raise an error — returns #f so user code can
 * test before assuming the logic is loaded. */
lizard_ast_node_t *lizard_primitive_set_logic(lz_list_t *args,
                                              lizard_env_t *env,
                                              lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg;
  int ok;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  ok = lizard_logic_set_bundle(name_arg->data.variable);
  return lizard_make_bool(heap, ok);
}

/* (current-logic)
 * Returns a symbol naming the current logic, or 'custom. */
lizard_ast_node_t *lizard_primitive_current_logic(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  const char *name;
  lizard_ast_node_t *n;
  char *buf;
  size_t len;
  (void)env; (void)args;
  name = lizard_logic_current_bundle();
  len = strlen(name) + 1;
  buf = lizard_heap_alloc(len);
  memcpy(buf, name, len);
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_SYMBOL;
  n->data.variable = buf;
  return n;
}

/* (list-logics)
 * Returns a list of symbols naming the predefined logic bundles. */
typedef struct {
  lizard_heap_t *heap;
  lizard_ast_node_t *head;
} list_logics_collect_t;

static int list_logics_collect_cb(const char *name, void *ud) {
  list_logics_collect_t *c = (list_logics_collect_t *)ud;
  lizard_ast_node_t *sym, *cons;
  char *buf;
  size_t len;
  len = strlen(name) + 1;
  buf = lizard_heap_alloc(len);
  memcpy(buf, name, len);
  sym = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  sym->type = AST_SYMBOL;
  sym->data.variable = buf;
  cons = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  cons->type = AST_PAIR;
  cons->data.pair.car = sym;
  cons->data.pair.cdr = c->head;
  c->head = cons;
  return 0;
}

lizard_ast_node_t *lizard_primitive_list_logics(lz_list_t *args,
                                                lizard_env_t *env,
                                                lizard_heap_t *heap) {
  list_logics_collect_t c;
  lizard_ast_node_t *nil;
  (void)env; (void)args;
  nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  nil->type = AST_NIL;
  c.heap = heap;
  c.head = nil;
  lizard_logic_bundles_walk(list_logics_collect_cb, &c);
  return c.head;
}

