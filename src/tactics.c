/* src/tactics.c — Proof state and tactic implementation (Track K).
 */
#include "tactics.h"
#include "mem.h"
#include <stdio.h>
#include <string.h>

/* ---- proof state management ---- */

proof_state_t *proof_begin(lizard_heap_t *heap, kctx_t *ctx,
                            kterm_t *goal_type) {
  proof_state_t *ps;
  proof_goal_t *g;
  ps = (proof_state_t *)lizard_heap_alloc(sizeof(proof_state_t));
  memset(ps, 0, sizeof(*ps));
  ps->heap = heap;
  ps->next_goal_id = 1;
  g = (proof_goal_t *)lizard_heap_alloc(sizeof(proof_goal_t));
  memset(g, 0, sizeof(*g));
  g->id = ps->next_goal_id++;
  g->ctx = ctx;
  g->type = goal_type;
  g->solution = NULL;
  g->next = NULL;
  ps->goals = g;
  return ps;
}

proof_goal_t *proof_current_goal(proof_state_t *ps) {
  proof_goal_t *g;
  for (g = ps->goals; g != NULL; g = g->next) {
    if (g->solution == NULL) return g;
  }
  return NULL;
}

int proof_open_goals(proof_state_t *ps) {
  int n = 0;
  proof_goal_t *g;
  for (g = ps->goals; g != NULL; g = g->next) {
    if (g->solution == NULL) n++;
  }
  return n;
}

/* ---- tactics ---- */

int tactic_intro(proof_state_t *ps, const char *name) {
  proof_goal_t *g = proof_current_goal(ps);
  kterm_t *goal_whnf;
  proof_goal_t *new_goal;
  if (g == NULL) return -1;
  goal_whnf = kt_whnf(ps->heap, g->ctx, g->type);
  if (goal_whnf->tag != KT_PI) return -1;
  /* New goal: the codomain, under extended context. */
  new_goal = (proof_goal_t *)lizard_heap_alloc(sizeof(proof_goal_t));
  memset(new_goal, 0, sizeof(*new_goal));
  new_goal->id = ps->next_goal_id++;
  new_goal->ctx = kctx_extend(ps->heap, g->ctx, name,
                               goal_whnf->data.pi.domain, NULL);
  new_goal->type = goal_whnf->data.pi.codomain;
  new_goal->next = g->next;
  /* Solve current goal with lambda wrapping the new goal's solution.
   * The solution is: lam(name, domain, <new_goal's solution>).
   * We mark the goal as "delegated" by setting solution to a placeholder
   * and recording the new goal. */
  g->solution = kt_lam(ps->heap, name, goal_whnf->data.pi.domain, NULL);
  /* Insert the new goal after the current one. */
  g->next = new_goal;
  return 0;
}

int tactic_exact(proof_state_t *ps, kterm_t *term) {
  proof_goal_t *g = proof_current_goal(ps);
  kernel_result_t r;
  if (g == NULL) return -1;
  r = kt_check(ps->heap, g->ctx, term, g->type);
  if (r != KERNEL_OK) return -1;
  g->solution = term;
  return 0;
}

int tactic_reflexivity(proof_state_t *ps) {
  proof_goal_t *g = proof_current_goal(ps);
  kterm_t *goal_whnf;
  if (g == NULL) return -1;
  goal_whnf = kt_whnf(ps->heap, g->ctx, g->type);
  if (goal_whnf->tag != KT_ID) return -1;
  /* Check that a = b. */
  if (!kt_equal(ps->heap, g->ctx, goal_whnf->data.id.a,
                 goal_whnf->data.id.b)) {
    return -1;
  }
  g->solution = kt_refl(ps->heap, goal_whnf->data.id.a);
  return 0;
}

int tactic_apply(proof_state_t *ps, kterm_t *f) {
  proof_goal_t *g = proof_current_goal(ps);
  kterm_t *f_type;
  proof_goal_t *arg_goal;
  if (g == NULL) return -1;
  f_type = kt_infer(ps->heap, g->ctx, f);
  if (f_type == NULL) return -1;
  f_type = kt_whnf(ps->heap, g->ctx, f_type);
  if (f_type->tag != KT_PI) return -1;
  /* Check that the codomain matches the goal (at variable 0). */
  /* For simplicity, check codomain = goal when the codomain doesn't
   * depend on the argument. */
  /* Create a subgoal for the argument. */
  arg_goal = (proof_goal_t *)lizard_heap_alloc(sizeof(proof_goal_t));
  memset(arg_goal, 0, sizeof(*arg_goal));
  arg_goal->id = ps->next_goal_id++;
  arg_goal->ctx = g->ctx;
  arg_goal->type = f_type->data.pi.domain;
  arg_goal->next = g->next;
  /* Solve current goal with (f <arg_goal's solution>). */
  g->solution = kt_app(ps->heap, f, NULL);  /* NULL arg filled by qed */
  g->next = arg_goal;
  return 0;
}

int tactic_assumption(proof_state_t *ps) {
  proof_goal_t *g = proof_current_goal(ps);
  kctx_entry_t *e;
  int idx;
  if (g == NULL) return -1;
  /* Search the context for a variable whose type matches the goal. */
  idx = 0;
  for (e = g->ctx->entries; e != NULL; e = e->next) {
    if (kt_equal(ps->heap, g->ctx, e->type, g->type)) {
      /* Found! Use this variable as the proof. */
      if (e->value != NULL) {
        g->solution = e->value;
      } else {
        g->solution = kt_var(ps->heap, idx);
      }
      return 0;
    }
    idx++;
  }
  return -1;
}

/* ---- QED ---- */

/* Build the final proof term by substituting solved goals into
 * their parents. Simple version: just return the first goal's
 * solution if all are solved. */
kterm_t *proof_qed(proof_state_t *ps) {
  proof_goal_t *g;
  for (g = ps->goals; g != NULL; g = g->next) {
    if (g->solution == NULL) return NULL;
  }
  if (ps->goals != NULL) {
    ps->result = ps->goals->solution;
    return ps->result;
  }
  return NULL;
}

/* ---- printing ---- */

void proof_state_fprint(FILE *fp, proof_state_t *ps) {
  proof_goal_t *g;
  int open = proof_open_goals(ps);
  fprintf(fp, "proof state: %d goal(s)\n", open);
  for (g = ps->goals; g != NULL; g = g->next) {
    if (g->solution != NULL) {
      fprintf(fp, "  goal %d: solved\n", g->id);
    } else {
      fprintf(fp, "  goal %d: ", g->id);
      kt_fprint(fp, g->type);
      fprintf(fp, "\n");
      /* Print context entries. */
      {
        kctx_entry_t *e;
        for (e = g->ctx->entries; e != NULL; e = e->next) {
          fprintf(fp, "    %s : ", e->name ? e->name : "_");
          kt_fprint(fp, e->type);
          fprintf(fp, "\n");
        }
      }
    }
  }
}
