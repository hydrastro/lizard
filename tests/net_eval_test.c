/* net_eval_test.c — the net as evaluator (Phase 16, started).
 *
 * Validates that running a closed kernel term on the interaction net and reading
 * the result back into a kterm (kt_eval_via_net) agrees with the trusted reducer
 * kt_whnf, and exercises the gate + kt_whnf fallback (kt_normalize_gated).
 *
 * Build (standalone, via the heap shim):
 *   cc -std=c89 -O2 -Isrc -Iinclude tests/net_eval_test.c src/net_eval.c \
 *      src/kt_to_core.c src/ic_lower.c src/ic.c src/kernel.c <heap-shim>.c -lgmp -o net_eval_test
 */
#include "net_eval.h"
#include "kernel.h"
#include "mem.h"
#include <stdio.h>
#include <string.h>

static lizard_heap_t *H;
static kctx_t *CTX;
static long fails = 0, checks = 0;

static kterm_t *kmk(kterm_tag_t tag) {
  kterm_t *t = (kterm_t *)lizard_heap_alloc(sizeof *t);
  memset(t, 0, sizeof *t);
  t->tag = tag;
  return t;
}
static kterm_t *num(int k) { kterm_t *t = kt_zero(H); while (k-- > 0) t = kt_succ(H, t); return t; }
static kterm_t *ktrue(void)  { return kmk(KT_TRUE); }
static kterm_t *kfalse(void) { return kmk(KT_FALSE); }

/* nat_rec C zc sc scrut */
static kterm_t *knatrec(kterm_t *zc, kterm_t *sc, kterm_t *scrut) {
  kterm_t *t = kmk(KT_NAT_REC);
  t->data.nat_rec.motive     = kmk(KT_NAT);
  t->data.nat_rec.zero_case  = zc;
  t->data.nat_rec.succ_case  = sc;
  t->data.nat_rec.scrutinee  = scrut;
  return t;
}
/* a closed  \p:Nat. \r:Nat. <body(r)>  */
static kterm_t *succ_case_succ_r(void) {        /* \p r. succ r */
  return kt_lam(H, "p", kt_nat(H), kt_lam(H, "r", kt_nat(H), kt_succ(H, kt_var(H, 0))));
}
static kterm_t *succ_case_ss_r(void) {          /* \p r. succ (succ r) */
  return kt_lam(H, "p", kt_nat(H),
                kt_lam(H, "r", kt_nat(H), kt_succ(H, kt_succ(H, kt_var(H, 0)))));
}
/* bool_rec selecting */
static kterm_t *kif(kterm_t *c, kterm_t *tc, kterm_t *fc) {
  kterm_t *t = kmk(KT_BOOL_REC);
  t->data.bool_rec.motive     = kmk(KT_BOOL);
  t->data.bool_rec.true_case  = tc;
  t->data.bool_rec.false_case = fc;
  t->data.bool_rec.scrutinee  = c;
  return t;
}

/* the trusted reducer, fully forced to a numeral / boolean (the authority) */
static long kernel_nat(kterm_t *t) {
  long k = 0;
  for (;;) {
    kterm_t *nf = kt_whnf(H, CTX, t);
    if (nf->tag == KT_ZERO) return k;
    if (nf->tag == KT_SUCC) { k++; if (k > 100000) return -1; t = nf->data.succ.pred; }
    else return -1;
  }
}
static int kernel_bool(kterm_t *t) {
  kterm_t *nf = kt_whnf(H, CTX, t);
  if (nf->tag == KT_TRUE)  return 1;
  if (nf->tag == KT_FALSE) return 0;
  return -1;
}

/* read a kterm numeral back to a long (-1 if not a numeral) */
static long numeral_of(kterm_t *t) {
  long k = 0;
  while (t->tag == KT_SUCC) { k++; t = t->data.succ.pred; }
  return (t->tag == KT_ZERO) ? k : -1;
}

static void cknat(const char *name, kterm_t *t, long want) {
  kterm_t *out = NULL;
  long kn = kernel_nat(t), nv;
  checks++;
  if (!kt_eval_via_net(H, NULL, t, NET_RESULT_NAT, &out)) {
    fails++; printf("  FAIL %s: net declined a fragment term\n", name); return;
  }
  nv = numeral_of(out);
  if (nv != kn || kn != want) {
    fails++; printf("  FAIL %s: net=%ld kernel=%ld want=%ld\n", name, nv, kn, want);
  }
}
static void ckbool(const char *name, kterm_t *t, int want) {
  kterm_t *out = NULL;
  int kb = kernel_bool(t), nv;
  checks++;
  if (!kt_eval_via_net(H, NULL, t, NET_RESULT_BOOL, &out)) {
    fails++; printf("  FAIL %s: net declined a fragment term\n", name); return;
  }
  nv = (out->tag == KT_TRUE) ? 1 : (out->tag == KT_FALSE) ? 0 : -1;
  if (nv != kb || kb != want) {
    fails++; printf("  FAIL %s: net=%d kernel=%d want=%d\n", name, nv, kb, want);
  }
}

