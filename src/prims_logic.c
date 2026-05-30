/* prims_logic.c — extracted from primitives.c (#7 monolith split).
 * Registration stays in primitives.c; definitions linked from here. */
#include "primitives.h"
#include "env.h"
#include "errors.h"
#include "lizard_internal.h"
#include "mem.h"
#include "parser.h"
#include "printer.h"
#include "runtime.h"
#include "tokenizer.h"
#include "prims_shared.h"
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  lizard_heap_t *heap;
  lizard_ast_node_t *head;  /* growing list, prepended */
} logic_config_collect_t;
typedef struct {
  lizard_heap_t *heap;
  lizard_ast_node_t *head;
} list_logics_collect_t;

/* forward decls (defs below) */
static int logic_config_collect_cb(const char *name, int enabled, void *ud);
static int list_logics_collect_cb(const char *name, void *ud);

lizard_ast_node_t *lizard_primitive_logic_rule_register(lz_list_t *args,
                                                        lizard_env_t *env,
                                                        lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  lizard_logic_rule_register(name_arg->data.variable, 0);
  return lizard_make_bool(heap, 1);
}
lizard_ast_node_t *lizard_primitive_logic_rule_enable(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  lizard_logic_rule_enable(name_arg->data.variable);
  return lizard_make_bool(heap, 1);
}
lizard_ast_node_t *lizard_primitive_logic_rule_disable(lz_list_t *args,
                                                       lizard_env_t *env,
                                                       lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  lizard_logic_rule_disable(name_arg->data.variable);
  return lizard_make_bool(heap, 1);
}
lizard_ast_node_t *lizard_primitive_logic_rule_enabledp(lz_list_t *args,
                                                        lizard_env_t *env,
                                                        lizard_heap_t *heap) {
  lizard_ast_node_t *name_arg;
  int r;
  (void)env;
  if (!single_arg(args)) {
    return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  }
  name_arg = nth_arg(args, 0);
  if (name_arg == NULL || name_arg->type != AST_SYMBOL) {
    return lizard_make_error(heap, LIZARD_ERROR_PLUS_ARGT);
  }
  r = lizard_logic_rule_enabled(name_arg->data.variable);
  if (r == 1) return lizard_make_bool(heap, 1);
  if (r == 0) return lizard_make_bool(heap, 0);
  /* Unknown — return the symbol 'unknown. */
  {
    char *buf = lizard_heap_alloc(8);
    lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    strcpy(buf, "unknown");
    n->type = AST_SYMBOL;
    n->data.variable = buf;
    return n;
  }
}
static int logic_config_collect_cb(const char *name, int enabled, void *ud) {
  logic_config_collect_t *c = (logic_config_collect_t *)ud;
  lizard_ast_node_t *pair, *sym, *val, *cons;
  char *namedup;
  size_t namelen;
  /* Build (name . enabled?) as a pair. */
  pair = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  pair->type = AST_PAIR;
  namelen = strlen(name) + 1;
  namedup = lizard_heap_alloc(namelen);
  memcpy(namedup, name, namelen);
  sym = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  sym->type = AST_SYMBOL;
  sym->data.variable = namedup;
  val = lizard_make_bool(c->heap, enabled);
  pair->data.pair.car = sym;
  pair->data.pair.cdr = val;
  /* Prepend to head. */
  cons = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  cons->type = AST_PAIR;
  cons->data.pair.car = pair;
  cons->data.pair.cdr = c->head;
  c->head = cons;
  return 0;
}
lizard_ast_node_t *lizard_primitive_logic_config(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  logic_config_collect_t c;
  lizard_ast_node_t *nil;
  (void)env; (void)args;
  /* Build the empty list (nil) as the initial tail. */
  nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  nil->type = AST_NIL;
  c.heap = heap;
  c.head = nil;
  lizard_logic_config_walk(logic_config_collect_cb, &c);
  return c.head;
}
lizard_ast_node_t *lizard_primitive_logic_config_size(lz_list_t *args,
                                                     lizard_env_t *env,
                                                     lizard_heap_t *heap) {
  lizard_ast_node_t *n;
  (void)env; (void)args;
  n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_NUMBER;
  mpz_init(n->data.number);
  mpz_set_si(n->data.number, lizard_logic_config_size());
  return n;
}
lizard_ast_node_t *lizard_primitive_logic_config_reset(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap) {
  (void)env; (void)args;
  lizard_logic_config_reset();
  return lizard_make_bool(heap, 1);
}
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
