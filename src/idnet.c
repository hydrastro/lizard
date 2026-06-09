/* idnet.c -- Id-by-observation realised as an INTERACTION NET.
 *
 * id_observe.c is the executable SPEC: it computes Id_A(x,y) by recursion on the
 * structure of A, as a tree-rewriting system. This module realises the SAME
 * semantics as graph rewriting -- the duality-framework claim that "Id is the
 * observation side, and each rule is `Id meets a type-former and reduces by that
 * former's structural rule`." Here that is literal: an ID agent's principal port
 * meets a type-former agent's principal port, and the interaction rule for that
 * collision is exactly the former's structural Id-rule.
 *
 *     ID . TY_UNIT            -->  Unit
 *     ID . TY_U   (A,B)       -->  Equiv A B                  [univalence]
 *     ID . TY_PROD(A,B)       -->  (Id A a c) * (Id B b d)    [componentwise; values
 *                                  a,b / c,d are projected by UNPAIR . PAIR]
 *     ID . TY_BOOL            -->  observe x then y (case analysis): Unit / Empty
 *     ID . TY_NAT             -->  observe x then y, RECURSING on predecessors:
 *                                  Id_Nat(succ m, succ n) spawns Id_Nat(m,n)
 *
 * The product rule is genuinely recursive in the type; the Nat rule is genuinely
 * recursive in the value (an IDN agent re-spawns itself on the predecessors).
 * Reduction is by local interaction only; the result type is read back as an
 * id_node and checked against id_nf() (the spec) in tests/idnet_test.c, including
 * a 200k-case differential fuzz over random types and values.
 *
 * Scope: the function case (Id_(A->B) f g = Pi z. Id B (f z)(g z), funext) needs
 * the lambda/application machinery and is the documented next extension -- the net
 * read-back REFUSES it (returns NULL) rather than emit a wrong type, matching the
 * project's soundness discipline. Unit / Bool / Nat / Prod / U cover the inductive
 * and universe fragment, exhibiting structural recursion on the type, recursion on
 * values, and univalence at the universe.
 *
 * Build: cc <flags> -Isrc tests/idnet_test.c src/idnet.c src/id_observe.c -o idnet_test
 */
#include "id_observe.h"
#include "idnet.h"
#include <stdlib.h>

enum {
  N_ROOT,
  TY_UNIT, TY_EMPTY, TY_U, TY_BOOL, TY_NAT, TY_PROD,   /* type formers          */
  TY_ARR, TY_PI, TY_SIGMA,                             /* function / pair type formers */
  VAL_STAR, VAL_PAIR,                                  /* unit / product values */
  VAL_TRUE, VAL_FALSE, VAL_ZERO, VAL_SUCC,             /* bool / nat values     */
  N_LAM, N_VAR,                                        /* function value, bound variable */
  N_APP, N_IF,                                         /* application, Bool eliminator   */
  N_REFL, N_UA, N_TR,                                  /* refl path, ua path, transport  */
  N_ID, N_EQUIV, N_UNPAIR,                             /* Id / result / projection */
  N_IDB, N_EQT, N_EQF,                                 /* Bool observation       */
  N_IDN, N_ISZ, N_SCS,                                 /* Nat observation (rec)  */
  N_PI, N_NID,                                         /* result Pi-type, neutral Id */
  N_SIGD,                                              /* Sigma-Id decision (faces the first-component Id result) */
  N_ERA                                                /* eraser / gc            */
};

#define MAXN  200000
#define PORTS 4

static int  k_[MAXN];                 /* kind of each node                       */
static int  vidx_[MAXN];              /* N_VAR: de Bruijn index                  */
static int  pt_[MAXN * PORTS];        /* port target node (-1 = free)            */
static int  pp_[MAXN * PORTS];        /* port target port                        */
static int  n_;                       /* node count                             */
static long inter_;                   /* interaction count                      */
static int  over_;                    /* arena/step overflow                    */

static int mk(int kind) {
  int a = n_++, p;
  if (n_ > MAXN) { over_ = 1; return 0; }
  k_[a] = kind;
  for (p = 0; p < PORTS; p++) { pt_[a * PORTS + p] = -1; pp_[a * PORTS + p] = -1; }
  return a;
}
static void link_(int a, int pa, int b, int pb) {
  pt_[a * PORTS + pa] = b; pp_[a * PORTS + pa] = pb;
  pt_[b * PORTS + pb] = a; pp_[b * PORTS + pb] = pa;
}
/* connect whatever was attached to (a,pa) to (b,pb) -- splice past node a */
static void splice(int a, int pa, int b, int pb) {
  int t = pt_[a * PORTS + pa], q = pp_[a * PORTS + pa];
  if (t >= 0) link_(t, q, b, pb);
  else { pt_[b * PORTS + pb] = -1; pp_[b * PORTS + pb] = -1; }
}
static void era_at(int a, int pa) {            /* drop the subnet hanging off (a,pa) */
  int e = mk(N_ERA);
  splice(a, pa, e, 0);
}
/* connect the neighbour of (a,pa) directly to the neighbour of (b,pb), splicing
 * past BOTH nodes a and b (used when an agent meets data and both sides already
 * have live wires, e.g. UNPAIR . PAIR forwarding each component to its use site) */
