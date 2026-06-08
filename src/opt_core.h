/* opt_core.h — optimal reduction (Asperti-Guerrini sharing graphs), staged.
 *
 * This unit provides three independently-validated pieces toward Levy-optimal
 * reduction for the pure lambda-calculus, the missing ingredient that would let
 * the engine share reduction inside a duplicated lambda (the work-optimality gap
 * measured by tests/ic_sharing_test.c):
 *
 *   1. A reference normal-order beta-normaliser (the ground-truth oracle for the
 *      lambda fragment; the kernel does not normalise open lambda-terms).
 *   2. The Asperti-Guerrini optimal-reduction interaction ENGINE: indexed agents
 *      lambda_i, @_i, fan delta_i, croissant cap_i, bracket cup_i, eraser, with
 *      the exact annihilation + propagation rules from the literature.
 *   3. A term -> net encoding + read-back, validated against the reference on a
 *      fragment (it correctly reduces duplicated functions). The fully general
 *      index-correct encoding (matched brackets and croissants for arbitrary
 *      nesting) is the classic hard part and is NOT claimed here.
 *
 * Refs: Levy 1980; Lamping, "An algorithm for optimal lambda calculus reduction"
 * (POPL 1990); Asperti & Guerrini, "The Optimal Implementation of Functional
 * Programming Languages" (CUP 1998), pp. 39-42; Salikhmetov, "Token-passing
 * Optimal Reduction with Embedded Read-back" (arXiv:1609.03644) for the rule set.
 */
#ifndef OPT_CORE_H
#define OPT_CORE_H

/* ---- lambda terms (de Bruijn) ---- */
typedef enum { LC_VAR, LC_LAM, LC_APP } lc_tag;
typedef struct lc_term {
  lc_tag tag;
  int idx;                 /* de Bruijn index for LC_VAR */
  struct lc_term *f, *a;   /* LC_LAM: f = body; LC_APP: f = fun, a = arg */
} lc_term;

lc_term *lc_var(int idx);
lc_term *lc_lam(lc_term *body);
lc_term *lc_app(lc_term *fun, lc_term *arg);

/* reference normal-order normaliser (ground truth). Returns a fresh term. */
lc_term *lc_nf_ref(const lc_term *t);
/* structural (alpha-) equality on de Bruijn terms */
int lc_eq(const lc_term *a, const lc_term *b);

/* ---- AG optimal-reduction engine ---- */

/* Rule-level self test of the interaction engine on hand-built active pairs.
 * Returns the number of failed checks (0 = all rules behave per the literature). */
int opt_engine_selftest(void);

/* Encode `t`, reduce to normal form with the optimal engine, and read back.
 * On success returns the read-back normal form and sets *interactions.
 * If a same-index/different-type ("uncovered") active pair arises -- the marker
 * that the term needs the full bracket oracle -- returns NULL and sets
 * *uncovered to the number of such pairs. */
lc_term *opt_normalize(const lc_term *t, long *interactions, int *uncovered);

#endif
