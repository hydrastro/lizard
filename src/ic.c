/* ic.c — the interaction-calculus core (Phase 11: the FOUR agents).
 *
 * See ic.h for the high-level description and the rule table.  The net is a
 * heap of binary AGENTS (CON/DUP/SUP/OPR/OP1) plus nullary ERA/NUM, wired
 * port-to-port through VAR slots, with a stack of active pairs (REDEXES).
 *
 * The structure follows inet.c, which is correct and well-tested; the new
 * content is the SUP agent, labels on CON/DUP/SUP, the DUP~SUP rule that makes
 * the construction/observation duality compute, superposition-aware readback,
 * and a small parser so the core is a usable engine on its own.
 */

#include "ic.h"
#include <stdlib.h>
#include <string.h>

/* ---- port encoding (4-bit tag; value in the high bits) ---------------- */
typedef unsigned long Port;
#define T_VARP 0u
#define T_ERA  1u
#define T_NUM  2u
#define T_CON  3u
#define T_DUP  4u
#define T_SUP  5u
#define T_OPR  6u
#define T_OP1  7u
#define T_EMP  8u           /* sentinel: an unbound var slot */
#define T_PAIR 9u           /* Σ pair constructor (binary)   */
#define T_FST  10u          /* Σ first projection (eliminator) */
#define T_SND  11u          /* Σ second projection (eliminator) */
#define T_TRANSP 12u        /* transport / Id-by-observation (unary) */
#define T_REF  13u          /* recursive-definition call (unary): principal faces
                             * the argument; expands the named definition lazily   */

#define MKP(tag, val) ( ((Port)(val) << 4) | (Port)(tag) )
#define PTAG(p)       ((unsigned)((p) & 15u))
#define PVAL(p)       ((unsigned long)((p) >> 4))
#define P_ERAP        (MKP(T_ERA, 0))
#define P_EMPTY       (MKP(T_EMP, 0))

#define ISBIN(t) ((t) == T_CON || (t) == T_DUP || (t) == T_SUP || \
                  (t) == T_OPR || (t) == T_OP1 || (t) == T_PAIR)
#define ISNUL(t) ((t) == T_ERA || (t) == T_NUM)
/* an agent occupies a node (has a principal port); FST/SND are unary eliminators
 * handled explicitly, so they are not in ISBIN but are still agents. */
#define ISAGENT(t) (ISBIN(t) || (t) == T_FST || (t) == T_SND || (t) == T_TRANSP)

/* family used by the annihilate/commute decision:
 *   CON         -> 0  (lambda / application / constructors)
 *   DUP and SUP -> 1  (the dual pair: same family, so DUP~SUP can annihilate)
 *   OPR         -> 2
 *   OP1         -> 3
 *   PAIR        -> 4  (its own family: PAIR meets its eliminators FST/SND, not
 *                      another PAIR, so PAIR~PAIR has no annihilation rule)     */
static int fam(unsigned t) {
  if (t == T_CON) return 0;
  if (t == T_DUP || t == T_SUP) return 1;
  if (t == T_OPR) return 2;
  if (t == T_PAIR) return 4;
  return 3; /* T_OP1 */
}

/* ---- capacities (single-threaded, static) ----------------------------- */
#define NODECAP (1 << 20)
#define VARCAP  (1 << 21)
#define NUMCAP  (1 << 19)
#define RDXCAP  (1 << 20)
#define STEP_LIMIT 400000000L

static Port g_p1[NODECAP];
static Port g_p2[NODECAP];
static int  g_tag[NODECAP];     /* T_CON / T_DUP / T_SUP / T_OPR / T_OP1     */
static int  g_lab[NODECAP];     /* CON/DUP/SUP label, or OPR/OP1 op code     */
static long g_ex[NODECAP];      /* OP1: stored-left number index; else -1    */
static int  g_nnode;
static int  g_freelist[NODECAP];
static int  g_nfree;

static Port g_vars[VARCAP];
static int  g_nvar;

static mpz_t g_num[NUMCAP];
static int   g_nnum;
static int   g_num_hw;

static Port g_ra[RDXCAP];
static Port g_rb[RDXCAP];
static int  g_nrdx;

static long g_inter;
static int  g_oom;
static Port g_root;

/* ---- allocators ------------------------------------------------------- */
static unsigned long new_var(void);

/* Reduction order.  The default is LIFO (a stack); setting FIFO consumes the
 * redex queue front-to-back instead.  Interaction nets are strongly confluent
 * (one-step diamond), so BOTH orders reach the same normal form in the same
 * NUMBER of interactions — the property that makes parallel scheduling sound.
 * tests/ic_confluence_test.c checks this empirically.  (FIFO does not reclaim
 * consumed slots, so it is a validation mode for bounded reductions, not the
 * production reducer.) */
static int g_reduce_fifo = 0;
static int g_rdx_head = 0;
void ic_set_reduce_fifo(int on) { g_reduce_fifo = on ? 1 : 0; }

static void ic_reset(void) {
  g_nnode = 0; g_nfree = 0; g_nvar = 0; g_nnum = 0;
  g_nrdx = 0; g_inter = 0; g_oom = 0; g_rdx_head = 0;
}

static int new_node(int tag, int lab) {
  int i;
  if (g_nfree > 0) {
    i = g_freelist[--g_nfree];
  } else if (g_nnode < NODECAP) {
    i = g_nnode++;
  } else {
    g_oom = 1;
    return 0;
  }
  g_tag[i] = tag; g_lab[i] = lab; g_ex[i] = -1;
  g_p1[i] = MKP(T_VARP, new_var());
  g_p2[i] = MKP(T_VARP, new_var());
  return i;
}

static void free_node(int i) {
  if (g_nfree < NODECAP) g_freelist[g_nfree++] = i;
}

static unsigned long new_var(void) {
  unsigned long v;
  if (g_nvar < VARCAP) {
    v = (unsigned long)g_nvar++;
  } else {
    g_oom = 1;
    return 0;
  }
  g_vars[v] = P_EMPTY;
  return v;
}

static unsigned long new_num_z(const mpz_t z) {
  unsigned long i;
  if (g_nnum >= NUMCAP) { g_oom = 1; return 0; }
  i = (unsigned long)g_nnum++;
  if ((int)i >= g_num_hw) { mpz_init(g_num[i]); g_num_hw = (int)i + 1; }
  mpz_set(g_num[i], z);
  return i;
}

/* ---- redex stack ------------------------------------------------------ */
static void push_redex(Port a, Port b) {
  if (g_nrdx < RDXCAP) { g_ra[g_nrdx] = a; g_rb[g_nrdx] = b; g_nrdx++; }
  else g_oom = 1;
}

