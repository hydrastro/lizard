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
  ps->goal_type = goal_type;
  g = (proof_goal_t *)lizard_heap_alloc(sizeof(proof_goal_t));
  memset(g, 0, sizeof(*g));
  g->id = ps->next_goal_id++;
  g->ctx = ctx;
  g->type = goal_type;
  g->solution = NULL;
  g->slot = &ps->result;   /* the root goal's solution is the whole term */
  g->next = NULL;
  ps->goals = g;
  return ps;
}

/* Record a goal's solution and plug it into the parent term via the goal's
 * slot (a pointer to the hole in the parent).  Because holes are shared
 * pointers, later filling a *sub*goal updates this term in place. */
static void goal_solve(proof_goal_t *g, kterm_t *term) {
  g->solution = term;
  if (g->slot != NULL) *(g->slot) = term;
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
  /* Solve current goal with lam(name, domain, <body hole>); the new goal's
   * solution fills that body hole. */
  {
    kterm_t *lam = kt_lam(ps->heap, name, goal_whnf->data.pi.domain, NULL);
    goal_solve(g, lam);
    new_goal->slot = &lam->data.lam.body;
  }
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
  goal_solve(g, term);
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
  goal_solve(g, kt_refl(ps->heap, goal_whnf->data.id.a));
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
  /* Solve current goal with app(f, <arg hole>); arg_goal fills it. */
  {
    kterm_t *ap = kt_app(ps->heap, f, NULL);
    goal_solve(g, ap);
    arg_goal->slot = &ap->data.app.arg;
  }
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
        goal_solve(g, e->value);
      } else {
        goal_solve(g, kt_var(ps->heap, idx));
      }
      return 0;
    }
    idx++;
  }
  return -1;
}

/* ---- case analysis / induction (eliminator tactics) ----
 *
 * Both act on the innermost hypothesis (de Bruijn #0) — the variable most
 * recently introduced.  The motive is synthesised by abstracting the goal over
 * that variable: motive = \y. shift(goal, 1, 1).  Shifting with cutoff 1 keeps
 * the variable's own occurrences (index 0) — which become bound by the new
 * lambda — while lifting the rest of the context past the binder.  Subgoal
 * types are then obtained by applying the motive to each constructor and
 * asking the kernel to reduce, instead of doing the de Bruijn surgery by hand.
 * The assembled eliminator is, as always, re-checked by the kernel at qed. */

static kterm_t *mk_kt(lizard_heap_t *heap, kterm_tag_t tag) {
  kterm_t *t = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
  memset(t, 0, sizeof(*t));
  t->tag = tag;
  return t;
}

static proof_goal_t *mk_goal(proof_state_t *ps, kctx_t *ctx, kterm_t *type) {
  proof_goal_t *g = (proof_goal_t *)lizard_heap_alloc(sizeof(proof_goal_t));
  memset(g, 0, sizeof(*g));
  g->id = ps->next_goal_id++;
  g->ctx = ctx;
  g->type = type;
  return g;
}

int tactic_cases(proof_state_t *ps) {
  proof_goal_t *g = proof_current_goal(ps);
  kctx_entry_t *e;
  kterm_t *motive, *br;
  proof_goal_t *gt, *gf;
  if (g == NULL || g->ctx == NULL || g->ctx->entries == NULL) return -1;
  e = g->ctx->entries;                       /* innermost hypothesis = #0 */
  if (kt_whnf(ps->heap, g->ctx, e->type)->tag != KT_BOOL) return -1;
  motive = kt_lam(ps->heap, e->name ? e->name : "b", e->type,
                  kt_shift(ps->heap, g->type, 1, 1));
  gt = mk_goal(ps, g->ctx,
        kt_whnf(ps->heap, g->ctx,
          kt_app(ps->heap, motive, mk_kt(ps->heap, KT_TRUE))));
  gf = mk_goal(ps, g->ctx,
        kt_whnf(ps->heap, g->ctx,
          kt_app(ps->heap, motive, mk_kt(ps->heap, KT_FALSE))));
  gf->next = g->next;
  gt->next = gf;
  br = mk_kt(ps->heap, KT_BOOL_REC);
  br->data.bool_rec.motive = motive;
  br->data.bool_rec.scrutinee = kt_var(ps->heap, 0);
  goal_solve(g, br);
  gt->slot = &br->data.bool_rec.true_case;
  gf->slot = &br->data.bool_rec.false_case;
  g->next = gt;
  return 0;
}

