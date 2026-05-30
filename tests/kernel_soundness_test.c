/* tests/kernel_soundness_test.c
 *
 * Phase 2 invariant: the kernel is the trusted core.  Every term it accepts
 * is independently type-checked by kt_infer/kt_check, so an ill-typed term is
 * rejected *no matter how it was produced* — even by a buggy elaborator or a
 * hand-written converter.  These cases construct ill-typed KernelTerms
 * directly (no surface syntax, no elaborator) and assert the kernel refuses
 * them (kt_infer returns NULL).  A handful of well-typed sanity cases confirm
 * the constructions themselves are valid, so a NULL from a "reject" case means
 * the kernel rejected an ill-typed term, not that the term was malformed.
 */
#include "../src/kernel.h"
#include "../src/mem.h"
#include <stdio.h>
#include <string.h>

static int pass_count = 0;
static int fail_count = 0;

static void check(const char *name, int cond) {
  if (cond) {
    pass_count++;
  } else {
    fail_count++;
    printf("  FAIL  %s\n", name);
  }
}

static kterm_t *mk(kterm_tag_t tag) {
  kterm_t *t = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
  memset(t, 0, sizeof(*t));
  t->tag = tag;
  return t;
}

/* reject: kt_infer must return NULL for an ill-typed term. */
static void reject(const char *name, kctx_t *ctx, kterm_t *t) {
  check(name, kt_infer(heap, ctx, t) == NULL);
}

/* accept: kt_infer must succeed for a well-typed term (sanity). */
static void accept(const char *name, kctx_t *ctx, kterm_t *t) {
  check(name, kt_infer(heap, ctx, t) != NULL);
}

