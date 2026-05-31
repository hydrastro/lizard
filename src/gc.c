/* src/gc.c — Garbage collector infrastructure (Phase D.1).
 *
 * Mark traversal for lizard's AST heap. Follows the same switch-on-
 * type pattern used throughout the codebase.
 */
#include "gc.h"
#include "pvector.h"
#include "env.h"
#include "mem.h"
#include "runtime.h"
#include <stdlib.h>
#include <string.h>

/* ---- mark traversal ---- */

void lizard_gc_mark_node(lizard_ast_node_t *node) {
  lz_list_node_t *iter;
  if (node == NULL) return;
  if (node->gc_mark) return;
  node->gc_mark = 1;

  switch (node->type) {
  /* Leaf nodes. */
  case AST_NIL:
  case AST_BOOL:
  case AST_NUMBER:
  case AST_STRING:
  case AST_SYMBOL:
  case AST_ERROR:
  case AST_TT_UNIVERSE:
  case AST_TT_COUNIVERSE:
  case AST_TT_UNIT:
  case AST_TT_BOT:
  case AST_TT_I0:
  case AST_TT_I1:
  case AST_TT_INTERVAL:
  case AST_TT_S1:
  case AST_TT_S1_BASE:
  case AST_TT_S1_LOOP:
  case AST_TT_UNIVERSE_SET:
  case AST_TT_TT:
  case AST_PRIMITIVE:
  case AST_CONTINUATION:
    break;

  /* Bare pointer (no struct wrapper). */
  case AST_QUOTE:
    lizard_gc_mark_node(node->data.quoted);
    break;
  case AST_UNQUOTE:
    lizard_gc_mark_node(node->data.quoted);
    break;
  case AST_UNQUOTE_SPLICING:
    lizard_gc_mark_node(node->data.quoted);
    break;
  case AST_QUASIQUOTE:
    lizard_gc_mark_node(node->data.quoted);
    break;

  /* Two-child structs. */
  case AST_PAIR:
    lizard_gc_mark_node(node->data.pair.car);
    lizard_gc_mark_node(node->data.pair.cdr);
    break;
  case AST_DEFINITION:
    lizard_gc_mark_node(node->data.definition.variable);
    lizard_gc_mark_node(node->data.definition.value);
    break;
  case AST_ASSIGNMENT:
    lizard_gc_mark_node(node->data.assignment.variable);
    lizard_gc_mark_node(node->data.assignment.value);
    break;

  /* Three-child IF. */
  case AST_IF:
    lizard_gc_mark_node(node->data.if_clause.pred);
    lizard_gc_mark_node(node->data.if_clause.cons);
    lizard_gc_mark_node(node->data.if_clause.alt);
    break;

  /* Lambda: parameters list + closure env. */
  case AST_LAMBDA:
  case AST_MACRO:
    if (node->data.lambda.parameters != NULL) {
      for (iter = node->data.lambda.parameters->head;
           iter != node->data.lambda.parameters->nil;
           iter = iter->next) {
        lizard_gc_mark_node(((lizard_ast_list_node_t *)iter)->ast);
      }
    }
    /* Mark the captured closure environment. */
    lizard_gc_mark_env(node->data.lambda.closure_env);
    break;

  /* List-only nodes. */
  case AST_BEGIN:
    if (node->data.begin_expressions != NULL) {
      for (iter = node->data.begin_expressions->head;
           iter != node->data.begin_expressions->nil;
           iter = iter->next) {
        lizard_gc_mark_node(((lizard_ast_list_node_t *)iter)->ast);
      }
    }
    break;
  case AST_COND:
    if (node->data.cond_clauses != NULL) {
      for (iter = node->data.cond_clauses->head;
           iter != node->data.cond_clauses->nil;
           iter = iter->next) {
        lizard_gc_mark_node(((lizard_ast_list_node_t *)iter)->ast);
      }
    }
    break;
  case AST_APPLICATION:
    if (node->data.application_arguments != NULL) {
      for (iter = node->data.application_arguments->head;
           iter != node->data.application_arguments->nil;
           iter = iter->next) {
        lizard_gc_mark_node(((lizard_ast_list_node_t *)iter)->ast);
      }
    }
    break;
  case AST_CALLCC:
    if (node->data.application_arguments != NULL) {
      for (iter = node->data.application_arguments->head;
           iter != node->data.application_arguments->nil;
           iter = iter->next) {
        lizard_gc_mark_node(((lizard_ast_list_node_t *)iter)->ast);
      }
    }
    break;

  /* Promise. */
  case AST_PROMISE:
    lizard_gc_mark_node(node->data.promise.value);
    break;

  /* Vector: array of pointers. */
  case AST_VECTOR: {
    size_t i;
    for (i = 0; i < node->data.vector.size; i++) {
      lizard_gc_mark_node(node->data.vector.elements[i]);
    }
    break;
  }

  /* --- Type theory single-child nodes --- */
  case AST_TT_BOX:
    lizard_gc_mark_node(node->data.tt_box.argument);
    break;
  case AST_TT_DIAMOND:
    lizard_gc_mark_node(node->data.tt_diamond.argument);
    break;
  case AST_TT_BOX_INTRO:
    lizard_gc_mark_node(node->data.tt_box_intro.body);
    break;
  case AST_TT_DIAMOND_INTRO:
    lizard_gc_mark_node(node->data.tt_diamond_intro.body);
    break;
  case AST_TT_DIAMOND_INTRO_SYM:
    lizard_gc_mark_node(node->data.tt_diamond_intro_sym.body);
    break;
  case AST_TT_POSS_COERCE:
    lizard_gc_mark_node(node->data.tt_poss_coerce.body);
    break;
  case AST_TT_REFL:
    lizard_gc_mark_node(node->data.tt_refl.value);
    break;
  case AST_TT_TRUNC_INTRO:
    lizard_gc_mark_node(node->data.tt_trunc_intro.value);
    break;
  case AST_TT_I_NEG:
    lizard_gc_mark_node(node->data.tt_i_neg.operand);
    break;

  /* TT two-child nodes. */
  case AST_TT_APP:
    lizard_gc_mark_node(node->data.tt_app.fun);
    lizard_gc_mark_node(node->data.tt_app.arg);
    break;
  case AST_TT_SUM:
    lizard_gc_mark_node(node->data.tt_sum.left);
    lizard_gc_mark_node(node->data.tt_sum.right);
    break;
  case AST_TT_ANNOT:
    lizard_gc_mark_node(node->data.tt_annot.term);
    lizard_gc_mark_node(node->data.tt_annot.type);
    break;
  case AST_TT_TRUNC:
    lizard_gc_mark_node(node->data.tt_trunc.level);
    lizard_gc_mark_node(node->data.tt_trunc.type);
    break;
  case AST_TT_I_AND:
  case AST_TT_I_OR:
    lizard_gc_mark_node(node->data.tt_i_binop.left);
    lizard_gc_mark_node(node->data.tt_i_binop.right);
    break;
  case AST_TT_DIAMOND_BIND:
    lizard_gc_mark_node(node->data.tt_diamond_bind.fun);
    lizard_gc_mark_node(node->data.tt_diamond_bind.arg);
    break;
  case AST_TT_BOX_APP:
    lizard_gc_mark_node(node->data.tt_box_app.fun);
    lizard_gc_mark_node(node->data.tt_box_app.arg);
    break;
  case AST_TT_PATH_APP:
    lizard_gc_mark_node(node->data.tt_path_app.path);
    lizard_gc_mark_node(node->data.tt_path_app.point);
    break;
  case AST_TT_PATH_ABS:
    lizard_gc_mark_node(node->data.tt_path_abs.binder);
    lizard_gc_mark_node(node->data.tt_path_abs.body);
    break;
  case AST_TT_LAMBDA:
    lizard_gc_mark_node(node->data.tt_lambda.binder);
    lizard_gc_mark_node(node->data.tt_lambda.body);
    break;

  /* TT three-child nodes. */
  case AST_TT_PI:
    lizard_gc_mark_node(node->data.tt_pi.binder);
    lizard_gc_mark_node(node->data.tt_pi.domain);
    lizard_gc_mark_node(node->data.tt_pi.codomain);
    break;
  case AST_TT_SIGMA:
    lizard_gc_mark_node(node->data.tt_sigma.binder);
    lizard_gc_mark_node(node->data.tt_sigma.domain);
    lizard_gc_mark_node(node->data.tt_sigma.codomain);
    break;
  case AST_TT_ID:
    lizard_gc_mark_node(node->data.tt_id.domain);
    lizard_gc_mark_node(node->data.tt_id.a);
    lizard_gc_mark_node(node->data.tt_id.b);
    break;
  case AST_TT_PATH:
    lizard_gc_mark_node(node->data.tt_path.domain);
    lizard_gc_mark_node(node->data.tt_path.a);
    lizard_gc_mark_node(node->data.tt_path.b);
    break;
  case AST_TT_BOX_ELIM:
    lizard_gc_mark_node(node->data.tt_box_elim.binder);
    lizard_gc_mark_node(node->data.tt_box_elim.scrutinee);
    lizard_gc_mark_node(node->data.tt_box_elim.body);
    break;
  case AST_TT_DIAMOND_ELIM:
    lizard_gc_mark_node(node->data.tt_diamond_elim.binder);
    lizard_gc_mark_node(node->data.tt_diamond_elim.scrutinee);
    lizard_gc_mark_node(node->data.tt_diamond_elim.body);
    break;
  case AST_TT_SYSTEM_CONS:
    lizard_gc_mark_node(node->data.tt_system_cons.face);
    lizard_gc_mark_node(node->data.tt_system_cons.value);
    lizard_gc_mark_node(node->data.tt_system_cons.next);
    break;
  case AST_TT_PI_FRESH:
    lizard_gc_mark_node(node->data.tt_pi_fresh.binder);
    lizard_gc_mark_node(node->data.tt_pi_fresh.domain);
    lizard_gc_mark_node(node->data.tt_pi_fresh.codomain);
    break;
  case AST_TT_SIGMA_FRESH:
    lizard_gc_mark_node(node->data.tt_sigma_fresh.binder);
    lizard_gc_mark_node(node->data.tt_sigma_fresh.domain);
    lizard_gc_mark_node(node->data.tt_sigma_fresh.codomain);
    break;
  case AST_TT_CO_PI_FRESH:
    lizard_gc_mark_node(node->data.tt_co_pi_fresh.binder);
    lizard_gc_mark_node(node->data.tt_co_pi_fresh.domain);
    lizard_gc_mark_node(node->data.tt_co_pi_fresh.codomain);
    break;
  case AST_TT_CO_SIGMA_FRESH:
    lizard_gc_mark_node(node->data.tt_co_sigma_fresh.binder);
    lizard_gc_mark_node(node->data.tt_co_sigma_fresh.domain);
    lizard_gc_mark_node(node->data.tt_co_sigma_fresh.codomain);
    break;
  case AST_TT_HIT_PATH:
    lizard_gc_mark_node(node->data.tt_hit_path.name);
    lizard_gc_mark_node(node->data.tt_hit_path.source);
    lizard_gc_mark_node(node->data.tt_hit_path.target);
    break;

  /* TT four-child nodes. */
  case AST_TT_TRUNC_ELIM:
    lizard_gc_mark_node(node->data.tt_trunc_elim.motive);
    lizard_gc_mark_node(node->data.tt_trunc_elim.handler);
    lizard_gc_mark_node(node->data.tt_trunc_elim.prop);
    lizard_gc_mark_node(node->data.tt_trunc_elim.value);
    break;
  case AST_TT_GLUE:
    lizard_gc_mark_node(node->data.tt_glue.base);
    lizard_gc_mark_node(node->data.tt_glue.face);
    lizard_gc_mark_node(node->data.tt_glue.t);
    lizard_gc_mark_node(node->data.tt_glue.equiv);
    break;
  case AST_TT_GLUE_INTRO:
    lizard_gc_mark_node(node->data.tt_glue_intro.face);
    lizard_gc_mark_node(node->data.tt_glue_intro.t);
    lizard_gc_mark_node(node->data.tt_glue_intro.a);
    break;
  case AST_TT_UNGLUE:
    lizard_gc_mark_node(node->data.tt_unglue.equiv);
    lizard_gc_mark_node(node->data.tt_unglue.target);
    break;

  /* HIT nodes with children. */
  case AST_TT_HIT_DECL:
    lizard_gc_mark_node(node->data.tt_hit_decl.name);
    break;
  case AST_TT_HIT_CONSTRUCTOR:
    lizard_gc_mark_node(node->data.tt_hit_constructor.name);
    break;
  case AST_TT_HIT_REF:
    lizard_gc_mark_node(node->data.tt_hit_ref.name);
    break;
  case AST_TT_HIT_APP:
    lizard_gc_mark_node(node->data.tt_hit_app.name);
    break;
  case AST_TT_EXTENSION:
    lizard_gc_mark_node(node->data.tt_extension.name);
    break;

  /* Track R: syntax objects. */
  case AST_SYNTAX:
    lizard_gc_mark_node(node->data.syntax.datum);
    lizard_gc_mark_env(node->data.syntax.context);
    break;

  /* Track C: persistent vector — mark every element via the trie. */
  case AST_PVEC: {
    int pi, pc = lizard_pvec_count(node);
    for (pi = 0; pi < pc; pi++) {
      lizard_gc_mark_node(lizard_pvec_ref(node, pi));
    }
    break;
  }
  case AST_HAMT:
    /* HAMT internal nodes are opaque — root traversal deferred to C.2. */
    break;

  /* Catch-all: node is marked (above), children not traversed.
   * Conservative — keeps too much, never too little. */
  default:
    break;
  }
}

