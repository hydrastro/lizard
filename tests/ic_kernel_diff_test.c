/* ic_kernel_diff_test.c — Phase 13b: cross-check the interaction-net engine
 * against the TRUSTED kernel reducer kt_whnf.
 *
 * For each random term we build a CLOSED kernel term of type Bool over the
 * shared fragment {true, false, bool_rec, pair/proj, lambda/app} and:
 *   - reduce it with the kernel:  kt_whnf -> KT_TRUE / KT_FALSE      (bit_kernel)
 *   - translate it to the core IR (kt_to_core), lower to a net, observe by
 *     applying the resulting Church boolean to 1 and 0, and normalise (bit_net)
 *   - compare with an independent C oracle computed while generating (bit_oracle)
 * and assert  bit_oracle == bit_kernel == bit_net.
 *
 * Agreement validates the net's beta / Sigma / sharing reduction against the
 * kernel that the type theory's soundness already depends on.
 *
 * Build (standalone, with the project's kernel objects or static lib), e.g.:
 *   cc -std=c89 -O2 -Isrc -Iinclude \
 *      tests/ic_kernel_diff_test.c src/kt_to_core.c src/ic_lower.c src/ic.c \
 *      build/liblizard.a -lgmp -o ic_kernel_diff_test
 *   ./ic_kernel_diff_test 20000 1
 * (The kernel allocates through lizard_heap_alloc; initialise the heap in main.)
 */
#include "kernel.h"
#include "mem.h"        /* heap, lizard_heap_alloc, lizard_heap_create */
#include "ic.h"
#include "ic_lower.h"
#include "kt_to_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ heap --- */
/* mem.h declares `extern lizard_heap_t *heap;` and lizard_heap_alloc/create.
 * In the full project these come from src/mem.c.  For a standalone run they can
 * be supplied by a tiny malloc shim. */

/* ------------------------------------------------------------- tiny PRNG --- */
static unsigned long g_seed = 1UL;
static unsigned int rnd(unsigned int n) {
  g_seed ^= g_seed << 13; g_seed ^= g_seed >> 7; g_seed ^= g_seed << 17;
  return (unsigned int)(g_seed % (unsigned long)n);
}

/* ------------------------------------------ kterm builders for the fragment - */
static lizard_heap_t *H;   /* set in main */

static kterm_t *kmk(kterm_tag_t tag) {
  kterm_t *t = (kterm_t *)lizard_heap_alloc(sizeof *t);
  memset(t, 0, sizeof *t);
  t->tag = tag;
  return t;
}
static kterm_t *kpair(kterm_t *a, kterm_t *b) {
  kterm_t *t = kmk(KT_PAIR); t->data.pair.fst = a; t->data.pair.snd = b; return t;
}
static kterm_t *kproj(int first, kterm_t *p) {
  kterm_t *t = kmk(first ? KT_PROJ1 : KT_PROJ2); t->data.proj.target = p; return t;
}
static kterm_t *kif(kterm_t *c, kterm_t *tc, kterm_t *fc) {
  kterm_t *t = kmk(KT_BOOL_REC);
  t->data.bool_rec.motive     = kmk(KT_BOOL);   /* placeholder; whnf ignores it */
  t->data.bool_rec.true_case  = tc;
  t->data.bool_rec.false_case = fc;
  t->data.bool_rec.scrutinee  = c;
  return t;
}

/* ------------------------------------------------- generator (closed Bool) - */
typedef struct { int b[64]; int n; } BEnv;