/* ---- link: wire two ports, chasing through var slots ------------------ */
static void link_ports(Port a, Port b) {
  for (;;) {
    if (PTAG(a) != T_VARP && PTAG(b) == T_VARP) { Port t = a; a = b; b = t; }
    if (PTAG(a) != T_VARP) {
      push_redex(a, b);          /* two principals (or nullaries): active pair */
      return;
    }
    {
      unsigned long va = PVAL(a);
      Port got = g_vars[va];
      if (PTAG(got) == T_EMP) { g_vars[va] = b; return; }
      g_vars[va] = P_EMPTY;
      a = got;
    }
  }
}

/* ---- exact arithmetic ------------------------------------------------- */
static unsigned long compute(int op, const mpz_t x, const mpz_t y) {
  mpz_t r;
  unsigned long idx;
  mpz_init(r);
  switch (op) {
    case IC_ADD: mpz_add(r, x, y); break;
    case IC_SUB: mpz_sub(r, x, y); break;
    case IC_MUL: mpz_mul(r, x, y); break;
    case IC_DIV: if (mpz_sgn(y) != 0) mpz_tdiv_q(r, x, y); break;
    case IC_MOD: if (mpz_sgn(y) != 0) mpz_tdiv_r(r, x, y); break;
    case IC_EQ:  mpz_set_si(r, mpz_cmp(x, y) == 0 ? 1 : 0); break;
    case IC_LT:  mpz_set_si(r, mpz_cmp(x, y) <  0 ? 1 : 0); break;
    case IC_GT:  mpz_set_si(r, mpz_cmp(x, y) >  0 ? 1 : 0); break;
    default: break;
  }
  idx = new_num_z(r);
  mpz_clear(r);
  return idx;
}

/* ---- the interaction rules -------------------------------------------- */
static void commute(unsigned ta, unsigned long ia, unsigned tb, unsigned long ib) {
  /* a ~ b, different family or different label: each duplicates the other.
   * This single rule covers DUP copying a CON (sharing), CON applied to a SUP
   * (search fan-out), a different-label DUP meeting a SUP, OPR over a SUP, etc. */
  int ca1 = new_node((int)ta, g_lab[ia]);
  int ca2 = new_node((int)ta, g_lab[ia]);
  int cb1 = new_node((int)tb, g_lab[ib]);
  int cb2 = new_node((int)tb, g_lab[ib]);
  if (ta == T_OP1) { g_ex[ca1] = g_ex[ia]; g_ex[ca2] = g_ex[ia]; }
  if (tb == T_OP1) { g_ex[cb1] = g_ex[ib]; g_ex[cb2] = g_ex[ib]; }
  link_ports(g_p1[ia], MKP(tb, cb1));
  link_ports(g_p2[ia], MKP(tb, cb2));
  link_ports(g_p1[ib], MKP(ta, ca1));
  link_ports(g_p2[ib], MKP(ta, ca2));
  link_ports(g_p1[ca1], g_p1[cb1]);
  link_ports(g_p2[ca1], g_p1[cb2]);
  link_ports(g_p1[ca2], g_p2[cb1]);
  link_ports(g_p2[ca2], g_p2[cb2]);
  free_node((int)ia);
  free_node((int)ib);
}

static void annihilate(unsigned long ia, unsigned long ib) {
  /* same family + same label: connect aux ports straight across.
   *   CON~CON  : beta (an application meets a lambda)
   *   DUP~DUP  : the two splitters merge their wires
   *   SUP~SUP  : two superpositions cancel
   *   DUP~SUP  : the two superposed values flow to the two consumers
   *              (the construction meets its matching observation) */
  link_ports(g_p1[ia], g_p1[ib]);
  link_ports(g_p2[ia], g_p2[ib]);
  free_node((int)ia);
  free_node((int)ib);
}

static void erase_bin(unsigned long bi) {
  link_ports(g_p1[bi], P_ERAP);
  link_ports(g_p2[bi], P_ERAP);
  free_node((int)bi);
}

/* FST/SND are unary eliminators (principal faces the pair producer; p1 = result;
 * p2 is an unused wire kept at ERA).  This handles every agent the projector's
 * principal can meet. */
static void do_proj(unsigned pt, unsigned long pi, unsigned xt, unsigned long xi) {
  if (xt == T_PAIR) {                          /* the projection fires */
    if (pt == T_FST) { link_ports(g_p1[pi], g_p1[xi]); link_ports(g_p2[xi], P_ERAP); }
    else             { link_ports(g_p1[pi], g_p2[xi]); link_ports(g_p1[xi], P_ERAP); }
    free_node((int)pi);
    free_node((int)xi);
    return;
  }
  if (xt == T_DUP || xt == T_SUP) {            /* commute past a duplicator/superposition */
    int xn = new_node((int)xt, g_lab[xi]);     /* recombine the two results */
    int p1n = new_node((int)pt, 0);
    int p2n = new_node((int)pt, 0);
    link_ports(g_p1[xi], MKP(pt, (unsigned long)p1n));   /* each producer -> a projector */
    link_ports(g_p2[xi], MKP(pt, (unsigned long)p2n));
    link_ports(g_p1[p1n], g_p1[xn]);           /* the two results -> the recombiner */
    link_ports(g_p1[p2n], g_p2[xn]);
    link_ports(g_p1[pi], MKP(xt, (unsigned long)xn)); /* consumer <- recombiner */
    link_ports(g_p2[p1n], P_ERAP);
    link_ports(g_p2[p2n], P_ERAP);
    free_node((int)pi);
    free_node((int)xi);
    return;
  }
  /* ERA / NUM / CON / OPR / OP1 / FST / SND on the pair side: ill-typed or dead.
   * The projection produces nothing; erase its result, and clean up a binary peer. */
  link_ports(g_p1[pi], P_ERAP);
  free_node((int)pi);
  if (ISBIN(xt)) erase_bin(xi);
  else if (xt == T_FST || xt == T_SND) { link_ports(g_p1[xi], P_ERAP); free_node((int)xi); }
  return;
}

/* TRANSP is the transport / Id-by-observation agent (unary: principal faces the
 * value, p1 = result, p2 an unused wire).  It computes by recursion on the value
 * former it meets -- transport over Sigma is componentwise, over Pi is pointwise,
 * trivial on the base -- which is exactly the by-observation reduction.  On a
 * reflexivity path it is the identity, so transp(v) normalises to v; the typed
 * version that dispatches on the *type* former and carries a real path sits on
 * top of these same structural rules (see docs/INET_ENGINE_PLAN.md, Phase 14b). */