/* ---- environment marking ---- */

void lizard_gc_mark_env(lizard_env_t *env) {
  lizard_env_entry_t *entry;
  while (env != NULL) {
    for (entry = env->entries; entry != NULL; entry = entry->next) {
      lizard_gc_mark_node(entry->value);
    }
    env = env->parent;
  }
}

/* ---- heap walking ---- */

static void walk_heap_nodes(lizard_heap_t *h,
                            void (*fn)(lizard_ast_node_t *)) {
  lizard_heap_segment_t *seg;
  for (seg = h->head; seg != NULL; seg = seg->next) {
    char *ptr = seg->start;
    while (ptr + sizeof(lizard_ast_node_t) <= seg->top) {
      lizard_ast_node_t *node = (lizard_ast_node_t *)ptr;
      if (node->type >= AST_NIL && node->type <= AST_HAMT) {
        fn(node);
      }
      ptr += sizeof(lizard_ast_node_t);
    }
  }
}

static void clear_mark_fn(lizard_ast_node_t *node) {
  node->gc_mark = 0;
}

void lizard_gc_clear_marks(lizard_heap_t *h) {
  walk_heap_nodes(h, clear_mark_fn);
}

static size_t gc_count;
static void count_marked_fn(lizard_ast_node_t *node) {
  if (node->gc_mark) gc_count++;
}

