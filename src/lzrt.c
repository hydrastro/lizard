/* lzrt.c — see lzrt.h.  Runtime support for compiled (compile-to-C) programs. */
#include "lzrt.h"
#include "mem.h"
#include "env.h"
#include "primitives.h"
#include "errors.h"
#include <gmp.h>
#include <string.h>

/* The compiled program's global environment (primitives + top-level defines). */
static lizard_env_t *g_env = NULL;

void lzrt_init(void) {
  mp_set_memory_functions(lizard_heap_alloc, lizard_heap_realloc,
                          lizard_heap_free_wrapper);
  heap = lizard_heap_create((size_t)1024 * 1024, (size_t)256 * 1024 * 1024);
  g_env = lizard_env_create(heap, NULL);
  lizard_install_primitives(heap, g_env);
}

lizard_env_t *lzrt_global_env(void) { return g_env; }

/* ---- literals ---- */

lizard_ast_node_t *lzrt_int(long v) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_NUMBER;
  mpz_init_set_si(n->data.number, v);
  return n;
}
lizard_ast_node_t *lzrt_int_str(const char *decimal) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_NUMBER;
  mpz_init_set_str(n->data.number, decimal, 10);
  return n;
}
lizard_ast_node_t *lzrt_real(double v) { return lizard_make_real(heap, v); }
lizard_ast_node_t *lzrt_bool(int v) { return lizard_make_bool(heap, v ? true : false); }
lizard_ast_node_t *lzrt_nil(void) { return lizard_make_nil(heap); }

lizard_ast_node_t *lzrt_string(const char *s) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  size_t len = strlen(s);
  char *copy = lizard_heap_alloc(len + 1);
  memcpy(copy, s, len + 1);
  n->type = AST_STRING;
  n->data.string = copy;
  return n;
}
lizard_ast_node_t *lzrt_symbol(const char *s) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  size_t len = strlen(s);
  char *copy = lizard_heap_alloc(len + 1);
  memcpy(copy, s, len + 1);
  n->type = AST_SYMBOL;
  n->data.variable = copy;
  return n;
}
lizard_ast_node_t *lzrt_cons(lizard_ast_node_t *a, lizard_ast_node_t *d) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_PAIR;
  n->data.pair.car = a;
  n->data.pair.cdr = d;
  return n;
}

/* ---- assignment-conversion boxes -------------------------------------- *
 * A variable that is the target of set! and is captured by a closure must
 * live in a shared, mutable cell so every closure observes the same value.
 * A box is just a one-slot mutable cell (reusing AST_PAIR); the compiler
 * wraps such a variable's binding in lzrt_box and routes reads/writes through
 * lzrt_box_get / lzrt_box_set. */
lizard_ast_node_t *lzrt_box(lizard_ast_node_t *v) {
  lizard_ast_node_t *n = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  n->type = AST_PAIR;
  n->data.pair.car = v;
  n->data.pair.cdr = lzrt_nil();
  return n;
}
lizard_ast_node_t *lzrt_box_get(lizard_ast_node_t *b) {
  return b->data.pair.car;
}
lizard_ast_node_t *lzrt_box_set(lizard_ast_node_t *b, lizard_ast_node_t *v) {
  b->data.pair.car = v;
  return v;
}

/* ---- environment ---- */

lizard_ast_node_t *lzrt_global_ref(const char *name) {
  lizard_ast_node_t *v = lizard_env_lookup(g_env, name);
  if (v == NULL) {
    return lizard_make_error(heap, LIZARD_ERROR_USER);
  }
  return v;
}
void lzrt_define_global(const char *name, lizard_ast_node_t *value) {
  lizard_env_define(heap, g_env, name, value);
}

/* ---- procedures + trampoline ---- */

lizard_ast_node_t **lzrt_argv(long n) {
  if (n <= 0) {
    return NULL;
  }
  return (lizard_ast_node_t **)lizard_heap_alloc((size_t)n *
                                                 sizeof(lizard_ast_node_t *));
}