static void do_transp(unsigned long ti, unsigned xt, unsigned long xi) {
  if (xt == T_PAIR) {                          /* Sigma: transport each component */
    int pr = new_node(T_PAIR, 0);
    int t1 = new_node(T_TRANSP, 0);
    int t2 = new_node(T_TRANSP, 0);
    link_ports(MKP(T_TRANSP, (unsigned long)t1), g_p1[xi]);
    link_ports(g_p1[t1], g_p1[pr]);
    link_ports(g_p2[t1], P_ERAP);
    link_ports(MKP(T_TRANSP, (unsigned long)t2), g_p2[xi]);
    link_ports(g_p1[t2], g_p2[pr]);
    link_ports(g_p2[t2], P_ERAP);
    link_ports(g_p1[ti], MKP(T_PAIR, (unsigned long)pr));
    free_node((int)ti); free_node((int)xi);
    return;
  }
  if (xt == T_CON) {                           /* Pi: transp (\x.b) = \x. transp b */
    int c  = new_node(T_CON, 0);
    int tb = new_node(T_TRANSP, 0);
    link_ports(g_p1[c], g_p1[xi]);             /* the new lambda shares the binder */
    link_ports(MKP(T_TRANSP, (unsigned long)tb), g_p2[xi]);
    link_ports(g_p1[tb], g_p2[c]);
    link_ports(g_p2[tb], P_ERAP);
    link_ports(g_p1[ti], MKP(T_CON, (unsigned long)c));
    free_node((int)ti); free_node((int)xi);
    return;
  }
  if (xt == T_DUP || xt == T_SUP) {            /* distribute over sharing/superposition */
    int xn = new_node((int)xt, g_lab[xi]);
    int t1 = new_node(T_TRANSP, 0);
    int t2 = new_node(T_TRANSP, 0);
    link_ports(g_p1[xi], MKP(T_TRANSP, (unsigned long)t1));
    link_ports(g_p2[xi], MKP(T_TRANSP, (unsigned long)t2));
    link_ports(g_p1[t1], g_p1[xn]);
    link_ports(g_p1[t2], g_p2[xn]);
    link_ports(g_p1[ti], MKP(xt, (unsigned long)xn));
    link_ports(g_p2[t1], P_ERAP);
    link_ports(g_p2[t2], P_ERAP);
    free_node((int)ti); free_node((int)xi);
    return;
  }
  if (xt == T_ERA) { link_ports(g_p1[ti], P_ERAP); free_node((int)ti); return; }
  /* base type (NUM) or any other producer: transport is the identity. */
  link_ports(g_p1[ti], MKP(xt, xi));
  free_node((int)ti);
  return;
}

/* ---- recursive definitions: the cycle, unfolded lazily ---------------- *
 * A (ref D a) node carries the definition id D in its label and faces its
 * argument a on the principal port.  When a reduces to a number n the node
 * fires: build_def_body(D, n) produces the body for that argument — which may
 * contain further (ref D ...) nodes — and we lower a *fresh* copy of it into the
 * live arena (compile is re-entrant) and wire it to the result.  Because the
 * builder branches on n in C, the base case introduces no further ref, so the
 * unfolding of the self-referential graph terminates on finite data.  This is
 * the same discipline an interaction-net runtime (HVM) uses for its definitions;
 * the C-level branch plays the role of pattern matching on the argument's
 * constructor (zero vs. successor). */
static Port compile(ic_term_t *t);

static ic_term_t *build_def_body(int def_id, const mpz_t n) {
  long k = mpz_get_si(n);
  switch (def_id) {
    case IC_DEF_FACT:
      if (k <= 0) return ic_num_si(1);
      return ic_op(IC_MUL, ic_num_z(n),
                   ic_ref(IC_DEF_FACT, ic_op(IC_SUB, ic_num_z(n), ic_num_si(1))));
    case IC_DEF_SUMTO:
      if (k <= 0) return ic_num_si(0);
      return ic_op(IC_ADD, ic_num_z(n),
                   ic_ref(IC_DEF_SUMTO, ic_op(IC_SUB, ic_num_z(n), ic_num_si(1))));
    case IC_DEF_POW2:
      if (k <= 0) return ic_num_si(1);
      return ic_op(IC_MUL, ic_num_si(2),
                   ic_ref(IC_DEF_POW2, ic_op(IC_SUB, ic_num_z(n), ic_num_si(1))));
    case IC_DEF_FIB:
      if (k < 2) return ic_num_z(n);
      return ic_op(IC_ADD,
                   ic_ref(IC_DEF_FIB, ic_op(IC_SUB, ic_num_z(n), ic_num_si(1))),
                   ic_ref(IC_DEF_FIB, ic_op(IC_SUB, ic_num_z(n), ic_num_si(2))));
    case IC_DEF_GCD: {
      /* a packed in n>>16, b in low 16 bits;  gcd a b = b=0 ? a : gcd b (a mod b) */
      long a = k >> 16, b = k & 0xffff;
      if (b == 0) return ic_num_si(a);
      return ic_ref(IC_DEF_GCD, ic_num_si((b << 16) | (a % b)));
    }
    default:
      return ic_num_si(0);
  }
}

static Port expand_def(int def_id, const mpz_t n) {
  ic_term_t *body = build_def_body(def_id, n);
  Port root = compile(body);                     /* fresh instantiation into the arena */
  ic_term_free(body);
  return root;
}