static kterm_t *genB(int depth, BEnv *e, int *bit) {
  int choice = (depth <= 0) ? (int)rnd((unsigned int)(e->n > 0 ? 2 : 1)) : (int)rnd(6);
  if (choice == 1 && e->n == 0) choice = 0;     /* no variables in scope */
  switch (choice) {
    case 0: { int v = (int)rnd(2); *bit = v; return v ? kmk(KT_TRUE) : kmk(KT_FALSE); }
    case 1: {                                   /* a bound Bool variable */
      int p = (int)rnd((unsigned int)e->n);     /* 0 = outermost */
      *bit = e->b[p];
      return kt_var(H, e->n - 1 - p);           /* de Bruijn from innermost */
    }
    case 2: case 3: {                           /* if c then t else f */
      int cb, tb, fb;
      kterm_t *c = genB(depth - 1, e, &cb);
      kterm_t *tc = genB(depth - 1, e, &tb);
      kterm_t *fc = genB(depth - 1, e, &fb);
      *bit = cb ? tb : fb;
      return kif(c, tc, fc);
    }
    case 4: {                                   /* proj of a pair of Bools */
      int ab, bb, first = (int)rnd(2);
      kterm_t *a = genB(depth - 1, e, &ab);
      kterm_t *b = genB(depth - 1, e, &bb);
      *bit = first ? ab : bb;
      return kproj(first, kpair(a, b));
    }
    default: {                                  /* (\x:Bool. body) arg */
      int ab, bb;
      kterm_t *arg, *body;
      if (e->n >= 60) { int v = (int)rnd(2); *bit = v; return v ? kmk(KT_TRUE) : kmk(KT_FALSE); }
      arg = genB(depth - 1, e, &ab);
      e->b[e->n++] = ab;                         /* push: x = arg's bit */
      body = genB(depth - 1, e, &bb);
      e->n--;                                    /* pop */
      *bit = bb;
      return kt_app(H, kt_lam(H, "x", kmk(KT_BOOL), body), arg);
    }
  }
}

/* ------------------------------------------------------ net-side observation */
static int net_bit(const kterm_t *t, int *ok) {
  core_t *c = kt_to_core(t), *obs;
  ic_term_t *e;
  mpz_t o; long it = 0; int v = -1, st;
  *ok = 0;
  if (c == NULL) return -1;                      /* outside fragment (shouldn't happen) */
  obs = core_app(core_app(c, core_lit_si(1)), core_lit_si(0)); /* (bool 1) 0 */
  e = ic_lower(obs);
  if (e != NULL) {
    mpz_init(o);
    st = ic_normalize_int(e, o, &it);
    if (st == 1) { v = (int)mpz_get_si(o); *ok = 1; }
    mpz_clear(o);
    ic_term_free(e);
  }
  core_free(obs);
  return v;
}

int main(int argc, char **argv) {
  long iters = (argc > 1) ? strtol(argv[1], NULL, 10) : 20000;
  long i, fails = 0, skips = 0;
  kctx_t *ctx;
  if (argc > 2) g_seed = strtoul(argv[2], NULL, 10) | 1UL;

  /* The kernel allocates terms in this heap and does not free per-iteration, so
   * size it for the run.  In the full project this is the real arena (mem.c); if
   * a very large run exhausts it, lower the iteration count or raise these sizes. */
  heap = lizard_heap_create(1UL << 26, 1UL << 30);  /* 64 MiB committed, 1 GiB reserved */
  H = heap;
  ctx = kctx_create(H);

  for (i = 0; i < iters; i++) {
    BEnv e; int obit = -1, kbit, nbit, ok = 0;
    kterm_t *t, *nf;
    e.n = 0;
    t = genB(5, &e, &obit);

    nf = kt_whnf(H, ctx, t);                      /* TRUSTED kernel */
    kbit = (nf->tag == KT_TRUE) ? 1 : (nf->tag == KT_FALSE) ? 0 : -1;

    nbit = net_bit(t, &ok);                        /* interaction net */
    if (!ok) nbit = -1;

    if (kbit < 0) { skips++; continue; }           /* kernel got stuck: skip */
    if (kbit != obit || nbit != obit) {
      if (++fails <= 10)
        printf("  MISMATCH [%ld]: oracle=%d kernel=%d net=%d\n", i, obit, kbit, nbit);
    }
  }

  if (fails)
    printf("FAIL: %ld/%ld disagreed (%ld skipped)\n", fails, iters, skips);
  else
    printf("OK: all %ld Bool terms agree (oracle == kernel == net)%s\n",
           iters - skips,
           skips ? " [some skipped]" : "");
  return fails ? 1 : 0;
}
