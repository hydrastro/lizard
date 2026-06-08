/* net_eval.c — see net_eval.h.  Runs a kernel term on the interaction net and
 * reads the result back into a kterm, behind a gate with a kt_whnf fallback. */
#include "net_eval.h"
#include "kt_to_core.h"   /* kt_to_core            */
#include "ic_lower.h"     /* core_t builders, ic_lower */
#include "ic.h"           /* ic_normalize_int, IC_ADD  */
#include "mem.h"          /* lizard_heap_alloc         */
#include <stdlib.h>
#include <string.h>
#include <gmp.h>

static int g_enabled = 0;
void kt_net_eval_set_enabled(int on) { g_enabled = on ? 1 : 0; }
int  kt_net_eval_enabled(void)       { return g_enabled; }

/* lower an observed core term and read its integer normal form (1 + value / 0) */
static int observe_int(core_t *obs, long *out) {
  ic_term_t *e = ic_lower(obs);
  int ok = 0;
  if (e != NULL) {
    mpz_t o; long it = 0;
    mpz_init(o);
    if (ic_normalize_int(e, o, &it) == 1) { *out = mpz_get_si(o); ok = 1; }
    mpz_clear(o);
    ic_term_free(e);
  }
  return ok;
}

static kterm_t *kt_numeral(lizard_heap_t *heap, long n) {
  kterm_t *t = kt_zero(heap);
  while (n-- > 0) t = kt_succ(heap, t);
  return t;
}

static kterm_t *kt_bool(lizard_heap_t *heap, int b) {
  kterm_t *t = (kterm_t *)lizard_heap_alloc(sizeof *t);
  memset(t, 0, sizeof *t);
  t->tag = b ? KT_TRUE : KT_FALSE;
  return t;
}

int kt_eval_via_net(lizard_heap_t *heap, kctx_t *ctx, const kterm_t *t,
                    net_result_kind kind, kterm_t **out) {
  core_t *c = kt_to_core(t), *obs;
  long v = 0;
  int ok;
  (void)ctx;
  if (c == NULL) return 0;                         /* outside the net's fragment */
  if (kind == NET_RESULT_BOOL) {
    /* observe a Church boolean as  (b 1) 0  ->  1 (true) / 0 (false) */
    obs = core_app(core_app(c, core_lit_si(1)), core_lit_si(0));
  } else {
    /* observe a Church numeral as  n (\x. x+1) 0  ->  the integer */
    obs = core_app(core_app(c, core_lam(core_prim(IC_ADD, core_var(0), core_lit_si(1)))),
                   core_lit_si(0));
  }
  ok = observe_int(obs, &v);
  core_free(obs);
  if (!ok) return 0;
  *out = (kind == NET_RESULT_BOOL) ? kt_bool(heap, v ? 1 : 0) : kt_numeral(heap, v);
  return 1;
}

kterm_t *kt_normalize_gated(lizard_heap_t *heap, kctx_t *ctx, kterm_t *t,
                            net_result_kind kind) {
  if (g_enabled) {
    kterm_t *out = NULL;
    if (kt_eval_via_net(heap, ctx, t, kind, &out)) return out;  /* the net evaluated it */
  }
  return kt_whnf(heap, ctx, t);                    /* fallback: the trusted reducer */
}

int kt_eval_via_net_auto(lizard_heap_t *heap, kctx_t *ctx, kterm_t *t,
                         kterm_t **out) {
  kterm_t *ty = kt_infer(heap, ctx, t);             /* infer the term's type */
  net_result_kind kind;
  if (ty == NULL) return 0;                          /* ill-typed: decline        */
  ty = kt_whnf(heap, ctx, ty);                       /* head-normalize the type    */
  if (ty->tag == KT_NAT)       kind = NET_RESULT_NAT;
  else if (ty->tag == KT_BOOL) kind = NET_RESULT_BOOL;
  else return 0;                                     /* unsupported result type     */
  return kt_eval_via_net(heap, ctx, t, kind, out);
}

kterm_t *kt_normalize_auto(lizard_heap_t *heap, kctx_t *ctx, kterm_t *t) {
  if (g_enabled) {
    kterm_t *out = NULL;
    if (kt_eval_via_net_auto(heap, ctx, t, &out)) return out;
  }
  return kt_whnf(heap, ctx, t);
}
