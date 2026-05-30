/* tests/kernel_test.c — Comprehensive kernel type-checker tests. */
#include "../src/kernel.h"
#include "../src/mem.h"
#include <stdio.h>
#include <string.h>

/* heap is the global from mem.h */
static int pass_count = 0;
static int fail_count = 0;

static void check(const char *name, int cond) {
  if (cond) { pass_count++; }
  else { fail_count++; printf("  FAIL  %s\n", name); }
}

static kterm_t *mk(kterm_tag_t tag) {
  kterm_t *t = (kterm_t *)lizard_heap_alloc(sizeof(kterm_t));
  memset(t, 0, sizeof(*t));
  t->tag = tag;
  return t;
}

static void test_nat(void) {
  kctx_t *ctx = kctx_create(heap);
  kterm_t *t;
  t = kt_infer(heap, ctx, kt_zero(heap));
  check("zero:Nat", t != NULL && t->tag == KT_NAT);
  t = kt_infer(heap, ctx, kt_succ(heap, kt_zero(heap)));
  check("succ:Nat", t != NULL && t->tag == KT_NAT);
  t = kt_infer(heap, ctx, kt_nat(heap));
  check("Nat:Sort0", t != NULL && t->tag == KT_SORT && t->data.sort.level == 0);
}

static void test_bool(void) {
  kctx_t *ctx = kctx_create(heap);
  kterm_t *t, *br, *r;
  t = kt_infer(heap, ctx, mk(KT_TRUE));
  check("true:Bool", t != NULL && t->tag == KT_BOOL);
  t = kt_infer(heap, ctx, mk(KT_FALSE));
  check("false:Bool", t != NULL && t->tag == KT_BOOL);
  /* if true 0 (succ 0) → 0 */
  br = mk(KT_BOOL_REC);
  br->data.bool_rec.scrutinee = mk(KT_TRUE);
  br->data.bool_rec.true_case = kt_zero(heap);
  br->data.bool_rec.false_case = kt_succ(heap, kt_zero(heap));
  r = kt_whnf(heap, ctx, br);
  check("if-true->0", r != NULL && r->tag == KT_ZERO);
  /* if false 0 (succ 0) → succ 0 */
  br = mk(KT_BOOL_REC);
  br->data.bool_rec.scrutinee = mk(KT_FALSE);
  br->data.bool_rec.true_case = kt_zero(heap);
  br->data.bool_rec.false_case = kt_succ(heap, kt_zero(heap));
  r = kt_whnf(heap, ctx, br);
  check("if-false->succ0", r != NULL && r->tag == KT_SUCC);
}

static void test_pi_beta(void) {
  kctx_t *ctx = kctx_create(heap);
  kterm_t *t, *r;
  /* lam(x:Nat, x) : Pi(x:Nat, Nat) */
  t = kt_infer(heap, ctx, kt_lam(heap, "x", kt_nat(heap), kt_var(heap, 0)));
  check("id:Pi", t != NULL && t->tag == KT_PI);
  /* (lam x. x) 0 → 0 */
  r = kt_whnf(heap, ctx,
    kt_app(heap, kt_lam(heap, "x", kt_nat(heap), kt_var(heap, 0)),
                 kt_zero(heap)));
  check("beta-id", r != NULL && r->tag == KT_ZERO);
  /* (lam x. succ x) 0 → succ 0 */
  r = kt_whnf(heap, ctx,
    kt_app(heap, kt_lam(heap, "x", kt_nat(heap), kt_succ(heap, kt_var(heap, 0))),
                 kt_zero(heap)));
  check("beta-succ", r != NULL && r->tag == KT_SUCC);
}

static void test_sigma_proj(void) {
  kctx_t *ctx = kctx_create(heap);
  kterm_t *p, *pr, *r;
  p = mk(KT_PAIR);
  p->data.pair.fst = kt_zero(heap);
  p->data.pair.snd = kt_succ(heap, kt_zero(heap));
  /* fst(pair(0,s0)) → 0 */
  pr = mk(KT_PROJ1);
  pr->data.proj.target = p;
  r = kt_whnf(heap, ctx, pr);
  check("fst-pair", r != NULL && r->tag == KT_ZERO);
  /* snd(pair(0,s0)) → s0 */
  pr = mk(KT_PROJ2);
  pr->data.proj.target = p;
  r = kt_whnf(heap, ctx, pr);
  check("snd-pair", r != NULL && r->tag == KT_SUCC);
}

static void test_id_refl(void) {
  kctx_t *ctx = kctx_create(heap);
  kterm_t *t;
  t = kt_infer(heap, ctx, kt_refl(heap, kt_zero(heap)));
  check("refl:Id", t != NULL && t->tag == KT_ID);
  check("0==0", kt_equal(heap, ctx, kt_zero(heap), kt_zero(heap)));
  check("0!=s0", !kt_equal(heap, ctx, kt_zero(heap), kt_succ(heap, kt_zero(heap))));
}

static void test_unify(void) {
  kctx_t *ctx = kctx_create(heap);
  meta_ctx_t *mctx = meta_ctx_create(heap);
  kterm_t *h0;
  meta_entry_t *e;
  int r;
  h0 = meta_fresh(heap, mctx, kt_nat(heap));
  r = kt_unify(heap, ctx, mctx, h0, kt_succ(heap, kt_zero(heap)));
  check("unify-ok", r == 1);
  e = meta_lookup(mctx, 0);
  check("meta-solved", e != NULL && e->solution != NULL && e->solution->tag == KT_SUCC);
  check("zonk", meta_zonk(heap, mctx, h0)->tag == KT_SUCC);
}

static void test_sort_hierarchy(void) {
  kctx_t *ctx = kctx_create(heap);
  kterm_t *t;
  t = kt_infer(heap, ctx, kt_sort(heap, 0));
  check("Sort0:Sort1", t != NULL && t->tag == KT_SORT && t->data.sort.level == 1);
  t = kt_infer(heap, ctx, kt_sort(heap, 5));
  check("Sort5:Sort6", t != NULL && t->tag == KT_SORT && t->data.sort.level == 6);
}

static void test_defn_eq(void) {
  kctx_t *ctx = kctx_create(heap);
  /* (lam x. x) 0 ≡ 0 */
  check("beta-eq",
    kt_equal(heap, ctx,
      kt_app(heap, kt_lam(heap, "x", kt_nat(heap), kt_var(heap, 0)),
                   kt_zero(heap)),
      kt_zero(heap)));
}

int main(void) {
  heap = lizard_heap_create(1024 * 1024, 1024 * 1024);
  printf("kernel_test:\n");
  test_nat();
  test_bool();
  test_pi_beta();
  test_sigma_proj();
  test_id_refl();
  test_unify();
  test_sort_hierarchy();
  test_defn_eq();
  printf("  %d passed, %d failed\n", pass_count, fail_count);
  lizard_heap_destroy(heap);
  return fail_count > 0 ? 1 : 0;
}