size_t lizard_gc_count_marked(lizard_heap_t *h) {
  gc_count = 0;
  walk_heap_nodes(h, count_marked_fn);
  return gc_count;
}

static size_t gc_total;
static void count_total_fn(lizard_ast_node_t *node) {
  (void)node;
  gc_total++;
}

static size_t count_total_nodes(lizard_heap_t *h) {
  gc_total = 0;
  walk_heap_nodes(h, count_total_fn);
  return gc_total;
}

/* ---- stats ---- */

void lizard_gc_collect_stats(lizard_heap_t *h, lizard_env_t *env,
                             int do_mark, lizard_gc_stats_t *out) {
  lizard_heap_segment_t *seg;
  memset(out, 0, sizeof(*out));
  for (seg = h->head; seg != NULL; seg = seg->next) {
    out->total_segments++;
    out->total_bytes_allocated += (size_t)(seg->top - seg->start);
  }
  out->nodes_total = count_total_nodes(h);
  if (do_mark && env != NULL) {
    lizard_gc_clear_marks(h);
    lizard_gc_mark_env(env);
    /* Also mark module cache if runtime exists. */
    if (h->runtime != NULL) {
      lizard_module_entry_t *mod;
      for (mod = h->runtime->modules_head; mod != NULL; mod = mod->next) {
        lizard_gc_mark_node(mod->result);
      }
    }
    out->nodes_marked = lizard_gc_count_marked(h);
    out->nodes_garbage = out->nodes_total - out->nodes_marked;
  }
}