static void bridge(int a, int pa, int b, int pb) {
  int ta = pt_[a * PORTS + pa], qa = pp_[a * PORTS + pa];
  int tb = pt_[b * PORTS + pb], qb = pp_[b * PORTS + pb];
  if (ta >= 0 && tb >= 0) link_(ta, qa, tb, qb);
  else if (ta >= 0) { pt_[ta * PORTS + qa] = -1; pp_[ta * PORTS + qa] = -1; }
  else if (tb >= 0) { pt_[tb * PORTS + qb] = -1; pp_[tb * PORTS + qb] = -1; }
}

/* the "output"/up port of a node -- where its consumer connects. Most nodes use
 * port 0; the eliminators face what they consume on port 0, so their result is
 * elsewhere: application/Id-family agents output on port 1, the Bool `if` on 3. */
static int up_port(int kind) {
  if (kind == N_IF) return 3;
  if (kind == N_APP || kind == N_TR || kind == N_ID || kind == N_UNPAIR ||
      kind == N_IDB || kind == N_EQT || kind == N_EQF ||
      kind == N_IDN || kind == N_ISZ || kind == N_SCS) return 1;
  return 0;
}

/* ---- encode an id_node (a type or a value) into a net; return its root node.
 * The node's principal (port 0) is left dangling for the caller to wire. ---- */
static int enc(const id_node *t) {
  switch (t->kind) {
    case ID_UNIT:  return mk(TY_UNIT);
    case ID_EMPTY: return mk(TY_EMPTY);
    case ID_U:     return mk(TY_U);
    case ID_BOOL:  return mk(TY_BOOL);
    case ID_NAT:   return mk(TY_NAT);
    case ID_STAR:  return mk(VAL_STAR);
    case ID_TRUE:  return mk(VAL_TRUE);
    case ID_FALSE: return mk(VAL_FALSE);
    case ID_ZERO:  return mk(VAL_ZERO);
    case ID_SUCC: { int a = mk(VAL_SUCC), p = enc(t->a); link_(a, 1, p, 0); return a; }
    case ID_PROD: { int a = mk(TY_PROD), x = enc(t->a), y = enc(t->b);
                    link_(a, 1, x, 0); link_(a, 2, y, 0); return a; }
    case ID_EQUIV:{ int a = mk(N_EQUIV), x = enc(t->a), y = enc(t->b);
                    link_(a, 1, x, 0); link_(a, 2, y, 0); return a; }
    case ID_PAIR: { int a = mk(VAL_PAIR), x = enc(t->a), y = enc(t->b);
                    link_(a, 1, x, 0); link_(a, 2, y, 0); return a; }
    case ID_ARR:  { int a = mk(TY_ARR), x = enc(t->a), y = enc(t->b);
                    link_(a, 1, x, 0); link_(a, 2, y, 0); return a; }
    case ID_PI:   { int a = mk(TY_PI), x = enc(t->a), y = enc(t->b);
                    link_(a, 1, x, 0); link_(a, 2, y, 0); return a; }
    case ID_SIGMA:{ int a = mk(TY_SIGMA), x = enc(t->a), y = enc(t->b);
                    link_(a, 1, x, up_port(k_[x])); link_(a, 2, y, up_port(k_[y])); return a; }
    case ID_LAM:  { int a = mk(N_LAM), b = enc(t->a); link_(a, 1, b, up_port(k_[b])); return a; }
    case ID_VAR:  { int a = mk(N_VAR); vidx_[a] = t->idx; return a; }
    case ID_ID:   { int I = mk(N_ID), A = enc(t->a), x = enc(t->b), y = enc(t->c);
                    link_(I, 0, A, up_port(k_[A]));   /* ID principal meets the type former */
                    link_(I, 2, x, up_port(k_[x]));
                    link_(I, 3, y, up_port(k_[y])); return I; }
    case ID_REFL: return mk(N_REFL);
    case ID_UA:   { int u = mk(N_UA), f = enc(t->a); link_(u, 1, f, up_port(k_[f])); return u; }
    case ID_APP:  { int ap = mk(N_APP), f = enc(t->a), x = enc(t->b);
                    link_(ap, 0, f, up_port(k_[f]));  /* APP principal meets the function */
                    link_(ap, 2, x, up_port(k_[x])); return ap; }
    case ID_IF:   { int q = mk(N_IF), c = enc(t->a), th = enc(t->b), el = enc(t->c);
                    link_(q, 0, c, up_port(k_[c]));   /* IF principal meets the scrutinee */
                    link_(q, 1, th, up_port(k_[th]));
                    link_(q, 2, el, up_port(k_[el])); return q; }
    case ID_TRANSP: {
                    if (t->a->kind != ID_LAM) return mk(N_ERA);   /* non-lambda family -> refuse */
                    { int body = enc(t->a->a), p = enc(t->b), x = enc(t->c), tr = mk(N_TR);
                      link_(tr, 0, body, up_port(k_[body]));  /* TR faces the family BODY */
                      link_(tr, 2, p, up_port(k_[p]));        /* the path                 */
                      link_(tr, 3, x, up_port(k_[x]));        /* the value                */
                      return tr; } }
    default:       return mk(N_ERA);   /* unsupported leaf: harmless stub */
  }
}

