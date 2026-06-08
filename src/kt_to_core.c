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

    default:
      return NULL;   /* outside the shared fragment */
  }
}