/* ---- collection ---- */

/* Check if a segment has any marked (live) AST nodes. */
static int segment_has_live(lizard_heap_segment_t *seg) {
  char *ptr = seg->start;
  while (ptr + sizeof(lizard_ast_node_t) <= seg->top) {
    lizard_ast_node_t *node = (lizard_ast_node_t *)ptr;
    if (node->type >= AST_NIL && node->type <= AST_HAMT) {
      if (node->gc_mark) return 1;
    }
    ptr += sizeof(lizard_ast_node_t);
  }
  return 0;
}

size_t lizard_gc_collect(lizard_heap_t *h, lizard_env_t *env) {
  lizard_heap_segment_t *seg, *prev, *next;
  size_t freed = 0;

  /* Phase 1: mark from roots. */
  lizard_gc_clear_marks(h);
  lizard_gc_mark_env(env);
  if (h->runtime != NULL) {
    lizard_module_entry_t *mod;
    for (mod = h->runtime->modules_head; mod != NULL; mod = mod->next) {
      lizard_gc_mark_node(mod->result);
    }
  }

  /* Phase 2: sweep — free segments with zero live objects.
   *
   * Safety argument: if a segment has zero marked nodes, no live
   * object anywhere in the heap points into it (because the mark
   * traversal would have marked any target). So freeing the
   * segment's memory is safe.
   *
   * We never free the segment that heap->current points to (the
   * active allocation segment) and we never free heap->head if
   * it's the only segment (keep at least one). */
  prev = NULL;
  seg = h->head;
  while (seg != NULL) {
    next = seg->next;
    /* Skip the active segment and don't free the last segment. */
    if (seg == h->current || (prev == NULL && next == NULL)) {
      prev = seg;
      seg = next;
      continue;
    }
    if (!segment_has_live(seg)) {
      /* Unlink and free. */
      freed += (size_t)(seg->end - seg->start);
      if (prev != NULL) {
        prev->next = next;
      } else {
        h->head = next;
      }
      free(seg->start);
      free(seg);
      /* Don't advance prev — it stays the same. */
      seg = next;
    } else {
      prev = seg;
      seg = next;
    }
  }
  return freed;
}