/* ---- the interaction rules ---- */

/* Copy the tree rooted at `node` (which connects to its parent via `entry`); every
 * OTHER port leads to a child, so we recurse on those. If arg>=0 we also perform a
 * de Bruijn substitution var(depth) := a fresh copy of `arg` (used for beta). The
 * substituted argument is assumed closed (true for the values we apply), so no
 * shifting of `arg` is needed. Trees only (values / types / paths / closed bodies). */
static int cs_guard;
static int cs(int node, int entry, int depth, int arg) {
  int k, nn, p;
  if (node < 0) return mk(N_ERA);
  if (++cs_guard > 300000) { cs_guard--; over_ = 1; return mk(N_ERA); }
  k = k_[node];
  if (k == N_VAR) {
    int vi = vidx_[node], r;
    cs_guard--;
    if (arg >= 0 && vi == depth) return cs(arg, up_port(k_[arg]), 0, -1);  /* substitute */
    r = mk(N_VAR); vidx_[r] = (arg >= 0 && vi > depth) ? vi - 1 : vi;       /* shift past removed binder */
    return r;
  }
  nn = mk(k); vidx_[nn] = vidx_[node];
  for (p = 0; p < PORTS; p++) {
    int c, cp, nd, nc;
    if (p == entry) continue;
    c = pt_[node * PORTS + p];
    if (c < 0) continue;
    cp = pp_[node * PORTS + p];
    nd = depth;
    if ((k == N_LAM && p == 1) || (k == N_PI && p == 2)) nd = depth + 1;   /* under one more binder */
    nc = cs(c, cp, nd, arg);
    link_(nn, p, nc, cp);
  }
  cs_guard--;
  return nn;
}

/* an "active pair" is two nodes whose principals (port 0) are wired together */
static int is_active_agent(int kind) {
  return kind == N_ID || kind == N_UNPAIR ||
         kind == N_IDB || kind == N_EQT || kind == N_EQF ||
         kind == N_IDN || kind == N_ISZ || kind == N_SCS ||
         kind == N_APP || kind == N_IF || kind == N_TR || kind == N_SIGD;
}

/* apply the rule for agent `a` meeting data `d` (both at principal port 0).
 * Sets keep_d=1 when the data node must survive (it becomes part of a neutral
 * result, e.g. the variable in Id_Nat(z)(y)); otherwise the caller erases it. */
