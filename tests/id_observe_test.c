/* id_observe_test.c — executable specification for the by-observation identity
 * reduction system (phase 14c).  Each case reduces an  Id A x y  (or a transport)
 * to normal form and checks it against a hand-computed expected normal form.
 * There is no kernel oracle for these rules (kt_whnf keeps Id canonical), so the
 * expected results are the written specification of the semantics.
 *
 * Build (standalone):
 *   cc -std=c89 -O2 -Isrc tests/id_observe_test.c src/id_observe.c -o id_observe_test
 */
#include "id_observe.h"
#include <stdio.h>

static long fails = 0, checks = 0;

static void check(const char *name, id_node *term, id_node *want) {
  id_node *got = id_nf(term);
  checks++;
  if (!id_eq(got, want)) {
    fails++;
    printf("  FAIL %s\n    got:  ", name); id_print(got);
    printf("\n    want: "); id_print(want); printf("\n");
  }
  id_free(term); id_free(got); id_free(want);
}

/* small builders to keep the cases readable */
static id_node *Bool_(void)  { return id_base(ID_BOOL); }
static id_node *Nat_(void)   { return id_base(ID_NAT); }
static id_node *Unit_(void)  { return id_base(ID_UNIT); }
static id_node *Empty_(void) { return id_base(ID_EMPTY); }
static id_node *U_(void)     { return id_base(ID_U); }
static id_node *T_(void)     { return id_base(ID_TRUE); }
static id_node *F_(void)     { return id_base(ID_FALSE); }
static id_node *Star_(void)  { return id_base(ID_STAR); }
/* the Boolean negation map  lam b. if b then false else true  (an equivalence) */
static id_node *Not_(void)   { return id_lam(id_if(id_var(0), id_base(ID_FALSE), id_base(ID_TRUE))); }

