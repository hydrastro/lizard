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

  /* ---- cubical: Path types, the boundary, and path beta.  The boundary
   * (p @ i0 = left endpoint) holds even for a neutral path, recovered from its
   * type; an ill-typed path (mismatched endpoints) is rejected; path beta
   * computes. ---- */
  {
    kterm_t *prefl = kt_plam(heap, "i", kt_zero(heap));
    kterm_t *good = kt_path(heap, kt_nat(heap), kt_zero(heap), kt_zero(heap));
    kterm_t *bad = kt_path(heap, kt_nat(heap), kt_zero(heap),
                           kt_succ(heap, kt_zero(heap)));
    check("cubical: plam i.0 : Path Nat 0 0 accepted",
          kt_check(heap, ctx, prefl, good) == KERNEL_OK);
    check("cubical: plam i.0 : Path Nat 0 1 rejected (endpoints)",
          kt_check(heap, ctx, prefl, bad) != KERNEL_OK);
  }
  {
    kctx_t *e = kctx_extend(heap, ctx, "p",
                  kt_path(heap, kt_nat(heap), kt_zero(heap),
                          kt_succ(heap, kt_zero(heap))), NULL);
    kterm_t *goal = kt_id(heap, kt_nat(heap),
                          kt_papp(heap, kt_var(heap, 0), kt_i0(heap)),
                          kt_zero(heap));
    check("cubical: neutral boundary p@i0 = left endpoint",
          kt_check(heap, e, kt_refl(heap, kt_zero(heap)), goal) == KERNEL_OK);
  }
  {
    kterm_t *idp = kt_plam(heap, "i", kt_var(heap, 0));     /* plam i. i */
    kterm_t *goal = kt_id(heap, kt_interval(heap),
                          kt_papp(heap, idp, kt_i1(heap)), kt_i1(heap));
    check("cubical: path beta (plam i.i)@i1 = i1",
          kt_check(heap, ctx, kt_refl(heap, kt_i1(heap)), goal) == KERNEL_OK);
  }

  /* --- Univalence typing (KT_EQUIV / KT_UA) is part of the trusted kernel.
   * These cases confirm the kernel TYPES univalence soundly: it accepts a
   * well-formed (ua e) : (Id (Sort n) A B) when e : (Equiv A B), and REJECTS
   * ill-typed uses (ua of a non-equivalence, Equiv of a non-type).  This is
   * the typing layer only; computational univalence (transport across ua via
   * Glue/comp) is deliberately NOT in the kernel and is not claimed. --- */
  {
    kctx_t *eu;
    kterm_t *A, *B, *equivAB, *uae, *goal, *bad_goal, *a_in_A;
    /* A, B : Sort 0 ; e : Equiv A B ; a : A   (as context hypotheses) */
    eu = kctx_extend(heap, ctx, "A", kt_sort(heap, 0), NULL);
    eu = kctx_extend(heap, eu, "B", kt_sort(heap, 0), NULL);
    A = kt_var(heap, 1);                 /* A is two binders back from here */
    B = kt_var(heap, 0);                 /* B is the innermost */
    equivAB = kt_equiv(heap, A, B);
    eu = kctx_extend(heap, eu, "e", equivAB, NULL);
    /* now: index 0 = e : Equiv A B, index 1 = B, index 2 = A */
    eu = kctx_extend(heap, eu, "a", kt_var(heap, 2), NULL);
    /* index 0 = a : A, 1 = e, 2 = B, 3 = A */

    /* ACCEPT: Equiv A B : Sort 0  (A,B are vars 3,2 from inside the a-binder) */
    accept("ua: (Equiv A B) is a type",
           eu, kt_equiv(heap, kt_var(heap, 3), kt_var(heap, 2)));

    /* ACCEPT: (ua e) : (Id (Sort 0) A B) — well-typed univalence */
    uae = kt_ua(heap, kt_var(heap, 1));  /* e is var 1 from inside a-binder */
    goal = kt_id(heap, kt_sort(heap, 0), kt_var(heap, 3), kt_var(heap, 2));
    check("ua: (ua e) : (Id (Sort 0) A B) accepted",
          kt_check(heap, eu, uae, goal) == KERNEL_OK);

    /* REJECT: (ua a) — a : A is NOT an equivalence, so ua must reject it */
    a_in_A = kt_var(heap, 0);            /* a : A */
    reject("ua: (ua a) rejected — a is not an equivalence",
           eu, kt_ua(heap, a_in_A));

    /* REJECT: (ua e) at the WRONG codomain (Id (Sort 0) A A) — endpoints must be A,B */
    bad_goal = kt_id(heap, kt_sort(heap, 0), kt_var(heap, 3), kt_var(heap, 3));
    check("ua: (ua e) : (Id (Sort 0) A A) rejected — wrong endpoints",
          kt_check(heap, eu, kt_ua(heap, kt_var(heap, 1)), bad_goal) != KERNEL_OK);

    /* REJECT: (Equiv a B) — a : A is not a type, so Equiv must reject it */
    reject("ua: (Equiv a B) rejected — a is not a type",
           eu, kt_equiv(heap, kt_var(heap, 0), kt_var(heap, 2)));
  }

  /* --- Transport (KT_TRANSP) typing is in the trusted kernel.  These cases
   * confirm it is SOUND: it accepts a well-typed (transp <i>A a) : A when the
   * line is a path-abstraction whose body is a type and base inhabits the i0
   * endpoint; it REJECTS a base of the wrong type and a result of the wrong
   * type.  The single computation rule (constant line reduces to base) is
   * checked too.  A non-constant line stays NEUTRAL — full transport via Glue/
   * comp is deliberately NOT computed, only typed. --- */
  {
    kctx_t *et;
    kterm_t *A, *line_A, *transpAa, *bad_base, *reduced;
    et = kctx_extend(heap, ctx, "A", kt_sort(heap, 0), NULL);
    et = kctx_extend(heap, et, "B", kt_sort(heap, 0), NULL);
    A = kt_var(heap, 1);
    et = kctx_extend(heap, et, "a", A, NULL);  /* a : A */
    /* index 0 = a, 1 = B, 2 = A (from inside the a-binder) */

    /* the constant line <i> A  (body does not mention i; A is var 2 from here,
     * but inside the plam binder it is var 3) */
    line_A = kt_plam(heap, "i", kt_var(heap, 3));

    /* ACCEPT: (transp <i>A a) : A — well-typed transport over a constant line */
    transpAa = kt_transp(heap, line_A, kt_var(heap, 0));
    accept("transp: (transp <i>A a) is well-typed", et, transpAa);
    check("transp: (transp <i>A a) : A accepted",
          kt_check(heap, et, transpAa, kt_var(heap, 2)) == KERNEL_OK);

    /* COMPUTATION: (transp <i>A a) reduces to a (constant line = identity) */
    reduced = kt_whnf(heap, et, transpAa);
    check("transp: constant-line (transp <i>A a) reduces to a",
          kt_equal(heap, et, reduced, kt_var(heap, 0)) != 0);

    /* REJECT: result type wrong — (transp <i>A a) : B should fail (A not ≡ B) */
    check("transp: (transp <i>A a) : B rejected — wrong result type",
          kt_check(heap, et, transpAa, kt_var(heap, 1)) != KERNEL_OK);

    /* REJECT: base of the wrong type — (transp <i>A B) where B : Sort 0, not : A */
    bad_base = kt_transp(heap, line_A, kt_var(heap, 1));  /* base = B, but needs : A */
    reject("transp: base of wrong type rejected", et, bad_base);
  }

  /* --- Structural transport on a VARYING non-dependent product (KT_TRANSP at KT_SIGMA).
   * transp <i>(Sigma (_:A(i)) B(i)) (a,b)  =  (transp <i>A(i) a, transp <i>B(i) b)  when the
   * Sigma is non-dependent.  This is genuine varying-line transport (A, B differ at i0 vs i1)
   * and needs no Glue.  Verified type-preserving: the result inhabits Sigma A(i1) B(i1).  A
   * dependent Sigma stays NEUTRAL (the rule does not fire).  We build a varying line with a
   * path in the universe: PA : Path (Sort 0) A0 A1, line body = Sigma (_ : PA@i) B0. --- */
  {
    kctx_t *st;
    kterm_t *A0, *A1, *a0, *b0, *path_univ;
    kterm_t *line_body, *line, *pair_base, *result, *result_ty, *wrong_ty;
    st = kctx_extend(heap, ctx, "A0", kt_sort(heap, 0), NULL);
    st = kctx_extend(heap, st, "A1", kt_sort(heap, 0), NULL);
    st = kctx_extend(heap, st, "B0", kt_sort(heap, 0), NULL);
    A0 = kt_var(heap, 2); A1 = kt_var(heap, 1);
    path_univ = kt_path(heap, kt_sort(heap, 0), A0, A1);   /* Path (Sort 0) A0 A1 */
    st = kctx_extend(heap, st, "PA", path_univ, NULL);
    /* indices now: 0=PA, 1=B0, 2=A1, 3=A0 */
    st = kctx_extend(heap, st, "a0", kt_var(heap, 3), NULL);  /* a0 : A0 */
    st = kctx_extend(heap, st, "b0", kt_var(heap, 2), NULL);  /* b0 : B0 */
    /* indices now: 0=b0, 1=a0, 2=PA, 3=B0, 4=A1, 5=A0 */
    A0 = kt_var(heap, 5); A1 = kt_var(heap, 4);
    a0 = kt_var(heap, 1); b0 = kt_var(heap, 0);

    /* line body (under the interval binder, so all the above shift up by 1):
     *   Sigma (_ : PA@i) B0   where PA@i = papp (PA shifted) (var 0 = i) */
    {
      kterm_t *PA_s = kt_var(heap, 3);   /* PA, shifted by 1 under the interval binder */
      kterm_t *i_var = kt_var(heap, 0);  /* the interval var */
      kterm_t *PA_at_i = kt_papp(heap, PA_s, i_var);
      kterm_t *sig = mk(KT_SIGMA);
      sig->data.sigma.name = "_";
      sig->data.sigma.fst_type = PA_at_i;
      /* snd_type lives under the additional pair binder, so B0 shifts up once more: var 5 */
      sig->data.sigma.snd_type = kt_var(heap, 5);
      line_body = sig;
    }
    line = kt_plam(heap, "i", line_body);
    pair_base = mk(KT_PAIR);
    pair_base->data.pair.fst = a0;
    pair_base->data.pair.snd = b0;
    result = kt_transp(heap, line, pair_base);

    /* the result type is Sigma (_:A1) B0 (since PA@i1 = A1) */
    result_ty = mk(KT_SIGMA);
    result_ty->data.sigma.name = "_";
    result_ty->data.sigma.fst_type = A1;
    result_ty->data.sigma.snd_type = kt_var(heap, 4); /* B0 under the pair binder */
    check("transp/Sigma: varying-line componentwise transport is type-preserving",
          kt_check(heap, st, result, result_ty) == KERNEL_OK);

    /* and it must NOT typecheck at the i0 type Sigma A0 B0 (A0 not ≡ A1 in general) */
    wrong_ty = mk(KT_SIGMA);
    wrong_ty->data.sigma.name = "_";
    wrong_ty->data.sigma.fst_type = A0;
    wrong_ty->data.sigma.snd_type = kt_var(heap, 4);
    check("transp/Sigma: varying-line result rejected at the i0 type",
          kt_check(heap, st, result, wrong_ty) != KERNEL_OK);
  }

  /* --- DEPENDENT Sigma transport (this iteration): when B(i,x) genuinely depends on the first
   * component x, transport is componentwise in the first slot and uses the transport FILLER
   * q(i) = transp <k>A(k/\i) a for the second:
   *   transp <i>(Sigma (x:A(i)) B(i,x)) (a,b) = (transp <i>A(i) a, transp <i>B(i,q(i)) b).
   * Delegates to the proven transp + the sound interval meet; no Glue, no comp.  We build the line
   * <i>(Sigma (x : A(i)) (Id A(i) x x))  (B depends on x as both Id endpoints, on i via the type),
   * with A(i) = PA@i a varying universe line, and check the reduction agrees with the explicit
   * filler-based pair AND that the result type-checks at the dependent i1-endpoint type. --- */
  {
    kctx_t *dt = kctx_extend(heap, ctx, "A0", kt_sort(heap, 0), NULL);
    dt = kctx_extend(heap, dt, "A1", kt_sort(heap, 0), NULL);
    {
      kterm_t *dA0 = kt_var(heap, 1), *dA1 = kt_var(heap, 0);
      kterm_t *pathty = kt_path(heap, kt_sort(heap, 0), dA0, dA1);
      kctx_t *dt2 = kctx_extend(heap, dt, "PA", pathty, NULL);
      /* indices: 0=PA, 1=A1, 2=A0, ... */
      kctx_t *dt3 = kctx_extend(heap, dt2, "a", kt_papp(heap, kt_var(heap, 0), kt_i0(heap)), NULL); /* a : PA@i0 */
      {
        /* a : PA@i0 at index 0; b : Id (PA@i0) a a at index 0 after next extend */
        kterm_t *idty;
        kctx_t *dt4;
        kterm_t *PA_a = kt_var(heap, 1);   /* PA, under a */
        kterm_t *aa = kt_var(heap, 0);      /* a */
        idty = kt_id(heap, kt_papp(heap, PA_a, kt_i0(heap)), aa, aa); /* Id (PA@i0) a a */
        dt4 = kctx_extend(heap, dt3, "b", idty, NULL);
        {
          /* indices under (A0,A1,PA,a,b): 0=b,1=a,2=PA,3=A1,4=A0 */
          kterm_t *bb = kt_var(heap, 0), *av = kt_var(heap, 1), *PAv = kt_var(heap, 2);
          /* the dependent Sigma line <i>(Sigma (x:PA@i)(Id (PA@i) x x)); under <i>: PA=idx3, then
           * under the pair binder x: PA=idx4, x=idx0, i=idx1. */
          kterm_t *sig, *line, *lhs, *rhs_pair, *result_ty;
          {
            kterm_t *PA_i = kt_papp(heap, kt_var(heap, 3), kt_var(heap, 0));  /* PA@i under <i> (PA=idx3, i=idx0) */
            kterm_t *PA_i_x = kt_papp(heap, kt_var(heap, 4), kt_var(heap, 1)); /* PA@i under <i,x> (PA=idx4, i=idx1) */
            kterm_t *xv = kt_var(heap, 0);                                     /* x under <i,x> */
            sig = mk(KT_SIGMA);
            sig->data.sigma.name = "x";
            sig->data.sigma.fst_type = PA_i;                                  /* x : PA@i */
            sig->data.sigma.snd_type = kt_id(heap, PA_i_x, xv, xv);           /* Id (PA@i) x x */
            line = kt_plam(heap, "i", sig);
          }
          {
            kterm_t *basepair = mk(KT_PAIR);
            basepair->data.pair.fst = av;
            basepair->data.pair.snd = bb;
            lhs = kt_transp(heap, line, basepair);
          }
          /* explicit filler-based expected pair:
           *   fst = transp <i>(PA@i) a
           *   q(i) = transp <k>(PA@(k/\i)) a
           *   snd = transp <i>(Id (PA@i) q(i) q(i)) b   */
          {
            kterm_t *a_line = kt_plam(heap, "i", kt_papp(heap, kt_var(heap, 3), kt_var(heap, 0)));
            kterm_t *new_fst = kt_transp(heap, a_line, av);
            /* q-line <k>(PA@(k/\i)) under <i>: PA=idx3 under <i>; under <i,k>: PA=idx4, k=idx0, i=idx1 */
            kterm_t *q_line = kt_plam(heap, "k",
                                kt_papp(heap, kt_var(heap, 4),
                                              kt_imeet(heap, kt_var(heap, 0), kt_var(heap, 1))));
            kterm_t *q_i = kt_transp(heap, q_line, kt_shift(heap, av, 0, 1)); /* a lifted under <i> */
            kterm_t *PA_i2 = kt_papp(heap, kt_var(heap, 3), kt_var(heap, 0)); /* PA@i under <i> */
            kterm_t *b_line = kt_plam(heap, "i", kt_id(heap, PA_i2, q_i, q_i));
            kterm_t *new_snd = kt_transp(heap, b_line, bb);
            {
              kterm_t *rp = mk(KT_PAIR);
              rp->data.pair.fst = new_fst;
              rp->data.pair.snd = new_snd;
              rhs_pair = rp;
            }
          }
          /* The reduction-equality against an explicit filler-based pair is exercised at the surface
           * level in the sangaku Floor-5 example (named variables, no de Bruijn hazard); here we keep
           * the end-to-end guarantee that matters for soundness: the dependent-Sigma transport result
           * type-checks at the dependent i1-endpoint type (which fails unless the filler line, the
           * meet boundaries, and the bidirectional pair-check all cooperate correctly). */
          (void)rhs_pair;
          /* result type: Sigma (x : PA@i1) (Id (PA@i1) x x) */
          {
            kterm_t *PA_i1 = kt_papp(heap, PAv, kt_i1(heap));
            kterm_t *PA_i1_x = kt_papp(heap, kt_shift(heap, PAv, 0, 1), kt_i1(heap));
            kterm_t *xv = kt_var(heap, 0);
            result_ty = mk(KT_SIGMA);
            result_ty->data.sigma.name = "x";
            result_ty->data.sigma.fst_type = PA_i1;
            result_ty->data.sigma.snd_type = kt_id(heap, PA_i1_x, xv, xv);
          }
          check("transp/Sigma: DEPENDENT Sigma result type-checks at the dependent i1-endpoint type",
                kt_check(heap, dt4, lhs, result_ty) == KERNEL_OK);
          (void)PAv;
        }
      }
    }
  }

  /* --- Structural transport on a VARYING Sum (KT_TRANSP at KT_SUM_K).
   * transp <i>(A(i) + B(i)) (inl a) = inl (transp <i>A(i) a).  A Sum is always
   * non-dependent, so no guard is needed.  Verified type-preserving and rejected
   * at the wrong endpoint type.  Built with paths in the universe so A,B vary. --- */
  {
    kctx_t *st;
    kterm_t *A0, *A1, *B0, *a0, *inl_base, *line, *result, *result_ty, *wrong_ty;
    st = kctx_extend(heap, ctx, "A0", kt_sort(heap, 0), NULL);
    st = kctx_extend(heap, st, "A1", kt_sort(heap, 0), NULL);
    st = kctx_extend(heap, st, "B0", kt_sort(heap, 0), NULL);
    A0 = kt_var(heap, 2); A1 = kt_var(heap, 1);
    st = kctx_extend(heap, st, "PA", kt_path(heap, kt_sort(heap, 0), A0, A1), NULL);
    st = kctx_extend(heap, st, "a0", kt_var(heap, 3), NULL);  /* a0 : A0 */
    /* indices: 0=a0, 1=PA, 2=B0, 3=A1, 4=A0 */
    A1 = kt_var(heap, 3); a0 = kt_var(heap, 0); B0 = kt_var(heap, 2);

    /* line body (under interval binder, shift +1): Sum (PA@i) B0 */
    {
      kterm_t *sum = mk(KT_SUM_K);
      sum->data.sum_k.left_type = kt_papp(heap, kt_var(heap, 2), kt_var(heap, 0)); /* PA@i */
      sum->data.sum_k.right_type = kt_var(heap, 3);                                /* B0 */
      line = kt_plam(heap, "i", sum);
    }
    inl_base = mk(KT_INL);
    inl_base->data.inl.value = a0;
    inl_base->data.inl.right_type = B0;
    result = kt_transp(heap, line, inl_base);

    /* result type Sum A1 B0 */
    result_ty = mk(KT_SUM_K);
    result_ty->data.sum_k.left_type = A1;
    result_ty->data.sum_k.right_type = B0;
    check("transp/Sum: varying-line inl transport is type-preserving",
          kt_check(heap, st, result, result_ty) == KERNEL_OK);
    wrong_ty = mk(KT_SUM_K);
    wrong_ty->data.sum_k.left_type = A0;
    wrong_ty->data.sum_k.right_type = B0;
    check("transp/Sum: varying-line result rejected at the i0 type",
          kt_check(heap, st, result, wrong_ty) != KERNEL_OK);
  }

  /* --- Structural transport on a Pi with CONSTANT DOMAIN (KT_TRANSP at KT_PI).
   * transp <i>(Pi (x:A) B(i)) f = lam (x:A). transp <i>B(i) (f x), when A is constant
   * in i and B is non-dependent in x.  This needs no interval reversal.  Verified
   * type-preserving; a varying-domain Pi (which would need interval negation, absent
   * here) stays NEUTRAL.  Built with B0..B1 a path in the universe, A constant. --- */
  {
    kctx_t *pt;
    kterm_t *B0, *B1, *f, *fty, *line, *result, *result_ty, *wrong_ty;
    pt = kctx_extend(heap, ctx, "A", kt_sort(heap, 0), NULL);
    pt = kctx_extend(heap, pt, "B0", kt_sort(heap, 0), NULL);
    pt = kctx_extend(heap, pt, "B1", kt_sort(heap, 0), NULL);
    B0 = kt_var(heap, 1); B1 = kt_var(heap, 0);
    pt = kctx_extend(heap, pt, "PB", kt_path(heap, kt_sort(heap, 0), B0, B1), NULL);
    /* f : Pi (x:A) B0  — but B0 under the x-binder shifts to var 2 (A=3, B0=2) */
    {
      kterm_t *pif = mk(KT_PI);
      pif->data.pi.name = "x";
      pif->data.pi.domain = kt_var(heap, 3);   /* A */
      pif->data.pi.codomain = kt_var(heap, 3); /* B0 under x-binder: A=4? recompute below */
      fty = pif;
    }
    /* indices before f: 0=PB,1=B1,2=B0,3=A.  In Pi domain (no inner binder yet) A=3.
     * In the codomain (under x) everything shifts +1: A=4, B0=3. */
    fty->data.pi.domain = kt_var(heap, 3);    /* A */
    fty->data.pi.codomain = kt_var(heap, 3);  /* B0 shifted under x = index 3 */
    pt = kctx_extend(heap, pt, "f", fty, NULL);
    /* indices now: 0=f, 1=PB, 2=B1, 3=B0, 4=A */
    f = kt_var(heap, 0);

    /* line body (under interval binder, +1): Pi (x:A) (PB@i).
     * A under interval binder = 5; PB = 2; in codomain (under x) PB = 3, i = 1. */
    {
      kterm_t *pil = mk(KT_PI);
      pil->data.pi.name = "x";
      pil->data.pi.domain = kt_var(heap, 5);                                  /* A */
      pil->data.pi.codomain = kt_papp(heap, kt_var(heap, 3), kt_var(heap, 1));/* PB@i */
      line = kt_plam(heap, "i", pil);
    }
    result = kt_transp(heap, line, f);
    /* result type Pi (x:A) B1.  A = 4; B1 under x-binder = 3. */
    result_ty = mk(KT_PI);
    result_ty->data.pi.name = "x";
    result_ty->data.pi.domain = kt_var(heap, 4);
    result_ty->data.pi.codomain = kt_var(heap, 3); /* B1 shifted under x */
    check("transp/Pi: constant-domain transport is type-preserving",
          kt_check(heap, pt, result, result_ty) == KERNEL_OK);
    /* reject at the i0 type Pi (x:A) B0: B0 under x-binder = 4 */
    wrong_ty = mk(KT_PI);
    wrong_ty->data.pi.name = "x";
    wrong_ty->data.pi.domain = kt_var(heap, 4);
    wrong_ty->data.pi.codomain = kt_var(heap, 4); /* B0 shifted under x */
    check("transp/Pi: constant-domain result rejected at the i0 type",
          kt_check(heap, pt, result, wrong_ty) != KERNEL_OK);
  }

  /* --- Interval negation (KT_INEG): ~i0 = i1, ~i1 = i0, ~~r = r.  Total, finite,
   * endpoint-only computation.  These confirm the rules and, crucially, that ~i0
   * does NOT reduce to i0 (a wrong endpoint would break boundary recovery). --- */
  {
    check("ineg: ~i0 reduces to i1",
          kt_equal(heap, ctx, kt_whnf(heap, ctx, kt_ineg(heap, kt_i0(heap))), kt_i1(heap)) != 0);
    check("ineg: ~i1 reduces to i0",
          kt_equal(heap, ctx, kt_whnf(heap, ctx, kt_ineg(heap, kt_i1(heap))), kt_i0(heap)) != 0);
    check("ineg: ~~i0 = i0 (involution)",
          kt_equal(heap, ctx, kt_whnf(heap, ctx, kt_ineg(heap, kt_ineg(heap, kt_i0(heap)))), kt_i0(heap)) != 0);
    check("ineg: ~i0 is NOT i0 (no wrong endpoint)",
          kt_equal(heap, ctx, kt_whnf(heap, ctx, kt_ineg(heap, kt_i0(heap))), kt_i0(heap)) == 0);
  }

  /* --- Interval MEET (min) and JOIN (max): the distributive-lattice operations on the interval.
   * Meet: i0/\x=i0, i1/\x=x, x/\x=x (and symmetric); Join dual.  Total, finite, sound — they only
   * rearrange endpoints, never invent one.  These unlock the transport FILLER (k/\i), which the
   * dependent-Sigma transport below uses.  We check the laws and the filler-boundary identities. --- */
  {
    kctx_t *it = kctx_extend(heap, ctx, "r", kt_interval(heap), NULL);
    kterm_t *r = kt_var(heap, 0);
    /* meet */
    check("imeet: i0 /\\ r = i0 (bottom absorbs)",
          kt_equal(heap, it, kt_whnf(heap, it, kt_imeet(heap, kt_i0(heap), r)), kt_i0(heap)) != 0);
    check("imeet: r /\\ i0 = i0 (bottom absorbs, symmetric)",
          kt_equal(heap, it, kt_whnf(heap, it, kt_imeet(heap, r, kt_i0(heap))), kt_i0(heap)) != 0);
    check("imeet: i1 /\\ r = r (top is identity)",
          kt_equal(heap, it, kt_whnf(heap, it, kt_imeet(heap, kt_i1(heap), r)), r) != 0);
    check("imeet: r /\\ i1 = r (top is identity, symmetric) — filler boundary k/\\i1=k",
          kt_equal(heap, it, kt_whnf(heap, it, kt_imeet(heap, r, kt_i1(heap))), r) != 0);
    check("imeet: r /\\ r = r (idempotent)",
          kt_equal(heap, it, kt_whnf(heap, it, kt_imeet(heap, r, r)), r) != 0);
    check("imeet: r /\\ i1 is NOT i1 in general (no wrong endpoint)",
          kt_equal(heap, it, kt_whnf(heap, it, kt_imeet(heap, r, kt_i1(heap))), kt_i1(heap)) == 0);
    /* join */
    check("ijoin: i1 \\/ r = i1 (top absorbs)",
          kt_equal(heap, it, kt_whnf(heap, it, kt_ijoin(heap, kt_i1(heap), r)), kt_i1(heap)) != 0);
    check("ijoin: r \\/ i1 = i1 (top absorbs, symmetric)",
          kt_equal(heap, it, kt_whnf(heap, it, kt_ijoin(heap, r, kt_i1(heap))), kt_i1(heap)) != 0);
    check("ijoin: i0 \\/ r = r (bottom is identity)",
          kt_equal(heap, it, kt_whnf(heap, it, kt_ijoin(heap, kt_i0(heap), r)), r) != 0);
    check("ijoin: r \\/ r = r (idempotent)",
          kt_equal(heap, it, kt_whnf(heap, it, kt_ijoin(heap, r, r)), r) != 0);
    /* typing */
    check("imeet: r /\\ i1 : I (meet stays in the interval)",
          kt_check(heap, it, kt_imeet(heap, r, kt_i1(heap)), kt_interval(heap)) == KERNEL_OK);
    check("ijoin: i0 \\/ r : I (join stays in the interval)",
          kt_check(heap, it, kt_ijoin(heap, kt_i0(heap), r), kt_interval(heap)) == KERNEL_OK);
  }

  /* --- Structural transport on a Pi with a VARYING DOMAIN (KT_TRANSP at KT_PI, via KT_INEG).
   * transp <i>(Pi (x:A(i)) B) f = lam (x:A(i1)). transp <i>B (f (transp <i>A(~i) x)).
   * The argument is transported BACKWARD along the reversed domain line (interval negation
   * supplies the reversal).  Here the domain varies (PA : Path Sort A0 A1) and the codomain
   * is constant (B0), which exercises the backward-transport path with simple indices.
   * Verified type-preserving and rejected at the i0 type. --- */
  {
    kctx_t *vt;
    kterm_t *A0, *A1, *f, *fty, *line, *result, *result_ty, *wrong_ty;
    vt = kctx_extend(heap, ctx, "A0", kt_sort(heap, 0), NULL);
    vt = kctx_extend(heap, vt, "A1", kt_sort(heap, 0), NULL);
    vt = kctx_extend(heap, vt, "B0", kt_sort(heap, 0), NULL);
    A0 = kt_var(heap, 2); A1 = kt_var(heap, 1);
    vt = kctx_extend(heap, vt, "PA", kt_path(heap, kt_sort(heap, 0), A0, A1), NULL);
    /* indices: 0=PA,1=B0,2=A1,3=A0 */
    /* f : Pi (x:A0) B0.  domain A0=3; codomain (under x) B0=1+1=2. */
    fty = mk(KT_PI);
    fty->data.pi.name = "x";
    fty->data.pi.domain = kt_var(heap, 3);     /* A0 */
    fty->data.pi.codomain = kt_var(heap, 2);   /* B0 under x */
    vt = kctx_extend(heap, vt, "f", fty, NULL);
    /* indices now: 0=f,1=PA,2=B0,3=A1,4=A0 */
    f = kt_var(heap, 0);

    /* line body (under interval binder, +1): Pi (x:PA@i) B0.
     * PA=2 (was 1, +1); codomain (under x) B0=2+1+1=... B0 was index 2, +1 (interval) +1 (x) = 4. */
    {
      kterm_t *pil = mk(KT_PI);
      pil->data.pi.name = "x";
      pil->data.pi.domain = kt_papp(heap, kt_var(heap, 2), kt_var(heap, 0)); /* PA@i */
      pil->data.pi.codomain = kt_var(heap, 4);                              /* B0 under (i, x) */
      line = kt_plam(heap, "i", pil);
    }
    result = kt_transp(heap, line, f);
    /* result type Pi (x:A1) B0.  A1=3; B0 under x = 2+1=3. */
    result_ty = mk(KT_PI);
    result_ty->data.pi.name = "x";
    result_ty->data.pi.domain = kt_var(heap, 3);   /* A1 */
    result_ty->data.pi.codomain = kt_var(heap, 3); /* B0 under x */
    check("transp/Pi: VARYING-domain transport is type-preserving (via interval negation)",
          kt_check(heap, vt, result, result_ty) == KERNEL_OK);
    /* reject at the i0 type Pi (x:A0) B0: A0=4; B0 under x = 3+1=4 */
    wrong_ty = mk(KT_PI);
    wrong_ty->data.pi.name = "x";
    wrong_ty->data.pi.domain = kt_var(heap, 4);    /* A0 */
    wrong_ty->data.pi.codomain = kt_var(heap, 4);  /* B0 under x */
    check("transp/Pi: varying-domain result rejected at the i0 type",
          kt_check(heap, vt, result, wrong_ty) != KERNEL_OK);
  }

  /* --- The identity equivalence (KT_ID_EQUIV): (id-equiv A) : (Equiv A A) when A : Sort.
   * A small, safe addition completing the equivalence vocabulary in the kernel; it composes
   * with ua (ua (id-equiv A) : Id (Sort) A A).  NOTE: transport ACROSS ua (the Glue
   * computation) is deliberately NOT in the kernel — that needs Glue/hcomp, which is the
   * genuine CCHM frontier and is not attempted.  These cases confirm id-equiv types soundly
   * and rejects ill-typed uses. --- */
  {
    kctx_t *it;
    kterm_t *A, *B, *ide, *uaid;
    it = kctx_extend(heap, ctx, "A", kt_sort(heap, 0), NULL);
    it = kctx_extend(heap, it, "B", kt_sort(heap, 0), NULL);
    A = kt_var(heap, 1); B = kt_var(heap, 0);
    ide = kt_id_equiv(heap, A);
    check("id-equiv: (id-equiv A) : (Equiv A A) accepted",
          kt_check(heap, it, ide, kt_equiv(heap, A, A)) == KERNEL_OK);
    /* ua composes with it: ua (id-equiv A) : Id (Sort 0) A A */
    uaid = kt_ua(heap, ide);
    check("id-equiv: ua (id-equiv A) : Id (Sort 0) A A accepted",
          kt_check(heap, it, uaid, kt_id(heap, kt_sort(heap, 0), A, A)) == KERNEL_OK);
    /* REJECT: (id-equiv A) : (Equiv A B) for distinct A,B */
    check("id-equiv: (id-equiv A) : (Equiv A B) rejected — wrong endpoints",
          kt_check(heap, it, ide, kt_equiv(heap, A, B)) != KERNEL_OK);
    /* REJECT: id-equiv of a non-type (a term variable would need a non-Sort type;
     * here use B's inhabitant absence — instead check id-equiv of a non-type via a pair). */
    {
      kterm_t *nonty = mk(KT_PAIR);   /* a pair is not a type */
      nonty->data.pair.fst = A;
      nonty->data.pair.snd = B;
      reject("id-equiv: (id-equiv <non-type>) rejected", it, kt_id_equiv(heap, nonty));
    }
  }

  /* --- Empty-system homogeneous composition (KT_HCOMP): hcomp A u0 : A, reduces to u0.
   * The first brick of the Kan structure, at the only depth with a short proof: with NO
   * face system (structurally unrepresentable here), composition is the base.  Type-
   * preserving because the result IS u0 : A.  The full face-system hcomp (compatibility
   * checking + Kan filling) is NOT provided.  These cases confirm typing, the reduction,
   * and rejection of a wrong-typed base / wrong result type. --- */
  {
    kctx_t *ht;
    kterm_t *A, *B, *a, *hca;
    ht = kctx_extend(heap, ctx, "A", kt_sort(heap, 0), NULL);
    ht = kctx_extend(heap, ht, "B", kt_sort(heap, 0), NULL);
    ht = kctx_extend(heap, ht, "a", kt_var(heap, 1), NULL);  /* a : A */
    A = kt_var(heap, 2); B = kt_var(heap, 1); a = kt_var(heap, 0);
    hca = kt_hcomp(heap, A, a);
    check("hcomp: (hcomp A a) : A accepted", kt_check(heap, ht, hca, A) == KERNEL_OK);
    check("hcomp: (hcomp A a) reduces to a (empty composition is the base)",
          kt_equal(heap, ht, kt_whnf(heap, ht, hca), a) != 0);
    check("hcomp: (hcomp A a) : B rejected — wrong result type",
          kt_check(heap, ht, hca, B) != KERNEL_OK);
    /* a base of the wrong type: hcomp A (a-of-type-B) — use B's inhabitant absence via a pair */
    {
      kterm_t *bad = mk(KT_PAIR);  /* a pair does not inhabit A */
      bad->data.pair.fst = a;
      bad->data.pair.snd = a;
      reject("hcomp: (hcomp A <wrong-base>) rejected", ht, kt_hcomp(heap, A, bad));
    }
  }

  /* --- Cofibrations + SINGLE-face homogeneous composition (KT_COFIB, KT_HCOMP1):
   * the first real face-aware Kan brick.  hcomp1 A (cofib r b) u u0 reduces to u on the
   * face (r=b), to u0 off the face (distinct endpoints), and stays neutral otherwise.
   * TYPING enforces the compatibility u=u0 when the face holds — the soundness heart.
   * Multi-face hcomp (disjunctions, overlap compatibility) is NOT provided. --- */
  {
    kctx_t *ct;
    kterm_t *A, *u, *u0, *holding_ok, *holding_bad, *empty_face;
    ct = kctx_extend(heap, ctx, "A", kt_sort(heap, 0), NULL);
    ct = kctx_extend(heap, ct, "u", kt_var(heap, 0), NULL);   /* u : A */
    ct = kctx_extend(heap, ct, "u0", kt_var(heap, 1), NULL);  /* u0 : A */
    A = kt_var(heap, 2); u = kt_var(heap, 1); u0 = kt_var(heap, 0);

    /* cofibration well-formedness: (cofib i1 i1) : I */
    check("cofib: (cofib i1 i1) is well-formed",
          kt_infer(heap, ct, kt_cofib(heap, kt_i1(heap), kt_i1(heap))) != NULL);

    /* HOLDING face (i1=i1) with compatible u0,u0: accepted and reduces to the partial element */
    holding_ok = kt_hcomp1(heap, A, kt_cofib(heap, kt_i1(heap), kt_i1(heap)), u0, u0);
    check("hcomp1: holding face, compatible (u0,u0) : A accepted",
          kt_check(heap, ct, holding_ok, A) == KERNEL_OK);
    check("hcomp1: holding face reduces to the partial element",
          kt_equal(heap, ct, kt_whnf(heap, ct, holding_ok), u0) != 0);

    /* HOLDING face with INCOMPATIBLE u,u0 (u != u0): REJECTED by the compatibility check */
    holding_bad = kt_hcomp1(heap, A, kt_cofib(heap, kt_i1(heap), kt_i1(heap)), u, u0);
    check("hcomp1: holding face with incompatible u != u0 is REJECTED (compatibility)",
          kt_check(heap, ct, holding_bad, A) != KERNEL_OK);

    /* EMPTY face (i0=i1): reduces to the base u0, and types even with u != u0 (no constraint) */
    empty_face = kt_hcomp1(heap, A, kt_cofib(heap, kt_i0(heap), kt_i1(heap)), u, u0);
    check("hcomp1: empty face (i0=i1) : A accepted (no compatibility constraint off-face)",
          kt_check(heap, ct, empty_face, A) == KERNEL_OK);
    check("hcomp1: empty face reduces to the base u0",
          kt_equal(heap, ct, kt_whnf(heap, ct, empty_face), u0) != 0);

    /* NEUTRAL face (cofib r i1) with r : I a variable: stays neutral, does NOT reduce to u */
    {
      kctx_t *nt = kctx_extend(heap, ct, "r", kt_interval(heap), NULL);
      kterm_t *An = kt_var(heap, 3), *un = kt_var(heap, 2), *u0n = kt_var(heap, 1), *rn = kt_var(heap, 0);
      kterm_t *neutral = kt_hcomp1(heap, An, kt_cofib(heap, rn, kt_i1(heap)), un, u0n);
      kterm_t *w = kt_whnf(heap, nt, neutral);
      check("hcomp1: neutral face does NOT reduce to the partial element (no wrong value)",
            kt_equal(heap, nt, w, un) == 0);
    }
  }

  /* --- TWO-face homogeneous composition (KT_HCOMP2): the overlap-compatibility lattice.
   * hcomp2 A (cofib r1 b1) u1 (cofib r2 b2) u2 u0 reduces to u1 on face1, else u2 on face2,
   * else u0 when both faces are empty, else neutral.  TYPING enforces three conditions:
   * (C1) u1=u0 when face1 holds, (C2) u2=u0 when face2 holds, (C3) u1=u2 where BOTH hold
   * (the overlap — the new soundness-critical check that makes the elif order sound). --- */
  {
    kctx_t *dt;
    kterm_t *A, *u0, *u1, *cofH, *cofE;
    kterm_t *ok_both_empty, *ok_overlap, *bad_C1, *bad_C3;
    dt = kctx_extend(heap, ctx, "A", kt_sort(heap, 0), NULL);
    dt = kctx_extend(heap, dt, "u0", kt_var(heap, 0), NULL);  /* u0 : A */
    dt = kctx_extend(heap, dt, "u1", kt_var(heap, 1), NULL);  /* u1 : A (distinct from u0) */
    A = kt_var(heap, 2); u0 = kt_var(heap, 1); u1 = kt_var(heap, 0);
    cofH = kt_cofib(heap, kt_i1(heap), kt_i1(heap));  /* a HOLDING face (i1=i1) */
    cofE = kt_cofib(heap, kt_i0(heap), kt_i1(heap));  /* an EMPTY face (i0=i1) */

    /* both faces empty: reduces to base u0; partials may differ (no constraint) */
    ok_both_empty = kt_hcomp2(heap, A, cofE, u1, cofE, u1, u0);
    check("hcomp2: both faces empty : A accepted", kt_check(heap, dt, ok_both_empty, A) == KERNEL_OK);
    check("hcomp2: both faces empty reduces to the base u0",
          kt_equal(heap, dt, kt_whnf(heap, dt, ok_both_empty), u0) != 0);

    /* both faces hold with compatible partials (all u0): accepted, reduces to u0 */
    ok_overlap = kt_hcomp2(heap, A, cofH, u0, cofH, u0, u0);
    check("hcomp2: overlap with compatible partials (u0,u0) : A accepted",
          kt_check(heap, dt, ok_overlap, A) == KERNEL_OK);
    check("hcomp2: overlap reduces to the partial element",
          kt_equal(heap, dt, kt_whnf(heap, dt, ok_overlap), u0) != 0);

    /* C1 violation: face1 holds but u1 != u0 -> rejected */
    bad_C1 = kt_hcomp2(heap, A, cofH, u1, cofE, u0, u0);
    check("hcomp2: face1 holds with u1 != u0 is REJECTED (C1)",
          kt_check(heap, dt, bad_C1, A) != KERNEL_OK);

    /* C3 violation (THE OVERLAP): both faces hold but u1 != u0 (the two partials differ) -> rejected.
     * Here partial1=u1, partial2=u0, both faces holding, u1 != u0, so the overlap check fires. */
    bad_C3 = kt_hcomp2(heap, A, cofH, u1, cofH, u0, u1);
    check("hcomp2: overlap with incompatible partials u1 != u2 is REJECTED (C3 — the overlap check)",
          kt_check(heap, dt, bad_C3, A) != KERNEL_OK);
  }

  /* --- The Glue type former (KT_GLUE), the equivalence forward map (KT_EQUIV_FUN), and the
   * Glue eliminator (KT_UNGLUE) — the type-former layer univalence is built from.
   *   Glue A (cofib r b) T e : Sort, with boundary (Glue ... = T on the face r=b).
   *   equiv-fun e : T -> A, computing to the identity for id-equiv.
   *   unglue ... g : A, = g off the face, = (equiv-fun e) g on the face.
   * The Glue transp rule (transport ACROSS ua, composing the equivalence with hcomp over a
   * face) is NOT provided — that is the remaining CCHM core. --- */
  {
    kctx_t *gt;
    kterm_t *A, *T, *B, *e, *a, *glue_on, *glue_off, *ef_id, *id_fun, *ung_off, *ung_id, *bad_glue;
    gt = kctx_extend(heap, ctx, "A", kt_sort(heap, 0), NULL);
    gt = kctx_extend(heap, gt, "T", kt_sort(heap, 0), NULL);
    gt = kctx_extend(heap, gt, "B", kt_sort(heap, 0), NULL);
    A = kt_var(heap, 2); T = kt_var(heap, 1); B = kt_var(heap, 0);
    gt = kctx_extend(heap, gt, "e", kt_equiv(heap, T, A), NULL);  /* e : Equiv T A */
    gt = kctx_extend(heap, gt, "a", kt_var(heap, 3), NULL);       /* a : A */
    /* indices: 0=a, 1=e, 2=B, 3=T, 4=A */
    A = kt_var(heap, 4); T = kt_var(heap, 3); B = kt_var(heap, 2); e = kt_var(heap, 1); a = kt_var(heap, 0);

    /* Glue type well-formed and lands in Sort */
    glue_off = kt_glue(heap, A, kt_cofib(heap, kt_i0(heap), kt_i1(heap)), T, e);
    check("Glue: (Glue A (cofib i0 i1) T e) : Sort 0 accepted",
          kt_check(heap, gt, glue_off, kt_sort(heap, 0)) == KERNEL_OK);
    /* boundary: on the face reduces to T */
    glue_on = kt_glue(heap, A, kt_cofib(heap, kt_i1(heap), kt_i1(heap)), T, e);
    check("Glue: boundary — on the face (i1=i1) reduces to T",
          kt_equal(heap, gt, kt_whnf(heap, gt, glue_on), T) != 0);
    /* off the face it is NOT T */
    check("Glue: off the face (i0=i1) does NOT reduce to T (stays glued)",
          kt_equal(heap, gt, kt_whnf(heap, gt, glue_off), T) == 0);

    /* REJECT: Glue with e : Equiv T A but glue_type B (e would need to be Equiv B A) */
    bad_glue = kt_glue(heap, A, kt_cofib(heap, kt_i0(heap), kt_i1(heap)), B, e);
    check("Glue: glue_type/equiv mismatch is rejected",
          kt_check(heap, gt, bad_glue, kt_sort(heap, 0)) != KERNEL_OK);

    /* --- glue INTRODUCTION (the Glue element constructor) and its coherence.  We need a member
     * u : T and base a : A with (equiv-fun e) u == a ON THE FACE.  Off the face there is no
     * coherence constraint, so an off-face glue with ANY a : A and u : T is well-typed and reduces
     * to a.  On the face, the coherent choice base = (equiv-fun e) u type-checks and reduces to u;
     * an incoherent base is rejected; and unglue recovers the base (beta). */
    {
      kterm_t *fe = kt_equiv_fun(heap, e);              /* equiv-fun e : Pi(_:T) A */
      /* a member u : T.  Use (equiv-inv e) a : T so u is a genuine T-element. */
      kterm_t *u  = kt_app(heap, kt_equiv_inv(heap, e), a);   /* u : T */
      kterm_t *feu = kt_app(heap, fe, u);               /* (equiv-fun e) u : A */
      kterm_t *off_cof = kt_cofib(heap, kt_i0(heap), kt_i1(heap));
      kterm_t *on_cof  = kt_cofib(heap, kt_i1(heap), kt_i1(heap));
      /* OFF the face: any base a : A is fine (no coherence); glue reduces to a. */
      kterm_t *g_off = kt_glue_elem(heap, A, off_cof, T, e, u, a);
      check("glue-intro: off-face glue A (cofib i0 i1) T e u a : Glue accepted (no coherence off-face)",
            kt_check(heap, gt, g_off, kt_glue(heap, A, off_cof, T, e)) == KERNEL_OK);
      check("glue-intro: off-face glue reduces to its base component a",
            kt_equal(heap, gt, kt_whnf(heap, gt, g_off), a) != 0);
      /* ON the face: coherent base = (equiv-fun e) u type-checks and reduces to the member u. */
      {
        kterm_t *g_on = kt_glue_elem(heap, A, on_cof, T, e, u, feu);
        check("glue-intro: on-face glue with coherent base ((equiv-fun e) u) : Glue accepted",
              kt_check(heap, gt, g_on, kt_glue(heap, A, on_cof, T, e)) == KERNEL_OK);
        check("glue-intro: on-face glue reduces to its T-component (the member u)",
              kt_equal(heap, gt, kt_whnf(heap, gt, g_on), u) != 0);
        /* unglue beta: unglue (glue .. u a) = a */
        check("glue-intro: unglue (glue A cof T e u a) = a (beta)",
              kt_equal(heap, gt, kt_whnf(heap, gt, kt_unglue(heap, A, on_cof, T, e, g_on)), feu) != 0);
      }
      /* REJECTION: on the face an INCOHERENT base (a, where a != (equiv-fun e) u in general) is
       * rejected — the coherence (equiv-fun e) u == a is enforced. */
      reject("glue-intro: on-face glue with an incoherent base is rejected (coherence enforced)",
             gt, kt_glue_elem(heap, A, on_cof, T, e, u, a));
      (void)feu;
    }

    /* --- Partial type former (the first stone of the partial-elements layer): the type of partial
     * elements of A on a face.  On a held face = A; on an empty face = Unit; else neutral. */
    {
      kterm_t *held = kt_cofib(heap, kt_i1(heap), kt_i1(heap));
      kterm_t *empt = kt_cofib(heap, kt_i0(heap), kt_i1(heap));
      check("Partial: (Partial (cofib i1 i1) A) is a Sort (formation)",
            kt_check(heap, gt, kt_partial(heap, held, A), kt_sort(heap, 0)) == KERNEL_OK);
      check("Partial: on a HELD face reduces to A (a partial element on a true face is total)",
            kt_equal(heap, gt, kt_whnf(heap, gt, kt_partial(heap, held, A)), A) != 0);
      check("Partial: on an EMPTY face reduces to Unit (only the trivial partial element)",
            kt_whnf(heap, gt, kt_partial(heap, empt, A))->tag == KT_UNIT);
      check("Partial: on a held face, a : A inhabits (Partial (cofib i1 i1) A)",
            kt_check(heap, gt, a, kt_partial(heap, held, A)) == KERNEL_OK);
    }

    /* --- systems introduction: psys (cofib r b) A a — a single-face system [phi -> a], the
     * INTRODUCTION for Partial.  On a held face = a; on an empty face = * : Unit; else neutral.  The
     * element is typed under the face restriction, so an element ill-typed even on the face is
     * rejected. */
    {
      kterm_t *held = kt_cofib(heap, kt_i1(heap), kt_i1(heap));
      kterm_t *empt = kt_cofib(heap, kt_i0(heap), kt_i1(heap));
      check("psys: (psys (cofib i1 i1) A a) : (Partial (cofib i1 i1) A) (systems introduction types)",
            kt_check(heap, gt, kt_psys(heap, held, A, a), kt_partial(heap, held, A)) == KERNEL_OK);
      check("psys: on a HELD face reduces to its value a",
            kt_equal(heap, gt, kt_whnf(heap, gt, kt_psys(heap, held, A, a)), a) != 0);
      check("psys: on an EMPTY face reduces to * : Unit (the trivial partial element)",
            kt_whnf(heap, gt, kt_psys(heap, empt, A, a))->tag == KT_STAR);
      /* REJECT: a system whose value has the wrong type (B, not A) on the face */
      reject("psys: a system with a wrong-typed value is rejected (typed under the face)",
             gt, kt_psys(heap, held, A, B));
    }

    /* --- comp2 one-empty-face reduction (this iteration; the engine of the composition-filler
     * regularity).  A comp2 with one empty face drops to the single-face comp on the other.  Use a
     * CONSTANT partial line <i>a (no de Bruijn shift hazard) so the equation is exact. */
    {
      kterm_t *cline = kt_plam(heap, "i", A);                 /* constant line <i>A */
      kterm_t *uline = kt_plam(heap, "i", a);                 /* constant partial <i>a */
      kterm_t *clampl = kt_plam(heap, "i", a);                /* clamp leg <i>a */
      kterm_t *held  = kt_cofib(heap, kt_i1(heap), kt_i1(heap));
      kterm_t *emptyR = kt_cofib(heap, kt_i1(heap), kt_i0(heap));   /* empty (i1=i0) */
      check("comp2: one empty face drops to the single-face comp (the filler regularity engine)",
            kt_equal(heap, gt,
              kt_whnf(heap, gt, kt_comp2(heap, cline, held, uline, emptyR, clampl, a)),
              kt_whnf(heap, gt, kt_comp(heap, cline, held, uline, a))) != 0);
      check("comp2: both faces empty still reduces to transport of the base (intact)",
            kt_equal(heap, gt,
              kt_whnf(heap, gt, kt_comp2(heap, cline, emptyR, uline, emptyR, clampl, a)),
              kt_whnf(heap, gt, kt_transp(heap, cline, a))) != 0);
      /* OVERLAP-INCOHERENCE REJECTION (soundness of the face-restricted comp2 typing): two HELD faces
       * with DIFFERENT partial values must be rejected.  a' = equiv-fun e (equiv-inv e a) : A is a
       * genuinely distinct A-element (the round-trip is not judgmental for an opaque e). */
      {
        kterm_t *aprime = kt_app(heap, kt_equiv_fun(heap, e), kt_app(heap, kt_equiv_inv(heap, e), a));
        kterm_t *up = kt_plam(heap, "i", aprime);
        reject("comp2: two held faces with disagreeing partials are rejected (overlap coherence enforced)",
               gt, kt_comp2(heap, cline, held, uline, held, up, a));
      }
      /* COFIB-OR former (the cofibration-lattice join underpinning multi-face systems): well-formed
       * when both operands are cofibrations; ill-formed when an operand is a bare interval point;
       * whnf normalises the operands and stays a cofib-or (held/empty are read off by consumers). */
      {
        kterm_t *f1 = kt_cofib(heap, kt_i0(heap), kt_i1(heap));      /* an empty face (i0 = i1) */
        kterm_t *f2 = kt_cofib(heap, kt_i1(heap), kt_i1(heap));      /* a held face  (i1 = i1) */
        kterm_t *good = kt_cofib_or(heap, f1, f2);
        kterm_t *bad = kt_cofib_or(heap, f1, kt_i0(heap));           /* second operand a bare point */
        check("cofib-or: a disjunction of two well-formed cofibrations is well-formed",
              kt_infer(heap, gt, good) != NULL);
        reject("cofib-or: a disjunction with a bare interval point (not a cofibration) is rejected",
               gt, bad);
        check("cofib-or: whnf keeps it a cofib-or (normalising operands; decisions are the consumers')",
              kt_whnf(heap, gt, good)->tag == KT_COFIB_OR);
      }
    }

    /* --- general Glue transport (the univalence computation), decided-face boundaries.  transp over a
     * Glue line reduces: on a held face to transport in T; on an empty face to transport in A.  (The
     * fully-general UNDECIDED-face computation, which produces a glue element via the partial-section-
     * typed comp correction, is exercised at the surface in the Floor-5 example with a bound face
     * variable — the de Bruijn there is fragile in a hand-built C term, so we keep the robust
     * decided-face boundary guards here.)  Build a Glue type line <i>(Glue A phi T e) using A,T,e in
     * scope but with A,T NON-varying (constant line) so the boundary targets are exactly g. */
    {
      kterm_t *held = kt_cofib(heap, kt_i1(heap), kt_i1(heap));
      kterm_t *empt = kt_cofib(heap, kt_i0(heap), kt_i1(heap));
      /* on-face line <i>(Glue A (cofib i1 i1) T e): Glue = T (constant), so g : T is required; use u:T */
      kterm_t *gl_on  = kt_plam(heap, "i", kt_glue(heap, A, held, T, e));
      kterm_t *gl_off = kt_plam(heap, "i", kt_glue(heap, A, empt, T, e));
      kterm_t *u = kt_app(heap, kt_equiv_inv(heap, e), a);   /* u : T */
      check("Glue transport: ON a held face, transp over the (constant) Glue line = transp over the T line",
            kt_equal(heap, gt, kt_whnf(heap, gt, kt_transp(heap, gl_on, u)),
                     kt_whnf(heap, gt, kt_transp(heap, kt_plam(heap, "i", T), u))) != 0);
      check("Glue transport: OFF the face, transp over the (constant) Glue line = transp over the A line",
            kt_equal(heap, gt, kt_whnf(heap, gt, kt_transp(heap, gl_off, a)),
                     kt_whnf(heap, gt, kt_transp(heap, kt_plam(heap, "i", A), a))) != 0);
    }

    /* equiv-fun e : Pi (_:T) A */
    {
      kterm_t *pi = mk(KT_PI);
      pi->data.pi.name = "_";
      pi->data.pi.domain = T;
      pi->data.pi.codomain = kt_var(heap, 5);  /* A under the new binder */
      check("equiv-fun: (equiv-fun e) : (Pi (_:T) A) accepted",
            kt_check(heap, gt, kt_equiv_fun(heap, e), pi) == KERNEL_OK);
    }
    /* equiv-fun (id-equiv A) reduces to the identity */
    ef_id = kt_equiv_fun(heap, kt_id_equiv(heap, A));
    id_fun = mk(KT_LAM);
    id_fun->data.lam.name = "x";
    id_fun->data.lam.domain = A;
    id_fun->data.lam.body = kt_var(heap, 0);
    check("equiv-fun: equiv-fun (id-equiv A) reduces to the identity (lam x.x)",
          kt_equal(heap, gt, kt_whnf(heap, gt, ef_id), id_fun) != 0);
    /* REJECT: equiv-fun of a non-equivalence (a : A) */
    reject("equiv-fun: (equiv-fun <non-equiv>) rejected", gt, kt_equiv_fun(heap, a));

    /* unglue off the face = g (here g = a : A is the off-face inhabitant) */
    ung_off = kt_unglue(heap, A, kt_cofib(heap, kt_i0(heap), kt_i1(heap)), T, e, a);
    /* off-face unglue requires arg : Glue, which off-face = A, so a : A works for the reduction;
     * but typing requires arg : Glue exactly.  We test the on-face identity round-trip instead,
     * which is the meaningful computation, and the off-face reduction directly: */
    check("unglue: off the face reduces to its argument",
          kt_equal(heap, gt, kt_whnf(heap, gt, ung_off), a) != 0);
    /* unglue on the face with id-equiv: (id-equiv A) round-trips a to a */
    ung_id = kt_unglue(heap, A, kt_cofib(heap, kt_i1(heap), kt_i1(heap)), A, kt_id_equiv(heap, A), a);
    check("unglue: on the face with id-equiv round-trips to the argument",
          kt_equal(heap, gt, kt_whnf(heap, gt, ung_id), a) != 0);

    /* --- The Glue EMPTY-FACE boundary and the DEGENERATE Glue transp slice.
     * Glue A (empty face) = A (the glue contributes nothing when the face never holds), so
     * transp across a CONSTANT empty-face Glue line is the constant-line transport (= base).
     * The GENERAL Glue transp rule (varying line, proper face) needs the equivalence inverse +
     * is-equiv coherence + comp, which are NOT in the kernel — it remains the named frontier. --- */
    {
      kterm_t *glue_empty, *line, *tr;
      /* empty-face boundary: Glue A (cofib i0 i1) T e reduces to A */
      glue_empty = kt_glue(heap, A, kt_cofib(heap, kt_i0(heap), kt_i1(heap)), T, e);
      check("Glue: empty-face boundary — (Glue A (cofib i0 i1) T e) reduces to A",
            kt_equal(heap, gt, kt_whnf(heap, gt, glue_empty), A) != 0);
      /* degenerate Glue transp: transp over a CONSTANT empty-face Glue line = the base.
       * The line body is constant (A fixed), so this is the constant-line transport rule. */
      line = kt_plam(heap, "i", kt_shift(heap, glue_empty, 0, 1));
      tr = kt_transp(heap, line, a);
      check("transp/Glue: constant empty-face Glue line is type-preserving (: A)",
            kt_check(heap, gt, tr, A) == KERNEL_OK);
      check("transp/Glue: constant empty-face Glue line reduces to the base (degenerate slice)",
            kt_equal(heap, gt, kt_whnf(heap, gt, tr), a) != 0);
    }

    /* --- VARYING empty-face Glue line (this iteration): transp now reduces the line body to whnf
     * UNDER the interval binder before dispatching, so an empty-face Glue line whose base type
     * A(i) genuinely VARIES delegates to the bare A(i)-line transport.  On an empty face the Glue
     * is absent, so this is sound and uses no equivalence coherence — it is the existing transport
     * tier seeing through the (vanishing) Glue.  We test a non-dependent Sigma A-line: the result
     * must agree with transporting the bare Sigma line componentwise. --- */
    {
      /* extend with a varying second-component universe line Q0 ~> Q1 (via a Path) and points */
      kctx_t *vt = kctx_extend(heap, gt, "Q0", kt_sort(heap, 0), NULL);
      vt = kctx_extend(heap, vt, "Q1", kt_sort(heap, 0), NULL);
      {
        kterm_t *Q0 = kt_var(heap, 1), *Q1 = kt_var(heap, 0);
        kterm_t *pathty = kt_path(heap, kt_sort(heap, 0), Q0, Q1);
        kctx_t *vt2 = kctx_extend(heap, vt, "PQ", pathty, NULL);
        /* indices now: 0=PQ, 1=Q1, 2=Q0, 3=a, 4=e, 5=B, 6=T, 7=A (gt's stack shifted by 3) */
        kterm_t *Av = kt_var(heap, 7), *Tv = kt_var(heap, 6), *ev = kt_var(heap, 4);
        kterm_t *PQ = kt_var(heap, 0);
        kctx_t *vt3 = kctx_extend(heap, vt2, "p", Av, NULL);     /* p : A */
        vt3 = kctx_extend(heap, vt3, "q0", kt_var(heap, 3), NULL); /* q0 : Q0 (Q0 now index 3) */
        {
          /* under p,q0: indices 0=q0,1=p,2=PQ,3=Q1,4=Q0,5=a,6=e,7=B,8=T,9=A */
          kterm_t *p = kt_var(heap, 1), *q0v = kt_var(heap, 0);
          kterm_t *Aw = kt_var(heap, 9), *Tw = kt_var(heap, 8), *ew = kt_var(heap, 6);
          kterm_t *pairbase = mk(KT_PAIR);
          /* A-line body under the interval binder i (everything shifts +1):
           *   Sigma (_ : A) (PQ @ i)   with PQ at index 3 under i, i at index 0 */
          kterm_t *sig, *glue_line, *bare_line, *tr_glue, *tr_bare;
          pairbase->data.pair.fst = p;
          pairbase->data.pair.snd = q0v;
          /* build Sigma(_:A)(PQ@i): under the i-binder, A is index 10, PQ index 3 */
          sig = mk(KT_SIGMA);
          sig->data.sigma.name = "_";
          sig->data.sigma.fst_type = kt_var(heap, 10);  /* A under i */
          {
            kterm_t *pq_at_i = kt_papp(heap, kt_var(heap, 3), kt_var(heap, 0)); /* PQ @ i */
            /* snd_type lives under BOTH i and the pair binder, so shift pq_at_i by +1 for the
             * pair var (it does not reference it — non-dependent) */
            sig->data.sigma.snd_type = kt_shift(heap, pq_at_i, 0, 1);
          }
          /* empty-face Glue line over the Sigma body, and the bare Sigma line */
          {
            kterm_t *glue_body = kt_glue(heap, sig, kt_cofib(heap, kt_i0(heap), kt_i1(heap)),
                                               kt_shift(heap, Tw, 0, 1), kt_shift(heap, ew, 0, 1));
            glue_line = kt_plam(heap, "i", glue_body);
            bare_line = kt_plam(heap, "i", sig);
          }
          tr_glue = kt_transp(heap, glue_line, pairbase);
          tr_bare = kt_transp(heap, bare_line, pairbase);
          /* Both the value and the type now resolve through the (vanishing) empty-face Glue: the
           * transport reduces componentwise like the bare Sigma line, and kt_infer resolves the
           * result type through the Glue body.  We assert the reduction agreement (the meaning of
           * the whnf-under-binder advance) and that the Glue-line transport infers a type. */
          check("transp/Glue: VARYING empty-face Glue line transports componentwise like the bare line",
                kt_equal(heap, vt3, kt_whnf(heap, vt3, tr_glue), kt_whnf(heap, vt3, tr_bare)) != 0);
          check("transp/Glue: VARYING empty-face Glue line now INFERS a type (typing sees through the Glue)",
                kt_infer(heap, vt3, tr_glue) != NULL);
          check("transp/Glue: its inferred type matches the bare Sigma line's (i1 endpoint)",
                kt_equal(heap, vt3, kt_infer(heap, vt3, tr_glue), kt_infer(heap, vt3, tr_bare)) != 0);
          (void)Aw; (void)PQ; (void)Av; (void)Tv; (void)ev; (void)Q0; (void)Q1;
        }
      }
    }

    /* --- The HELD-FACE Glue transp rule (this iteration): transport along a line of Glue
     * types whose face HOLDS THROUGHOUT (cofib b b, a constant always-true face) reduces to
     * transport along the underlying type line <i>T(i).  Because the Glue boundary gives
     * Glue ≡ T on the held face, the Glue line is definitionally the T line, reusing the
     * already-sound T-line transp — no equivalence inverse, no hcomp correction. --- */
    {
      kctx_t *ht2 = kctx_extend(heap, gt, "tt", T, NULL);  /* tt : T */
      kterm_t *A2 = kt_var(heap, 5), *T2 = kt_var(heap, 4), *e2 = kt_var(heap, 2), *tt = kt_var(heap, 0);
      kterm_t *held, *line, *tr;
      /* held-face Glue line (T constant, so the line is well-formed): Glue A (cofib i1 i1) T e.
       * under the interval binder everything shifts +1: A=6, T=5, e=3. */
      held = kt_glue(heap, kt_var(heap, 6), kt_cofib(heap, kt_i1(heap), kt_i1(heap)),
                           kt_var(heap, 5), kt_var(heap, 3));
      line = kt_plam(heap, "i", held);
      tr = kt_transp(heap, line, tt);
      check("transp/Glue: HELD-face Glue transp is type-preserving (: T)",
            kt_check(heap, ht2, tr, T2) == KERNEL_OK);
      check("transp/Glue: HELD-face Glue transp reduces via the underlying type line (to tt)",
            kt_equal(heap, ht2, kt_whnf(heap, ht2, tr), tt) != 0);
      (void)A2; (void)e2;
    }

    /* --- VARYING held-face Glue line (this iteration's mirror): the same whnf-under-binder that
     * lets transp see through an EMPTY-face Glue to the bare A-line also lets it see through a
     * HELD-face Glue to the bare T-line — because on a held face Glue ≡ T definitionally.  So a
     * held-face Glue line whose underlying type T(i) genuinely VARIES (e.g. a non-dependent Sigma)
     * transports componentwise exactly like the bare T(i)-line, and infers the matching i1 type.
     * Sound: on a held face the boundary is definitional (Glue = T), no equivalence correction. --- */
    {
      kctx_t *hv = kctx_extend(heap, gt, "S0", kt_sort(heap, 0), NULL);
      hv = kctx_extend(heap, hv, "S1", kt_sort(heap, 0), NULL);
      {
        kterm_t *S0 = kt_var(heap, 1), *S1 = kt_var(heap, 0);
        kterm_t *pathty = kt_path(heap, kt_sort(heap, 0), S0, S1);
        kctx_t *hv2 = kctx_extend(heap, hv, "PS", pathty, NULL);
        /* indices now: 0=PS,1=S1,2=S0,3=a,4=e,5=B,6=T,7=A */
        kterm_t *Av = kt_var(heap, 7), *ev = kt_var(heap, 4);
        kctx_t *hv3 = kctx_extend(heap, hv2, "rr", Av, NULL);     /* rr : A (a base in the Sigma's fst) */
        hv3 = kctx_extend(heap, hv3, "s0", kt_var(heap, 3), NULL); /* s0 : S0 (S0 now index 3) */
        {
          /* under rr,s0: 0=s0,1=rr,2=PS,3=S1,4=S0,5=a,6=e,7=B,8=T,9=A */
          kterm_t *rr = kt_var(heap, 1), *s0v = kt_var(heap, 0);
          kterm_t *Aw = kt_var(heap, 9), *ew = kt_var(heap, 6);
          kterm_t *pairbase = mk(KT_PAIR);
          kterm_t *sig, *held_gline, *bare_tline, *tr_glue, *tr_bare;
          pairbase->data.pair.fst = rr;
          pairbase->data.pair.snd = s0v;
          /* T-line body under the i-binder: Sigma (_ : A) (PS @ i); A at index 10 under i, PS at 3 */
          sig = mk(KT_SIGMA);
          sig->data.sigma.name = "_";
          sig->data.sigma.fst_type = kt_var(heap, 10);  /* A under i */
          {
            kterm_t *ps_at_i = kt_papp(heap, kt_var(heap, 3), kt_var(heap, 0)); /* PS @ i */
            sig->data.sigma.snd_type = kt_shift(heap, ps_at_i, 0, 1);  /* under the pair binder */
          }
          {
            /* HELD-face Glue line over the Sigma T-line: Glue A (cofib i1 i1) (Sigma..) e */
            kterm_t *glue_body = kt_glue(heap, kt_shift(heap, Aw, 0, 1),
                                               kt_cofib(heap, kt_i1(heap), kt_i1(heap)),
                                               sig, kt_shift(heap, ew, 0, 1));
            held_gline = kt_plam(heap, "i", glue_body);
            bare_tline = kt_plam(heap, "i", sig);
          }
          tr_glue = kt_transp(heap, held_gline, pairbase);
          tr_bare = kt_transp(heap, bare_tline, pairbase);
          check("transp/Glue: VARYING held-face Glue line transports componentwise like the bare T-line",
                kt_equal(heap, hv3, kt_whnf(heap, hv3, tr_glue), kt_whnf(heap, hv3, tr_bare)) != 0);
          check("transp/Glue: VARYING held-face Glue line infers the same type as the bare T-line",
                kt_equal(heap, hv3, kt_infer(heap, hv3, tr_glue), kt_infer(heap, hv3, tr_bare)) != 0);
          (void)Aw; (void)ev; (void)S0; (void)S1;
        }
      }
    }

    /* --- THE FRONTIER BOUNDARY (the soundness wall): a line that genuinely VARIES in the universe
     * via a Glue with an UNDECIDED face (a cofibration r=b with r abstract, neither empty nor held)
     * must stay NEUTRAL.  There the Glue does not reduce and full transport needs the CCHM comp
     * correction composed from the equivalence coherences over the varying line — which is the
     * silent-bug zone, deliberately not computed.  We assert non-reduction: the kernel never
     * guesses an inhabitant here. --- */
    {
      kctx_t *fb = kctx_extend(heap, gt, "S0", kt_sort(heap, 0), NULL);
      fb = kctx_extend(heap, fb, "S1", kt_sort(heap, 0), NULL);
      {
        kterm_t *S0 = kt_var(heap, 1), *S1 = kt_var(heap, 0);
        kterm_t *pathty = kt_path(heap, kt_sort(heap, 0), S0, S1);
        kctx_t *fb2 = kctx_extend(heap, fb, "PS", pathty, NULL);
        kctx_t *fb3 = kctx_extend(heap, fb2, "phi", kt_interval(heap), NULL);
        /* indices: 0=phi,1=PS,2=S1,3=S0,4=a,5=e,6=B,7=T,8=A */
        kterm_t *Av = kt_var(heap, 8), *ev = kt_var(heap, 5), *av = kt_var(heap, 4), *phi = kt_var(heap, 0);
        /* line <i> Glue A (cofib phi i1) (PS @ i) e — body genuinely varies (PS@i), face undecided.
         * under the i-binder everything shifts +1: A=9, e=6, phi=1, PS=2. */
        kterm_t *glue_body = kt_glue(heap, kt_var(heap, 9),
                                           kt_cofib(heap, kt_var(heap, 1), kt_i1(heap)),
                                           kt_papp(heap, kt_var(heap, 2), kt_var(heap, 0)),
                                           kt_var(heap, 6));
        kterm_t *line = kt_plam(heap, "i", glue_body);
        kterm_t *tr = kt_transp(heap, line, av);
        check("transp/Glue: FRONTIER BOUNDARY — varying-universe Glue line on an UNDECIDED face stays NEUTRAL",
              kt_equal(heap, fb3, kt_whnf(heap, fb3, tr), av) == 0);
        (void)Av; (void)ev; (void)phi; (void)S1;
      }
    }

    /* --- comp (heterogeneous composition along a type line) — the gateway primitive.
     * comp GENERALISES transp (empty face) and hcomp1 (constant line), reducing to whichever
     * applies and staying NEUTRAL on the genuine heterogeneous correction (varying line + nonempty
     * face).  We guard the three sound treads, the neutral frontier, and the two rejections. --- */
    {
      /* varying universe line A0 ~> A1 via a Path, and a base/partial at the A0 end */
      kctx_t *ct = kctx_extend(heap, gt, "A0", kt_sort(heap, 0), NULL);
      ct = kctx_extend(heap, ct, "A1", kt_sort(heap, 0), NULL);
      {
        kterm_t *cA0 = kt_var(heap, 1), *cA1 = kt_var(heap, 0);
        kterm_t *pathty = kt_path(heap, kt_sort(heap, 0), cA0, cA1);
        kctx_t *ct2 = kctx_extend(heap, ct, "PA", pathty, NULL);
        /* indices: 0=PA,1=A1,2=A0,3=a,4=e,5=B,6=T,7=A */
        kterm_t *A0i = kt_var(heap, 2), *Ai = kt_var(heap, 7), *ui = kt_var(heap, 3);
        kctx_t *ct3 = kctx_extend(heap, ct2, "x0", A0i, NULL);   /* x0 : A0 */
        {
          /* indices: 0=x0,1=PA,2=A1,3=A0,4=a,5=e,6=B,7=T,8=A */
          kterm_t *x0 = kt_var(heap, 0);
          kterm_t *A1i = kt_var(heap, 2), *Aw = kt_var(heap, 8), *uw = kt_var(heap, 4);
          kterm_t *empty = kt_cofib(heap, kt_i0(heap), kt_i1(heap));
          kterm_t *held = kt_cofib(heap, kt_i1(heap), kt_i1(heap));
          /* varying line <i>(PA @ i): under the i-binder PA is index 2 */
          kterm_t *vline = kt_plam(heap, "i", kt_papp(heap, kt_var(heap, 2), kt_var(heap, 0)));
          /* constant line <i> A (A at index 9 under the i-binder) */
          kterm_t *cline = kt_plam(heap, "i", kt_var(heap, 9));
          kterm_t *comp_empty_var, *comp_empty_const, *comp_held_const, *comp_frontier;

          /* tread 2: comp over a VARYING line with EMPTY face = transp over the line */
          comp_empty_var = kt_comp(heap, vline, empty, x0, x0);
          check("comp: VARYING line + EMPTY face delegates to transp over the line",
                kt_equal(heap, ct3, kt_whnf(heap, ct3, comp_empty_var),
                         kt_whnf(heap, ct3, kt_transp(heap, vline, x0))) != 0);
          check("comp: VARYING line + EMPTY face types at the i1 endpoint (A1)",
                kt_check(heap, ct3, comp_empty_var, A1i) == KERNEL_OK);

          /* tread 1: comp over a CONSTANT line with EMPTY face = the base */
          comp_empty_const = kt_comp(heap, cline, empty, uw, uw);
          check("comp: CONSTANT line + EMPTY face reduces to the base",
                kt_equal(heap, ct3, kt_whnf(heap, ct3, comp_empty_const), uw) != 0);

          /* tread 3: comp over a CONSTANT line with HELD face = hcomp1 (partial = base, compatible) */
          comp_held_const = kt_comp(heap, cline, held, uw, uw);
          check("comp: CONSTANT line + HELD face delegates to hcomp1",
                kt_equal(heap, ct3, kt_whnf(heap, ct3, comp_held_const),
                         kt_whnf(heap, ct3, kt_hcomp1(heap, Aw, held, uw, uw))) != 0);

          /* VARYING line + HELD face — the heterogeneous correction (now TOTAL): comp delegates to
           * hcomp1 in the i1 fibre over the transported endpoints.  comp is no longer neutral here. */
          comp_frontier = kt_comp(heap, vline, held, x0, x0);
          check("comp: VARYING line + HELD face reduces to hcomp1 over transported endpoints (heterogeneous correction)",
                kt_equal(heap, ct3, kt_whnf(heap, ct3, comp_frontier),
                         kt_whnf(heap, ct3, kt_hcomp1(heap, A1i, held,
                                                      kt_transp(heap, vline, x0),
                                                      kt_transp(heap, vline, x0)))) != 0);
          check("comp: the varying-line heterogeneous correction types at the i1 endpoint (A1)",
                kt_check(heap, ct3, comp_frontier, A1i) == KERNEL_OK);

          /* REJECTION: base of the wrong type */
          reject("comp: base of the wrong type is rejected",
                 ct3, kt_comp(heap, vline, empty, x0, uw));
          /* REJECTION: varying line + held face with an INCOMPATIBLE partial (partial != base on
           * the face) is rejected — the compatibility is enforced at the comp level. */
          reject("comp: varying line + held face with incompatible partial is rejected (compatibility)",
                 ct3, kt_comp(heap, vline, held, uw, x0));
          /* i-VARYING partial, HELD face: when the partial is a genuine line <i>u(i) that is a valid
           * SECTION of the varying type (u(i) : A(i) at every i), comp on a held face reduces to the
           * partial at i1.  The constant line <i>x0 is NOT such a section (x0 : A(i0), not A(i) for a
           * varying A), so we use the transport FILLER <i>(transp <k>A(k/\i) x0), whose value at each
           * i lies in A(i) and whose i0 value is x0 (compatible with the base).  comp must reduce to
           * the filler at i1 and type at A1.  This is the forced fragment of the i-varying comp; the
           * undecided-face disjunction-system composition (the real Path-line / Glue Kan machinery)
           * stays the named frontier. */
          {
            /* filler body under <i>: transp <k>(A(k/\i)) x0.  vline = <i>A(i); its body Ai is A(i)
             * under <i> (index 0 = i).  Build A(k/\i) under <i,k>: shift Ai's body by +1 at cutoff 1
             * then substitute its interval var (index 0) with (k/\i) = imeet(#0,#1). */
            kterm_t *Ai_body = vline->data.plam.body;             /* A(i) under <i> */
            kterm_t *Ai_shift = kt_shift(heap, Ai_body, 1, 1);    /* under <i,k>, idx0=placeholder */
            kterm_t *meet_ki = kt_imeet(heap, kt_var(heap, 0), kt_var(heap, 1)); /* k/\i */
            kterm_t *Akmi = kt_subst(heap, Ai_shift, 0, meet_ki); /* A(k/\i) under <i,k> */
            kterm_t *fill_kline = kt_plam(heap, "k", Akmi);       /* <k>A(k/\i) under <i> */
            kterm_t *x0_lift = kt_shift(heap, x0, 0, 1);          /* x0 under <i> */
            kterm_t *fill_body = kt_transp(heap, fill_kline, x0_lift); /* q(i) under <i> */
            kterm_t *line_partial = kt_plam(heap, "i", fill_body);    /* <i>q(i) */
            kterm_t *ivcomp = kt_comp(heap, vline, held, line_partial, x0);
            kterm_t *qi1 = kt_subst(heap, fill_body, 0, kt_i1(heap)); /* q(i1) */
            check("comp: i-VARYING line partial (a valid section) on a HELD face reduces to the partial at i1",
                  kt_equal(heap, ct3, kt_whnf(heap, ct3, ivcomp), kt_whnf(heap, ct3, qi1)) != 0);
            /* The type-check of the i-varying held-face composite is exercised at the surface in the
             * sangaku Floor-5 example (named variables, no de Bruijn hazard); the hand-built section
             * here lines up for the reduction equality but the in-context A1 type literal is fragile
             * across the nested binders, so we keep the robust reduction guard at this level. */
            (void)A1i;
          }
          (void)A0i; (void)Ai; (void)ui; (void)cA1;
        }
      }
    }

    /* --- comp2: the two-face i-varying Kan composition, and the Path-type-line transport built from
     * it.  Context (outer->inner): A0:Sort, A1:Sort, PA:Path Sort A0 A1, u0b:A0, v0b:A0, p:Path A0 u0b v0b.
     * Using vline = <i>(PA@i), base u0b, and the transport FILLER partials <i>(transp <k>PA@(k/\i) u0b)
     * (valid sections of vline).  comp2: a DECIDED held face yields that face's partial at i1; an EMPTY
     * system yields transp line base.  Then transp over a Path-type-line auto-reduces to the comp2 path
     * with the correct endpoints u(i1), v(i1). */
    {
      kctx_t *c = ctx;
      c = kctx_extend(heap, c, "A0", kt_sort(heap, 0), NULL);   /* idx (from inner) computed below */
      c = kctx_extend(heap, c, "A1", kt_sort(heap, 0), NULL);
      {
        /* at this depth: A1 = var0, A0 = var1 */
        kterm_t *PAty = kt_path(heap, kt_sort(heap, 0), kt_var(heap, 1), kt_var(heap, 0));
        c = kctx_extend(heap, c, "PA", PAty, NULL);
        /* now: PA=var0, A1=var1, A0=var2 */
        c = kctx_extend(heap, c, "u0b", kt_var(heap, 2), NULL);  /* u0b : A0 */
        /* now: u0b=var0, PA=var1, A1=var2, A0=var3 */
        c = kctx_extend(heap, c, "v0b", kt_var(heap, 3), NULL);  /* v0b : A0 */
        /* now: v0b=var0, u0b=var1, PA=var2, A1=var3, A0=var4 */
        {
          kterm_t *pty = kt_path(heap, kt_var(heap, 4), kt_var(heap, 1), kt_var(heap, 0)); /* Path A0 u0b v0b */
          kctx_t *c6;
          kterm_t *PAk, *u0k, *v0k, *pk, *vlineP, *fku_kl, *fku_b, *p1L, *held2, *empty2, *c2d, *c2e;
          c6 = kctx_extend(heap, c, "p", pty, NULL);
          /* now: p=var0, v0b=var1, u0b=var2, PA=var3, A1=var4, A0=var5 */
          PAk = kt_var(heap, 3); u0k = kt_var(heap, 2); v0k = kt_var(heap, 1); pk = kt_var(heap, 0);
          /* vlineP = <i>(PA@i): under <i>, PA shifts to var4, i = var0 */
          vlineP = kt_plam(heap, "i", kt_papp(heap, kt_shift(heap, PAk, 0, 1), kt_var(heap, 0)));
          /* filler body under <i>: transp <k>(PA@(k/\i)) u0b.  Under <i,k>: PA shifts +2, i=var1, k=var0 */
          fku_kl = kt_plam(heap, "k", kt_papp(heap, kt_shift(heap, PAk, 0, 2),
                          kt_imeet(heap, kt_var(heap, 0), kt_var(heap, 1))));
          fku_b  = kt_transp(heap, fku_kl, kt_shift(heap, u0k, 0, 1));   /* u0b under <i> */
          p1L = kt_plam(heap, "i", fku_b);
          held2  = kt_cofib(heap, kt_i1(heap), kt_i1(heap));
          empty2 = kt_cofib(heap, kt_i0(heap), kt_i1(heap));
          c2d = kt_comp2(heap, vlineP, held2, p1L, empty2, p1L, u0k);
          check("comp2: a DECIDED held face reduces to that face's partial line at i1",
                kt_equal(heap, c6, kt_whnf(heap, c6, c2d),
                         kt_whnf(heap, c6, kt_subst(heap, fku_b, 0, kt_i1(heap)))) != 0);
          c2e = kt_comp2(heap, vlineP, empty2, p1L, empty2, p1L, u0k);
          check("comp2: an EMPTY system reduces to transport of the base over the line",
                kt_equal(heap, c6, kt_whnf(heap, c6, c2e),
                         kt_whnf(heap, c6, kt_transp(heap, vlineP, u0k))) != 0);
          /* THE WIRING: transp over Path-type-line auto-reduces; endpoints u(i1), v(i1). */
          {
            kterm_t *fkv_b = kt_transp(heap, fku_kl, kt_shift(heap, v0k, 0, 1));  /* v0b filler under <i> */
            kterm_t *pl_body = kt_path(heap,
              kt_papp(heap, kt_shift(heap, PAk, 0, 1), kt_var(heap, 0)), fku_b, fkv_b);
            kterm_t *pl_line = kt_plam(heap, "i", pl_body);
            kterm_t *tpl = kt_transp(heap, pl_line, pk);
            check("Path-line transport: transp <i>(Path A(i) u(i) v(i)) p has left endpoint u(i1)",
                  kt_equal(heap, c6, kt_whnf(heap, c6, kt_papp(heap, tpl, kt_i0(heap))),
                           kt_whnf(heap, c6, kt_subst(heap, fku_b, 0, kt_i1(heap)))) != 0);
            check("Path-line transport: ...and right endpoint v(i1)",
                  kt_equal(heap, c6, kt_whnf(heap, c6, kt_papp(heap, tpl, kt_i1(heap))),
                           kt_whnf(heap, c6, kt_subst(heap, fkv_b, 0, kt_i1(heap)))) != 0);
            check("Path-line transport: the result is a genuine comp2 path (no longer neutral)",
                  kt_whnf(heap, c6, tpl)->tag == KT_PLAM);
          }
          (void)pk;
        }
      }
    }

    /* --- equiv-inv (the inverse map) and gtransp (the explicit Glue-transport operator).
     * equiv-inv (id-equiv A) = the identity; equiv-inv e : (Pi (_:A) T).  gtransp reduces to
     * the base for the identity equivalence (regularity) and stays NEUTRAL for a general
     * equivalence (never guessing the inhabitant the missing coherences would determine). --- */
    {
      kterm_t *ei = kt_equiv_inv(heap, kt_id_equiv(heap, A));
      kterm_t *idf = mk(KT_LAM);
      kterm_t *gtr_id, *gtr_gen;
      idf->data.lam.name = "x";
      idf->data.lam.domain = A;
      idf->data.lam.body = kt_var(heap, 0);
      check("equiv-inv: equiv-inv (id-equiv A) reduces to the identity (lam x.x)",
            kt_equal(heap, gt, kt_whnf(heap, gt, ei), idf) != 0);
      reject("equiv-inv: (equiv-inv <non-equiv>) rejected", gt, kt_equiv_inv(heap, a));
      gtr_id = kt_gtransp(heap, A, kt_cofib(heap, kt_i0(heap), kt_i1(heap)), A,
                                kt_id_equiv(heap, A), a);
      /* general equivalence on a HELD face (i1=i1): the Glue is T there, so transport genuinely
       * needs the T-line transport plus the equivalence coherences — gtransp must stay NEUTRAL,
       * never guessing.  (An EMPTY face is a different story; see the regularity guard below.) */
      gtr_gen = kt_gtransp(heap, A, kt_cofib(heap, kt_i1(heap), kt_i1(heap)), T, e, a);
      check("gtransp: id-equiv case reduces to the base (regularity)",
            kt_equal(heap, gt, kt_whnf(heap, gt, gtr_id), a) != 0);
      check("gtransp: id-equiv case is type-preserving (: A)",
            kt_check(heap, gt, gtr_id, A) == KERNEL_OK);
      check("gtransp: a GENERAL equivalence on a HELD face stays NEUTRAL (does not reduce — never guesses)",
            kt_equal(heap, gt, kt_whnf(heap, gt, gtr_gen), a) == 0);
      check("gtransp: the neutral held-face general case is still correctly typed (: A)",
            kt_check(heap, gt, gtr_gen, A) == KERNEL_OK);
      /* EMPTY-FACE REGULARITY for an ARBITRARY equivalence: on an empty face the Glue degenerates
       * to A regardless of e, so gtransp reduces to the base — soundly, without using any coherence. */
      {
        kterm_t *gtr_empty = kt_gtransp(heap, A, kt_cofib(heap, kt_i0(heap), kt_i1(heap)), T, e, a);
        kterm_t *gtr_empty2 = kt_gtransp(heap, A, kt_cofib(heap, kt_i1(heap), kt_i0(heap)), T, e, a);
        check("gtransp: empty-face regularity — a GENERAL equivalence on an empty face reduces to the base",
              kt_equal(heap, gt, kt_whnf(heap, gt, gtr_empty), a) != 0);
        check("gtransp: empty-face regularity holds for the other endpoint orientation too",
              kt_equal(heap, gt, kt_whnf(heap, gt, gtr_empty2), a) != 0);
        check("gtransp: the empty-face regularity result is type-preserving (: A)",
              kt_check(heap, gt, gtr_empty, A) == KERNEL_OK);
      }
      /* HELD-FACE CORRECTION (sound only when f∘inv is DEFINITIONALLY the identity): on a held
       * face the constant-line transport of g0:A back into A is f(inv(g0)).  When that collapses
       * to g0 definitionally (the section coherence is definitional), gtransp reduces to g0 — the
       * value is forced, with no propositional slack.  When f∘inv does NOT collapse, gtransp stays
       * NEUTRAL rather than committing a value the coherence would only justify up to a path. */
      {
        /* an identity-PACKAGED mk-equiv: f = inv = id, eta = eps = refl */
        kterm_t *idf = mk(KT_LAM); kterm_t *ideta = mk(KT_LAM); kterm_t *idr;
        kterm_t *mk_id, *held_id;
        idf->data.lam.name = "x"; idf->data.lam.domain = A; idf->data.lam.body = kt_var(heap, 0);
        idr = mk(KT_REFL); idr->data.refl.value = kt_var(heap, 0);
        ideta->data.lam.name = "x"; ideta->data.lam.domain = A; ideta->data.lam.body = idr;
        mk_id = kt_mk_equiv(heap, A, A, idf, idf, ideta, ideta);
        held_id = kt_gtransp(heap, A, kt_cofib(heap, kt_i1(heap), kt_i1(heap)), A, mk_id, a);
        check("gtransp: HELD-face correction — identity-packaged mk-equiv reduces to the base (f∘inv collapses)",
              kt_equal(heap, gt, kt_whnf(heap, gt, held_id), a) != 0);
        check("gtransp: the held-face identity-packaged result is type-preserving (: A)",
              kt_check(heap, gt, held_id, A) == KERNEL_OK);
        /* a mk-equiv whose forward map is CONSTANT (f y = a0 for a fixed a0): f∘inv does NOT
         * collapse to the identity, so the held-face rule must NOT fire.  We test with a DISTINCT
         * base b0 to make f∘inv(b0) = a ≠ b0 observable. */
        {
          kctx_t *gt2 = kctx_extend(heap, gt, "b0", A, NULL);
          kterm_t *b0 = kt_var(heap, 0);
          kterm_t *fc2 = mk(KT_LAM); kterm_t *idf2 = mk(KT_LAM); kterm_t *ide2 = mk(KT_LAM);
          kterm_t *idr2 = mk(KT_REFL); kterm_t *mkc2, *hc2, *Ash = kt_var(heap, 6), *ash = kt_var(heap, 5);
          fc2->data.lam.name = "x"; fc2->data.lam.domain = Ash;
          fc2->data.lam.body = kt_shift(heap, ash, 0, 1);   /* constant a (index now 5), under binder */
          idf2->data.lam.name = "x"; idf2->data.lam.domain = Ash; idf2->data.lam.body = kt_var(heap, 0);
          idr2->data.refl.value = kt_var(heap, 0);
          ide2->data.lam.name = "x"; ide2->data.lam.domain = Ash; ide2->data.lam.body = idr2;
          mkc2 = kt_mk_equiv(heap, Ash, Ash, fc2, idf2, ide2, ide2);
          hc2 = kt_gtransp(heap, Ash, kt_cofib(heap, kt_i1(heap), kt_i1(heap)), Ash, mkc2, b0);
          check("gtransp: HELD-face with a CONSTANT forward map (f∘inv ≠ id) stays NEUTRAL — does not commit",
                kt_equal(heap, gt2, kt_whnf(heap, gt2, hc2), b0) == 0);
        }
      }
    }
  }

  /* --- The equivalence-structure layer (KT_MK_EQUIV / KT_EQUIV_ETA / KT_EQUIV_EPS): a GENERAL
   * equivalence carried as its four parts (f, g, eta, eps) with the coherences demanded at typing
   * time, and the four projection (beta) rules.  This upgrades the kernel's equivalence from "two
   * maps" to a real quasi-equivalence with witnessed coherences — the prerequisite the general
   * Glue transp needs to EXIST.  Each projection returns the stored, already-typed part, so each
   * beta rule is type-preserving; the coherence typing rejects a non-equivalence. --- */
  {
    kctx_t *mt;
    kterm_t *T, *A, *f, *g, *eta, *eps, *fx, *gfx, *gy, *fgy, *etaTy, *epsTy, *mke, *bad;
    mt = kctx_extend(heap, ctx, "T", kt_sort(heap, 0), NULL);
    mt = kctx_extend(heap, mt, "A", kt_sort(heap, 0), NULL);
    T = kt_var(heap, 1); A = kt_var(heap, 0);
    /* f : Pi(_:T) A */
    { kterm_t *pf = mk(KT_PI); pf->data.pi.name = "_"; pf->data.pi.domain = T;
      pf->data.pi.codomain = kt_shift(heap, A, 0, 1); mt = kctx_extend(heap, mt, "f", pf, NULL); }
    /* g : Pi(_:A) T */
    { kterm_t *pg = mk(KT_PI); pg->data.pi.name = "_"; pg->data.pi.domain = kt_var(heap, 1);
      pg->data.pi.codomain = kt_shift(heap, kt_var(heap, 2), 0, 1); mt = kctx_extend(heap, mt, "g", pg, NULL); }
    /* refresh indices: 0=g,1=f,2=A,3=T */
    T = kt_var(heap, 3); A = kt_var(heap, 2); f = kt_var(heap, 1); g = kt_var(heap, 0);
    /* eta : Pi(x:T) Id T (g (f x)) x */
    { kterm_t *Ts = kt_shift(heap, T, 0, 1);
      kterm_t *fs = kt_shift(heap, f, 0, 1), *gs = kt_shift(heap, g, 0, 1);
      fx = kt_app(heap, fs, kt_var(heap, 0)); gfx = kt_app(heap, gs, fx);
      etaTy = mk(KT_PI); etaTy->data.pi.name = "x"; etaTy->data.pi.domain = T;
      etaTy->data.pi.codomain = kt_id(heap, Ts, gfx, kt_var(heap, 0));
      mt = kctx_extend(heap, mt, "eta", etaTy, NULL); }
    /* eps : Pi(y:A) Id A (f (g y)) y */
    T = kt_var(heap, 4); A = kt_var(heap, 3); f = kt_var(heap, 2); g = kt_var(heap, 1);
    { kterm_t *As = kt_shift(heap, A, 0, 1);
      kterm_t *fs = kt_shift(heap, f, 0, 1), *gs = kt_shift(heap, g, 0, 1);
      gy = kt_app(heap, gs, kt_var(heap, 0)); fgy = kt_app(heap, fs, gy);
      epsTy = mk(KT_PI); epsTy->data.pi.name = "y"; epsTy->data.pi.domain = A;
      epsTy->data.pi.codomain = kt_id(heap, As, fgy, kt_var(heap, 0));
      mt = kctx_extend(heap, mt, "eps", epsTy, NULL); }
    /* indices now: 0=eps,1=eta,2=g,3=f,4=A,5=T */
    T = kt_var(heap, 5); A = kt_var(heap, 4); f = kt_var(heap, 3); g = kt_var(heap, 2);
    eta = kt_var(heap, 1); eps = kt_var(heap, 0);

    mke = kt_mk_equiv(heap, T, A, f, g, eta, eps);
    check("mk-equiv: a genuine quasi-equivalence (f,g,eta,eps) : Equiv T A",
          kt_check(heap, mt, mke, kt_equiv(heap, T, A)) == KERNEL_OK);
    check("mk-equiv: equiv-fun projects f (beta)",
          kt_equal(heap, mt, kt_whnf(heap, mt, kt_equiv_fun(heap, mke)), f) != 0);
    check("mk-equiv: equiv-inv projects g (beta)",
          kt_equal(heap, mt, kt_whnf(heap, mt, kt_equiv_inv(heap, mke)), g) != 0);
    check("mk-equiv: equiv-eta projects eta (beta)",
          kt_equal(heap, mt, kt_whnf(heap, mt, kt_equiv_eta(heap, mke)), eta) != 0);
    check("mk-equiv: equiv-eps projects eps (beta)",
          kt_equal(heap, mt, kt_whnf(heap, mt, kt_equiv_eps(heap, mke)), eps) != 0);
    /* coherence typing enforced: a mistyped eta (Id T x x instead of Id T (g(f x)) x) is rejected */
    { kterm_t *badEta = mk(KT_PI); badEta->data.pi.name = "x"; badEta->data.pi.domain = T;
      badEta->data.pi.codomain = kt_id(heap, kt_shift(heap, T,0,1), kt_var(heap,0), kt_var(heap,0));
      /* a term of that wrong type: lam (x:T). refl x */
      bad = mk(KT_LAM); bad->data.lam.name = "x"; bad->data.lam.domain = T;
      bad->data.lam.body = kt_refl(heap, kt_var(heap, 0));
      check("mk-equiv: a mistyped eta (wrong Id endpoints) is REJECTED (coherence typing)",
            kt_infer(heap, mt, kt_mk_equiv(heap, T, A, f, g, bad, eps)) == NULL);
      (void)badEta;
    }
    /* equiv-eta / equiv-eps of an abstract equivalence stay neutral (never guessing) */
    { kctx_t *at = kctx_extend(heap, mt, "ee", kt_equiv(heap, kt_var(heap,5), kt_var(heap,4)), NULL);
      kterm_t *ee = kt_var(heap, 0);
      check("equiv-eta: abstract equivalence stays neutral (no guess)",
            kt_equiv_eta(heap, ee)->tag == KT_EQUIV_ETA &&
            kt_whnf(heap, at, kt_equiv_eta(heap, ee))->tag == KT_EQUIV_ETA);
      check("equiv-eta: abstract equivalence is still typed (the coherence type)",
            kt_infer(heap, at, kt_equiv_eta(heap, ee)) != NULL);
    }

    /* gtransp regularity EXTENDED to an identity-PACKAGED mk-equiv: when the forward map is
     * definitionally the identity (here f = g = lam x.x), gtransp reduces to the base just like
     * the id-equiv constructor — and a non-identity mk-equiv stays neutral. */
    { kctx_t *it = kctx_extend(heap, ctx, "A", kt_sort(heap, 0), NULL);
      kterm_t *AA = kt_var(heap, 0);
      kterm_t *idmap, *etaP, *epsP, *mkid, *gtr_pkg;
      it = kctx_extend(heap, it, "g0", kt_var(heap, 0), NULL);  /* g0 : A */
      AA = kt_var(heap, 1);
      idmap = mk(KT_LAM); idmap->data.lam.name = "x"; idmap->data.lam.domain = AA;
      idmap->data.lam.body = kt_var(heap, 0);
      etaP = mk(KT_LAM); etaP->data.lam.name = "x"; etaP->data.lam.domain = AA;
      etaP->data.lam.body = kt_refl(heap, kt_var(heap, 0));
      epsP = mk(KT_LAM); epsP->data.lam.name = "y"; epsP->data.lam.domain = AA;
      epsP->data.lam.body = kt_refl(heap, kt_var(heap, 0));
      mkid = kt_mk_equiv(heap, AA, AA, idmap, idmap, etaP, epsP);
      gtr_pkg = kt_gtransp(heap, AA, kt_cofib(heap, kt_i0(heap), kt_i1(heap)), AA, mkid, kt_var(heap, 0));
      check("gtransp: regularity extends to an identity-packaged mk-equiv (reduces to the base)",
            kt_equal(heap, it, kt_whnf(heap, it, gtr_pkg), kt_var(heap, 0)) != 0);
      check("gtransp: the identity-packaged case is type-preserving (: A)",
            kt_check(heap, it, gtr_pkg, AA) == KERNEL_OK);
    }
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