static int keep_d;
static void rule(int a, int d) {
  int ka = k_[a], kd = k_[d];
  keep_d = 0;
  if (ka == N_ID) {
    /* I.1 = output, I.2 = x, I.3 = y ; d = the type former at I.0 */
    if (kd == TY_UNIT || kd == TY_EMPTY) {
      int u = mk(kd);                       /* Id_Unit(*,*) = Unit ; Id_Empty = Empty */
      splice(a, 1, u, 0);                   /* output := Unit/Empty                   */
      era_at(a, 2); era_at(a, 3);           /* discard the two values                 */
      return;
    }
    if (kd == TY_U) {
      int e = mk(N_EQUIV);                  /* Id_U(A,B) = Equiv A B (x,y are A,B)    */
      splice(a, 1, e, 0);
      splice(a, 2, e, 1);                   /* A                                      */
      splice(a, 3, e, 2);                   /* B                                      */
      return;
    }
    if (kd == TY_PROD) {
      /* Id_(A*B)((a,b),(c,d)) = (Id A a c) * (Id B b d) -- but only when both sides
       * are literal pairs; if either is neutral (e.g. a variable of product type)
       * the whole Id stays neutral, since the pair cannot be decomposed. */
      int xx = pt_[a * PORTS + 2], yy = pt_[a * PORTS + 3];
      if ((xx >= 0 && k_[xx] == N_VAR) || (yy >= 0 && k_[yy] == N_VAR)) {
        int nid = mk(N_NID);
        splice(a, 1, nid, 0);               /* output -> neutral Id                   */
        link_(nid, 1, d, 0);                /* type = the product former (kept)       */
        splice(a, 2, nid, 2);               /* x                                      */
        splice(a, 3, nid, 3);               /* y                                      */
        keep_d = 1; return;
      }
      {
        int P  = mk(TY_PROD);
        int Ia = mk(N_ID), Ib = mk(N_ID);
        int Ux = mk(N_UNPAIR), Uy = mk(N_UNPAIR);
        splice(a, 1, P, 0);                   /* result product -> output               */
        splice(d, 1, Ia, 0);                  /* A -> Ia principal                      */
        splice(d, 2, Ib, 0);                  /* B -> Ib principal                      */
        splice(a, 2, Ux, 0);                  /* x -> UNPAIR Ux                         */
        splice(a, 3, Uy, 0);                  /* y -> UNPAIR Uy                         */
        link_(Ux, 1, Ia, 2); link_(Uy, 1, Ia, 3);   /* a,c  -> Ia.x, Ia.y              */
        link_(Ux, 2, Ib, 2); link_(Uy, 2, Ib, 3);   /* b,d  -> Ib.x, Ib.y              */
        link_(Ia, 1, P, 1);  link_(Ib, 1, P, 2);    /* component Ids -> product slots  */
      }
      return;
    }
    if (kd == TY_BOOL) {
      /* Id_Bool(x,y): observe x, then observe y. IDB holds y until x is known. */
      int B = mk(N_IDB);
      splice(a, 2, B, 0);                   /* x -> IDB principal                     */
      splice(a, 1, B, 1);                   /* output                                 */
      splice(a, 3, B, 2);                   /* y (held)                               */
      return;
    }
    if (kd == TY_NAT) {
      /* Id_Nat(x,y): observe x, then observe y; recurse on predecessors. */
      int N = mk(N_IDN);
      splice(a, 2, N, 0);                   /* x -> IDN principal                     */
      splice(a, 1, N, 1);                   /* output                                 */
      splice(a, 3, N, 2);                   /* y (held)                               */
      return;
    }
    if (kd == TY_ARR || kd == TY_PI) {
      /* funext: Id (Pi z:A. B) f g = Pi z:A. Id B (f z)(g z). Applying f,g to the
       * fresh binder z is, in de Bruijn, just their bodies (the lambda's var 0 and
       * the new Pi's var 0 coincide), so no beta is needed -- the bodies are reused
       * directly with var 0 now denoting z. (Requires f,g to be lambdas and the
       * codomain closed; otherwise read-back refuses.) */
      int f = pt_[a * PORTS + 2], g = pt_[a * PORTS + 3];
      if (f < 0 || g < 0 || k_[f] != N_LAM || k_[g] != N_LAM) return;   /* not lambdas -> stuck */
      {
        int P = mk(N_PI), In = mk(N_ID);
        splice(a, 1, P, 0);                 /* output -> Pi                    */
        splice(d, 1, P, 1);                 /* domain A -> Pi.1                */
        splice(d, 2, In, 0);                /* codomain B -> Inner Id principal */
        link_(In, 1, P, 2);                 /* Inner result -> Pi body         */
        splice(f, 1, In, 2);                /* body_f -> Inner.x               */
        splice(g, 1, In, 3);                /* body_g -> Inner.y               */
        k_[f] = N_ERA; pt_[f * PORTS] = -1; /* consume the two lambda nodes    */
        k_[g] = N_ERA; pt_[g * PORTS] = -1;
      }
      return;
    }
    if (kd == TY_SIGMA) {
      /* Id (Sigma x:A. B x)((a,b),(a',b')): observe Id_A a a' (-> Unit/Empty, or a
       * product of those), then a decision agent picks Id (B a) b b' (equal) or
       * Empty (unequal).  B is instantiated at a by substitution (cs).  Requires
       * both sides to be literal pairs and A inductive; otherwise refuse. */
      int xx = pt_[a * PORTS + 2], yy = pt_[a * PORTS + 3];
      if (xx < 0 || yy < 0 || k_[xx] != VAL_PAIR || k_[yy] != VAL_PAIR) return;
      {
        int A  = pt_[d * PORTS + 1], Ap = pp_[d * PORTS + 1];     /* first-component type */
        int Bb = pt_[d * PORTS + 2], Bp = pp_[d * PORTS + 2];     /* family body (var0 = x) */
        int a1 = pt_[xx * PORTS + 1], a1p = pp_[xx * PORTS + 1];  /* a  */
        int b1 = pt_[xx * PORTS + 2], b1p = pp_[xx * PORTS + 2];  /* b  */
        int a2 = pt_[yy * PORTS + 1], a2p = pp_[yy * PORTS + 1];  /* a' */
        int b2 = pt_[yy * PORTS + 2], b2p = pp_[yy * PORTS + 2];  /* b' */
        int Ba = cs(Bb, Bp, 0, a1);                              /* B(a): body with var0 := copy(a) */
        int IdA = mk(N_ID), IdB = mk(N_ID), Dec = mk(N_SIGD);
        link_(IdA, 0, A, Ap);  link_(IdA, 2, a1, a1p); link_(IdA, 3, a2, a2p);   /* Id_A a a' */
        link_(IdB, 0, Ba, up_port(k_[Ba])); link_(IdB, 2, b1, b1p); link_(IdB, 3, b2, b2p);  /* Id (B a) b b' */
        link_(Dec, 0, IdA, 1);                 /* decision faces the first-component Id result */
        link_(Dec, 1, IdB, 1);                 /* then-branch: the second-component Id */
        splice(a, 1, Dec, 2);                  /* decision output -> the Sigma-Id output */
        k_[xx] = N_ERA; pt_[xx * PORTS] = -1;  /* the two pair nodes are consumed */
        k_[yy] = N_ERA; pt_[yy * PORTS] = -1;
      }
      return;
    }
    /* unsupported former: leave inert -- read-back refuses */
    return;
  }
  if (ka == N_APP) {
    /* beta: (lam. body) arg.  Substitute var0 := arg into the body (a fresh copy),
     * then wire the reduced body to the application's output. */
    if (kd == N_LAM) {
      int bn = pt_[d * PORTS + 1], bp = pp_[d * PORTS + 1];   /* lambda body root + up port */
      int arg = pt_[a * PORTS + 2];                            /* the argument               */
      int r = cs(bn, bp, 0, arg);
      splice(a, 1, r, bp);                                     /* output := substituted body */
      return;                                                  /* erase APP(a) and LAM(d)    */
    }
    return;   /* application of a non-lambda (neutral) -> stuck -> refuse */
  }
  if (ka == N_IF) {
    /* Bool elimination: if true -> then, if false -> else */
    if (kd == VAL_TRUE)  { bridge(a, 1, a, 3); era_at(a, 2); return; }
    if (kd == VAL_FALSE) { bridge(a, 2, a, 3); era_at(a, 1); return; }
    return;   /* scrutinee neutral -> stuck -> refuse */
  }
  if (ka == N_TR) {
    /* transport^(lam i. <body>) p x, the agent facing the family BODY at port 0,
     * path on port 2, value on port 3.  Recurse on the body's structure, dual to Id. */
    int path = pt_[a * PORTS + 2];
    int pk   = (path >= 0) ? k_[path] : -1;
    int v    = pt_[a * PORTS + 3], vp = pp_[a * PORTS + 3];
    if (pk == N_REFL) {                       /* transport along refl is the identity */
      splice(a, 1, v, vp); era_at(a, 2); return;
    }
    if (kd == TY_BOOL || kd == TY_NAT || kd == TY_UNIT || kd == TY_EMPTY || kd == TY_U) {
      splice(a, 1, v, vp); era_at(a, 2); return;   /* closed (constant) family -> identity */
    }
    if (kd == N_VAR) {
      if (vidx_[d] != 0) { splice(a, 1, v, vp); era_at(a, 2); return; }   /* var_k>0: constant */
      if (pk == N_UA) {                       /* identity family + ua f : transport = f x */
        int f = pt_[path * PORTS + 1], fp = pp_[path * PORTS + 1];
        int ap = mk(N_APP);
        link_(ap, 0, f, fp);                  /* APP faces the forward map f */
        link_(ap, 2, v, vp);                  /* argument = x                */
        splice(a, 1, ap, 1);                  /* output := f x (then beta fires) */
        k_[path] = N_ERA; pt_[path * PORTS] = -1;   /* consume the ua node */
        return;
      }
      return;   /* identity family + non-ua/non-refl path -> neutral -> refuse */
    }
    if (kd == TY_PROD) {                       /* product family: componentwise (HoTT 2.6.4) */
      if (v < 0 || k_[v] != VAL_PAIR) return;  /* value not a pair -> refuse */
      {
        int L = pt_[d * PORTS + 1], Lp = pp_[d * PORTS + 1];
        int R = pt_[d * PORTS + 2], Rp = pp_[d * PORTS + 2];
        int u = pt_[v * PORTS + 1], uq = pp_[v * PORTS + 1];
        int w = pt_[v * PORTS + 2], wq = pp_[v * PORTS + 2];
        int pcopy = cs(path, pp_[a * PORTS + 2], 0, -1);   /* duplicate the path for R */
        int trL = mk(N_TR), trR = mk(N_TR), P = mk(VAL_PAIR);
        splice(a, 1, P, 0);                    /* output := the pair */
        link_(trL, 0, L, Lp); link_(trL, 2, path, pp_[a * PORTS + 2]); link_(trL, 3, u, uq); link_(trL, 1, P, 1);
        link_(trR, 0, R, Rp); link_(trR, 2, pcopy, up_port(k_[pcopy]));  link_(trR, 3, w, wq); link_(trR, 1, P, 2);
        return;
      }
    }
    return;   /* function family (non-refl) / other -> refuse */
  }
  if (ka == N_SIGD) {
    /* the first-component Id has reduced; decide the Sigma-Id.  Dec.1 = then (the
     * second-component Id), Dec.2 = out. */
    if (kd == TY_UNIT)  { bridge(a, 1, a, 2); return; }                 /* equal: use Id (B a) b b' */
    if (kd == TY_EMPTY) { int e = mk(TY_EMPTY); splice(a, 2, e, 0); era_at(a, 1); return; }  /* unequal: Empty */
    if (kd == TY_PROD) {
      /* a product first-component path is contractible iff BOTH halves are: chain
       * two decisions.  Dec(L x R) = decide L ? (decide R ? then : Empty) : Empty. */
      int L = pt_[d * PORTS + 1], Lp = pp_[d * PORTS + 1];
      int R = pt_[d * PORTS + 2], Rp = pp_[d * PORTS + 2];
      int DR = mk(N_SIGD), DL = mk(N_SIGD);
      link_(DR, 0, R, Rp); splice(a, 1, DR, 1);    /* DR faces R; DR.then = orig then */
      link_(DL, 0, L, Lp); link_(DL, 1, DR, 2);    /* DL faces L; DL.then = DR.out    */
      splice(a, 2, DL, 2);                          /* DL.out = orig out               */
      return;                                       /* Dec(a) and the product erased   */
    }
    return;   /* first-component path neutral (e.g. Equiv) -> stuck -> refuse */
  }
  if (ka == N_IDB) {
    /* x is known (d = TRUE/FALSE); now observe y with the matching tester */
    if (kd == VAL_TRUE)  { int e = mk(N_EQT); splice(a, 2, e, 0); splice(a, 1, e, 1); return; }
    if (kd == VAL_FALSE) { int e = mk(N_EQF); splice(a, 2, e, 0); splice(a, 1, e, 1); return; }
    if (kd == N_VAR) {   /* Id_Bool(z)(y) is neutral */
      int nid = mk(N_NID), tb = mk(TY_BOOL);
      link_(nid, 1, tb, 0); link_(nid, 2, d, 0); splice(a, 2, nid, 3); splice(a, 1, nid, 0);
      keep_d = 1; return;
    }
    return;
  }
  if (ka == N_EQT) {       /* x was true: Id is Unit iff y is true, else Empty */
    if (kd == VAL_TRUE)  { int u = mk(TY_UNIT);  splice(a, 1, u, 0); return; }
    if (kd == VAL_FALSE) { int u = mk(TY_EMPTY); splice(a, 1, u, 0); return; }
    if (kd == N_VAR) {   /* Id_Bool(true)(z) neutral */
      int nid = mk(N_NID), tb = mk(TY_BOOL), tt = mk(VAL_TRUE);
      link_(nid, 1, tb, 0); link_(nid, 2, tt, 0); link_(nid, 3, d, 0); splice(a, 1, nid, 0);
      keep_d = 1; return;
    }
    return;
  }
  if (ka == N_EQF) {       /* x was false: Id is Unit iff y is false, else Empty */
    if (kd == VAL_TRUE)  { int u = mk(TY_EMPTY); splice(a, 1, u, 0); return; }
    if (kd == VAL_FALSE) { int u = mk(TY_UNIT);  splice(a, 1, u, 0); return; }
    if (kd == N_VAR) {   /* Id_Bool(false)(z) neutral */
      int nid = mk(N_NID), tb = mk(TY_BOOL), ff = mk(VAL_FALSE);
      link_(nid, 1, tb, 0); link_(nid, 2, ff, 0); link_(nid, 3, d, 0); splice(a, 1, nid, 0);
      keep_d = 1; return;
    }
    return;
  }
  if (ka == N_IDN) {
    /* x known (d = ZERO/SUCC m); observe y accordingly */
    if (kd == VAL_ZERO) {                   /* x = 0 : Id is Unit iff y = 0           */
      int z = mk(N_ISZ); splice(a, 2, z, 0); splice(a, 1, z, 1); return;
    }
    if (kd == VAL_SUCC) {                    /* x = succ m : compare y, recurse on m   */
      int s = mk(N_SCS);
      splice(a, 2, s, 0);                   /* y -> SCS principal                     */
      splice(a, 1, s, 1);                   /* output                                 */
      splice(d, 1, s, 2);                   /* m (predecessor of x) held              */
      return;
    }
    if (kd == N_VAR) {   /* Id_Nat(z)(y) neutral */
      int nid = mk(N_NID), tn = mk(TY_NAT);
      link_(nid, 1, tn, 0); link_(nid, 2, d, 0); splice(a, 2, nid, 3); splice(a, 1, nid, 0);
      keep_d = 1; return;
    }
    return;
  }
  if (ka == N_ISZ) {       /* x was 0: Id Nat is Unit iff y is 0, else Empty */
    if (kd == VAL_ZERO) { int u = mk(TY_UNIT);  splice(a, 1, u, 0); return; }
    if (kd == VAL_SUCC) { int u = mk(TY_EMPTY); splice(a, 1, u, 0); era_at(d, 1); return; }
    if (kd == N_VAR) {   /* Id_Nat(0)(z) neutral */
      int nid = mk(N_NID), tn = mk(TY_NAT), z0 = mk(VAL_ZERO);
      link_(nid, 1, tn, 0); link_(nid, 2, z0, 0); link_(nid, 3, d, 0); splice(a, 1, nid, 0);
      keep_d = 1; return;
    }
    return;
  }
  if (ka == N_SCS) {       /* x was succ m: y=0 -> Empty ; y=succ n -> Id Nat m n */
    if (kd == VAL_ZERO) { int u = mk(TY_EMPTY); splice(a, 1, u, 0); era_at(a, 2); return; }
    if (kd == VAL_SUCC) {                    /* recurse: Id_Nat(m, n) */
      int N = mk(N_IDN);
      splice(a, 2, N, 0);                   /* m -> new IDN principal (the new x)     */
      splice(d, 1, N, 2);                   /* n = predecessor of y (the new y)       */
      splice(a, 1, N, 1);                   /* output                                 */
      return;
    }
    if (kd == N_VAR) {   /* Id_Nat(succ m)(z) neutral; rebuild succ m from held m */
      int nid = mk(N_NID), tn = mk(TY_NAT), sc = mk(VAL_SUCC);
      link_(nid, 1, tn, 0);
      splice(a, 2, sc, 1);                  /* m (held) -> succ's predecessor          */
      link_(nid, 2, sc, 0);                 /* x = succ m                              */
      link_(nid, 3, d, 0);                  /* y = variable                            */
      splice(a, 1, nid, 0);
      keep_d = 1; return;
    }
    return;
  }
  if (ka == N_UNPAIR) {
    /* U.1 = out_a, U.2 = out_b ; d = a VAL_PAIR at U.0.  Forward each component
     * to whatever the UNPAIR output is wired to (its use site), not to the dead
     * pair's port -- otherwise the result points back through the consumed pair. */
    if (kd == VAL_PAIR) {
      bridge(a, 1, d, 1);                   /* use-site of out_a  <->  pair.fst       */
      bridge(a, 2, d, 2);                   /* use-site of out_b  <->  pair.snd       */
      return;
    }
    return;
  }
}