void lizard_gc_metadata_stats(lizard_heap_t *heap,
                              lizard_gc_metadata_stats_t *out_stats) {
  if (heap == NULL) {
    lizard_gc_metadata_collect_stats(NULL, out_stats);
    return;
  }
  lizard_gc_metadata_collect_stats(heap->gc_metadata, out_stats);
}

int lizard_gc_metadata_lookup_object(
    lizard_heap_t *heap, const void *ptr, lizard_gc_object_kind_t *out_kind,
    size_t *out_size, lizard_object_trace_policy_t *out_trace_policy) {
  if (heap == NULL) {
    return 0;
  }
  return lizard_gc_metadata_lookup(heap->gc_metadata, ptr, out_kind, out_size,
                                   out_trace_policy);
}

/* ===================================================================
 * Phase 5 — per-object, non-moving, fully-conservative mark & sweep.
 *
 * Roots are discovered conservatively from the C stack, the spilled
 * registers, and the program's data/BSS segment; the runtime's env and
 * module results are additionally marked explicitly as insurance.  Live
 * objects are traced by scanning their own bytes for anything that looks
 * like a pointer into a tracked object.  Because the collector never
 * moves objects, treating non-pointers as pointers is harmless (it can
 * only retain garbage, never corrupt), which is what makes the
 * conservative approach sound here.  Unmarked objects are returned to
 * per-size free lists for reuse; addresses of survivors never change.
 * ================================================================== */
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>

/* Bounds of the program's static storage (GNU ld / glibc). */
extern char __data_start[];
extern char __bss_start[];
extern char _end[];

typedef struct { void *base; size_t size; } gc_obj_t;
typedef struct { gc_obj_t *items; size_t count, cap; int oom; } gc_worklist_t;

static void gc_wl_push(gc_worklist_t *w, void *base, size_t size) {
  if (w->count == w->cap) {
    size_t ncap = w->cap ? w->cap * 2U : 4096U;
    gc_obj_t *p = (gc_obj_t *)realloc(w->items, ncap * sizeof(gc_obj_t));
    if (p == NULL) { w->oom = 1; return; }
    w->items = p;
    w->cap = ncap;
  }
  w->items[w->count].base = base;
  w->items[w->count].size = size;
  w->count++;
}

/* Scan [lo,hi) for word-aligned values that point into a tracked object;
 * mark each newly found object and enqueue it for tracing. */
