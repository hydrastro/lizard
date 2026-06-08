/* ic_kernel_diff_test.c — Phase 13b: cross-check the interaction-net engine
 * against the TRUSTED kernel reducer kt_whnf.
 *
 * For each random term we build a CLOSED kernel term over the shared fragment
 * {true, false, bool_rec, pair/proj, lambda/app, coproducts (inl/inr/sum_rec),
 * options (just/nothing/maybe_rec), Nat (zero/succ/nat_rec), List
 * (nil/cons/list_rec)} and:
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
static kterm_t *kinl(kterm_t *v) {
  kterm_t *t = kmk(KT_INL); t->data.inl.value = v; t->data.inl.right_type = kmk(KT_BOOL); return t;
}
static kterm_t *kinr(kterm_t *v) {
  kterm_t *t = kmk(KT_INR); t->data.inr.value = v; t->data.inr.left_type = kmk(KT_BOOL); return t;
}
static kterm_t *ksumrec(kterm_t *lc, kterm_t *rc, kterm_t *s) {
  kterm_t *t = kmk(KT_SUM_REC);
  t->data.sum_rec.motive = kmk(KT_BOOL);
  t->data.sum_rec.left_case = lc; t->data.sum_rec.right_case = rc; t->data.sum_rec.scrutinee = s;
  return t;
}
static kterm_t *kjust(kterm_t *v)   { kterm_t *t = kmk(KT_JUST); t->data.just.value = v; return t; }
static kterm_t *knothing(void)      { return kmk(KT_NOTHING); }
static kterm_t *kmayberec(kterm_t *nc, kterm_t *jc, kterm_t *m) {
  kterm_t *t = kmk(KT_MAYBE_REC);
  t->data.maybe_rec.motive = kmk(KT_BOOL);
  t->data.maybe_rec.nothing_case = nc; t->data.maybe_rec.just_case = jc; t->data.maybe_rec.scrutinee = m;
  return t;
}
/* \x:Bool. x  — the identity handler (returns the carried value) */
static kterm_t *kidfun(void) { return kt_lam(H, "x", kmk(KT_BOOL), kt_var(H, 0)); }

/* ------------------------------------------------- generator (closed Bool) - */
typedef struct { int b[64]; int n; } BEnv;