static void reduce(void) {
  int prog = 1; long steps = 0;
  while (prog && !over_) {
    int a; prog = 0;
    for (a = 0; a < n_; a++) {
      int b;
      if (k_[a] == N_ROOT) continue;
      if (pt_[a * PORTS + 0] < 0 || pp_[a * PORTS + 0] != 0) continue;
      b = pt_[a * PORTS + 0];
      if (b < 0 || pp_[b * PORTS + 0] != 0) continue;
      /* a,b are an active pair; orient agent vs data */
      if (is_active_agent(k_[a]) && !is_active_agent(k_[b])) {
        rule(a, b);
        k_[a] = N_ERA; pt_[a*PORTS] = -1;
        if (!keep_d) { k_[b] = N_ERA; pt_[b*PORTS] = -1; }
        inter_++; prog = 1; if (++steps > 4000000L) { over_ = 1; return; }
      } else if (is_active_agent(k_[b]) && !is_active_agent(k_[a])) {
        rule(b, a);
        k_[b] = N_ERA; pt_[b*PORTS] = -1;
        if (!keep_d) { k_[a] = N_ERA; pt_[a*PORTS] = -1; }
        inter_++; prog = 1; if (++steps > 4000000L) { over_ = 1; return; }
      }
    }
  }
}

/* ---- read the result type net back to an id_node ; *bad on a stuck/unsupported net ---- */
static int rb_bad;
static int rb_depth;
static id_node *rb(int a, int p) {
  if (++rb_depth > 100000) { rb_depth--; rb_bad = 1; return id_base(ID_UNIT); }
  for (;;) {
    if (a < 0) { rb_depth--; rb_bad = 1; return id_base(ID_UNIT); }
    if (k_[a] == N_ROOT) { int n = pt_[a*PORTS+0], q = pp_[a*PORTS+0]; a = n; p = q; continue; }
    break;
  }
  {
    id_node *r;
    switch (k_[a]) {
      case TY_UNIT:  r = id_base(ID_UNIT);  break;
      case TY_EMPTY: r = id_base(ID_EMPTY); break;
      case TY_U:     r = id_base(ID_U);     break;
      case TY_BOOL:  r = id_base(ID_BOOL);  break;
      case TY_NAT:   r = id_base(ID_NAT);   break;
      case VAL_STAR: r = id_base(ID_STAR);  break;
      case VAL_TRUE: r = id_base(ID_TRUE);  break;
      case VAL_FALSE:r = id_base(ID_FALSE); break;
      case VAL_ZERO: r = id_base(ID_ZERO);  break;
      case VAL_SUCC: r = id_succ(rb(pt_[a*PORTS+1], pp_[a*PORTS+1])); break;
      case N_VAR:    r = id_var(vidx_[a]); break;
      case TY_PROD:  r = id_prod(rb(pt_[a*PORTS+1], pp_[a*PORTS+1]),
                                 rb(pt_[a*PORTS+2], pp_[a*PORTS+2])); break;
      case TY_ARR:   r = id_arr(rb(pt_[a*PORTS+1], pp_[a*PORTS+1]),
                                rb(pt_[a*PORTS+2], pp_[a*PORTS+2])); break;
      case N_PI: case TY_PI:
                     r = id_pi(rb(pt_[a*PORTS+1], pp_[a*PORTS+1]),
                               rb(pt_[a*PORTS+2], pp_[a*PORTS+2])); break;
      case TY_SIGMA: r = id_sigma(rb(pt_[a*PORTS+1], pp_[a*PORTS+1]),
                                  rb(pt_[a*PORTS+2], pp_[a*PORTS+2])); break;
      case N_LAM:    r = id_lam(rb(pt_[a*PORTS+1], pp_[a*PORTS+1])); break;
      case N_REFL:   r = id_refl(id_base(ID_STAR)); break;     /* refl carrier irrelevant here */
      case N_UA:     r = id_ua(rb(pt_[a*PORTS+1], pp_[a*PORTS+1])); break;
      case N_IF:     r = id_if(rb(pt_[a*PORTS+0], pp_[a*PORTS+0]),    /* neutral if: cond/then/else */
                               rb(pt_[a*PORTS+1], pp_[a*PORTS+1]),
                               rb(pt_[a*PORTS+2], pp_[a*PORTS+2])); break;
      case N_APP:    r = id_app(rb(pt_[a*PORTS+0], pp_[a*PORTS+0]),   /* neutral application */
                                rb(pt_[a*PORTS+2], pp_[a*PORTS+2])); break;
      case N_EQUIV:  r = id_equiv(rb(pt_[a*PORTS+1], pp_[a*PORTS+1]),
                                  rb(pt_[a*PORTS+2], pp_[a*PORTS+2])); break;
      case N_NID:    r = id_idty(rb(pt_[a*PORTS+1], pp_[a*PORTS+1]),    /* neutral Id A x y */
                                 rb(pt_[a*PORTS+2], pp_[a*PORTS+2]),
                                 rb(pt_[a*PORTS+3], pp_[a*PORTS+3])); break;
      case VAL_PAIR: r = id_pair(rb(pt_[a*PORTS+1], pp_[a*PORTS+1]),
                                 rb(pt_[a*PORTS+2], pp_[a*PORTS+2])); break;
      default:       rb_bad = 1; r = id_base(ID_UNIT); break;   /* a stuck agent in a term slot */
    }
    rb_depth--; (void)p; return r;
  }
}

/* Public: reduce an id_node on the interaction net (Id A x y, or transport^P p x).
 * Returns the read-back result, or NULL if the net could not be linearised (a stuck
 * agent was reached -- an unsupported case, refused rather than mis-computed).
 * *steps (optional) = interaction count. */
id_node *idnet_id_nf(const id_node *t, long *steps) {
  int R, root;
  id_node *res;
  if (!t) return (id_node *)0;
  if (t->kind != ID_ID && t->kind != ID_TRANSP) return (id_node *)0;
  n_ = 0; inter_ = 0; over_ = 0; rb_bad = 0; rb_depth = 0; cs_guard = 0;
  R = mk(N_ROOT);
  root = enc(t);
  link_(R, 0, root, up_port(k_[root]));
  reduce();
  if (steps) *steps = inter_;
  if (over_) return (id_node *)0;
  res = rb(pt_[R * PORTS + 0], pp_[R * PORTS + 0]);
  if (rb_bad) { return (id_node *)0; }
  return res;
}