lizard_ast_node_t *lzrt_closure(
    lizard_ast_node_t *(*entry)(lizard_ast_node_t **, lizard_ast_node_t **, long),
    lizard_ast_node_t **freevar_values, int nfree, int arity, int variadic) {
  lizard_ast_node_t *p = lizard_heap_alloc(sizeof(lizard_ast_node_t));
  p->type = AST_COMPILED_PROC;
  p->data.compiled.entry = entry;
  p->data.compiled.nfree = nfree;
  p->data.compiled.arity = arity;
  p->data.compiled.variadic = variadic;
  if (nfree > 0) {
    int i;
    p->data.compiled.freevars = (lizard_ast_node_t **)lizard_heap_alloc(
        (size_t)nfree * sizeof(lizard_ast_node_t *));
    for (i = 0; i < nfree; i++) {
      p->data.compiled.freevars[i] = freevar_values[i];
    }
  } else {
    p->data.compiled.freevars = NULL;
  }
  return p;
}

/* Single-threaded trampoline register (the runtime is cooperative/1-thread). */
static lizard_ast_node_t *g_bounce_fn = NULL;
static lizard_ast_node_t **g_bounce_argv = NULL;
static long g_bounce_argc = 0;
static int g_bounce_pending = 0;
static lizard_ast_node_t g_bounce_sentinel; /* unique address only */

lizard_ast_node_t *lzrt_tailcall(lizard_ast_node_t *fn,
                                 lizard_ast_node_t **argv, long argc) {
  g_bounce_fn = fn;
  g_bounce_argv = argv;
  g_bounce_argc = argc;
  g_bounce_pending = 1;
  return &g_bounce_sentinel;
}

/* Build an lz_list_t of argv and call a primitive. */
static lizard_ast_node_t *lzrt_call_primitive(lizard_ast_node_t *prim,
                                              lizard_ast_node_t **argv,
                                              long argc) {
  lz_list_t *args = list_create_alloc(lizard_heap_alloc, lizard_heap_free);
  long i;
  for (i = 0; i < argc; i++) {
    lizard_ast_list_node_t *node =
        lizard_heap_alloc(sizeof(lizard_ast_list_node_t));
    node->ast = argv[i];
    list_append(args, &node->node);
  }
  return prim->data.primitive(args, g_env, heap);
}

/* Bind argv to a compiled procedure's parameters (handling rest args) and call
 * its entry.  Returns whatever the entry returns (a value or the bounce). */
static lizard_ast_node_t *lzrt_invoke_compiled(lizard_ast_node_t *fn,
                                               lizard_ast_node_t **argv,
                                               long argc) {
  int arity = fn->data.compiled.arity;
  if (!fn->data.compiled.variadic) {
    if (argc != (long)arity) {
      return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
    }
    return fn->data.compiled.entry(fn->data.compiled.freevars, argv, argc);
  } else {
    /* collect args beyond the first (arity-1) into a list in the last slot */
    int fixed = arity - 1;
    lizard_ast_node_t **bound;
    lizard_ast_node_t *rest;
    long i;
    if (argc < (long)fixed) {
      return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
    }
    bound = lzrt_argv((long)arity);
    for (i = 0; i < (long)fixed; i++) {
      bound[i] = argv[i];
    }
    rest = lizard_make_nil(heap);
    for (i = argc - 1; i >= (long)fixed; i--) {
      rest = lzrt_cons(argv[i], rest);
    }
    bound[fixed] = rest;
    return fn->data.compiled.entry(fn->data.compiled.freevars, bound,
                                   (long)arity);
  }
}

lizard_ast_node_t *lzrt_apply(lizard_ast_node_t *fn, lizard_ast_node_t **argv,
                              long argc) {
  lizard_ast_node_t *r;
  for (;;) {
    if (fn == NULL) {
      return lizard_make_error(heap, LIZARD_ERROR_USER);
    }
    if (fn->type == AST_PRIMITIVE) {
      r = lzrt_call_primitive(fn, argv, argc);
    } else if (fn->type == AST_COMPILED_PROC) {
      r = lzrt_invoke_compiled(fn, argv, argc);
    } else {
      return lizard_make_error(heap, LIZARD_ERROR_USER); /* not applicable */
    }
    if (g_bounce_pending && r == &g_bounce_sentinel) {
      g_bounce_pending = 0;
      fn = g_bounce_fn;
      argv = g_bounce_argv;
      argc = g_bounce_argc;
      continue; /* bounce the pending tail call */
    }
    return r;
  }
}

/* ---- helpers ---- */

int lzrt_truthy(lizard_ast_node_t *v) {
  return !(v != NULL && v->type == AST_BOOL && v->data.boolean == false);
}
lizard_ast_node_t *lzrt_error(const char *msg) {
  (void)msg;
  return lizard_make_error(heap, LIZARD_ERROR_USER);
}