int main(void) {
  kctx_t *ctx;
  kterm_t *id_nat; /* lambda (x : Nat). x  */

  heap = lizard_heap_create(1024 * 1024, 1024 * 1024);
  ctx = kctx_create(heap);

  id_nat = kt_lam(heap, "x", kt_nat(heap), kt_var(heap, 0));

  /* ---- sanity: well-typed terms are accepted ---- */
  accept("sanity zero:Nat", ctx, kt_zero(heap));
  accept("sanity succ zero:Nat", ctx, kt_succ(heap, kt_zero(heap)));
  accept("sanity id:Pi", ctx, id_nat);
  accept("sanity (id zero):Nat", ctx, kt_app(heap, id_nat, kt_zero(heap)));
  accept("sanity Nat:Sort", ctx, kt_nat(heap));
  accept("sanity Pi(Nat,Nat):Sort", ctx,
         kt_pi(heap, "x", kt_nat(heap), kt_nat(heap)));

  /* ---- adversarial: ill-typed terms must be rejected ---- */

  /* (1) apply a value that is not a function: 0 applied to 0. */
  reject("reject app non-function (0 0)", ctx,
         kt_app(heap, kt_zero(heap), kt_zero(heap)));

  /* (2) apply a *type* as if it were a function: Nat applied to 0. */
  reject("reject app of a type (Nat 0)", ctx,
         kt_app(heap, kt_nat(heap), kt_zero(heap)));

  /* (3) succ of a non-Nat: succ true. */
  reject("reject succ of Bool (succ true)", ctx,
         kt_succ(heap, mk(KT_TRUE)));

  /* (4) argument type mismatch: (id : Nat->Nat) applied to true:Bool. */
  reject("reject arg mismatch (id true)", ctx,
         kt_app(heap, id_nat, mk(KT_TRUE)));

  /* (5) Pi whose domain is a value, not a type: Pi (x : 0). Nat. */
  reject("reject Pi non-type domain", ctx,
         kt_pi(heap, "x", kt_zero(heap), kt_nat(heap)));

  /* (6) Lambda whose domain is a value, not a type: lambda (x : 0). x. */
  reject("reject lambda non-type domain", ctx,
         kt_lam(heap, "x", kt_zero(heap), kt_var(heap, 0)));

  /* (7) unbound variable in the empty context. */
  reject("reject unbound var", ctx, kt_var(heap, 0));

  /* (8) refl whose value is ill-typed propagates rejection. */
  reject("reject refl of ill-typed", ctx,
         kt_refl(heap, kt_app(heap, kt_zero(heap), kt_zero(heap))));

  /* ---- constructor well-formedness (data-type introduction forms) ---- */
  {
    kterm_t *m;

    /* cons with a tail that is not a list of the head's type:
     * cons 0 true  — head 0:Nat, tail true:Bool.  Must be rejected. */
    m = mk(KT_CONS_K);
    m->data.cons_k.head = kt_zero(heap);
    m->data.cons_k.tail = mk(KT_TRUE);
    reject("reject cons mismatched tail (cons 0 true)", ctx, m);

    /* well-typed cons 0 (nil {Nat}) : List Nat — sanity. */
    {
      kterm_t *nil = mk(KT_NIL_K);
      nil->data.nil_k.elem_type = kt_nat(heap);
      m = mk(KT_CONS_K);
      m->data.cons_k.head = kt_zero(heap);
      m->data.cons_k.tail = nil;
      accept("sanity cons 0 (nil Nat)", ctx, m);
    }

    /* nil whose element annotation is a value, not a type: nil {0}. */
    m = mk(KT_NIL_K);
    m->data.nil_k.elem_type = kt_zero(heap);
    reject("reject nil non-type elem (nil 0)", ctx, m);

    /* inl whose right_type annotation is a value, not a type. */
    m = mk(KT_INL);
    m->data.inl.value = kt_zero(heap);
    m->data.inl.right_type = kt_zero(heap);
    reject("reject inl non-type right", ctx, m);

    /* inr whose left_type annotation is a value, not a type. */
    m = mk(KT_INR);
    m->data.inr.value = kt_zero(heap);
    m->data.inr.left_type = kt_zero(heap);
    reject("reject inr non-type left", ctx, m);

    /* well-typed inl 0 {Bool} : Sum Nat Bool — sanity. */
    m = mk(KT_INL);
    m->data.inl.value = kt_zero(heap);
    m->data.inl.right_type = mk(KT_BOOL);
    accept("sanity inl 0 {Bool}", ctx, m);
  }

  /* Id whose endpoints are not of the stated type: Id Nat true false. */
  reject("reject Id mismatched endpoints (Id Nat true false)", ctx,
         kt_id(heap, kt_nat(heap), mk(KT_TRUE), mk(KT_FALSE)));

  /* well-typed Id Nat 0 (succ 0) — sanity. */
  accept("sanity Id Nat 0 (succ 0)", ctx,
         kt_id(heap, kt_nat(heap), kt_zero(heap), kt_succ(heap, kt_zero(heap))));

  /* ---- projections and annotations ---- */
  {
    kterm_t *m;

    /* fst of a non-pair (fst 0): 0 is not in a Sigma type — reject. */
    m = mk(KT_PROJ1);
    m->data.proj.target = kt_zero(heap);
    reject("reject proj1 of non-pair (fst 0)", ctx, m);

    /* snd of a non-pair — reject. */
    m = mk(KT_PROJ2);
    m->data.proj.target = kt_zero(heap);
    reject("reject proj2 of non-pair (snd 0)", ctx, m);

    /* annotation against a non-matching type: (true : Nat) — reject. */
    m = mk(KT_ANNOT);
    m->data.annot.term = mk(KT_TRUE);
    m->data.annot.type = kt_nat(heap);
    reject("reject annot wrong type (true : Nat)", ctx, m);

    /* well-typed annotation (0 : Nat) — sanity. */
    m = mk(KT_ANNOT);
    m->data.annot.term = kt_zero(heap);
    m->data.annot.type = kt_nat(heap);
    accept("sanity annot (0 : Nat)", ctx, m);
  }

  /* ---- eliminators ---- */
  {
    kterm_t *m;

    /* absurd is now typed: absurd {Nat} : Empty -> Nat (accept). */
    m = mk(KT_ABSURD);
    m->data.absurd.target_type = kt_nat(heap);
    accept("sanity absurd {Nat} : Empty -> Nat", ctx, m);

    /* absurd with a non-type target must be rejected. */
    m = mk(KT_ABSURD);
    m->data.absurd.target_type = kt_zero(heap);
    reject("reject absurd non-type target", ctx, m);

    /* The dependent recursors are now typed.  bool_rec with a constant
     * motive C = (lambda (b:Bool). Nat): C true = C false = Nat. */
    {
      kterm_t *cmot = kt_lam(heap, "b", mk(KT_BOOL), kt_nat(heap));

      /* well-typed: bool_rec C 0 (succ 0) true : Nat. */
      m = mk(KT_BOOL_REC);
      m->data.bool_rec.motive = cmot;
      m->data.bool_rec.true_case = kt_zero(heap);
      m->data.bool_rec.false_case = kt_succ(heap, kt_zero(heap));
      m->data.bool_rec.scrutinee = mk(KT_TRUE);
      accept("sanity bool_rec (const Nat motive)", ctx, m);

      /* ill-typed: true_case has type Bool, motive wants Nat. */
      m = mk(KT_BOOL_REC);
      m->data.bool_rec.motive = cmot;
      m->data.bool_rec.true_case = mk(KT_TRUE);
      m->data.bool_rec.false_case = kt_succ(heap, kt_zero(heap));
      m->data.bool_rec.scrutinee = mk(KT_TRUE);
      reject("reject bool_rec true_case type mismatch", ctx, m);

      /* ill-typed: scrutinee is not a Bool. */
      m = mk(KT_BOOL_REC);
      m->data.bool_rec.motive = cmot;
      m->data.bool_rec.true_case = kt_zero(heap);
      m->data.bool_rec.false_case = kt_zero(heap);
      m->data.bool_rec.scrutinee = kt_zero(heap);
      reject("reject bool_rec non-Bool scrutinee", ctx, m);

      /* a bool_rec with no motive is still rejected (cannot be typed). */
      m = mk(KT_BOOL_REC);
      m->data.bool_rec.true_case = kt_zero(heap);
      m->data.bool_rec.false_case = kt_zero(heap);
      m->data.bool_rec.scrutinee = mk(KT_TRUE);
      reject("reject bool_rec without motive", ctx, m);
    }

    /* nat_rec with a constant motive C = (lambda (n:Nat). Nat). */
    {
      kterm_t *cmot = kt_lam(heap, "n", kt_nat(heap), kt_nat(heap));
      /* s = (lambda (k:Nat). (lambda (ih:Nat). (succ ih))) : Nat->Nat->Nat */
      kterm_t *good_s = kt_lam(heap, "k", kt_nat(heap),
                          kt_lam(heap, "ih", kt_nat(heap),
                            kt_succ(heap, kt_var(heap, 0))));

      m = mk(KT_NAT_REC);
      m->data.nat_rec.motive = cmot;
      m->data.nat_rec.zero_case = kt_zero(heap);
      m->data.nat_rec.succ_case = good_s;
      m->data.nat_rec.scrutinee = kt_succ(heap, kt_succ(heap, kt_zero(heap)));
      accept("sanity nat_rec (const Nat motive)", ctx, m);

      /* ill-typed: zero_case is a Bool, not C 0 = Nat. */
      m = mk(KT_NAT_REC);
      m->data.nat_rec.motive = cmot;
      m->data.nat_rec.zero_case = mk(KT_TRUE);
      m->data.nat_rec.succ_case = good_s;
      m->data.nat_rec.scrutinee = kt_zero(heap);
      reject("reject nat_rec zero_case mismatch", ctx, m);

      /* ill-typed: succ_case body is a Bool, not C (succ k) = Nat. */
      m = mk(KT_NAT_REC);
      m->data.nat_rec.motive = cmot;
      m->data.nat_rec.zero_case = kt_zero(heap);
      m->data.nat_rec.succ_case = kt_lam(heap, "k", kt_nat(heap),
                                    kt_lam(heap, "ih", kt_nat(heap),
                                      mk(KT_TRUE)));
      m->data.nat_rec.scrutinee = kt_zero(heap);
      reject("reject nat_rec succ_case mismatch", ctx, m);

      /* ill-typed: scrutinee is not a Nat. */
      m = mk(KT_NAT_REC);
      m->data.nat_rec.motive = cmot;
      m->data.nat_rec.zero_case = kt_zero(heap);
      m->data.nat_rec.succ_case = good_s;
      m->data.nat_rec.scrutinee = mk(KT_TRUE);
      reject("reject nat_rec non-Nat scrutinee", ctx, m);
    }

    /* maybe_rec with constant motive C = (lambda (m:Maybe Nat). Nat). */
    {
      kterm_t *maybe_nat = mk(KT_MAYBE);
      kterm_t *cmot, *just0;
      maybe_nat->data.maybe.elem_type = kt_nat(heap);
      cmot = kt_lam(heap, "m", maybe_nat, kt_nat(heap));
      just0 = mk(KT_JUST);
      just0->data.just.value = kt_zero(heap);

      m = mk(KT_MAYBE_REC);
      m->data.maybe_rec.motive = cmot;
      m->data.maybe_rec.nothing_case = kt_zero(heap);
      m->data.maybe_rec.just_case = kt_lam(heap, "a", kt_nat(heap),
                                            kt_var(heap, 0));
      m->data.maybe_rec.scrutinee = just0;
      accept("sanity maybe_rec (const Nat motive)", ctx, m);

      /* ill-typed: just_case returns a Bool, not Nat. */
      m = mk(KT_MAYBE_REC);
      m->data.maybe_rec.motive = cmot;
      m->data.maybe_rec.nothing_case = kt_zero(heap);
      m->data.maybe_rec.just_case = kt_lam(heap, "a", kt_nat(heap),
                                           mk(KT_TRUE));
      m->data.maybe_rec.scrutinee = just0;
      reject("reject maybe_rec just_case mismatch", ctx, m);
    }

    /* sum_rec with constant motive C = (lambda (s:Sum Nat Bool). Nat). */
    {
      kterm_t *sum_nb = mk(KT_SUM_K);
      kterm_t *cmot, *inl0;
      sum_nb->data.sum_k.left_type = kt_nat(heap);
      sum_nb->data.sum_k.right_type = mk(KT_BOOL);
      cmot = kt_lam(heap, "s", sum_nb, kt_nat(heap));
      inl0 = mk(KT_INL);
      inl0->data.inl.value = kt_zero(heap);
      inl0->data.inl.right_type = mk(KT_BOOL);

      m = mk(KT_SUM_REC);
      m->data.sum_rec.motive = cmot;
      m->data.sum_rec.left_case = kt_lam(heap, "a", kt_nat(heap),
                                         kt_var(heap, 0));
      m->data.sum_rec.right_case = kt_lam(heap, "b", mk(KT_BOOL),
                                          kt_zero(heap));
      m->data.sum_rec.scrutinee = inl0;
      accept("sanity sum_rec (const Nat motive)", ctx, m);

      /* ill-typed: right_case returns a Bool, not Nat. */
      m = mk(KT_SUM_REC);
      m->data.sum_rec.motive = cmot;
      m->data.sum_rec.left_case = kt_lam(heap, "a", kt_nat(heap),
                                         kt_var(heap, 0));
      m->data.sum_rec.right_case = kt_lam(heap, "b", mk(KT_BOOL),
                                          mk(KT_TRUE));
      m->data.sum_rec.scrutinee = inl0;
      reject("reject sum_rec right_case mismatch", ctx, m);
    }

    /* list_rec with constant motive C = (lambda (xs:List Nat). Nat):
     * the cons_case shape is Pi(h:Nat).Pi(t:List Nat).Pi(ih:Nat).Nat. */
    {
      kterm_t *list_nat = mk(KT_LIST);
      kterm_t *cmot, *nil_nat, *xs, *good_c;
      list_nat->data.list.elem_type = kt_nat(heap);
      cmot = kt_lam(heap, "xs", list_nat, kt_nat(heap));
      nil_nat = mk(KT_NIL_K);
      nil_nat->data.nil_k.elem_type = kt_nat(heap);
      /* xs = cons 0 (nil Nat) : List Nat */
      xs = mk(KT_CONS_K);
      xs->data.cons_k.head = kt_zero(heap);
      xs->data.cons_k.tail = nil_nat;
      /* c_case = lambda h t ih. (succ ih)  — a length-shaped step */
      good_c = kt_lam(heap, "h", kt_nat(heap),
                 kt_lam(heap, "t", list_nat,
                   kt_lam(heap, "ih", kt_nat(heap),
                     kt_succ(heap, kt_var(heap, 0)))));

      m = mk(KT_LIST_REC);
      m->data.list_rec.motive = cmot;
      m->data.list_rec.nil_case = kt_zero(heap);
      m->data.list_rec.cons_case = good_c;
      m->data.list_rec.scrutinee = xs;
      accept("sanity list_rec (const Nat motive)", ctx, m);

      /* ill-typed: nil_case is a Bool, not Nat. */
      m = mk(KT_LIST_REC);
      m->data.list_rec.motive = cmot;
      m->data.list_rec.nil_case = mk(KT_TRUE);
      m->data.list_rec.cons_case = good_c;
      m->data.list_rec.scrutinee = xs;
      reject("reject list_rec nil_case mismatch", ctx, m);

      /* ill-typed: cons_case body is a Bool, not Nat. */
      m = mk(KT_LIST_REC);
      m->data.list_rec.motive = cmot;
      m->data.list_rec.nil_case = kt_zero(heap);
      m->data.list_rec.cons_case = kt_lam(heap, "h", kt_nat(heap),
                                     kt_lam(heap, "t", list_nat,
                                       kt_lam(heap, "ih", kt_nat(heap),
                                         mk(KT_TRUE))));
      m->data.list_rec.scrutinee = xs;
      reject("reject list_rec cons_case mismatch", ctx, m);
    }

    /* J with a constant motive C = (lambda (x:Nat)(y:Nat)(p:Id Nat x y). Nat),
     * base d = (lambda (x:Nat). 0) : Pi(x:Nat). C x x (refl x) = Pi(x:Nat).Nat.
     * J C d Nat 0 0 (refl 0) : C 0 0 (refl 0) = Nat. */
    {
      kterm_t *cmot = kt_lam(heap, "x", kt_nat(heap),
                       kt_lam(heap, "y", kt_nat(heap),
                         kt_lam(heap, "p",
                           kt_id(heap, kt_nat(heap), kt_var(heap, 1),
                                 kt_var(heap, 0)),
                           kt_nat(heap))));
      kterm_t *good_d = kt_lam(heap, "x", kt_nat(heap), kt_zero(heap));

      m = mk(KT_J);
      m->data.j.motive = cmot;
      m->data.j.base_case = good_d;
      m->data.j.type = kt_nat(heap);
      m->data.j.a = kt_zero(heap);
      m->data.j.b = kt_zero(heap);
      m->data.j.proof = kt_refl(heap, kt_zero(heap));
      accept("sanity J (const Nat motive)", ctx, m);

      /* ill-typed: base_case returns a Bool, not C x x (refl x) = Nat. */
      m = mk(KT_J);
      m->data.j.motive = cmot;
      m->data.j.base_case = kt_lam(heap, "x", kt_nat(heap), mk(KT_TRUE));
      m->data.j.type = kt_nat(heap);
      m->data.j.a = kt_zero(heap);
      m->data.j.b = kt_zero(heap);
      m->data.j.proof = kt_refl(heap, kt_zero(heap));
      reject("reject J base_case mismatch", ctx, m);

      /* ill-typed: proof is not an Id. */
      m = mk(KT_J);
      m->data.j.motive = cmot;
      m->data.j.base_case = good_d;
      m->data.j.type = kt_nat(heap);
      m->data.j.a = kt_zero(heap);
      m->data.j.b = kt_zero(heap);
      m->data.j.proof = kt_zero(heap);
      reject("reject J non-Id proof", ctx, m);

      /* ill-typed: endpoint a is not in A. */
      m = mk(KT_J);
      m->data.j.motive = cmot;
      m->data.j.base_case = good_d;
      m->data.j.type = kt_nat(heap);
      m->data.j.a = mk(KT_TRUE);
      m->data.j.b = kt_zero(heap);
      m->data.j.proof = kt_refl(heap, kt_zero(heap));
      reject("reject J endpoint not in carrier", ctx, m);
    }

    /* let-bindings (definitional).  value zero : Nat, body uses it as var 0. */
    {
      kterm_t *lt;
      /* well-typed: let x = 0 in (succ x) : Nat. */
      lt = mk(KT_LET);
      lt->data.let.name = "x";
      lt->data.let.value = kt_zero(heap);
      lt->data.let.body = kt_succ(heap, kt_var(heap, 0));
      accept("sanity let (x=0 in succ x)", ctx, lt);

      /* ill-typed: let x = true in (succ x) — succ needs a Nat, x : Bool. */
      lt = mk(KT_LET);
      lt->data.let.name = "x";
      lt->data.let.value = mk(KT_TRUE);
      lt->data.let.body = kt_succ(heap, kt_var(heap, 0));
      reject("reject let with type-incorrect body", ctx, lt);
    }
  }

  /* ---- regression: kt_subst / kt_equal must descend into eliminator nodes.
   * A neutral bool_rec (scrutinee = a free variable) once fell through the
   * default cases: subst left the scrutinee untouched and kt_equal reported a
   * term unequal to itself.  These checks pin the fixes. ---- */
  {
    kterm_t *br, *br2, *br3, *s, *w;
    br = mk(KT_BOOL_REC);
    br->data.bool_rec.motive = kt_lam(heap, "x", mk(KT_BOOL), mk(KT_BOOL));
    br->data.bool_rec.true_case = mk(KT_TRUE);
    br->data.bool_rec.false_case = mk(KT_FALSE);
    br->data.bool_rec.scrutinee = kt_var(heap, 0);
    /* structurally-identical copy with distinct pointers */
    br2 = mk(KT_BOOL_REC);
    br2->data.bool_rec.motive = kt_lam(heap, "x", mk(KT_BOOL), mk(KT_BOOL));
    br2->data.bool_rec.true_case = mk(KT_TRUE);
    br2->data.bool_rec.false_case = mk(KT_FALSE);
    br2->data.bool_rec.scrutinee = kt_var(heap, 0);
    check("neutral bool_rec equals identical copy",
          kt_equal(heap, ctx, br, br2));
    /* a structurally-different neutral eliminator must NOT compare equal */
    br3 = mk(KT_BOOL_REC);
    br3->data.bool_rec.motive = kt_lam(heap, "x", mk(KT_BOOL), mk(KT_BOOL));
    br3->data.bool_rec.true_case = mk(KT_FALSE);   /* branches swapped */
    br3->data.bool_rec.false_case = mk(KT_TRUE);
    br3->data.bool_rec.scrutinee = kt_var(heap, 0);
    check("distinct neutral bool_rec NOT equal",
          !kt_equal(heap, ctx, br, br3));
    /* substituting the scrutinee variable then reducing must fire the branch */
    s = kt_subst(heap, br, 0, mk(KT_TRUE));
    w = kt_whnf(heap, ctx, s);
    check("subst into bool_rec scrutinee reduces to true branch",
          w != NULL && w->tag == KT_TRUE);
  }

  /* ---- regression: kt_unify must also descend into eliminator nodes (it
   * shared the missing-default bug with subst/shift/equal).  A metavariable
   * unifies with any term; two identical neutral eliminators unify; a meta
   * inside a neutral eliminator gets solved. ---- */
  {
    meta_ctx_t *mc = meta_ctx_create(heap);
    kterm_t *m = meta_fresh(heap, mc, NULL);
    kterm_t *br_a, *br_b;
    check("meta unifies with a concrete term",
          kt_unify(heap, ctx, mc, m, kt_zero(heap)));
    br_a = mk(KT_BOOL_REC);
    br_a->data.bool_rec.motive = kt_lam(heap, "x", mk(KT_BOOL), mk(KT_BOOL));
    br_a->data.bool_rec.true_case = mk(KT_TRUE);
    br_a->data.bool_rec.false_case = mk(KT_FALSE);
    br_a->data.bool_rec.scrutinee = kt_var(heap, 0);
    br_b = mk(KT_BOOL_REC);
    br_b->data.bool_rec.motive = kt_lam(heap, "x", mk(KT_BOOL), mk(KT_BOOL));
    br_b->data.bool_rec.true_case = mk(KT_TRUE);
    br_b->data.bool_rec.false_case = mk(KT_FALSE);
    br_b->data.bool_rec.scrutinee = kt_var(heap, 0);
    check("identical neutral bool_rec unify",
          kt_unify(heap, ctx, meta_ctx_create(heap), br_a, br_b));
  }

  /* ---- regression: a variable's type must be shifted into the occurrence's
   * context (de Bruijn).  Polymorphic identity \A:Type. \x:A. x : (A:Type) ->
   * (x:A) -> A only type-checks with the shift; the body type is A=#1, not the
   * value x=#0.  Without the shift the kernel wrongly rejected it. ---- */
  {
    kterm_t *poly = kt_lam(heap, "A", kt_sort(heap, 0),
                     kt_lam(heap, "x", kt_var(heap, 0), kt_var(heap, 0)));
    kterm_t *good = kt_pi(heap, "A", kt_sort(heap, 0),
                     kt_pi(heap, "x", kt_var(heap, 0), kt_var(heap, 1)));
    kterm_t *bad = kt_pi(heap, "A", kt_sort(heap, 0),
                     kt_pi(heap, "x", kt_var(heap, 0), kt_sort(heap, 0)));
    check("polymorphic identity checks against (A:*)->(x:A)->A",
          kt_check(heap, ctx, poly, good) == KERNEL_OK);
    check("polymorphic identity rejects wrong codomain",
          kt_check(heap, ctx, poly, bad) != KERNEL_OK);
  }
  /* kt_unify must compare constants by name (it shared the missing-default
   * bug). */
  {
    meta_ctx_t *mc = meta_ctx_create(heap);
    kterm_t *p1 = mk(KT_CONST), *p2 = mk(KT_CONST), *q = mk(KT_CONST);
    p1->data.constant.name = "P"; p2->data.constant.name = "P";
    q->data.constant.name = "Q";
    check("identical constants unify", kt_unify(heap, ctx, mc, p1, p2));
    check("distinct constants do not unify",
          !kt_unify(heap, ctx, meta_ctx_create(heap), p1, q));
  }

  /* ---- user-defined inductives: strict positivity is soundness-critical.
   * A recursive occurrence in a strictly-positive position is accepted; a
   * recursive occurrence in a negative position (a Pi domain) is rejected,
   * which is exactly what blocks `data Bad = mkBad (Bad -> Empty)` and the
   * contradiction it would license. ---- */
  {
    kdef_ctx_t *d = kdef_ctx_create(heap);
    kind_decl_t decl;
    const char *cn[2];
    int na[2];
    kterm_t **cat[2];
    kterm_t *catN[1];
    kterm_t *constN = mk(KT_CONST);
    constN->data.constant.name = "Npos";
    cn[0] = "zc"; cn[1] = "sc"; na[0] = 0; na[1] = 1;
    catN[0] = constN; cat[0] = NULL; cat[1] = catN;
    decl.name = "Npos"; decl.rec_name = "Npos-rec"; decl.n_params = 0;
    decl.param_names = NULL; decl.param_types = NULL; decl.sort_level = 0;
    decl.n_ctors = 2; decl.ctor_names = cn; decl.ctor_nargs = na;
    decl.ctor_argtypes = cat; decl.ctor_recflags = NULL;
    kdef_add(heap, d, "Npos", kind_former_type(heap, &decl), NULL);
    check("inductive: strictly-positive recursion accepted",
          kind_declare(heap, d, &decl) == 1);
  }
  {
    kdef_ctx_t *d = kdef_ctx_create(heap);
    kind_decl_t decl;
    const char *cn[1];
    int na[1];
    kterm_t **cat[1];
    kterm_t *catB[1];
    kterm_t *constB = mk(KT_CONST);
    constB->data.constant.name = "Bad";
    catB[0] = kt_pi(heap, "x", constB, mk(KT_EMPTY));   /* Bad -> Empty */
    cn[0] = "mkBad"; na[0] = 1; cat[0] = catB;
    decl.name = "Bad"; decl.rec_name = "Bad-rec"; decl.n_params = 0;
    decl.param_names = NULL; decl.param_types = NULL; decl.sort_level = 0;
    decl.n_ctors = 1; decl.ctor_names = cn; decl.ctor_nargs = na;
    decl.ctor_argtypes = cat; decl.ctor_recflags = NULL;
    kdef_add(heap, d, "Bad", kind_former_type(heap, &decl), NULL);
    check("inductive: negative occurrence REJECTED",
          kind_declare(heap, d, &decl) == 0);
  }

  printf("kernel soundness: %d passed, %d failed\n", pass_count, fail_count);
  /* --- An opaque constant (axiom reference) with no definition context must
   * be rejected, not crash: kt_infer needs ctx->defs to resolve it, and a
   * bare context has none. --- */
  {
    kterm_t *c = mk(KT_CONST);
    c->data.constant.name = "nonexistent-axiom";
    reject("reject KT_CONST with no def context", ctx, c);
  }

  lizard_heap_destroy(heap);
  return fail_count > 0 ? 1 : 0;
}