static void interact(Port a, Port b) {
  unsigned ta = PTAG(a), tb = PTAG(b);
  unsigned long ia = PVAL(a), ib = PVAL(b);
  g_inter++;

  /* Σ eliminators are handled first, as unary agents. */
  if (ta == T_FST || ta == T_SND || tb == T_FST || tb == T_SND) {
    if (ta == T_FST || ta == T_SND) do_proj(ta, ia, tb, ib);
    else                            do_proj(tb, ib, ta, ia);
    return;
  }
  /* transport (Id-by-observation), also unary. */
  if (ta == T_TRANSP || tb == T_TRANSP) {
    if (ta == T_TRANSP) do_transp(ia, tb, ib);
    else                do_transp(ib, ta, ia);
    return;
  }
  /* recursive-definition call: fires once its argument is a concrete number. */
  if (ta == T_REF || tb == T_REF) {
    unsigned long ri; unsigned ot; unsigned long oi;
    if (ta == T_REF) { ri = ia; ot = tb; oi = ib; } else { ri = ib; ot = ta; oi = ia; }
    if (ot == T_NUM) {
      Port body = expand_def(g_lab[ri], g_num[oi]);
      link_ports(g_p1[ri], body);                /* result <- the unfolded body */
    } else {
      link_ports(g_p1[ri], P_ERAP);              /* no concrete argument: nothing to unfold */
    }
    free_node((int)ri);
    return;
  }

  if (ISNUL(ta) && ISNUL(tb)) {
    return;                                   /* VOID: both vanish */
  }
  if (ISNUL(ta) || ISNUL(tb)) {
    unsigned nt; unsigned long ni; unsigned bt; unsigned long bi;
    if (ISNUL(ta)) { nt = ta; ni = ia; bt = tb; bi = ib; }
    else           { nt = tb; ni = ib; bt = ta; bi = ia; }
    if (nt == T_ERA) { erase_bin(bi); return; }
    /* nt == T_NUM */
    if (bt == T_DUP || bt == T_SUP) {          /* copy the number into both wires */
      link_ports(g_p1[bi], MKP(T_NUM, ni));
      link_ports(g_p2[bi], MKP(T_NUM, ni));
      free_node((int)bi);
      return;
    }
    if (bt == T_OPR) {                         /* absorb the left operand */
      int o = new_node(T_OP1, g_lab[bi]);
      g_ex[o] = (long)ni;
      link_ports(g_p1[bi], MKP(T_OP1, o));     /* right operand -> OP1 principal */
      link_ports(g_p2[bi], g_p1[o]);           /* OP1 result aux -> result dest  */
      link_ports(g_p2[o], P_ERAP);
      free_node((int)bi);
      return;
    }
    if (bt == T_OP1) {                         /* compute left op right */
      unsigned long r = compute(g_lab[bi],
                                g_num[(unsigned long)g_ex[bi]], g_num[ni]);
      link_ports(g_p1[bi], MKP(T_NUM, r));
      free_node((int)bi);
      return;
    }
    /* NUM ~ CON : a number used as a function; erase defensively */
    erase_bin(bi);
    return;
  }

  /* both binary: annihilate iff same family (CON, or DUP/SUP) and same label */
  if (fam(ta) == fam(tb) && (fam(ta) == 0 || fam(ta) == 1) &&
      g_lab[ia] == g_lab[ib]) {
    annihilate(ia, ib);
  } else {
    commute(ta, ia, tb, ib);
  }
}

static void reduce(void) {
  while (!g_oom && g_inter < STEP_LIMIT) {
    Port a, b;
    if (g_reduce_fifo) {                 /* consume the queue front-to-back */
      if (g_rdx_head >= g_nrdx) break;
      a = g_ra[g_rdx_head]; b = g_rb[g_rdx_head]; g_rdx_head++;
    } else {                             /* default: LIFO stack (unchanged) */
      if (g_nrdx <= 0) break;
      a = g_ra[--g_nrdx]; b = g_rb[g_nrdx];
    }
    interact(a, b);
  }
}

/* ======================================================================= */
/*  surface terms                                                          */
/* ======================================================================= */
static ic_term_t *talloc(ic_tkind_t k) {
  ic_term_t *t = (ic_term_t *)malloc(sizeof(ic_term_t));
  t->kind = k; t->name = NULL; t->name2 = NULL;
  t->a = NULL; t->b = NULL; t->op = 0; t->label = 0;
  return t;
}
static char *dupstr(const char *s) {
  size_t n = strlen(s) + 1;
  char *c = (char *)malloc(n);
  memcpy(c, s, n);
  return c;
}

ic_term_t *ic_var(const char *name) {
  ic_term_t *t = talloc(IC_TVAR); t->name = dupstr(name); return t;
}
ic_term_t *ic_lam(const char *name, ic_term_t *body) {
  ic_term_t *t = talloc(IC_TLAM); t->name = dupstr(name); t->a = body; return t;
}
ic_term_t *ic_app(ic_term_t *f, ic_term_t *a) {
  ic_term_t *t = talloc(IC_TAPP); t->a = f; t->b = a; return t;
}
ic_term_t *ic_num_si(long v) {
  ic_term_t *t = talloc(IC_TNUM); mpz_init_set_si(t->num, v); return t;
}
ic_term_t *ic_num_str(const char *decimal) {
  ic_term_t *t = talloc(IC_TNUM); mpz_init_set_str(t->num, decimal, 10); return t;
}
ic_term_t *ic_num_z(const mpz_t v) {
  ic_term_t *t = talloc(IC_TNUM); mpz_init_set(t->num, v); return t;
}
ic_term_t *ic_op(int op, ic_term_t *l, ic_term_t *r) {
  ic_term_t *t = talloc(IC_TOP); t->op = op; t->a = l; t->b = r; return t;
}
ic_term_t *ic_sup(int label, ic_term_t *l, ic_term_t *r) {
  ic_term_t *t = talloc(IC_TSUP); t->label = label; t->a = l; t->b = r; return t;
}
ic_term_t *ic_dup(int label, const char *x, const char *y,
                  ic_term_t *value, ic_term_t *body) {
  ic_term_t *t = talloc(IC_TDUP);
  t->label = label; t->name = dupstr(x); t->name2 = dupstr(y);
  t->a = value; t->b = body; return t;
}
ic_term_t *ic_era(void) { return talloc(IC_TERA); }
ic_term_t *ic_pair(ic_term_t *f, ic_term_t *s) {
  ic_term_t *t = talloc(IC_TPAIR); t->a = f; t->b = s; return t;
}
ic_term_t *ic_fst(ic_term_t *p) { ic_term_t *t = talloc(IC_TFST); t->a = p; return t; }
ic_term_t *ic_snd(ic_term_t *p) { ic_term_t *t = talloc(IC_TSND); t->a = p; return t; }
ic_term_t *ic_transp(ic_term_t *v) { ic_term_t *t = talloc(IC_TTRANSP); t->a = v; return t; }
ic_term_t *ic_ref(int def_id, ic_term_t *arg) {
  ic_term_t *t = talloc(IC_TREF); t->op = def_id; t->a = arg; return t;
}

void ic_term_free(ic_term_t *t) {
  if (!t) return;
  if (t->kind == IC_TNUM) mpz_clear(t->num);
  if (t->name)  free(t->name);
  if (t->name2) free(t->name2);
  ic_term_free(t->a);
  ic_term_free(t->b);
  free(t);
}

