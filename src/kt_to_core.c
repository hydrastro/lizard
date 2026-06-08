/* kt_to_core.c — see kt_to_core.h. */
#include "kt_to_core.h"
#include <stddef.h>

/* Church booleans in the untyped core (de Bruijn: index 0 = innermost binder). */
static core_t *church_true(void)  { return core_lam(core_lam(core_var(1))); } /* \t.\f. t */
static core_t *church_false(void) { return core_lam(core_lam(core_var(0))); } /* \t.\f. f */

/* Church naturals (closed combinators, so applying them needs no shifting). */
static core_t *church_zero(void) { return core_lam(core_lam(core_var(0))); }   /* \s.\z. z */
static core_t *church_succ(void) {                                             /* \n.\s.\z. s (n s z) */
  return core_lam(core_lam(core_lam(
    core_app(core_var(1),
             core_app(core_app(core_var(2), core_var(1)), core_var(0))))));
}

/* Church lists (right fold), closed combinators. */
static core_t *church_nil(void) { return core_lam(core_lam(core_var(0))); }    /* \c.\n. n */
static core_t *church_cons(void) {                                             /* \h.\t.\c.\n. c h (t c n) */
  return core_lam(core_lam(core_lam(core_lam(
    core_app(core_app(core_var(1), core_var(3)),
             core_app(core_app(core_var(2), core_var(1)), core_var(0)))))));
}

/* de Bruijn shift: return a fresh copy of t with every free index >= cutoff
 * raised by d.  Needed when a translated subterm is placed under a new binder
 * (the natrec step function below).  With d == 0 this is a structural deep copy. */
static core_t *core_shift(const core_t *t, int d, int cutoff) {
  if (t == NULL) return NULL;
  switch (t->kind) {
    case CT_VAR:   return core_var(t->idx >= cutoff ? t->idx + d : t->idx);
    case CT_LAM:   return core_lam(core_shift(t->a, d, cutoff + 1));
    case CT_APP:   return core_app(core_shift(t->a, d, cutoff), core_shift(t->b, d, cutoff));
    case CT_PAIR:  return core_pair(core_shift(t->a, d, cutoff), core_shift(t->b, d, cutoff));
    case CT_PROJ1: return core_proj1(core_shift(t->a, d, cutoff));
    case CT_PROJ2: return core_proj2(core_shift(t->a, d, cutoff));
    case CT_LET:   return core_let(core_shift(t->a, d, cutoff), core_shift(t->b, d, cutoff + 1));
    case CT_LIT:   return core_lit_z(t->num);
    case CT_PRIM:  return core_prim(t->op, core_shift(t->a, d, cutoff), core_shift(t->b, d, cutoff));
  }
  return NULL; /* unreachable: all core kinds handled above */
}