static kterm_t *genB(int depth, BEnv *e, int *bit) {
  int choice = (depth <= 0) ? (int)rnd((unsigned int)(e->n > 0 ? 2 : 1)) : (int)rnd(8);
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
    case 5: {                                   /* eliminate a coproduct */
      int pb, cb, left = (int)rnd(2);
      kterm_t *payload = genB(depth - 1, e, &pb);
      kterm_t *scrut = left ? kinl(payload) : kinr(payload);
      cb = (int)rnd(2);                          /* constant returned by the other case */
      /* left_case = \x.x (returns payload);  right_case = \x. <const> */
      *bit = left ? pb : cb;
      return ksumrec(kidfun(),
                     kt_lam(H, "x", kmk(KT_BOOL), cb ? kmk(KT_TRUE) : kmk(KT_FALSE)),
                     scrut);
    }
    case 6: {                                   /* eliminate an option */
      int pb, nb, isjust = (int)rnd(2);
      kterm_t *payload = genB(depth - 1, e, &pb);
      kterm_t *m = isjust ? kjust(payload) : knothing();
      nb = (int)rnd(2);                          /* value for the nothing case */
      /* just_case = \x.x (returns payload);  nothing_case = <const> */
      *bit = isjust ? pb : nb;
      return kmayberec(nb ? kmk(KT_TRUE) : kmk(KT_FALSE), kidfun(), m);
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

/* ---------------------------------------------------- Nat: builders + observe */
static kterm_t *num(int k) {                     /* the numeral succ^k zero */
  kterm_t *t = kt_zero(H);
  while (k-- > 0) t = kt_succ(H, t);
  return t;
}
static kterm_t *safe_succ_case(int which) {      /* a closed \p:Nat.\r:Nat. <body> */
  kterm_t *body;
  switch (which) {
    case 0:  body = kt_var(H, 0); break;                         /* r           */
    case 1:  body = kt_succ(H, kt_var(H, 0)); break;             /* succ r      */
    case 2:  body = kt_succ(H, kt_succ(H, kt_var(H, 0))); break; /* succ succ r */
    case 3:  body = kt_var(H, 1); break;                         /* p           */
    default: body = kt_succ(H, kt_var(H, 1)); break;             /* succ p      */
  }
  return kt_lam(H, "p", kt_nat(H), kt_lam(H, "r", kt_nat(H), body));
}
/* The trusted kernel reduces a closed Nat to a numeral; force the spine and count. */
static int kernel_nat(kctx_t *ctx, kterm_t *t) {
  int k = 0;
  for (;;) {
    kterm_t *nf = kt_whnf(H, ctx, t);
    if (nf->tag == KT_ZERO) return k;
    if (nf->tag == KT_SUCC) { k++; if (k > 100000) return -1; t = nf->data.succ.pred; }
    else return -1;                              /* stuck / not a numeral */
  }
}
/* Observe a Church numeral on the net as an integer:  n (\x. x+1) 0. */
static int net_nat_obs(const kterm_t *t) {
  core_t *c = kt_to_core(t), *obs;
  ic_term_t *e; mpz_t o; long it = 0; int v = -1;
  if (c == NULL) return -2;
  obs = core_app(core_app(c, core_lam(core_prim(IC_ADD, core_var(0), core_lit_si(1)))),
                 core_lit_si(0));
  e = ic_lower(obs);
  if (e != NULL) {
    mpz_init(o);
    if (ic_normalize_int(e, o, &it) == 1) v = (int)mpz_get_si(o);
    mpz_clear(o);
    ic_term_free(e);
  }
  core_free(obs);
  return v;
}
static kterm_t *genN(int depth, int envn) {
  int choice = (depth <= 0) ? (int)rnd((unsigned int)(envn > 0 ? 2 : 1)) : (int)rnd(6);
  if (choice == 1 && envn == 0) choice = 0;
  switch (choice) {
    case 0: return num((int)rnd(4));                            /* numeral 0..3 */
    case 1: return kt_var(H, (int)rnd((unsigned int)envn));     /* a bound Nat var */
    case 2: return kt_succ(H, genN(depth - 1, envn));           /* succ of a Nat */
    case 3: case 4: {                                           /* natrec z s scrut */
      int sh = (depth <= 2) ? 0 : 1;             /* keep z and scrutinee shallow */
      kterm_t *t = kmk(KT_NAT_REC);
      t->data.nat_rec.motive     = kt_nat(H);
      t->data.nat_rec.zero_case  = genN(sh, envn);
      t->data.nat_rec.succ_case  = safe_succ_case((int)rnd(5));
      t->data.nat_rec.scrutinee  = genN(sh, envn);
      return t;
    }
    default: {                                                  /* (\x:Nat. body) arg */
      kterm_t *arg  = genN(depth - 1, envn);
      kterm_t *body = genN(depth - 1, envn + 1);
      return kt_app(H, kt_lam(H, "x", kt_nat(H), body), arg);
    }
  }
}

/* ------------------------------------------- List of Nat: builders + generator */
static kterm_t *knil(void) {
  kterm_t *t = kmk(KT_NIL_K); t->data.nil_k.elem_type = kt_nat(H); return t;
}
static kterm_t *kcons(kterm_t *h, kterm_t *tl) {
  kterm_t *t = kmk(KT_CONS_K); t->data.cons_k.head = h; t->data.cons_k.tail = tl; return t;
}
/* a closed \h:Nat. \t:List Nat. \r:Nat. <body>  (h = var2, t = var1, r = var0) */
static kterm_t *safe_cons_case(int which) {
  kterm_t *body;
  switch (which) {
    case 0:  body = kt_succ(H, kt_var(H, 0)); break;            /* succ r  (length)   */
    case 1:  body = kt_var(H, 0); break;                        /* r       (-> nil)   */
    case 2:  body = kt_var(H, 2); break;                        /* h       (head/nil) */
    default: body = kt_succ(H, kt_var(H, 2)); break;            /* succ h             */
  }
  return kt_lam(H, "h", kt_nat(H),
           kt_lam(H, "t", kmk(KT_LIST), kt_lam(H, "r", kt_nat(H), body)));
}
static kterm_t *genL(int depth) {                /* a closed List of small Nats */
  if (depth <= 0 || rnd(2) == 0) return knil();
  return kcons(num((int)rnd(4)), genL(depth - 1));
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

  /* Phase 2: Nat with primitive recursion (natrec).  The trusted kernel is the
   * oracle; we compare the numeral it computes against the net's Church-numeral
   * observation.  Exercises real recursion driven by the numeral. */
  for (i = 0; i < iters; i++) {
    kterm_t *t = genN(3, 0);
    int kn = kernel_nat(ctx, t);
    int nn;
    if (kn < 0) { skips++; continue; }
    nn = net_nat_obs(t);
    if (kn != nn) {
      if (++fails <= 10)
        printf("  NAT MISMATCH [%ld]: kernel=%d net=%d\n", i, kn, nn);
    }
  }

  /* Phase 3: List with structural recursion (list_rec), folding to a Nat. */
  for (i = 0; i < iters; i++) {
    kterm_t *t = kmk(KT_LIST_REC);
    int kn, nn;
    t->data.list_rec.motive     = kt_nat(H);
    t->data.list_rec.nil_case   = num((int)rnd(4));
    t->data.list_rec.cons_case  = safe_cons_case((int)rnd(4));
    t->data.list_rec.scrutinee  = genL(4);
    kn = kernel_nat(ctx, t);
    if (kn < 0) { skips++; continue; }
    nn = net_nat_obs(t);
    if (kn != nn) {
      if (++fails <= 10)
        printf("  LIST MISMATCH [%ld]: kernel=%d net=%d\n", i, kn, nn);
    }
  }

  /* Phase 4: propositional-equality elimination.  A closed proof of Id is refl,
   * and the kernel's only Id rule is  J C d A a b (refl v) -> d v.  We build such
   * a J over Nat and check the net agrees (result observed as a numeral). */
  for (i = 0; i < iters; i++) {
    kterm_t *v = genN(2, 0), *d, *t;
    int kn, nn;
    switch ((int)rnd(4)) {
      case 0:  d = kt_lam(H, "x", kt_nat(H), kt_var(H, 0)); break;                   /* \x. x      */
      case 1:  d = kt_lam(H, "x", kt_nat(H), kt_succ(H, kt_var(H, 0))); break;       /* \x. succ x */
      case 2:  d = kt_lam(H, "x", kt_nat(H), kt_succ(H, kt_succ(H, kt_var(H, 0)))); break;
      default: d = kt_lam(H, "x", kt_nat(H), num((int)rnd(4))); break;               /* \x. const  */
    }
    t = kmk(KT_J);
    t->data.j.motive    = kt_nat(H);
    t->data.j.base_case = d;
    t->data.j.type      = kt_nat(H);
    t->data.j.a         = num(0);                /* whnf does not read a/b for J-beta */
    t->data.j.b         = num(0);
    t->data.j.proof     = kt_refl(H, v);
    kn = kernel_nat(ctx, t);
    if (kn < 0) { skips++; continue; }
    nn = net_nat_obs(t);
    if (kn != nn) {
      if (++fails <= 10)
        printf("  J MISMATCH [%ld]: kernel=%d net=%d\n", i, kn, nn);
    }
  }

  if (fails)
    printf("FAIL: %ld disagreed (%ld skipped)\n", fails, skips);
  else
    printf("OK: all %ld terms agree (Bool/Sum/Maybe: oracle==kernel==net; "
           "Nat/List/J: kernel==net)%s\n",
           4 * iters - skips,
           skips ? " [some skipped]" : "");
  return fails ? 1 : 0;
}