/* ---- compilation: term -> net ----------------------------------------- *
 * A scope frame collects the var-port handles where a binder is used.  Each
 * lambda/dup binder gets a *fresh* DUP label with DUP_BINDER_BIT set, so the
 * sharing DUPs the compiler inserts never collide with the small user-chosen
 * labels on SUP/DUP terms.  Consequently a user superposition flowing into a
 * shared binder COMMUTES (it fans out into both copies) — the correct default. */
#define MAXUSE 65536
#define MAXDEPTH 4096
#define DUP_BINDER_BIT (1 << 28)
typedef struct { const char *name; Port *uses; int n; int cap; } Frame;
static Frame g_scope[MAXDEPTH];
static int   g_sp;
static int   g_label;           /* fresh binder-dup label counter */

static void frame_push(const char *name) {
  if (g_sp >= MAXDEPTH) { g_oom = 1; return; }
  g_scope[g_sp].name = name;
  g_scope[g_sp].n = 0;
  g_scope[g_sp].cap = 8;
  g_scope[g_sp].uses = (Port *)malloc(sizeof(Port) * 8);
  g_sp++;
}
static void frame_add_use(int idx, Port use) {
  Frame *f = &g_scope[idx];
  if (f->n >= f->cap) {
    int nc = f->cap * 2;
    if (nc > MAXUSE) { g_oom = 1; return; }
    f->uses = (Port *)realloc(f->uses, sizeof(Port) * (size_t)nc);
    f->cap = nc;
  }
  f->uses[f->n++] = use;
}

static void wire_binder(Port binder, Port *uses, int n) {
  if (n == 0) {
    link_ports(binder, P_ERAP);
  } else if (n == 1) {
    link_ports(binder, uses[0]);
  } else {
    int lab = (++g_label) | DUP_BINDER_BIT;     /* fresh, disjoint from user labels */
    Port cur = binder;
    int i;
    for (i = 0; i < n - 1; i++) {
      int d = new_node(T_DUP, lab);
      link_ports(cur, MKP(T_DUP, d));
      link_ports(g_p1[d], uses[i]);
      cur = g_p2[d];
    }
    link_ports(cur, uses[n - 1]);
  }
}

static Port compile(ic_term_t *t) {
  switch (t->kind) {
    case IC_TVAR: {
      int i;
      unsigned long v = new_var();
      Port use = MKP(T_VARP, v);
      for (i = g_sp - 1; i >= 0; i--) {
        if (strcmp(g_scope[i].name, t->name) == 0) {
          frame_add_use(i, use);
          return use;
        }
      }
      link_ports(use, P_ERAP);   /* free variable: erase it */
      return use;
    }
    case IC_TLAM: {
      int con = new_node(T_CON, 0);
      Port out = MKP(T_CON, con);
      Port body;
      frame_push(t->name);
      body = compile(t->a);
      link_ports(g_p2[con], body);
      g_sp--;
      wire_binder(g_p1[con], g_scope[g_sp].uses, g_scope[g_sp].n);
      free(g_scope[g_sp].uses);
      return out;
    }
    case IC_TAPP: {
      int app = new_node(T_CON, 0);
      Port fout = compile(t->a);
      Port aout = compile(t->b);
      link_ports(MKP(T_CON, app), fout);
      link_ports(g_p1[app], aout);
      return g_p2[app];
    }
    case IC_TNUM: {
      return MKP(T_NUM, new_num_z(t->num));
    }
    case IC_TOP: {
      int opr = new_node(T_OPR, t->op);
      Port lout = compile(t->a);
      Port rout = compile(t->b);
      link_ports(MKP(T_OPR, opr), lout);
      link_ports(g_p1[opr], rout);
      return g_p2[opr];
    }
    case IC_TSUP: {
      int sup = new_node(T_SUP, t->label & (DUP_BINDER_BIT - 1));
      Port out = MKP(T_SUP, sup);          /* the superposition value = principal */
      Port l = compile(t->a);
      Port r = compile(t->b);
      link_ports(g_p1[sup], l);
      link_ports(g_p2[sup], r);
      return out;
    }
    case IC_TDUP: {
      int d = new_node(T_DUP, t->label & (DUP_BINDER_BIT - 1));
      Port val = compile(t->a);
      Port bodyout;
      link_ports(MKP(T_DUP, d), val);      /* principal consumes the value */
      frame_push(t->name);                 /* x -> aux1 */
      frame_push(t->name2);                /* y -> aux2 */
      bodyout = compile(t->b);
      g_sp--;
      wire_binder(g_p2[d], g_scope[g_sp].uses, g_scope[g_sp].n);
      free(g_scope[g_sp].uses);
      g_sp--;
      wire_binder(g_p1[d], g_scope[g_sp].uses, g_scope[g_sp].n);
      free(g_scope[g_sp].uses);
      return bodyout;
    }
    case IC_TPAIR: {
      int pr = new_node(T_PAIR, 0);
      Port out = MKP(T_PAIR, pr);          /* the pair value = principal */
      Port l = compile(t->a);
      Port r = compile(t->b);
      link_ports(g_p1[pr], l);             /* p1 = fst field */
      link_ports(g_p2[pr], r);             /* p2 = snd field */
      return out;
    }
    case IC_TFST: {
      int f = new_node(T_FST, 0);
      Port pout = compile(t->a);
      link_ports(MKP(T_FST, f), pout);     /* principal faces the pair producer */
      link_ports(g_p2[f], P_ERAP);         /* unused wire */
      return g_p1[f];                      /* result */
    }
    case IC_TSND: {
      int s = new_node(T_SND, 0);
      Port pout = compile(t->a);
      link_ports(MKP(T_SND, s), pout);
      link_ports(g_p2[s], P_ERAP);
      return g_p1[s];
    }
    case IC_TTRANSP: {
      int tr = new_node(T_TRANSP, 0);
      Port pout = compile(t->a);
      link_ports(MKP(T_TRANSP, tr), pout); /* principal faces the value producer */
      link_ports(g_p2[tr], P_ERAP);
      return g_p1[tr];                     /* result */
    }
    case IC_TREF: {
      int r = new_node(T_REF, t->op);      /* def id carried in the label */
      Port aout = compile(t->a);           /* the argument */
      link_ports(MKP(T_REF, r), aout);     /* principal faces the argument producer */
      link_ports(g_p2[r], P_ERAP);         /* unused aux wire */
      return g_p1[r];                      /* result */
    }
    case IC_TERA: {
      return P_ERAP;
    }
  }
  return P_ERAP;
}

/* ---- readback --------------------------------------------------------- */
static Port enter(Port p) {
  while (PTAG(p) == T_VARP) {
    Port nx = g_vars[PVAL(p)];
    if (PTAG(nx) == T_EMP) break;
    p = nx;
  }
  return p;
}