/* well-typed recursors (proper motives), needed because the auto path type-checks */
static kterm_t *knatrec_typed(kterm_t *zc, kterm_t *sc, kterm_t *scrut) {
  kterm_t *t = kmk(KT_NAT_REC);
  t->data.nat_rec.motive    = kt_lam(H, "n", kt_nat(H), kt_nat(H));   /* lam n:Nat. Nat */
  t->data.nat_rec.zero_case = zc; t->data.nat_rec.succ_case = sc; t->data.nat_rec.scrutinee = scrut;
  return t;
}
static kterm_t *kif_typed(kterm_t *c, kterm_t *tc, kterm_t *fc) {
  kterm_t *t = kmk(KT_BOOL_REC);
  t->data.bool_rec.motive     = kt_lam(H, "b", kmk(KT_BOOL), kmk(KT_BOOL)); /* lam b:Bool. Bool */
  t->data.bool_rec.true_case  = tc; t->data.bool_rec.false_case = fc; t->data.bool_rec.scrutinee = c;
  return t;
}

/* type-directed dispatch: NO kind passed -- it is inferred from the term's type */
static void cknat_auto(const char *name, kterm_t *t, long want) {
  kterm_t *out = NULL;
  long kn = kernel_nat(t), nv;
  checks++;
  if (!kt_eval_via_net_auto(H, CTX, t, &out)) {
    fails++; printf("  FAIL %s: auto-dispatch declined\n", name); return;
  }
  nv = numeral_of(out);
  if (nv != kn || kn != want) {
    fails++; printf("  FAIL %s: net=%ld kernel=%ld want=%ld\n", name, nv, kn, want);
  }
}
static void ckbool_auto(const char *name, kterm_t *t, int want) {
  kterm_t *out = NULL;
  int kb = kernel_bool(t), nv;
  checks++;
  if (!kt_eval_via_net_auto(H, CTX, t, &out)) {
    fails++; printf("  FAIL %s: auto-dispatch declined\n", name); return;
  }
  nv = (out->tag == KT_TRUE) ? 1 : (out->tag == KT_FALSE) ? 0 : -1;
  if (nv != kb || kb != want) {
    fails++; printf("  FAIL %s: net=%d kernel=%d want=%d\n", name, nv, kb, want);
  }
}

int main(void) {
  kterm_t *r_off, *r_on, *r_fallback, *decl = NULL;
  int declined;

  H = lizard_heap_create((size_t)1 << 20, (size_t)1 << 20);
  CTX = kctx_create(H);

  /* the net, as evaluator, agrees with the kernel and reads back as a kterm */
  cknat("eval 0",        num(0), 0);
  cknat("eval 7",        num(7), 7);
  cknat("eval succ^3 0", kt_succ(H, kt_succ(H, kt_succ(H, num(0)))), 3);
  cknat("natrec 2 (\\p r. succ r) 3   = 5", knatrec(num(2), succ_case_succ_r(), num(3)), 5);
  cknat("natrec 0 (\\p r. succ succ r) 3 = 6", knatrec(num(0), succ_case_ss_r(), num(3)), 6);

  ckbool("eval true",  ktrue(),  1);
  ckbool("eval false", kfalse(), 0);
  ckbool("if true  then true else false  = true",  kif(ktrue(),  ktrue(), kfalse()), 1);
  ckbool("if false then false else true  = true",  kif(kfalse(), kfalse(), ktrue()), 1);

  /* type-directed dispatch: the evaluator infers Nat vs Bool from kt_infer */
  cknat_auto("auto: eval 7 (inferred Nat)",            num(7), 7);
  cknat_auto("auto: natrec 2 (\\p r. succ r) 3 = 5",   knatrec_typed(num(2), succ_case_succ_r(), num(3)), 5);
  ckbool_auto("auto: eval true (inferred Bool)",        ktrue(), 1);
  ckbool_auto("auto: if false then false else true = true", kif_typed(kfalse(), kfalse(), ktrue()), 1);

  if (fails == 0) printf("ALL %ld NET-EVALUATOR CHECKS PASSED (net result == kernel)\n", checks);
  else            printf("%ld/%ld net-evaluator checks FAILED\n", fails, checks);

  /* ---- the gate + fallback (the Phase 16 seam) ---- */
  printf("\ngate / fallback:\n");

  /* gate OFF (default): kt_normalize_gated routes to the trusted reducer */
  r_off = kt_normalize_gated(H, CTX, num(5), NET_RESULT_NAT);
  printf("  gate off : normalize(5)    -> %s\n",
         (r_off->tag == KT_SUCC) ? "kt_whnf head (KT_SUCC)" : "?");

  /* gate ON: the net evaluates the same term to a full value */
  kt_net_eval_set_enabled(1);
  r_on = kt_normalize_gated(H, CTX, num(5), NET_RESULT_NAT);
  printf("  gate on  : normalize(5)    -> net value %ld\n", numeral_of(r_on));

  /* fully automatic (no kind): kt_normalize_auto infers the type and routes */
  {
    kterm_t *ra = kt_normalize_auto(H, CTX, num(9));
    printf("  gate on  : normalize_auto(9) [kind inferred] -> net value %ld\n", numeral_of(ra));
  }

  /* fallback: a term outside the net's fragment (a sort) is declined, so the
   * gated path falls back to kt_whnf even with the gate on */
  declined = !kt_eval_via_net(H, NULL, kt_sort(H, 0), NET_RESULT_NAT, &decl);
  r_fallback = kt_normalize_gated(H, CTX, kt_sort(H, 0), NET_RESULT_NAT);
  printf("  gate on  : net declines Sort: %s; gated falls back -> kterm tag %d\n",
         declined ? "yes" : "NO (bug)", (int)r_fallback->tag);

  if (numeral_of(r_on) != 5 || !declined) {
    printf("  GATE/FALLBACK CHECK FAILED\n");
    return 1;
  }
  return fails ? 1 : 0;
}