int main(void) {
  /* ---- inductive types: Id computes by structural recursion (observation) ---- */
  check("Id Bool true true",   id_idty(Bool_(), T_(), T_()), Unit_());
  check("Id Bool true false",  id_idty(Bool_(), T_(), F_()), Empty_());
  check("Id Bool false false", id_idty(Bool_(), F_(), F_()), Unit_());

  check("Id Nat 0 0", id_idty(Nat_(), id_nat_lit(0), id_nat_lit(0)), Unit_());
  check("Id Nat 2 2", id_idty(Nat_(), id_nat_lit(2), id_nat_lit(2)), Unit_());
  check("Id Nat 2 3", id_idty(Nat_(), id_nat_lit(2), id_nat_lit(3)), Empty_());
  check("Id Nat 3 2", id_idty(Nat_(), id_nat_lit(3), id_nat_lit(2)), Empty_());
  check("Id Nat 5 5", id_idty(Nat_(), id_nat_lit(5), id_nat_lit(5)), Unit_());

  check("Id Unit * *", id_idty(Unit_(), Star_(), Star_()), Unit_());

  /* ---- product: componentwise (Id over A*B is the product of the Ids) ---- */
  check("Id (Bool*Nat) (true,2) (true,2)",
        id_idty(id_prod(Bool_(), Nat_()), id_pair(T_(), id_nat_lit(2)), id_pair(T_(), id_nat_lit(2))),
        id_prod(Unit_(), Unit_()));
  check("Id (Bool*Nat) (true,2) (false,2)",
        id_idty(id_prod(Bool_(), Nat_()), id_pair(T_(), id_nat_lit(2)), id_pair(F_(), id_nat_lit(2))),
        id_prod(Empty_(), Unit_()));
  check("Id (Bool*Nat) (true,2) (true,3)",
        id_idty(id_prod(Bool_(), Nat_()), id_pair(T_(), id_nat_lit(2)), id_pair(T_(), id_nat_lit(3))),
        id_prod(Unit_(), Empty_()));

  /* ---- function: pointwise / functional extensionality, with beta under Pi ---- */
  check("Id (Unit->Bool) (lam true) (lam true)",
        id_idty(id_arr(Unit_(), Bool_()), id_lam(T_()), id_lam(T_())),
        id_pi(Unit_(), Unit_()));
  check("Id (Unit->Bool) (lam true) (lam false)",
        id_idty(id_arr(Unit_(), Bool_()), id_lam(T_()), id_lam(F_())),
        id_pi(Unit_(), Empty_()));
  /* identity functions: the body is applied to the bound variable, so the inner
   * Id is over a variable and stays neutral -> Pi z:Nat. Id Nat z z */
  check("Id (Nat->Nat) (lam #0) (lam #0)",
        id_idty(id_arr(Nat_(), Nat_()), id_lam(id_var(0)), id_lam(id_var(0))),
        id_pi(Nat_(), id_idty(Nat_(), id_var(0), id_var(0))));

  /* ---- universe: univalence recipe (paths in U are equivalences) ---- */
  check("Id U Bool Bool", id_idty(U_(), Bool_(), Bool_()), id_equiv(Bool_(), Bool_()));
  check("Id U Bool Nat",  id_idty(U_(), Bool_(), Nat_()),  id_equiv(Bool_(), Nat_()));

  /* ---- transport along refl is the identity ---- */
  check("transport (lam Nat) (refl 0) (succ 0)",
        id_transp(id_lam(Nat_()), id_refl(id_nat_lit(0)), id_nat_lit(1)),
        id_nat_lit(1));

  /* ---- Phase 15: transport reduces by the structure of its type family ---- */
  /* Bool elimination (needed below): if fires on a concrete scrutinee */
  check("if true 1 2",  id_if(T_(), id_nat_lit(1), id_nat_lit(2)), id_nat_lit(1));
  check("if false 1 2", id_if(F_(), id_nat_lit(1), id_nat_lit(2)), id_nat_lit(2));
  check("not true",  id_app(Not_(), T_()), F_());
  check("not false", id_app(Not_(), F_()), T_());

  /* a CONSTANT family ignores the path: transport is the identity even along a
   * non-trivial (univalence) path.  This is why transport never intrudes in the
   * simply-typed fragment. */
  check("transport (lam _. Nat) (ua not) (succ 0)  [constant family]",
        id_transp(id_lam(Nat_()), id_ua(Not_()), id_nat_lit(1)),
        id_nat_lit(1));

  /* the IDENTITY family over the universe is the univalent case:
   *   transport^(lam X. X) (ua f) x  =  f x
   * transporting a Bool along the negation equivalence flips it. */
  check("transport (lam X. X) refl true",
        id_transp(id_lam(id_var(0)), id_refl(T_()), T_()), T_());
  check("transport (lam X. X) (ua not) true   [univalence: negation]",
        id_transp(id_lam(id_var(0)), id_ua(Not_()), T_()), F_());
  check("transport (lam X. X) (ua not) false  [univalence: negation]",
        id_transp(id_lam(id_var(0)), id_ua(Not_()), F_()), T_());

  /* ---- DEPENDENT function types: Id over a Pi (dependent funext) ---- */
  /* a Pi whose codomain ignores the argument degenerates to the arrow case */
  check("Id (Pi Unit. Bool) (lam true) (lam true)",
        id_idty(id_pi(Unit_(), Bool_()), id_lam(T_()), id_lam(T_())),
        id_pi(Unit_(), Unit_()));
  check("Id (Pi Unit. Bool) (lam true) (lam false)",
        id_idty(id_pi(Unit_(), Bool_()), id_lam(T_()), id_lam(F_())),
        id_pi(Unit_(), Empty_()));
  /* identity functions at a Pi type: body applied to the bound var, stays neutral */
  check("Id (Pi Nat. Nat) (lam #0) (lam #0)",
        id_idty(id_pi(Nat_(), Nat_()), id_lam(id_var(0)), id_lam(id_var(0))),
        id_pi(Nat_(), id_idty(Nat_(), id_var(0), id_var(0))));
  /* a GENUINELY dependent codomain (if #0 then Unit else Unit): it is carried,
   * unshifted, into the resulting Id, and the applications reduce to star.
   * Result: Pi z:Bool. Id (if z then Unit else Unit) star star  (neutral body). */
  check("Id (Pi Bool. if #0 Unit Unit) (lam star)(lam star)",
        id_idty(id_pi(Bool_(), id_if(id_var(0), Unit_(), Unit_())),
                id_lam(Star_()), id_lam(Star_())),
        id_pi(Bool_(), id_idty(id_if(id_var(0), Unit_(), Unit_()), Star_(), Star_())));

  /* ---- transport in a PRODUCT family is componentwise (HoTT Thm 2.6.4) ---- */
  /* transport^(lam X. X*X) (ua not) (true,false) = (not true, not false) = (false,true) */
  check("transport (lam X. X*X) (ua not) (true,false)  [componentwise univalence]",
        id_transp(id_lam(id_prod(id_var(0), id_var(0))), id_ua(Not_()), id_pair(T_(), F_())),
        id_pair(F_(), T_()));
  /* mixed family lam X. X*Nat : first component transported, second is constant */
  check("transport (lam X. X*Nat) (ua not) (true,5)  [mixed family]",
        id_transp(id_lam(id_prod(id_var(0), Nat_())), id_ua(Not_()), id_pair(T_(), id_nat_lit(5))),
        id_pair(F_(), id_nat_lit(5)));

  /* ---- neutral cases: no canonicity on open terms (variables) ---- */
  check("Id Nat #0 #0 (neutral)",
        id_idty(Nat_(), id_var(0), id_var(0)),
        id_idty(Nat_(), id_var(0), id_var(0)));
  check("Id (Nat*Bool) (#0,true) (#0,false)  [neutral fst, concrete snd]",
        id_idty(id_prod(Nat_(), Bool_()), id_pair(id_var(0), T_()), id_pair(id_var(0), F_())),
        id_prod(id_idty(Nat_(), id_var(0), id_var(0)), Empty_()));

  /* ---- a couple of fully-concrete nested products ---- */
  check("Id (Nat*Nat) (1,2) (1,2)",
        id_idty(id_prod(Nat_(), Nat_()), id_pair(id_nat_lit(1), id_nat_lit(2)), id_pair(id_nat_lit(1), id_nat_lit(2))),
        id_prod(Unit_(), Unit_()));
  check("Id (Nat*Nat) (2,2) (1,2)",
        id_idty(id_prod(Nat_(), Nat_()), id_pair(id_nat_lit(2), id_nat_lit(2)), id_pair(id_nat_lit(1), id_nat_lit(2))),
        id_prod(Empty_(), Unit_()));

  if (fails == 0)
    printf("ALL %ld BY-OBSERVATION Id CHECKS PASSED (phase 14c reduction system)\n", checks);
  else
    printf("%ld/%ld by-observation Id checks FAILED\n", fails, checks);

  /* a few illustrative reductions, printed */
  if (fails == 0) {
    struct { const char *lhs; id_node *t; } ex[3];
    int i;
    ex[0].lhs = "Id Nat 3 3";
    ex[0].t = id_idty(Nat_(), id_nat_lit(3), id_nat_lit(3));
    ex[1].lhs = "Id (Bool*Nat) (true,1) (false,1)";
    ex[1].t = id_idty(id_prod(Bool_(), Nat_()), id_pair(T_(), id_nat_lit(1)), id_pair(F_(), id_nat_lit(1)));
    ex[2].lhs = "Id U Nat Nat";
    ex[2].t = id_idty(U_(), Nat_(), Nat_());
    printf("\nexamples:\n");
    for (i = 0; i < 3; i++) {
      id_node *nf = id_nf(ex[i].t);
      printf("  %-34s ~>  ", ex[i].lhs); id_print(nf); printf("\n");
      id_free(ex[i].t); id_free(nf);
    }
  }
  return fails ? 1 : 0;
}