int ic_normalize_int(ic_term_t *t, mpz_t out, long *interactions) {
  Port r, root;
  ic_reset();
  g_sp = 0; g_label = 0;
  root = compile(t);
  g_root = MKP(T_VARP, new_var());
  link_ports(g_root, root);
  reduce();
  if (interactions) *interactions = g_inter;
  if (g_oom) return -1;
  r = enter(g_root);
  if (PTAG(r) == T_NUM) {
    mpz_set(out, g_num[PVAL(r)]);
    return 1;
  }
  return 0;
}

long ic_live_nodes(void) { return (long)g_nnode - (long)g_nfree; }

/* readback to text.  We build a bidirectional partner snapshot (like inet.c)
 * and walk it.  Ports are 3*node + slot (0=principal,1=aux1,2=aux2); leaves
 * encode as small negatives.  Term readback succeeds for the affine fragment
 * (no residual DUP) plus superpositions and numbers; when residual DUP-sharing
 * remains we fall back to a faithful net rendering rather than risk a wrong
 * tree (the labelled-dup "bracket" bookkeeping of optimal reduction is left to
 * a later phase). */
#define RB_NONE (-1)
#define RB_ERA  (-2)
#define RB_NUM_BIAS 16

static int   g_partner[3 * NODECAP];
static int   g_bkt_a[VARCAP];
static int   g_bkt_b[VARCAP];
static int   g_bind_depth[NODECAP];
static char  g_live[NODECAP];

static char  *g_rb_buf;
static size_t g_rb_cap;
static size_t g_rb_pos;
static int    g_rb_of;
static int    g_rb_ok;          /* term still representable as a tree */
static long   g_rb_steps;

static void rb_putc(char c) {
  if (g_rb_of) return;
  if (g_rb_pos + 1u >= g_rb_cap) { g_rb_of = 1; return; }
  g_rb_buf[g_rb_pos++] = c;
}
static void rb_puts(const char *s) { while (*s) { rb_putc(*s); s++; } }
static void rb_putint(int n) {
  char tmp[16]; int k = 0, j;
  if (n == 0) { rb_putc('0'); return; }
  if (n < 0) { rb_putc('-'); n = -n; }
  while (n > 0 && k < 15) { tmp[k++] = (char)('0' + (n % 10)); n /= 10; }
  for (j = k - 1; j >= 0; j--) rb_putc(tmp[j]);
}
static void rb_putmpz(const mpz_t z) {
  size_t need = mpz_sizeinbase(z, 10) + 2u;
  if (g_rb_of) return;
  if (g_rb_pos + need >= g_rb_cap) { g_rb_of = 1; return; }
  (void)mpz_get_str(g_rb_buf + g_rb_pos, 10, z);
  g_rb_pos += strlen(g_rb_buf + g_rb_pos);
}

static void rb_build_partners(void) {
  int i, slot;
  for (i = 0; i < g_nnode; i++) g_live[i] = 1;
  for (i = 0; i < g_nfree; i++) g_live[g_freelist[i]] = 0;
  for (i = 0; i < 3 * g_nnode; i++) g_partner[i] = RB_NONE;
  for (i = 0; i < g_nvar; i++) { g_bkt_a[i] = -1; g_bkt_b[i] = -1; }
  for (i = 0; i < g_nnode; i++) {
    if (!g_live[i]) continue;
    for (slot = 1; slot <= 2; slot++) {
      Port ap = (slot == 1) ? g_p1[i] : g_p2[i];
      int auxid = i * 3 + slot;
      Port t = enter(ap);
      unsigned tg = PTAG(t);
      if (ISBIN(tg)) {
        int m = (int)PVAL(t);
        g_partner[auxid] = m * 3;
        g_partner[m * 3] = auxid;
      } else if (tg == T_NUM) {
        g_partner[auxid] = -(int)PVAL(t) - RB_NUM_BIAS;
      } else if (tg == T_ERA) {
        g_partner[auxid] = RB_ERA;
      } else {                       /* aux-aux rendezvous through a var slot */
        unsigned long vt = PVAL(t);
        if (g_bkt_a[vt] < 0) g_bkt_a[vt] = auxid;
        else g_bkt_b[vt] = auxid;
      }
    }
  }
  for (i = 0; i < g_nvar; i++) {
    if (g_bkt_a[i] >= 0 && g_bkt_b[i] >= 0) {
      g_partner[g_bkt_a[i]] = g_bkt_b[i];
      g_partner[g_bkt_b[i]] = g_bkt_a[i];
    }
  }
}

static void rb_read_from(int consumer_id, int depth);

static void rb_read_producer(int id, int depth) {
  int m, slot, tag;
  if (!g_rb_ok) return;
  if (++g_rb_steps > 20000000L) { g_rb_ok = 0; return; }
  m = id / 3; slot = id % 3; tag = g_tag[m];
  if (slot == 0) {
    if (tag == T_CON) {
      g_bind_depth[m] = depth;
      rb_puts("(lam ");
      rb_read_from(m * 3 + 2, depth + 1);
      rb_putc(')');
    } else if (tag == T_SUP) {
      rb_putc('{');
      rb_read_from(m * 3 + 1, depth);
      rb_putc(' ');
      rb_read_from(m * 3 + 2, depth);
      rb_putc('}');
    } else if (tag == T_PAIR) {
      rb_puts("(pair ");
      rb_read_from(m * 3 + 1, depth);
      rb_putc(' ');
      rb_read_from(m * 3 + 2, depth);
      rb_putc(')');
    } else {
      g_rb_ok = 0;                   /* residual DUP/OPR/OP1 -> net fallback */
    }
  } else if (slot == 2) {
    if (tag == T_CON) {
      rb_putc('(');
      rb_read_from(m * 3 + 0, depth);
      rb_putc(' ');
      rb_read_from(m * 3 + 1, depth);
      rb_putc(')');
    } else {
      g_rb_ok = 0;
    }
  } else { /* slot == 1 */
    if (tag == T_CON) {
      rb_putc('#');
      rb_putint(depth - g_bind_depth[m] - 1);
    } else {
      g_rb_ok = 0;
    }
  }
}

static void rb_read_from(int consumer_id, int depth) {
  int p;
  if (!g_rb_ok) return;
  p = g_partner[consumer_id];
  if (p == RB_NONE) { g_rb_ok = 0; return; }
  if (p == RB_ERA)  { rb_putc('*'); return; }
  if (p <= -RB_NUM_BIAS) { rb_putmpz(g_num[-p - RB_NUM_BIAS]); return; }
  rb_read_producer(p, depth);
}

