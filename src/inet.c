/* inet.c — interaction-combinator runtime (Phase 10: the syntax GRAPH).
 *
 * See inet.h for the high-level description.  The net is stored as:
 *   - a heap of binary AGENTS (CON, DUP, OPR, OP1), each holding its two
 *     auxiliary ports; an agent's principal port is implicit (referenced by
 *     whoever points at it);
 *   - VAR slots, each holding the port at the other end of a wire (this is how
 *     auxiliary-to-auxiliary wires and dangling binders are represented);
 *   - a pool of exact GMP integers for NUM agents;
 *   - a stack of REDEXES (active pairs: two principal ports wired together).
 *
 * A Port is a packed (tag, value):
 *   VAR -> value is a var-slot index;  CON/DUP/OPR/OP1 -> value is a heap index;
 *   NUM -> value is a number-pool index;  ERA -> nullary.
 *
 * `link` performs wire-chasing substitution and pushes a redex when it joins
 * two principal ports.  `interact` applies the rewrite for one active pair.
 * `reduce` runs to normal form.  Two rules cover the pure fragment:
 *   ANNIHILATE (same agent: CON~CON is beta, DUP~DUP is wire-merge) and
 *   COMMUTE (different agents: this is how DUP copies a function -> sharing).
 * ERA erases; NUM~OPR/OP1 perform exact arithmetic; NUM~DUP copies a number.
 */

#include "inet.h"
#include <stdlib.h>
#include <string.h>

/* ---- port encoding ---------------------------------------------------- */
typedef unsigned long Port;
#define T_VARP 0u
#define T_ERA  1u
#define T_NUM  2u
#define T_CON  3u
#define T_DUP  4u
#define T_OPR  5u
#define T_OP1  6u
#define T_EMP  7u           /* sentinel: an unbound var slot */

#define MKP(tag, val) ( ((Port)(val) << 3) | (Port)(tag) )
#define PTAG(p)       ((unsigned)((p) & 7u))
#define PVAL(p)       ((unsigned long)((p) >> 3))
#define P_ERAP        (MKP(T_ERA, 0))
#define P_EMPTY       (MKP(T_EMP, 0))

#define ISBIN(t) ((t) == T_CON || (t) == T_DUP || (t) == T_OPR || (t) == T_OP1)
#define ISNUL(t) ((t) == T_ERA || (t) == T_NUM)

/* ---- capacities (single-threaded, static) ----------------------------- */
#define NODECAP (1 << 19)
#define VARCAP  (1 << 20)
#define NUMCAP  (1 << 18)
#define RDXCAP  (1 << 19)
#define STEP_LIMIT 200000000L

/* binary agents */
static Port g_p1[NODECAP];
static Port g_p2[NODECAP];
static int  g_tag[NODECAP];     /* T_CON / T_DUP / T_OPR / T_OP1            */
static int  g_lab[NODECAP];     /* DUP label, or OPR/OP1 operator code      */
static long g_ex[NODECAP];      /* OP1: stored-left number index; else -1   */
static int  g_nnode;
static int  g_freelist[NODECAP];
static int  g_nfree;

/* var slots */
static Port g_vars[VARCAP];
static int  g_nvar;

/* exact-integer pool (lazily initialised; reused across runs) */
static mpz_t g_num[NUMCAP];
static int   g_nnum;
static int   g_num_hw;          /* high-water: how many mpz_t are init'd    */

/* redex stack */
static Port g_ra[RDXCAP];
static Port g_rb[RDXCAP];
static int  g_nrdx;

static long g_inter;            /* interaction (rewrite) counter            */
static int  g_oom;              /* set on resource exhaustion               */
static Port g_root;             /* the program's single free output wire    */

/* ---- allocators ------------------------------------------------------- */
static unsigned long new_var(void);