core_t *kt_to_core(const kterm_t *t) {
  if (t == NULL) return NULL;
  switch (t->tag) {
    case KT_VAR:
      return core_var(t->data.var.index);

    case KT_LAM: {
      core_t *b = kt_to_core(t->data.lam.body);   /* domain dropped */
      if (b == NULL) return NULL;
      return core_lam(b);
    }
    case KT_APP: {
      core_t *f = kt_to_core(t->data.app.fun);
      core_t *a = kt_to_core(t->data.app.arg);
      if (f == NULL || a == NULL) { core_free(f); core_free(a); return NULL; }
      return core_app(f, a);
    }
    case KT_PAIR: {
      core_t *a = kt_to_core(t->data.pair.fst);
      core_t *b = kt_to_core(t->data.pair.snd);
      if (a == NULL || b == NULL) { core_free(a); core_free(b); return NULL; }
      return core_pair(a, b);
    }
    case KT_PROJ1: {
      core_t *p = kt_to_core(t->data.proj.target);
      return (p == NULL) ? NULL : core_proj1(p);
    }
    case KT_PROJ2: {
      core_t *p = kt_to_core(t->data.proj.target);
      return (p == NULL) ? NULL : core_proj2(p);
    }
    case KT_LET: {
      core_t *v = kt_to_core(t->data.let.value);
      core_t *b = kt_to_core(t->data.let.body);
      if (v == NULL || b == NULL) { core_free(v); core_free(b); return NULL; }
      return core_let(v, b);
    }

    case KT_TRUE:  return church_true();
    case KT_FALSE: return church_false();
    case KT_BOOL_REC: {
      /* bool_rec C t f s  ==>  ((s t) f)   (Church: true t f = t, false t f = f) */
      core_t *s  = kt_to_core(t->data.bool_rec.scrutinee);
      core_t *tc = kt_to_core(t->data.bool_rec.true_case);
      core_t *fc = kt_to_core(t->data.bool_rec.false_case);
      if (s == NULL || tc == NULL || fc == NULL) {
        core_free(s); core_free(tc); core_free(fc); return NULL;
      }
      return core_app(core_app(s, tc), fc);
    }

    /* Coproducts and options are finite (non-recursive) data: encode a value as a
     * first-class Sigma pair (tag, payload) with a Church-boolean tag, reusing the
     * PAIR/FST/SND agents.  The payload sits in a pair, not under a binder, so no
     * de Bruijn shifting is needed.  The eliminators match the kernel's whnf:
     * sum_rec applies the chosen case to the value; maybe's nothing-case is a value. */
    case KT_INL: {                               /* inl v  ==>  (true, v)  */
      core_t *v = kt_to_core(t->data.inl.value);
      return (v == NULL) ? NULL : core_pair(church_true(), v);
    }
    case KT_INR: {                               /* inr v  ==>  (false, v) */
      core_t *v = kt_to_core(t->data.inr.value);
      return (v == NULL) ? NULL : core_pair(church_false(), v);
    }
    case KT_SUM_REC: {
      /* sum_rec C lc rc s  ==>  ((fst s) (lc (snd s))) (rc (snd s))
       * s is used three times; build independent copies so the core tree has no
       * aliasing and introduces no binder. */
      core_t *s1 = kt_to_core(t->data.sum_rec.scrutinee);
      core_t *s2 = kt_to_core(t->data.sum_rec.scrutinee);
      core_t *s3 = kt_to_core(t->data.sum_rec.scrutinee);
      core_t *lc = kt_to_core(t->data.sum_rec.left_case);
      core_t *rc = kt_to_core(t->data.sum_rec.right_case);
      if (!s1 || !s2 || !s3 || !lc || !rc) {
        core_free(s1); core_free(s2); core_free(s3); core_free(lc); core_free(rc);
        return NULL;
      }
      return core_app(core_app(core_proj1(s1), core_app(lc, core_proj2(s2))),
                      core_app(rc, core_proj2(s3)));
    }
    case KT_NOTHING:                             /* nothing ==> (false, 0)  */
      return core_pair(church_false(), core_lit_si(0));
    case KT_JUST: {                              /* just v  ==> (true, v)   */
      core_t *v = kt_to_core(t->data.just.value);
      return (v == NULL) ? NULL : core_pair(church_true(), v);
    }
    case KT_MAYBE_REC: {
      /* maybe_rec C nc jc m  ==>  ((fst m) (jc (snd m))) nc
       * the nothing-case nc is a value (kernel does not apply it). */
      core_t *m1 = kt_to_core(t->data.maybe_rec.scrutinee);
      core_t *m2 = kt_to_core(t->data.maybe_rec.scrutinee);
      core_t *jc = kt_to_core(t->data.maybe_rec.just_case);
      core_t *nc = kt_to_core(t->data.maybe_rec.nothing_case);
      if (!m1 || !m2 || !jc || !nc) {
        core_free(m1); core_free(m2); core_free(jc); core_free(nc);
        return NULL;
      }
      return core_app(core_app(core_proj1(m1), core_app(jc, core_proj2(m2))), nc);
    }

    /* Nat as Church numerals, with primitive recursion derived from iteration.
     * This is genuine recursion in the net: the numeral drives the iteration,
     * which reduces through real CON/DUP interactions.  No special agent is
     * required, and (crucially for the cross-check) it terminates because the
     * numeral is finite. */
    case KT_ZERO: return church_zero();
    case KT_SUCC: {                              /* succ n ==> church_succ n */
      core_t *n = kt_to_core(t->data.succ.pred);
      return (n == NULL) ? NULL : core_app(church_succ(), n);
    }
    case KT_NAT_REC: {
      /* natrec C z s scrut  ==>  snd (scrut step base)         [pair trick]
       *   base = (church_zero, z)
       *   step = \p. (church_succ (fst p),  s (fst p) (snd p))
       * scrut is a Church numeral, so (scrut step base) applies step |scrut|
       * times to base; the first component tracks the predecessor and the second
       * accumulates  s applied to (predecessor, recursive result). */
      core_t *zc = kt_to_core(t->data.nat_rec.zero_case);
      core_t *sc = kt_to_core(t->data.nat_rec.succ_case);
      core_t *nc = kt_to_core(t->data.nat_rec.scrutinee);
      core_t *base, *step, *sc_shifted;
      if (zc == NULL || sc == NULL || nc == NULL) {
        core_free(zc); core_free(sc); core_free(nc); return NULL;
      }
      base = core_pair(church_zero(), zc);
      sc_shifted = core_shift(sc, 1, 0);         /* s goes under the step's binder */
      core_free(sc);
      step = core_lam(core_pair(
               core_app(church_succ(), core_proj1(core_var(0))),
               core_app(core_app(sc_shifted, core_proj1(core_var(0))),
                        core_proj2(core_var(0)))));
      return core_proj2(core_app(core_app(nc, step), base));
    }

    /* List, same shape as Nat: structural recursion via the foldr pair-trick.
     * listrec C n c (cons h t) = c h t (listrec ... t), so the carried pair is
     * (reconstructed tail, accumulator) and c receives (head, tail, recursion). */
    case KT_NIL_K:  return church_nil();
    case KT_CONS_K: {                            /* cons h t ==> church_cons h t */
      core_t *h = kt_to_core(t->data.cons_k.head);
      core_t *tl = kt_to_core(t->data.cons_k.tail);
      if (h == NULL || tl == NULL) { core_free(h); core_free(tl); return NULL; }
      return core_app(core_app(church_cons(), h), tl);
    }
    case KT_LIST_REC: {
      /* listrec C n c scrut  ==>  snd (scrut step base)
       *   base = (church_nil, n)
       *   step = \h.\p. (church_cons h (fst p),  c h (fst p) (snd p)) */
      core_t *ncase = kt_to_core(t->data.list_rec.nil_case);
      core_t *ccase = kt_to_core(t->data.list_rec.cons_case);
      core_t *scrut = kt_to_core(t->data.list_rec.scrutinee);
      core_t *base, *step, *cc_shifted;
      if (ncase == NULL || ccase == NULL || scrut == NULL) {
        core_free(ncase); core_free(ccase); core_free(scrut); return NULL;
      }
      base = core_pair(church_nil(), ncase);
      cc_shifted = core_shift(ccase, 2, 0);      /* c goes under \h.\p (two binders) */
      core_free(ccase);
      step = core_lam(core_lam(core_pair(        /* \h.\p. ... ; h = var 1, p = var 0 */
               core_app(core_app(church_cons(), core_var(1)), core_proj1(core_var(0))),
               core_app(core_app(core_app(cc_shifted, core_var(1)),
                                 core_proj1(core_var(0))),
                        core_proj2(core_var(0))))));
      return core_proj2(core_app(core_app(scrut, step), base));
    }

    /* Propositional equality eliminator.  By canonicity a closed proof of
     * Id_A(a,b) reduces to refl, and the kernel's only Id computation rule is the
     * J-beta step  J C d A a b (refl v) -> d v.  refl is therefore transparent
     * (it carries its witness), and J becomes an application of the base case to
     * that witness — matching how the other eliminators reduce on closed terms.
     * (This is the J computation rule only; it is NOT the by-observation Id-as-a-
     * type computation, which this kernel does not implement either.) */
    case KT_REFL: return kt_to_core(t->data.refl.value);
    case KT_J: {                                 /* J C d A a b p  ==>  d p   (p ~ refl v) */
      core_t *d = kt_to_core(t->data.j.base_case);
      core_t *p = kt_to_core(t->data.j.proof);
      if (d == NULL || p == NULL) { core_free(d); core_free(p); return NULL; }
      return core_app(d, p);
    }

    default:
      return NULL;   /* outside the shared fragment */
  }
}
