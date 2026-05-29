/* src/prims_kernel_proof.c -- optional proof/tactic primitives.
 * Tactics are an untrusted front-end over kernel terms, not privileged.
 */
#include "primitives.h"
#include "errors.h"
#include "kernel.h"
#include "kernel_sexp.h"
#include "runtime.h"
#include "mem.h"

#include <stdio.h>
#include <string.h>

#include "tactics.h"

static int kernel_single_arg(lz_list_t *args) {
  return args->head != args->nil && args->head->next == args->nil;
}


/* ============================================================
 * Track K: Tactic primitives.
 * ============================================================ */

static proof_state_t *fallback_current_proof = NULL;

static proof_state_t *current_proof_get(lizard_heap_t *heap) {
  if (heap != NULL && heap->runtime != NULL) {
    return (proof_state_t *)heap->runtime->kernel_current_proof;
  }
  return fallback_current_proof;
}

static void current_proof_set(lizard_heap_t *heap, proof_state_t *proof) {
  if (heap != NULL && heap->runtime != NULL) {
    heap->runtime->kernel_current_proof = proof;
  } else {
    fallback_current_proof = proof;
  }
}

lizard_ast_node_t *lizard_primitive_begin_proof(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  lizard_ast_node_t *type_expr;
  kterm_t *goal_type;
  kctx_t *ctx;
  (void)env;
  if (!kernel_single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  type_expr = ((lizard_ast_list_node_t *)args->head)->ast;
  goal_type = lizard_kernel_sexp_to_kterm(heap, type_expr);
  if (goal_type == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  ctx = kctx_create(heap);
  current_proof_set(heap, proof_begin(heap, ctx, goal_type));
  printf("Proof started.\n");
  proof_state_fprint(stdout, current_proof_get(heap));
  return lizard_make_bool(heap, 1);
}

lizard_ast_node_t *lizard_primitive_proof_state(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  (void)args; (void)env;
  if (current_proof_get(heap) == NULL) { printf("No active proof.\n"); }
  else { proof_state_fprint(stdout, current_proof_get(heap)); }
  return lizard_make_nil(heap);
}

lizard_ast_node_t *lizard_primitive_tactic_intro(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *name_node;
  const char *name;
  int r;
  (void)env;
  if (current_proof_get(heap) == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  if (!kernel_single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  name_node = ((lizard_ast_list_node_t *)args->head)->ast;
  name = (name_node->type == AST_STRING) ? name_node->data.string
       : (name_node->type == AST_SYMBOL) ? name_node->data.variable : "x";
  r = tactic_intro(current_proof_get(heap), name);
  if (r < 0) { printf("tactic-intro: goal is not a Pi type.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof_get(heap));
  return lizard_make_bool(heap, 1);
}

lizard_ast_node_t *lizard_primitive_tactic_exact(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  lizard_ast_node_t *expr;
  kterm_t *term;
  int r;
  (void)env;
  if (current_proof_get(heap) == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  if (!kernel_single_arg(args)) return lizard_make_error(heap, LIZARD_ERROR_PREDICATE_ARGC);
  expr = ((lizard_ast_list_node_t *)args->head)->ast;
  term = lizard_kernel_sexp_to_kterm(heap, expr);
  if (term == NULL) { printf("tactic-exact: cannot parse term.\n"); return lizard_make_bool(heap, 0); }
  r = tactic_exact(current_proof_get(heap), term);
  if (r < 0) { printf("tactic-exact: type mismatch.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof_get(heap));
  return lizard_make_bool(heap, 1);
}

lizard_ast_node_t *lizard_primitive_tactic_refl(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof_get(heap) == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_reflexivity(current_proof_get(heap));
  if (r < 0) { printf("tactic-refl: goal is not Id a a.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof_get(heap));
  return lizard_make_bool(heap, 1);
}

lizard_ast_node_t *lizard_primitive_qed(lz_list_t *args,
                                         lizard_env_t *env,
                                         lizard_heap_t *heap) {
  kterm_t *proof;
  (void)args; (void)env;
  if (current_proof_get(heap) == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  proof = proof_qed(current_proof_get(heap));
  if (proof == NULL) {
    printf("qed: %d goal(s) remaining.\n", proof_open_goals(current_proof_get(heap)));
    return lizard_make_bool(heap, 0);
  }
  printf("Qed! Proof term: ");
  kt_fprint(stdout, proof);
  printf("\n");
  current_proof_set(heap, NULL);
  return lizard_make_bool(heap, 1);
}

/* (tactic-assumption) — search context for a matching hypothesis. */
lizard_ast_node_t *lizard_primitive_tactic_assumption(lz_list_t *args,
                                                       lizard_env_t *env,
                                                       lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof_get(heap) == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_assumption(current_proof_get(heap));
  if (r < 0) {
    printf("tactic-assumption: no matching hypothesis found.\n");
    return lizard_make_bool(heap, 0);
  }
  proof_state_fprint(stdout, current_proof_get(heap));
  return lizard_make_bool(heap, 1);
}

/* (tactic-simpl) — reduce goal to WHNF. */
lizard_ast_node_t *lizard_primitive_tactic_simpl(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof_get(heap) == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_simpl(current_proof_get(heap));
  if (r < 0) { printf("tactic-simpl: no goal.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof_get(heap));
  return lizard_make_bool(heap, 1);
}

/* (tactic-split) — split a Sigma goal into two subgoals. */
lizard_ast_node_t *lizard_primitive_tactic_split(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof_get(heap) == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_split(current_proof_get(heap));
  if (r < 0) { printf("tactic-split: goal is not a Sigma type.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof_get(heap));
  return lizard_make_bool(heap, 1);
}

/* (tactic-left) — choose left for Sum goal. */
lizard_ast_node_t *lizard_primitive_tactic_left(lz_list_t *args,
                                                 lizard_env_t *env,
                                                 lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof_get(heap) == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_left(current_proof_get(heap));
  if (r < 0) { printf("tactic-left: goal is not a Sum type.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof_get(heap));
  return lizard_make_bool(heap, 1);
}

/* (tactic-right) — choose right for Sum goal. */
lizard_ast_node_t *lizard_primitive_tactic_right(lz_list_t *args,
                                                  lizard_env_t *env,
                                                  lizard_heap_t *heap) {
  int r;
  (void)args; (void)env;
  if (current_proof_get(heap) == NULL) return lizard_make_error(heap, LIZARD_ERROR_USER);
  r = tactic_right(current_proof_get(heap));
  if (r < 0) { printf("tactic-right: goal is not a Sum type.\n"); return lizard_make_bool(heap, 0); }
  proof_state_fprint(stdout, current_proof_get(heap));
  return lizard_make_bool(heap, 1);
}