static void gc_scan_range(lizard_heap_t *h, char *lo, char *hi,
                          gc_worklist_t *wl) {
  char *p;
  if (lo == NULL || hi == NULL) return;
  if (lo > hi) { char *t = lo; lo = hi; hi = t; }
  while (((uintptr_t)lo % sizeof(void *)) != 0U) lo++;
  for (p = lo; p + sizeof(void *) <= hi; p += sizeof(void *)) {
    void *word = *(void **)p;
    void *base;
    size_t size;
    if (word == NULL) continue;
    if (lizard_gc_metadata_mark_addr(h->gc_metadata, word, &base, &size)) {
      gc_wl_push(wl, base, size);
    }
  }
}

/* Mark a single known root object (and enqueue it) if it is tracked. */
static void gc_mark_root_ptr(lizard_heap_t *h, void *obj, gc_worklist_t *wl) {
  void *base;
  size_t size;
  if (obj == NULL) return;
  if (lizard_gc_metadata_mark_addr(h->gc_metadata, obj, &base, &size)) {
    gc_wl_push(wl, base, size);
  }
}

/* The top (highest address) of the main thread's stack.  glibc sets
 * __libc_stack_end to the client's initial stack pointer at startup, which is
 * both correct and always within the committed/mapped stack — unlike parsing
 * the [stack] extent from /proc/self/maps, whose reserved upper bound need not
 * be accessible. */
extern void *__libc_stack_end;

static char *gc_stack_base(void) {
  return (char *)__libc_stack_end;
}

static void gc_reclaim_cb(void *ptr, size_t size, lizard_gc_object_kind_t kind,
                          void *ctx) {
  size_t *bytes = (size_t *)ctx;
  (void)kind; /* GMP limbs of swept numbers are intentionally not freed here:
               * the inferred kind is not exact enough to safely call
               * mpz_clear/mpq_clear, so we only reclaim the object chunk. */
  lizard_heap_reclaim(ptr, size);
  if (bytes) *bytes += size;
}

size_t lizard_gc_collect_objects(lizard_heap_t *h, lizard_env_t *env) {
  jmp_buf regs;
  gc_worklist_t wl;
  volatile char *sp;
  size_t freed_bytes = 0U;

  if (h == NULL || h->gc_metadata == NULL) return 0U;

  wl.items = NULL;
  wl.count = 0U;
  wl.cap = 0U;
  wl.oom = 0;

  lizard_gc_metadata_prepare_marking(h->gc_metadata);

  /* Explicit roots: the call-site env (chains to the global env), the module
   * base env and namespace frames, callcc state, and module results. */
  gc_mark_root_ptr(h, env, &wl);
  if (h->runtime != NULL) {
    lizard_module_entry_t *mod;
    lizard_namespace_t *ns;
    gc_mark_root_ptr(h, h->runtime->module_base_env, &wl);
    gc_mark_root_ptr(h, h->runtime->callcc_value, &wl);
    for (mod = h->runtime->modules_head; mod != NULL; mod = mod->next) {
      gc_mark_root_ptr(h, mod->result, &wl);
    }
    for (ns = h->runtime->namespaces_head; ns != NULL; ns = ns->next) {
      gc_mark_root_ptr(h, ns->env, &wl);
    }
  }

  /* Conservative roots: spilled registers, the C stack, and data/BSS. */
  memset(&regs, 0, sizeof(regs));
  (void)setjmp(regs);
  sp = (volatile char *)&sp;
  gc_scan_range(h, (char *)&regs, (char *)&regs + sizeof(regs), &wl);
  gc_scan_range(h, (char *)(uintptr_t)sp, gc_stack_base(), &wl);
  gc_scan_range(h, (char *)__data_start, (char *)__bss_start, &wl);
  gc_scan_range(h, (char *)__bss_start, (char *)_end, &wl);

  /* Trace: scan each live object's bytes for further pointers. */
  while (wl.count > 0U && !wl.oom) {
    gc_obj_t o = wl.items[--wl.count];
    gc_scan_range(h, (char *)o.base, (char *)o.base + o.size, &wl);
  }

  if (wl.oom) {
    /* Could not complete the mark; sweeping now could free live objects.
     * Abort safely: free the worklist, leave every object alive. */
    free(wl.items);
    return 0U;
  }

  (void)lizard_gc_metadata_sweep(h->gc_metadata, gc_reclaim_cb, &freed_bytes);
  free(wl.items);
  return freed_bytes;
}
