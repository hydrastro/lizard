#ifndef ID_OBSERVE_H
#define ID_OBSERVE_H

/* id_observe — by-observation identity types (phase 14c).
 *
 * In a "3rd generation" (higher-observational) type theory the identity type
 * Id_A(x, y) is not given by an interval or by a single `refl` constructor with
 * a J eliminator; instead it COMPUTES by recursion on the structure of the type
 * A.  Equality is observed, not postulated.  This module is a small, totally
 * self-contained, terminating reduction system that realises that semantics on
 * the simply-typed + inductive + universe fragment:
 *
 *     Id Bool true true         -->  Unit          (true vs false  -->  Empty)
 *     Id Nat  (succ m) (succ n) -->  Id Nat m n     (0/0 --> Unit, 0/succ --> Empty)
 *     Id (A * B) (a,b) (c,d)    -->  (Id A a c) * (Id B b d)        [componentwise]
 *     Id (A -> B) f g           -->  Pi z:A. Id B (f z) (g z)       [pointwise: funext]
 *     Id U A B                  -->  Equiv A B                      [univalence recipe]
 *     transport P refl x        -->  x
 *
 * The point of the duality framework: Id is the OBSERVATION side, and each rule
 * is "Id meets a type-former, and reduces by that former's structural rule".
 * This is exactly the rule set that an Id agent and the type-former agents must
 * implement once they are wired into the interaction net — so this module is the
 * executable specification / oracle for that future net integration, runnable
 * and checkable on its own today.  The kernel's kt_whnf does NOT reduce KT_ID
 * structurally, so there is no oracle for these rules inside the trusted kernel;
 * the checks in tests/id_observe_test.c are against hand-computed normal forms.
 *
 * Scope: the genuinely dependent cases (transport threaded through the second
 * component of a dependent Sigma, and Id over a dependent Pi whose codomain
 * family varies) are the documented next extension; here Sigma/Pi are the
 * non-dependent product/function, which already exhibit the componentwise and
 * pointwise computation that is the conceptual content.
 */

typedef enum {
  ID_BOOL, ID_NAT, ID_UNIT, ID_EMPTY, ID_U,    /* base types and the universe   */
  ID_PROD, ID_ARR, ID_PI, ID_ID, ID_EQUIV,     /* type formers                  */
  ID_TRUE, ID_FALSE, ID_ZERO, ID_SUCC, ID_STAR,/* canonical terms               */
  ID_PAIR, ID_LAM, ID_VAR, ID_APP,             /* pairs / functions / variables */
  ID_REFL, ID_TRANSP                           /* equality witnesses            */
} id_kind;

typedef struct id_node {
  id_kind kind;
  int idx;                  /* ID_VAR: de Bruijn index (0 = innermost binder) */
  struct id_node *a, *b, *c;
} id_node;

/* constructors — each returns a freshly heap-allocated, fully owned tree.
 * Children are taken by ownership (they become part of the returned tree). */
id_node *id_base(id_kind k);                 /* nullary: BOOL/NAT/UNIT/EMPTY/U/TRUE/FALSE/ZERO/STAR */
id_node *id_succ(id_node *n);
id_node *id_var(int i);
id_node *id_pair(id_node *x, id_node *y);
id_node *id_lam(id_node *body);
id_node *id_app(id_node *f, id_node *x);
id_node *id_prod(id_node *A, id_node *B);
id_node *id_arr(id_node *A, id_node *B);
id_node *id_pi(id_node *dom, id_node *body);
id_node *id_idty(id_node *A, id_node *x, id_node *y);   /* Id A x y */
id_node *id_equiv(id_node *A, id_node *B);
id_node *id_refl(id_node *x);
id_node *id_transp(id_node *P, id_node *p, id_node *x);  /* transport^P p x */

id_node *id_nat_lit(int n);                  /* convenience: succ^n zero */
id_node *id_copy(const id_node *t);          /* deep copy */
void     id_free(id_node *t);

id_node *id_nf(const id_node *t);            /* full normal form (a fresh tree) */
int      id_eq(const id_node *x, const id_node *y);     /* structural equality (1/0) */
void     id_print(const id_node *t);         /* human-readable, for diagnostics */

#endif /* ID_OBSERVE_H */