static void inet_reset(void) {
  g_nnode = 0; g_nfree = 0; g_nvar = 0; g_nnum = 0;
  g_nrdx = 0; g_inter = 0; g_oom = 0;
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

/* allocate a NUM slot holding the given mpz value (by copy) */
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

/* ---- link: wire two ports, chasing through vars ----------------------- */
static void link_ports(Port a, Port b) {
  for (;;) {
    if (PTAG(a) != T_VARP && PTAG(b) == T_VARP) { Port t = a; a = b; b = t; }
    if (PTAG(a) != T_VARP) {
      /* both ends are principal ports (or nullary): active pair */
      push_redex(a, b);
      return;
    }
    /* a is a VAR slot */
    {
      unsigned long va = PVAL(a);
      Port got = g_vars[va];
      if (PTAG(got) == T_EMP) { g_vars[va] = b; return; }
      g_vars[va] = P_EMPTY;
      a = got;            /* continue: link what a was wired to, with b */
    }
  }
}

/* ---- exact arithmetic ------------------------------------------------- */
static unsigned long compute(int op, const mpz_t x, const mpz_t y) {
  mpz_t r;
  unsigned long idx;
  mpz_init(r);
  switch (op) {
    case INET_ADD: mpz_add(r, x, y); break;
    case INET_SUB: mpz_sub(r, x, y); break;
    case INET_MUL: mpz_mul(r, x, y); break;
    case INET_DIV: if (mpz_sgn(y) != 0) mpz_tdiv_q(r, x, y); break;
    case INET_MOD: if (mpz_sgn(y) != 0) mpz_tdiv_r(r, x, y); break;
    case INET_EQ:  mpz_set_si(r, mpz_cmp(x, y) == 0 ? 1 : 0); break;
    case INET_LT:  mpz_set_si(r, mpz_cmp(x, y) <  0 ? 1 : 0); break;
    case INET_GT:  mpz_set_si(r, mpz_cmp(x, y) >  0 ? 1 : 0); break;
    default: break;
  }
  idx = new_num_z(r);
  mpz_clear(r);
  return idx;
}

/* ---- the interaction rules -------------------------------------------- */
static void commute(unsigned ta, unsigned long ia, unsigned tb, unsigned long ib) {
  /* a (binary) ~ b (binary), different agents: each duplicates the other */
  int ca1 = new_node((int)ta, g_lab[ia]);
  int ca2 = new_node((int)ta, g_lab[ia]);
  int cb1 = new_node((int)tb, g_lab[ib]);
  int cb2 = new_node((int)tb, g_lab[ib]);
  /* OP1 carries a stored operand: copies must keep it */
  if (ta == T_OP1) { g_ex[ca1] = g_ex[ia]; g_ex[ca2] = g_ex[ia]; }
  if (tb == T_OP1) { g_ex[cb1] = g_ex[ib]; g_ex[cb2] = g_ex[ib]; }
  /* a's aux ports meet copies of b; b's aux ports meet copies of a */
  link_ports(g_p1[ia], MKP(tb, cb1));
  link_ports(g_p2[ia], MKP(tb, cb2));
  link_ports(g_p1[ib], MKP(ta, ca1));
  link_ports(g_p2[ib], MKP(ta, ca2));
  /* the 2x2 grid among the fresh agents' aux ports */
  link_ports(g_p1[ca1], g_p1[cb1]);
  link_ports(g_p2[ca1], g_p1[cb2]);
  link_ports(g_p1[ca2], g_p2[cb1]);
  link_ports(g_p2[ca2], g_p2[cb2]);
  free_node((int)ia);
  free_node((int)ib);
}

static void annihilate(unsigned long ia, unsigned long ib) {
  /* same agent: connect aux ports straight across (CON~CON is beta) */
  link_ports(g_p1[ia], g_p1[ib]);
  link_ports(g_p2[ia], g_p2[ib]);
  free_node((int)ia);
  free_node((int)ib);
}

static void erase_bin(unsigned long bi) {
  /* an eraser hit a binary agent: erase it, propagating ERA to its aux */
  link_ports(g_p1[bi], P_ERAP);
  link_ports(g_p2[bi], P_ERAP);
  free_node((int)bi);
}

static void interact(Port a, Port b) {
  unsigned ta = PTAG(a), tb = PTAG(b);
  unsigned long ia = PVAL(a), ib = PVAL(b);
  g_inter++;

  if (ISNUL(ta) && ISNUL(tb)) {
    return;                                   /* VOID: both vanish */
  }
  if (ISNUL(ta) || ISNUL(tb)) {
    unsigned nt; unsigned long ni; unsigned bt; unsigned long bi;
    if (ISNUL(ta)) { nt = ta; ni = ia; bt = tb; bi = ib; }
    else           { nt = tb; ni = ib; bt = ta; bi = ia; }
    if (nt == T_ERA) { erase_bin(bi); return; }
    /* nt == T_NUM */
    if (bt == T_DUP) {                         /* copy the number to both aux */
      link_ports(g_p1[bi], MKP(T_NUM, ni));
      link_ports(g_p2[bi], MKP(T_NUM, ni));
      free_node((int)bi);
      return;
    }
    if (bt == T_OPR) {                         /* absorb left operand */
      int o = new_node(T_OP1, g_lab[bi]);
      g_ex[o] = (long)ni;                      /* store left number */
      link_ports(g_p1[bi], MKP(T_OP1, o));     /* right operand -> OP1 principal */
      link_ports(g_p2[bi], g_p1[o]);           /* OP1 result aux -> result dest */
      link_ports(g_p2[o], P_ERAP);             /* OP1 unused aux */
      free_node((int)bi);
      return;
    }
    if (bt == T_OP1) {                         /* compute left op right */
      unsigned long r = compute(g_lab[bi], g_num[(unsigned long)g_ex[bi]], g_num[ni]);
      link_ports(g_p1[bi], MKP(T_NUM, r));
      free_node((int)bi);
      return;
    }
    /* NUM ~ CON : ill-typed (number used as a function); erase defensively */
    erase_bin(bi);
    return;
  }

  /* both binary */
  if (ta == tb && (ta == T_CON || (ta == T_DUP && g_lab[ia] == g_lab[ib]))) {
    annihilate(ia, ib);
  } else {
    commute(ta, ia, tb, ib);
  }
}

static void reduce(void) {
  while (g_nrdx > 0 && !g_oom && g_inter < STEP_LIMIT) {
    Port a = g_ra[--g_nrdx];
    Port b = g_rb[g_nrdx];
    interact(a, b);
  }
}

/* ======================================================================= */
/*  surface terms                                                          */
/* ======================================================================= */
static inet_term_t *talloc(inet_tkind_t k) {
  inet_term_t *t = (inet_term_t *)malloc(sizeof(inet_term_t)); /* ownership-audit: allow */
  t->kind = k; t->name = NULL; t->a = NULL; t->b = NULL; t->op = 0;
  return t;
}
static char *dupstr(const char *s) {
  size_t n = strlen(s) + 1;
  char *c = (char *)malloc(n); /* ownership-audit: allow */
  memcpy(c, s, n);
  return c;
}

inet_term_t *inet_var(const char *name) {
  inet_term_t *t = talloc(INET_TVAR); t->name = dupstr(name); return t;
}
inet_term_t *inet_lam(const char *name, inet_term_t *body) {
  inet_term_t *t = talloc(INET_TLAM); t->name = dupstr(name); t->a = body; return t;
}
inet_term_t *inet_app(inet_term_t *f, inet_term_t *a) {
  inet_term_t *t = talloc(INET_TAPP); t->a = f; t->b = a; return t;
}
inet_term_t *inet_num_si(long v) {
  inet_term_t *t = talloc(INET_TNUM); mpz_init_set_si(t->num, v); return t;
}
inet_term_t *inet_num_str(const char *decimal) {
  inet_term_t *t = talloc(INET_TNUM); mpz_init_set_str(t->num, decimal, 10); return t;
}
inet_term_t *inet_num_z(const mpz_t v) {
  inet_term_t *t = talloc(INET_TNUM); mpz_init_set(t->num, v); return t;
}
inet_term_t *inet_op(int op, inet_term_t *l, inet_term_t *r) {
  inet_term_t *t = talloc(INET_TOP); t->op = op; t->a = l; t->b = r; return t;
}
void inet_term_free(inet_term_t *t) {
  if (!t) return;
  if (t->kind == INET_TNUM) mpz_clear(t->num);
  if (t->name) free(t->name); /* ownership-audit: allow */
  inet_term_free(t->a);
  inet_term_free(t->b);
  free(t); /* ownership-audit: allow */
}

/* ---- compilation: term -> net ----------------------------------------- */
/* A scope frame collects the var-port handles where a binder is used, so the
 * binder can be wired to them (ERA for 0 uses, a wire for 1, a DUP tree for
 * many).  Distinct binders get distinct DUP labels. */
#define MAXUSE 4096
#define MAXDEPTH 1024
typedef struct { const char *name; Port uses[MAXUSE]; int n; } Frame;
static Frame g_scope[MAXDEPTH];
static int   g_sp;
static int   g_label;

static Port compile(inet_term_t *t);

static void wire_binder(Port binder, Port *uses, int n) {
  if (n == 0) {
    link_ports(binder, P_ERAP);
  } else if (n == 1) {
    link_ports(binder, uses[0]);
  } else {
    /* right-leaning DUP chain, one fresh label for this binder */
    int lab = ++g_label;
    Port cur = binder;
    int i;
    for (i = 0; i < n - 1; i++) {
      int d = new_node(T_DUP, lab);
      link_ports(cur, MKP(T_DUP, d));   /* principal of this dup */
      link_ports(g_p1[d], uses[i]);     /* left aux -> use i */
      cur = g_p2[d];                    /* right aux -> rest */
    }
    link_ports(cur, uses[n - 1]);
  }
}

static Port compile(inet_term_t *t) {
  switch (t->kind) {
    case INET_TVAR: {
      int i;
      unsigned long v = new_var();
      Port use = MKP(T_VARP, v);
      for (i = g_sp - 1; i >= 0; i--) {
        if (strcmp(g_scope[i].name, t->name) == 0) {
          if (g_scope[i].n < MAXUSE) g_scope[i].uses[g_scope[i].n++] = use;
          else g_oom = 1;
          return use;
        }
      }
      /* an unbound variable: leave it dangling, erased by an ERA agent */
      link_ports(use, P_ERAP);
      return use;
    }
    case INET_TLAM: {
      int con = new_node(T_CON, 0);
      Port out = MKP(T_CON, con);     /* the lambda value = its principal */
      Port body;
      if (g_sp < MAXDEPTH) {
        g_scope[g_sp].name = t->name; g_scope[g_sp].n = 0; g_sp++;
      } else { g_oom = 1; }
      body = compile(t->a);
      link_ports(g_p2[con], body);    /* body value -> aux2 */
      g_sp--;
      wire_binder(g_p1[con], g_scope[g_sp].uses, g_scope[g_sp].n);
      return out;
    }
    case INET_TAPP: {
      int app = new_node(T_CON, 0);
      Port fout = compile(t->a);
      Port aout = compile(t->b);
      link_ports(MKP(T_CON, app), fout);  /* principal meets the function */
      link_ports(g_p1[app], aout);        /* aux1 = argument */
      return g_p2[app];                   /* aux2 = result = this app's value */
    }
    case INET_TNUM: {
      return MKP(T_NUM, new_num_z(t->num));
    }
    case INET_TOP: {
      int opr = new_node(T_OPR, t->op);
      Port lout = compile(t->a);
      Port rout = compile(t->b);
      link_ports(MKP(T_OPR, opr), lout);  /* principal awaits left operand */
      link_ports(g_p1[opr], rout);        /* aux1 = right operand */
      return g_p2[opr];                   /* aux2 = result */
    }
  }
  return P_ERAP;
}

/* ---- readback (integers only) ----------------------------------------- */
static Port enter(Port p) {
  while (PTAG(p) == T_VARP) {
    Port nx = g_vars[PVAL(p)];
    if (PTAG(nx) == T_EMP) break;
    p = nx;
  }
  return p;
}

int inet_normalize_int(inet_term_t *t, mpz_t out, long *interactions) {
  Port r, root;
  inet_reset();
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
  return 0;   /* did not reduce to an integer */
}

long inet_live_nodes(void) {
  return (long)g_nnode - (long)g_nfree;
}

/* ---- readback to a lambda term (de Bruijn normal form) ---------------- *
 * Integer readback (above) only observes a NUM at the root.  To read back an
 * arbitrary normal form we need to walk the net as a term, which the reducer's
 * one-directional wiring does not allow directly (a use cannot find its
 * binder).  So after reduction we build a *bidirectional* partner snapshot:
 * for every port we record the port at the other end of its wire.  Ports are
 * numbered 3*node + slot (slot 0 = principal, 1 = aux1, 2 = aux2); leaves and
 * the root are encoded as small negative sentinels.  A lambda is a CON reached
 * through its principal; an application is a CON whose *aux2* is the output;
 * a bound-variable occurrence is a wire arriving at a lambda's aux1; sharing
 * (a DUP chain) is followed through its principal toward the binder, so every
 * occurrence of a duplicated variable reads back to the same de Bruijn index.
 * Variables are printed "#k" (de Bruijn, 0 = innermost binder); applications
 * "(f a)"; abstractions "(lam body)"; numbers in decimal.  Correct for closed
 * normal forms in the stratified fragment the front-end targets — including
 * combinators (e.g. S K K), Church numerals, and Church addition.  When a
 * BARE normal form retains residual sharing of a *compound* subterm (e.g.
 * Church multiplication or self-application like (2 2), whose normal form
 * keeps DUP nodes that are not in active-pair position), readback returns 0:
 * faithfully expanding that sharing into a tree needs the labelled-dup
 * (bracket) bookkeeping of optimal reduction, which this reader does not do.
 * The refusal is safe — such a term still reduces to the correct value when
 * demanded (e.g. (MUL 2 3) succ 0 = 6 via inet_normalize_int). */
#define RB_NONE (-1)
#define RB_ERA  (-2)
/* a NUM leaf with pool index k is encoded as -(k) - 16 (so it never collides
 * with RB_NONE/RB_ERA, and decodes as k = -code - 16) */
#define RB_NUM_BIAS 16

static int  g_partner[3 * NODECAP];
static int  g_varowner[VARCAP];      /* aux var -> node*4 + slot; else -1 */
static int  g_bkt_a[VARCAP];         /* rendezvous var -> first/second aux  */
static int  g_bkt_b[VARCAP];
static int  g_bind_depth[NODECAP];   /* lambda node -> binder depth         */
static char g_live[NODECAP];

static char  *g_rb_buf;
static size_t g_rb_cap;
static size_t g_rb_pos;
static int    g_rb_of;               /* output overflowed the buffer        */
static int    g_rb_ok;               /* term representable so far           */
static long   g_rb_steps;

static void rb_putc(char c) {
  if (g_rb_of) return;
  if (g_rb_pos + 1u >= g_rb_cap) { g_rb_of = 1; return; }
  g_rb_buf[g_rb_pos++] = c;
}
static void rb_puts(const char *s) { while (*s) { rb_putc(*s); s++; } }
static void rb_putint(int n) {
  char tmp[16];
  int k = 0, j;
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
  for (i = 0; i < g_nvar; i++) { g_varowner[i] = -1; g_bkt_a[i] = -1; g_bkt_b[i] = -1; }
  for (i = 0; i < g_nnode; i++) {
    if (!g_live[i]) continue;
    g_varowner[PVAL(g_p1[i])] = i * 4 + 1;
    g_varowner[PVAL(g_p2[i])] = i * 4 + 2;
  }
  for (i = 0; i < g_nnode; i++) {
    if (!g_live[i]) continue;
    for (slot = 1; slot <= 2; slot++) {
      Port ap = (slot == 1) ? g_p1[i] : g_p2[i];
      int auxid = i * 3 + slot;
      Port t = enter(ap);
      unsigned tg = PTAG(t);
      if (tg == T_CON || tg == T_DUP || tg == T_OPR || tg == T_OP1) {
        int m = (int)PVAL(t);
        g_partner[auxid] = m * 3;
        g_partner[m * 3] = auxid;
      } else if (tg == T_NUM) {
        g_partner[auxid] = -(int)PVAL(t) - RB_NUM_BIAS;
      } else if (tg == T_ERA) {
        g_partner[auxid] = RB_ERA;
      } else {                       /* T_VARP: an aux-aux rendezvous */
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
  if (++g_rb_steps > 5000000L) { g_rb_ok = 0; return; }
  m = id / 3; slot = id % 3; tag = g_tag[m];
  if (slot == 0) {
    if (tag == T_CON) {                    /* reached via principal -> lambda */
      g_bind_depth[m] = depth;
      rb_puts("(lam ");
      rb_read_from(m * 3 + 2, depth + 1);
      rb_putc(')');
    } else { g_rb_ok = 0; }                /* DUP/OPR principal: see boundary note */
  } else if (slot == 2) {
    if (tag == T_CON) {                    /* aux2 is the output -> application */
      rb_putc('(');
      rb_read_from(m * 3 + 0, depth);
      rb_putc(' ');
      rb_read_from(m * 3 + 1, depth);
      rb_putc(')');
    } else if (tag == T_DUP) {
      rb_read_from(m * 3 + 0, depth);      /* follow toward the binder */
    } else { g_rb_ok = 0; }
  } else {                                 /* slot == 1 */
    if (tag == T_CON) {                    /* a lambda's binder -> bound var */
      rb_putc('#');
      rb_putint(depth - g_bind_depth[m] - 1);
    } else if (tag == T_DUP) {
      rb_read_from(m * 3 + 0, depth);
    } else { g_rb_ok = 0; }
  }
}

static void rb_read_from(int consumer_id, int depth) {
  int p;
  if (!g_rb_ok) return;
  p = g_partner[consumer_id];
  if (p == RB_NONE) { g_rb_ok = 0; return; }
  if (p == RB_ERA) { rb_putc('_'); return; }
  if (p <= -RB_NUM_BIAS) { rb_putmpz(g_num[-p - RB_NUM_BIAS]); return; }
  rb_read_producer(p, depth);
}

int inet_readback(inet_term_t *t, char *buf, size_t cap, long *interactions) {
  Port r, root;
  unsigned tg;
  if (cap == 0) return 0;
  inet_reset();
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
    rb_putc('_');
  } else if (tg == T_CON || tg == T_DUP || tg == T_OPR || tg == T_OP1) {
    rb_read_producer((int)PVAL(r) * 3, 0);
  } else {                                 /* root meets an aux-aux rendezvous */
    int rp = g_bkt_a[PVAL(r)];
    if (rp < 0) g_rb_ok = 0;
    else rb_read_producer(rp, 0);
  }
  if (!g_rb_ok || g_rb_of) return 0;
  buf[g_rb_pos] = '\0';
  return 1;
}