/* faithful net rendering: always correct, used when a tree readback is not
 * available.  Lists each live agent and where its ports point. */
static const char *tagname(int t) {
  switch (t) {
    case T_CON: return "CON"; case T_DUP: return "DUP"; case T_SUP: return "SUP";
    case T_OPR: return "OPR"; case T_OP1: return "OP1";
    case T_PAIR: return "PAIR"; case T_FST: return "FST"; case T_SND: return "SND";
    case T_TRANSP: return "TRANSP";
    default: return "?";
  }
}
static void rb_port(Port p) {
  Port t = enter(p);
  unsigned tg = PTAG(t);
  if (ISAGENT(tg)) { rb_putc('n'); rb_putint((int)PVAL(t)); }
  else if (tg == T_NUM) { rb_puts("#"); rb_putmpz(g_num[PVAL(t)]); }
  else if (tg == T_ERA) { rb_putc('*'); }
  else if (tg == T_VARP) { rb_putc('v'); rb_putint((int)PVAL(t)); }  /* a wire junction */
  else if (tg == T_EMP)  { rb_putc('~'); }                           /* open/free wire   */
  else rb_putc('?');
}
static void rb_net_dump(void) {
  int i, first = 1;
  rb_puts("<net");
  for (i = 0; i < g_nnode; i++) {
    if (!g_live[i]) continue;
    rb_putc(first ? ' ' : ';'); first = 0;
    rb_putc(' ');
    rb_putc('n'); rb_putint(i); rb_putc(':');
    rb_puts(tagname(g_tag[i]));
    if (g_tag[i] == T_DUP || g_tag[i] == T_SUP)
      { rb_putc('^'); rb_putint(g_lab[i] & (DUP_BINDER_BIT - 1)); }
    rb_putc('[');
    rb_port(g_p1[i]); rb_putc(' '); rb_port(g_p2[i]);
    rb_putc(']');
  }
  rb_puts(" >");
}

int ic_readback(ic_term_t *t, char *buf, size_t cap, long *interactions) {
  Port r, root;
  unsigned tg;
  if (cap == 0) return 0;
  ic_reset();
  g_sp = 0; g_label = 0;
  root = compile(t);
  g_root = MKP(T_VARP, new_var());
  link_ports(g_root, root);
  reduce();
  if (interactions) *interactions = g_inter;
  if (g_oom) return -1;

  rb_build_partners();
  r = enter(g_root);
  g_rb_buf = buf; g_rb_cap = cap; g_rb_pos = 0; g_rb_of = 0;
  g_rb_ok = 1; g_rb_steps = 0;
  tg = PTAG(r);
  if (tg == T_NUM) {
    rb_putmpz(g_num[PVAL(r)]);
  } else if (tg == T_ERA) {
    rb_putc('*');
  } else if (ISBIN(tg)) {
    rb_read_producer((int)PVAL(r) * 3, 0);
  } else {                                /* root at an aux-aux rendezvous */
    int rp = g_bkt_a[PVAL(r)];
    if (rp < 0) g_rb_ok = 0;
    else rb_read_producer(rp, 0);
  }
  if (g_rb_ok && !g_rb_of) { buf[g_rb_pos] = '\0'; return 1; }
  /* tree readback unavailable: emit a faithful net rendering instead */
  g_rb_pos = 0; g_rb_of = 0;
  rb_net_dump();
  if (g_rb_of) return 0;
  buf[g_rb_pos] = '\0';
  return 1;
}

/* Render the compiled net itself (the abstract syntax graph), optionally after
 * reducing it.  Output is "<net n0:TAG[p1 p2]; n1:... >" where each nK is an
 * agent node, TAG its kind (CON/DUP^L/SUP^L/OPR/OP1/ERA/NUM), and p1/p2 are its
 * two auxiliary ports: "nK" means a wire to node K's principal port, "#n" a
 * number, "*" an eraser.  Variables do not appear — a variable is a wire, and a
 * shared value is reachable through a DUP node (so it is stored once, not
 * copied).  Returns 1, 0 if the buffer was too small, -1 on resource exhaustion. */
int ic_dump_net(ic_term_t *t, char *buf, size_t cap, int reduce_first) {
  Port root;
  if (cap == 0) return 0;
  ic_reset();
  g_sp = 0; g_label = 0;
  root = compile(t);
  g_root = MKP(T_VARP, new_var());
  link_ports(g_root, root);
  if (reduce_first) reduce();
  if (g_oom) return -1;
  rb_build_partners();          /* marks g_live[] (and readback scratch) */
  g_rb_buf = buf; g_rb_cap = cap; g_rb_pos = 0; g_rb_of = 0;
  rb_net_dump();
  if (g_rb_of) return 0;
  buf[g_rb_pos] = '\0';
  return 1;
}


/* ======================================================================= */
typedef struct { const char *s; size_t i, n; char *err; size_t errcap; } P;

static void p_err(P *p, const char *m) {
  if (p->err && p->errcap) { strncpy(p->err, m, p->errcap - 1); p->err[p->errcap - 1] = 0; }
}
static int is_delim(char c) { return c=='(' || c==')' || c=='{' || c=='}' || c=='*'; }
static void p_skip(P *p) {
  while (p->i < p->n) {
    char c = p->s[p->i];
    if (c==' '||c=='\t'||c=='\n'||c=='\r') { p->i++; continue; }
    if (c==';') { while (p->i<p->n && p->s[p->i]!='\n') p->i++; continue; }
    break;
  }
}
static char p_peek(P *p) { p_skip(p); return p->i < p->n ? p->s[p->i] : 0; }

/* read one token into out (returns length, 0 at end). single delimiter chars
 * are their own token; otherwise a maximal run of non-delim non-space chars. */
static size_t p_token(P *p, char *out, size_t cap) {
  size_t k = 0;
  char c;
  p_skip(p);
  if (p->i >= p->n) { if (cap) out[0]=0; return 0; }
  c = p->s[p->i];
  if (is_delim(c)) { p->i++; if (cap>=2){out[0]=c;out[1]=0;} return 1; }
  while (p->i < p->n) {
    c = p->s[p->i];
    if (c==' '||c=='\t'||c=='\n'||c=='\r'||is_delim(c)) break;
    if (k + 1 < cap) out[k] = c;
    k++; p->i++;
  }
  if (k < cap) out[k] = 0; else if (cap) out[cap-1]=0;
  return k;
}

