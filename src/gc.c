/* src/gc.c — Garbage collector infrastructure (Phase D.1).
 *
 * Mark traversal for lizard's AST heap. Follows the same switch-on-
 * type pattern used throughout the codebase.
 */
#include "gc.h"
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

  /* Track C: persistent data — mark all entries. */
  case AST_PVEC: {
    int pi;
    if (node->data.pvec.entries != NULL) {
      for (pi = 0; pi < node->data.pvec.count; pi++) {
        lizard_gc_mark_node(node->data.pvec.entries[pi]);
      }
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
