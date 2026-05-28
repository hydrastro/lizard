/* src/tactics.h — Proof state and basic tactics (Track K).
 *
 * A proof state is a list of goals. Each goal has a context and a
 * type to be inhabited. Tactics transform the proof state by
 * solving goals or breaking them into subgoals.
 *
 * The system produces kernel terms (kterm_t) that can be verified
 * by the trusted kernel independently.
 */
#ifndef LIZARD_TACTICS_H
#define LIZARD_TACTICS_H

#include "kernel.h"

/* ---- goal ---- */

typedef struct proof_goal {
  int id;
  kctx_t *ctx;            /* local context (assumptions) */
  kterm_t *type;           /* the type to inhabit */
  kterm_t *solution;       /* NULL until solved */
  struct proof_goal *next;
} proof_goal_t;

/* ---- proof state ---- */

typedef struct {
  proof_goal_t *goals;     /* linked list of open goals */
  int next_goal_id;
  kterm_t *result;          /* final proof term (NULL until QED) */
  lizard_heap_t *heap;
} proof_state_t;

/* ---- API ---- */

/* Create a new proof state with one goal. */
proof_state_t *proof_begin(lizard_heap_t *heap, kctx_t *ctx, kterm_t *goal_type);

/* Get the current (first unsolved) goal. */
proof_goal_t *proof_current_goal(proof_state_t *ps);

/* Count remaining unsolved goals. */
int proof_open_goals(proof_state_t *ps);

/* Tactic: intro — for a Pi goal, introduce the variable and produce
 * a subgoal for the body. Returns 0 on success, -1 on failure. */
int tactic_intro(proof_state_t *ps, const char *name);

/* Tactic: exact — provide an exact term for the current goal.
 * Returns 0 on success, -1 on type mismatch. */
int tactic_exact(proof_state_t *ps, kterm_t *term);

/* Tactic: reflexivity — solve an Id goal where both sides are equal.
 * Returns 0 on success, -1 if goal is not Id a a. */
int tactic_reflexivity(proof_state_t *ps);

/* Tactic: apply — given f : A → B and goal B, produce subgoal A.
 * Returns 0 on success. */
int tactic_apply(proof_state_t *ps, kterm_t *f);

/* Tactic: assumption — search the context for a hypothesis whose
 * type matches the goal. Returns 0 on success, -1 if not found. */
int tactic_assumption(proof_state_t *ps);

/* QED: check that all goals are solved and return the proof term.
 * Returns the term on success, NULL if goals remain. */
kterm_t *proof_qed(proof_state_t *ps);

/* Print the current proof state. */
void proof_state_fprint(FILE *fp, proof_state_t *ps);

#endif /* LIZARD_TACTICS_H */