static int is_number(const char *s) {
  int i = 0;
  if (s[0]=='-') { if (!s[1]) return 0; i = 1; }
  for (; s[i]; i++) if (s[i] < '0' || s[i] > '9') return 0;
  return 1;
}
static int op_of(const char *s) {
  if (!strcmp(s, "+")) return IC_ADD;
  if (!strcmp(s, "-")) return IC_SUB;
  if (!strcmp(s, "*")) return IC_MUL;
  if (!strcmp(s, "/")) return IC_DIV;
  if (!strcmp(s, "%")) return IC_MOD;
  if (!strcmp(s, "=")) return IC_EQ;
  if (!strcmp(s, "<")) return IC_LT;
  if (!strcmp(s, ">")) return IC_GT;
  return -1;
}

static ic_term_t *p_term(P *p);

/* parse an application body: one or more terms until ')' */
static ic_term_t *p_app_tail(P *p, ic_term_t *head) {
  for (;;) {
    char c = p_peek(p);
    if (c == 0) { p_err(p, "unexpected end of input in application"); ic_term_free(head); return NULL; }
    if (c == ')') { p->i++; return head; }
    {
      ic_term_t *arg = p_term(p);
      if (!arg) { ic_term_free(head); return NULL; }
      head = ic_app(head, arg);
    }
  }
}

static ic_term_t *p_term(P *p) {
  char tok[256];
  char c = p_peek(p);
  if (c == 0) { p_err(p, "unexpected end of input"); return NULL; }
  if (c == '*') { p->i++; return ic_era(); }
  if (c == '{') {
    int label = 0;
    ic_term_t *l, *r;
    p->i++;                       /* consume '{' */
    /* optional ':'-prefixed integer label: {:N a b}.  a bare leading number
     * is an ordinary element, so {2 3} is the unlabelled superposition. */
    {
      size_t save = p->i;
      size_t len = p_token(p, tok, sizeof tok);
      if (len >= 2 && tok[0] == ':' && is_number(tok + 1))
        label = (int)strtol(tok + 1, NULL, 10);
      else
        p->i = save;              /* not a label: put the token back */
    }
    l = p_term(p); if (!l) return NULL;
    r = p_term(p); if (!r) { ic_term_free(l); return NULL; }
    if (p_peek(p) != '}') { p_err(p, "expected '}'"); ic_term_free(l); ic_term_free(r); return NULL; }
    p->i++;
    return ic_sup(label, l, r);
  }
  if (c == '(') {
    size_t save;
    size_t len;
    p->i++;                       /* consume '(' */
    save = p->i;
    len = p_token(p, tok, sizeof tok);
    if (len && !strcmp(tok, "lam")) {
      char name[256]; ic_term_t *body;
      if (!p_token(p, name, sizeof name)) { p_err(p, "lam: missing binder"); return NULL; }
      body = p_term(p); if (!body) return NULL;
      if (p_peek(p) != ')') { p_err(p, "lam: expected ')'"); ic_term_free(body); return NULL; }
      p->i++;
      return ic_lam(name, body);
    }
    if (len && !strcmp(tok, "pair")) {
      ic_term_t *l, *r;
      l = p_term(p); if (!l) return NULL;
      r = p_term(p); if (!r) { ic_term_free(l); return NULL; }
      if (p_peek(p) != ')') { p_err(p, "pair: expected ')'"); ic_term_free(l); ic_term_free(r); return NULL; }
      p->i++;
      return ic_pair(l, r);
    }
    if (len && (!strcmp(tok, "fst") || !strcmp(tok, "snd"))) {
      int first = (tok[0] == 'f');
      ic_term_t *pr = p_term(p);
      if (!pr) return NULL;
      if (p_peek(p) != ')') { p_err(p, "fst/snd: expected ')'"); ic_term_free(pr); return NULL; }
      p->i++;
      return first ? ic_fst(pr) : ic_snd(pr);
    }
    if (len && !strcmp(tok, "transp")) {
      ic_term_t *v = p_term(p);
      if (!v) return NULL;
      if (p_peek(p) != ')') { p_err(p, "transp: expected ')'"); ic_term_free(v); return NULL; }
      p->i++;
      return ic_transp(v);
    }
    if (len && !strcmp(tok, "op")) {
      char ops[256]; int o; ic_term_t *l, *r;
      if (!p_token(p, ops, sizeof ops)) { p_err(p, "op: missing operator"); return NULL; }
      o = op_of(ops);
      if (o < 0) { p_err(p, "op: unknown operator"); return NULL; }
      l = p_term(p); if (!l) return NULL;
      r = p_term(p); if (!r) { ic_term_free(l); return NULL; }
      if (p_peek(p) != ')') { p_err(p, "op: expected ')'"); ic_term_free(l); ic_term_free(r); return NULL; }
      p->i++;
      return ic_op(o, l, r);
    }
    if (len && !strcmp(tok, "dup")) {
      int label = 0;
      char x[256], y[256]; ic_term_t *v, *body;
      size_t s2 = p->i;
      size_t l2 = p_token(p, x, sizeof x);
      if (l2 >= 2 && x[0] == ':' && is_number(x + 1)) label = (int)strtol(x + 1, NULL, 10);
      else p->i = s2;             /* not a label: put it back */
      if (!p_token(p, x, sizeof x)) { p_err(p, "dup: missing first binder"); return NULL; }
      if (!p_token(p, y, sizeof y)) { p_err(p, "dup: missing second binder"); return NULL; }
      v = p_term(p); if (!v) return NULL;
      body = p_term(p); if (!body) { ic_term_free(v); return NULL; }
      if (p_peek(p) != ')') { p_err(p, "dup: expected ')'"); ic_term_free(v); ic_term_free(body); return NULL; }
      p->i++;
      return ic_dup(label, x, y, v, body);
    }
    /* not a keyword: an application.  rewind and parse head + tail. */
    p->i = save;
    {
      ic_term_t *head = p_term(p);
      if (!head) return NULL;
      return p_app_tail(p, head);
    }
  }
  /* atom */
  {
    size_t len = p_token(p, tok, sizeof tok);
    if (!len) { p_err(p, "expected a term"); return NULL; }
    if (is_number(tok)) return ic_num_str(tok);
    return ic_var(tok);
  }
}

ic_term_t *ic_parse(const char *src, char *err, size_t errcap) {
  P p; ic_term_t *t;
  p.s = src; p.i = 0; p.n = strlen(src); p.err = err; p.errcap = errcap;
  if (err && errcap) err[0] = 0;
  t = p_term(&p);
  if (!t) return NULL;
  if (p_peek(&p) != 0) { p_err(&p, "trailing input after term"); ic_term_free(t); return NULL; }
  return t;
}