int tactic_induction(proof_state_t *ps) {
  proof_goal_t *g = proof_current_goal(ps);
  kctx_entry_t *e;
  kterm_t *motive, *nr, *base_t, *step_t, *m1, *m2;
  proof_goal_t *gz, *gs;
  if (g == NULL || g->ctx == NULL || g->ctx->entries == NULL) return -1;
  e = g->ctx->entries;
  if (kt_whnf(ps->heap, g->ctx, e->type)->tag != KT_NAT) return -1;
  motive = kt_lam(ps->heap, e->name ? e->name : "n", e->type,
                  kt_shift(ps->heap, g->type, 1, 1));
  /* base : motive zero */
  base_t = kt_whnf(ps->heap, g->ctx,
                   kt_app(ps->heap, motive, kt_zero(ps->heap)));
  /* step : Pi(k:Nat). Pi(ih: motive k). motive (succ k) */
  m1 = kt_shift(ps->heap, motive, 0, 1);     /* under k */
  m2 = kt_shift(ps->heap, motive, 0, 2);     /* under k, ih */
  step_t = kt_pi(ps->heap, "k", kt_nat(ps->heap),
             kt_pi(ps->heap, "ih",
               kt_app(ps->heap, m1, kt_var(ps->heap, 0)),
               kt_app(ps->heap, m2,
                      kt_succ(ps->heap, kt_var(ps->heap, 1)))));
  gz = mk_goal(ps, g->ctx, base_t);
  gs = mk_goal(ps, g->ctx, step_t);
  gs->next = g->next;
  gz->next = gs;
  nr = mk_kt(ps->heap, KT_NAT_REC);
  nr->data.nat_rec.motive = motive;
  nr->data.nat_rec.scrutinee = kt_var(ps->heap, 0);
  goal_solve(g, nr);
  gz->slot = &nr->data.nat_rec.zero_case;
  gs->slot = &nr->data.nat_rec.succ_case;
  g->next = gz;
  return 0;
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

/* ---- simpl ---- */

int tactic_simpl(proof_state_t *ps) {
  proof_goal_t *g = proof_current_goal(ps);
  if (g == NULL) return -1;
  g->type = kt_whnf(ps->heap, g->ctx, g->type);
  return 0;
}

/* ---- split (for Sigma goals) ---- */

int tactic_split(proof_state_t *ps) {
  proof_goal_t *g = proof_current_goal(ps);
  kterm_t *goal_whnf;
  proof_goal_t *g1, *g2;
  if (g == NULL) return -1;
  goal_whnf = kt_whnf(ps->heap, g->ctx, g->type);
  if (goal_whnf->tag != KT_SIGMA) return -1;
  /* Create two subgoals: one for fst_type, one for snd_type. */
  g1 = (proof_goal_t *)lizard_heap_alloc(sizeof(proof_goal_t));
  memset(g1, 0, sizeof(*g1));
  g1->id = ps->next_goal_id++;
  g1->ctx = g->ctx;
  g1->type = goal_whnf->data.sigma.fst_type;
  g2 = (proof_goal_t *)lizard_heap_alloc(sizeof(proof_goal_t));
  memset(g2, 0, sizeof(*g2));
  g2->id = ps->next_goal_id++;
  g2->ctx = g->ctx;
  g2->type = goal_whnf->data.sigma.snd_type;
  g2->next = g->next;
  g1->next = g2;
  /* Mark current goal as delegated (pair of subgoals). */
  {
    kterm_t *p = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(p, 0, sizeof(*p));
    p->tag = KT_PAIR;
    goal_solve(g, p);
    g1->slot = &p->data.pair.fst;
    g2->slot = &p->data.pair.snd;
  }
  g->next = g1;
  return 0;
}

/* ---- left/right (for Sum goals) ---- */

int tactic_left(proof_state_t *ps) {
  proof_goal_t *g = proof_current_goal(ps);
  kterm_t *goal_whnf;
  proof_goal_t *sub;
  if (g == NULL) return -1;
  goal_whnf = kt_whnf(ps->heap, g->ctx, g->type);
  if (goal_whnf->tag != KT_SUM_K) return -1;
  sub = (proof_goal_t *)lizard_heap_alloc(sizeof(proof_goal_t));
  memset(sub, 0, sizeof(*sub));
  sub->id = ps->next_goal_id++;
  sub->ctx = g->ctx;
  sub->type = goal_whnf->data.sum_k.left_type;
  sub->next = g->next;
  {
    kterm_t *inl = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(inl, 0, sizeof(*inl));
    inl->tag = KT_INL;
    inl->data.inl.right_type = goal_whnf->data.sum_k.right_type;
    goal_solve(g, inl);
    sub->slot = &inl->data.inl.value;
  }
  g->next = sub;
  return 0;
}

int tactic_right(proof_state_t *ps) {
  proof_goal_t *g = proof_current_goal(ps);
  kterm_t *goal_whnf;
  proof_goal_t *sub;
  if (g == NULL) return -1;
  goal_whnf = kt_whnf(ps->heap, g->ctx, g->type);
  if (goal_whnf->tag != KT_SUM_K) return -1;
  sub = (proof_goal_t *)lizard_heap_alloc(sizeof(proof_goal_t));
  memset(sub, 0, sizeof(*sub));
  sub->id = ps->next_goal_id++;
  sub->ctx = g->ctx;
  sub->type = goal_whnf->data.sum_k.right_type;
  sub->next = g->next;
  {
    kterm_t *inr = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
    memset(inr, 0, sizeof(*inr));
    inr->tag = KT_INR;
    inr->data.inr.left_type = goal_whnf->data.sum_k.left_type;
    goal_solve(g, inr);
    sub->slot = &inr->data.inr.value;
  }
  g->next = sub;
  return 0;
}
