/* kt_to_core.c — see kt_to_core.h. */
#include "kt_to_core.h"
#include <stddef.h>

/* Church booleans in the untyped core (de Bruijn: index 0 = innermost binder). */
static core_t *church_true(void)  { return core_lam(core_lam(core_var(1))); } /* \t.\f. t */
static core_t *church_false(void) { return core_lam(core_lam(core_var(0))); } /* \t.\f. f */

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

    default:
      return NULL;   /* outside the shared fragment */
  }
}
