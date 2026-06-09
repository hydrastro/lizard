/* idnet_test.c -- validate the interaction-net Id engine (idnet.c) against the
 * tree-rewriting SPEC (id_observe.c). For every supported case, computing Id_A(x,y)
 * by graph interaction must give the same normal form as id_nf(); for cases whose
 * former is not yet wired in (Bool/Nat/functions), the net read-back must REFUSE
 * (return NULL) rather than emit a wrong type -- the project's soundness discipline.
 *
 * Build: cc <flags> -Isrc tests/idnet_test.c src/idnet.c src/id_observe.c -o idnet_test
 */
#include "id_observe.h"
#include "idnet.h"
#include <stdio.h>

/* ---- random supported types and canonical values, for differential fuzzing ---- */
static unsigned long rng = 0x2545F4914F6CDD1DUL;
static unsigned rnd(void) { rng = rng * 6364136223846793005UL + 1442695040888963407UL; return (unsigned)(rng >> 33) & 0x7fffffffu; }

static id_node *rand_type(int d) {
  int c = (d <= 0) ? (int)(rnd() % 4) : (int)(rnd() % 5);
  switch (c) {
    case 0: return id_base(ID_UNIT);
    case 1: return id_base(ID_BOOL);
    case 2: return id_base(ID_NAT);
    case 3: return id_base(ID_U);
    default: return id_prod(rand_type(d - 1), rand_type(d - 1));
  }
}
/* an inductive type only (no U), so Id_A reduces to Unit/Empty (possibly a product
 * of those) -- the fragment where the Sigma-Id first-component path is decidable */
static id_node *rand_inductive(int d) {
  int c = (d <= 0) ? (int)(rnd() % 3) : (int)(rnd() % 4);
  switch (c) {
    case 0: return id_base(ID_UNIT);
    case 1: return id_base(ID_BOOL);
    case 2: return id_base(ID_NAT);
    default: return id_prod(rand_inductive(d - 1), rand_inductive(d - 1));
  }
}
static id_node *rand_val(const id_node *ty, int d) {
  switch (ty->kind) {
    case ID_UNIT: return id_base(ID_STAR);
    case ID_BOOL: return id_base((rnd() & 1) ? ID_TRUE : ID_FALSE);
    case ID_NAT:  return id_nat_lit((int)(rnd() % 4));
    case ID_U:    return rand_type(d);                     /* a value of U is a type */
    case ID_PROD: return id_pair(rand_val(ty->a, d), rand_val(ty->b, d));
    default:      return id_base(ID_STAR);
  }
}

/* ---- Nat-recursor step functions (rec z s n).  Inside the step body the de Bruijn
 * convention is: var 1 = the predecessor n, var 0 = the recursive result r. ---- */
static id_node *dbl_step(void) { return id_lam(id_lam(id_succ(id_succ(id_var(0))))); }            /* double: r |-> succ succ r */
static id_node *add_step(void) { return id_lam(id_lam(id_succ(id_var(0)))); }                     /* add m:  r |-> succ r      */
static id_node *evn_step(void) { return id_lam(id_lam(id_if(id_var(0), id_base(ID_FALSE), id_base(ID_TRUE)))); } /* even: r |-> if r false true (if in the step body) */
static id_node *prd_step(void) { return id_lam(id_lam(id_var(1))); }                              /* pred:   _ |-> n (the predecessor) */
/* build a random closed recursor program; the spec result is the differential oracle. */
static id_node *rand_recursor(void) {
  int kind = (int)(rnd() % 4);
  int n    = (int)(rnd() % 8);            /* scrutinee numeral 0..7 */
  switch (kind) {
    case 0: return id_rec(id_nat_lit(0), dbl_step(), id_nat_lit(n));                       /* 2n  : Nat  */
    case 1: return id_rec(id_nat_lit((int)(rnd() % 5)), add_step(), id_nat_lit(n));        /* m+n : Nat  */
    case 2: return id_rec(id_base(ID_TRUE), evn_step(), id_nat_lit(n));                    /* parity : Bool */
    default:return id_rec(id_nat_lit(0), prd_step(), id_nat_lit(n));                       /* n-1 : Nat  */
  }
}

/* ---- random lists, and List-recursor (foldr) step functions (var 1 = head, var 0 = result) ---- */
static id_node *rand_list(const id_node *Elem, int d) {
  int n = (int)(rnd() % 4), i;            /* 0..3 elements */
  id_node *t = id_nil(), *e[4];
  for (i = 0; i < n; i++) e[i] = rand_val(Elem, d);
  for (i = n - 1; i >= 0; i--) t = id_cons(e[i], t);
  return t;
}
static id_node *llen_step(void) { return id_lam(id_lam(id_succ(id_var(0)))); }                              /* length: _ r -> succ r */
static id_node *cnt_step(void)  { return id_lam(id_lam(id_if(id_var(1), id_succ(id_var(0)), id_var(0)))); }  /* count trues: if head then succ r else r */
static id_node *lall_step(void) { return id_lam(id_lam(id_if(id_var(1), id_var(0), id_base(ID_FALSE)))); }   /* all: if head then r else false */
static id_node *lmap_step(void) { return id_lam(id_lam(id_cons(id_succ(id_var(1)), id_var(0)))); }           /* map succ: cons (succ head) r */
static id_node *rand_listfold(void) {
  int kind = (int)(rnd() % 4);
  id_node *E, *xs;
  if (kind == 3) { E = id_base(ID_NAT);  xs = rand_list(E, 2); id_free(E); return id_listrec(id_nil(),         lmap_step(), xs); }
  if (kind == 0) { E = id_base(ID_NAT);  xs = rand_list(E, 2); id_free(E); return id_listrec(id_nat_lit(0),    llen_step(), xs); }
  E = id_base(ID_BOOL); xs = rand_list(E, 2); id_free(E);
  if (kind == 1) return id_listrec(id_nat_lit(0),    cnt_step(),  xs);
  return                id_listrec(id_base(ID_TRUE), lall_step(), xs);
}

/* ---- random coproduct values and case programs ---- */
static id_node *rand_inj(const id_node *A, const id_node *B, int d) {
  return (rnd() & 1u) ? id_inl(rand_val(A, d)) : id_inr(rand_val(B, d));
}
static id_node *rand_case(void) {
  id_node *A = rand_inductive(2), *B = rand_inductive(2);
  id_node *f = id_lam(id_pair(id_var(0), id_var(0)));   /* left branch:  x -> (x, x)  */
  id_node *g = id_lam(id_succ(id_var(0)));              /* right branch: y -> succ y  */
  id_node *scrut = rand_inj(A, B, 2);
  id_free(A); id_free(B);
  return id_case(scrut, f, g);
}

/* ---- random transport along a univalence path in a FUNCTION family (the inverse path) ---- */
static id_node *u_not(void)  { return id_lam(id_if(id_var(0), id_base(ID_FALSE), id_base(ID_TRUE))); }
static id_node *u_id(void)   { return id_lam(id_var(0)); }
/* 3-cycle on Bool+Unit and its (distinct) inverse */
static id_node *u_cyc(void)  { return id_lam(id_case(id_var(0), id_lam(id_if(id_var(0), id_inl(id_base(ID_FALSE)), id_inr(id_base(ID_STAR)))), id_lam(id_inl(id_base(ID_TRUE))))); }
static id_node *u_cyci(void) { return id_lam(id_case(id_var(0), id_lam(id_if(id_var(0), id_inr(id_base(ID_STAR)), id_inl(id_base(ID_TRUE)))), id_lam(id_inl(id_base(ID_FALSE))))); }
static id_node *bu_elem(unsigned k) { return (k % 3u == 0u) ? id_inl(id_base(ID_TRUE)) : (k % 3u == 1u) ? id_inl(id_base(ID_FALSE)) : id_inr(id_base(ID_STAR)); }
static id_node *rand_funtransp(void) {
  /* Two regimes: involutive Bool equivalences, and the genuine non-involutive 3-cycle. */
  if (rnd() & 1u) {
    id_node *fam, *path, *h, *elem, *ref;
    int v = (int)(rnd() % 3u);                  /* 0: X->X   1: Bool->X   2: X->Bool */
    fam  = (v == 0) ? id_lam(id_arr(id_var(0), id_var(0)))
         : (v == 1) ? id_lam(id_arr(id_base(ID_BOOL), id_var(0)))
                    : id_lam(id_arr(id_var(0), id_base(ID_BOOL)));
    path = (rnd() & 1u) ? id_ua(u_not()) : id_ua(u_id());
    h    = (rnd() & 1u) ? u_not() : u_id();
    elem = (rnd() & 1u) ? id_base(ID_TRUE) : id_base(ID_FALSE);
    ref  = (rnd() & 1u) ? id_base(ID_TRUE) : id_base(ID_FALSE);
    return id_idty(id_base(ID_BOOL), id_app(id_transp(fam, path, h), elem), ref);
  } else {
    id_node *fam  = id_lam(id_arr(id_var(0), id_var(0)));
    id_node *path = (rnd() & 1u) ? id_uae(u_cyc(), u_cyci()) : id_uae(u_cyci(), u_cyc());
    int hk = (int)(rnd() % 3u);
    id_node *h = (hk == 0) ? u_id() : (hk == 1) ? u_cyc() : u_cyci();
    id_node *elem = bu_elem(rnd());
    id_node *ref  = bu_elem(rnd());
    return id_idty(id_sum(id_base(ID_BOOL), id_base(ID_UNIT)), id_app(id_transp(fam, path, h), elem), ref);
  }
}

static int fails = 0;

/* a supported case: net result must equal the spec normal form */
static void ok_case(const char *name, id_node *idterm) {
  long steps = 0;
  id_node *spec = id_nf(idterm);
  id_node *net  = idnet_id_nf(idterm, &steps);
  if (!net) { fails++; printf("    FAIL %-26s net REFUSED a supported case\n", name); return; }
  if (!id_eq(net, spec)) {
    fails++; printf("    FAIL %-26s net=", name); id_print(net); printf("  spec="); id_print(spec); printf("\n");
    return;
  }
  printf("    ok   %-26s -> ", name); id_print(net); printf("   (%ld interactions)\n", steps);
}

/* an out-of-scope case: net must refuse (NULL), not return a wrong type */
static void refuse_case(const char *name, id_node *idterm) {
  long steps = 0;
  id_node *net = idnet_id_nf(idterm, &steps);
  if (net) { fails++; printf("    FAIL %-26s net returned a type for an unsupported former (should refuse)\n", name); return; }
  printf("    ok   %-26s correctly refused (former not yet wired in)\n", name);
}

int main(void) {
  printf("idnet -- Id-by-observation AS AN INTERACTION NET, vs the id_observe spec\n\n");

  printf("[1] structural + universe fragment (ID meets the type-former in the graph)\n");
  /* Id Unit * *  =  Unit */
  ok_case("Id Unit * *",
          id_idty(id_base(ID_UNIT), id_base(ID_STAR), id_base(ID_STAR)));
  /* Id U Bool Nat  =  Equiv Bool Nat   (univalence: x,y are the two types) */
  ok_case("Id U Bool Nat",
          id_idty(id_base(ID_U), id_base(ID_BOOL), id_base(ID_NAT)));
  /* Id U (Unit*Bool) Nat  =  Equiv (Unit*Bool) Nat */
  ok_case("Id U (Unit*Bool) Nat",
          id_idty(id_base(ID_U), id_prod(id_base(ID_UNIT), id_base(ID_BOOL)), id_base(ID_NAT)));
  /* Id (Unit*Unit) (*,*) (*,*)  =  Unit * Unit   (componentwise) */
  ok_case("Id (Unit*Unit) .. ..",
          id_idty(id_prod(id_base(ID_UNIT), id_base(ID_UNIT)),
                  id_pair(id_base(ID_STAR), id_base(ID_STAR)),
                  id_pair(id_base(ID_STAR), id_base(ID_STAR))));
  /* Id (Unit*U) (*,Bool) (*,Nat)  =  Unit * (Equiv Bool Nat)   (Id descends into both) */
  ok_case("Id (Unit*U) (*,B) (*,N)",
          id_idty(id_prod(id_base(ID_UNIT), id_base(ID_U)),
                  id_pair(id_base(ID_STAR), id_base(ID_BOOL)),
                  id_pair(id_base(ID_STAR), id_base(ID_NAT))));

  printf("[2] nested products -- recursion on the structure of the type\n");
  /* Id ((Unit*U)*Unit) ((*,Bool),*) ((*,Nat),*) = (Unit * Equiv Bool Nat) * Unit */
  ok_case("Id ((Unit*U)*Unit) ...",
          id_idty(id_prod(id_prod(id_base(ID_UNIT), id_base(ID_U)), id_base(ID_UNIT)),
                  id_pair(id_pair(id_base(ID_STAR), id_base(ID_BOOL)), id_base(ID_STAR)),
                  id_pair(id_pair(id_base(ID_STAR), id_base(ID_NAT)),  id_base(ID_STAR))));
  /* Id (U*U) (Bool,Unit) (Nat,Bool) = (Equiv Bool Nat) * (Equiv Unit Bool) */
  ok_case("Id (U*U) (B,U1) (N,B)",
          id_idty(id_prod(id_base(ID_U), id_base(ID_U)),
                  id_pair(id_base(ID_BOOL), id_base(ID_UNIT)),
                  id_pair(id_base(ID_NAT),  id_base(ID_BOOL))));

  printf("[3] Bool -- observation by case analysis (Id Bool x y)\n");
  ok_case("Id Bool true true",  id_idty(id_base(ID_BOOL), id_base(ID_TRUE),  id_base(ID_TRUE)));
  ok_case("Id Bool true false", id_idty(id_base(ID_BOOL), id_base(ID_TRUE),  id_base(ID_FALSE)));
  ok_case("Id Bool false false",id_idty(id_base(ID_BOOL), id_base(ID_FALSE), id_base(ID_FALSE)));

  printf("[4] Nat -- RECURSIVE observation (Id Nat (succ m)(succ n) -> Id Nat m n)\n");
  ok_case("Id Nat 0 0", id_idty(id_base(ID_NAT), id_nat_lit(0), id_nat_lit(0)));
  ok_case("Id Nat 2 2", id_idty(id_base(ID_NAT), id_nat_lit(2), id_nat_lit(2)));
  ok_case("Id Nat 3 3", id_idty(id_base(ID_NAT), id_nat_lit(3), id_nat_lit(3)));
  ok_case("Id Nat 2 3", id_idty(id_base(ID_NAT), id_nat_lit(2), id_nat_lit(3)));
  ok_case("Id Nat 3 1", id_idty(id_base(ID_NAT), id_nat_lit(3), id_nat_lit(1)));

  printf("[5] products of inductive types -- structural recursion bottoming out at Bool/Nat\n");
  /* Id (Bool*Nat) (true,2) (true,2) = Unit * Unit */
  ok_case("Id (Bool*Nat)(t,2)(t,2)",
          id_idty(id_prod(id_base(ID_BOOL), id_base(ID_NAT)),
                  id_pair(id_base(ID_TRUE), id_nat_lit(2)),
                  id_pair(id_base(ID_TRUE), id_nat_lit(2))));
  /* Id (Unit*Bool) (*,true) (*,false) = Unit * Empty */
  ok_case("Id (Unit*Bool)(*,t)(*,f)",
          id_idty(id_prod(id_base(ID_UNIT), id_base(ID_BOOL)),
                  id_pair(id_base(ID_STAR), id_base(ID_TRUE)),
                  id_pair(id_base(ID_STAR), id_base(ID_FALSE))));
  /* Id (Nat*Nat) (1,2) (1,3) = Unit * Empty */
  ok_case("Id (Nat*Nat)(1,2)(1,3)",
          id_idty(id_prod(id_base(ID_NAT), id_base(ID_NAT)),
                  id_pair(id_nat_lit(1), id_nat_lit(2)),
                  id_pair(id_nat_lit(1), id_nat_lit(3))));

  printf("[6] FUNCTIONS -- funext on the net: Id (A->B) f g = Pi z:A. Id B (f z)(g z)\n");
  /* constant functions: body ignores the argument */
  ok_case("Id (Unit->Bool)(l.t)(l.t)",
          id_idty(id_arr(id_base(ID_UNIT), id_base(ID_BOOL)), id_lam(id_base(ID_TRUE)), id_lam(id_base(ID_TRUE))));
  ok_case("Id (Unit->Bool)(l.t)(l.f)",
          id_idty(id_arr(id_base(ID_UNIT), id_base(ID_BOOL)), id_lam(id_base(ID_TRUE)), id_lam(id_base(ID_FALSE))));
  /* identity functions: body is the bound variable -> the inner Id stays neutral */
  ok_case("Id (Bool->Bool)(l.#0)(l.#0)",
          id_idty(id_arr(id_base(ID_BOOL), id_base(ID_BOOL)), id_lam(id_var(0)), id_lam(id_var(0))));
  ok_case("Id (Nat->Nat)(l.#0)(l.#0)",
          id_idty(id_arr(id_base(ID_NAT), id_base(ID_NAT)), id_lam(id_var(0)), id_lam(id_var(0))));
  /* structural functions: the successor map -- recursion peels succ, then neutral */
  ok_case("Id (Nat->Nat)(l.S#0)(l.S#0)",
          id_idty(id_arr(id_base(ID_NAT), id_base(ID_NAT)), id_lam(id_succ(id_var(0))), id_lam(id_succ(id_var(0)))));
  /* a function into a product type */
  ok_case("Id (Unit->Bool*Nat)(l.(t,0))..",
          id_idty(id_arr(id_base(ID_UNIT), id_prod(id_base(ID_BOOL), id_base(ID_NAT))),
                  id_lam(id_pair(id_base(ID_TRUE), id_nat_lit(0))),
                  id_lam(id_pair(id_base(ID_FALSE), id_nat_lit(0)))));
  /* DEPENDENT function type (Pi): same rule, codomain carried under the binder */
  ok_case("Id (Pi Unit. Bool)(l.t)(l.f)",
          id_idty(id_pi(id_base(ID_UNIT), id_base(ID_BOOL)), id_lam(id_base(ID_TRUE)), id_lam(id_base(ID_FALSE))));

  printf("[6b] BETA IN BODIES -- a function body that applies something now reduces on the net\n");
  /* body applies something (lam x. (lam y.y) x = identity after beta): the net now
   * has application + beta, so this reduces rather than being refused */
  ok_case("Id (Nat->Nat)(l.(l.#0)#0)..",
          id_idty(id_arr(id_base(ID_NAT), id_base(ID_NAT)),
                  id_lam(id_app(id_lam(id_var(0)), id_var(0))),
                  id_lam(id_app(id_lam(id_var(0)), id_var(0)))));

  printf("[7] DIFFERENTIAL FUZZ -- random types & values: net result == spec, every time\n");
  {
    int i, N = 200000, bad = 0, refused = 0;
    for (i = 0; i < N; i++) {
      id_node *ty = rand_type(3);
      id_node *x  = rand_val(ty, 2);
      id_node *y  = rand_val(ty, 2);
      id_node *idt = id_idty(id_copy(ty), x, y);   /* id_idty takes ownership of its args */
      id_node *spec = id_nf(idt);
      id_node *net  = idnet_id_nf(idt, (long *)0);
      if (!net) { refused++; }
      else if (!id_eq(net, spec)) { bad++; if (bad <= 3) { printf("      MISMATCH net="); id_print(net); printf(" spec="); id_print(spec); printf("\n"); } }
      id_free(ty); id_free(idt); id_free(spec); if (net) id_free(net);
    }
    if (bad || refused) { fails++; printf("    FAIL %d mismatches, %d unexpected refusals out of %d\n", bad, refused, N); }
    else printf("    ok   %d random Id_A(x,y) over Unit/Bool/Nat/U/Prod: net matches spec, 0 wrong, 0 refused\n", N);
  }

  printf("[8] FUNEXT FUZZ -- random function types A->B with constant & identity bodies\n");
  {
    int i, N = 100000, bad = 0, refused = 0;
    for (i = 0; i < N; i++) {
      id_node *A = rand_type(2), *B = rand_type(2);
      id_node *f, *g, *idt, *spec, *net;
      if (rnd() & 1) {                         /* constant functions  lam _. v */
        f = id_lam(rand_val(B, 2));
        g = id_lam(rand_val(B, 2));
      } else {                                 /* identity at A->A (B := A)     */
        id_free(B); B = id_copy(A);
        f = id_lam(id_var(0));
        g = id_lam(id_var(0));
      }
      idt  = id_idty(id_arr(id_copy(A), id_copy(B)), f, g);
      spec = id_nf(idt);
      net  = idnet_id_nf(idt, (long *)0);
      if (!net) refused++;
      else if (!id_eq(net, spec)) { bad++; if (bad <= 3) { printf("      MISMATCH net="); id_print(net); printf(" spec="); id_print(spec); printf("\n"); } }
      id_free(A); id_free(B); id_free(idt); id_free(spec); if (net) id_free(net);
    }
    if (bad || refused) { fails++; printf("    FAIL %d mismatches, %d unexpected refusals out of %d\n", bad, refused, N); }
    else printf("    ok   %d random Id (A->B) f g (funext): net matches spec, 0 wrong, 0 refused\n", N);
  }

  printf("[9] TRANSPORT -- transport^P p x on the net, incl. COMPUTATIONAL UNIVALENCE\n");
  {
    id_node *NOT = id_lam(id_if(id_var(0), id_base(ID_FALSE), id_base(ID_TRUE)));
    /* transport along refl is the identity, for any family */
    ok_case("transp^(lX.X) refl true",
            id_transp(id_lam(id_var(0)), id_refl(id_base(ID_STAR)), id_base(ID_TRUE)));
    /* constant family -> identity (even along a non-trivial path) */
    ok_case("transp^(l_.Bool) (ua not) true",
            id_transp(id_lam(id_base(ID_BOOL)), id_ua(id_copy(NOT)), id_base(ID_TRUE)));
    /* THE univalence computation: transport^(lam X.X)(ua not) b = not b, by graph rewriting */
    ok_case("transp^(lX.X) (ua not) true  = false",
            id_transp(id_lam(id_var(0)), id_ua(id_copy(NOT)), id_base(ID_TRUE)));
    ok_case("transp^(lX.X) (ua not) false = true",
            id_transp(id_lam(id_var(0)), id_ua(id_copy(NOT)), id_base(ID_FALSE)));
    /* product of identity families + ua: componentwise univalence -> (not t, not f) */
    ok_case("transp^(lX.X*X) (ua not) (t,f)",
            id_transp(id_lam(id_prod(id_var(0), id_var(0))), id_ua(id_copy(NOT)),
                      id_pair(id_base(ID_TRUE), id_base(ID_FALSE))));
    /* product, constant components -> identity */
    ok_case("transp^(l_.Bool*Nat) refl (t,2)",
            id_transp(id_lam(id_prod(id_base(ID_BOOL), id_base(ID_NAT))),
                      id_refl(id_base(ID_STAR)), id_pair(id_base(ID_TRUE), id_nat_lit(2))));
    id_free(NOT);

    printf("[9b] UNIVALENCE IN A FUNCTION FAMILY -- transport along a genuine equivalence\n");
    printf("     (forward f AND inverse g): transp^(lX.X->X)(uae f g) h = lz. f(h(g z)).\n");
    {
      id_node *XtoX = id_lam(id_arr(id_var(0), id_var(0)));
      id_node *NOT  = id_lam(id_if(id_var(0), id_base(ID_FALSE), id_base(ID_TRUE)));
      id_node *ID1  = id_lam(id_var(0));
      /* involutive Bool: transport id along (ua not) is (extensionally) the identity */
      ok_case("Id Bool (transp^(X->X)(ua not) id @ t) t",
              id_idty(id_base(ID_BOOL), id_app(id_transp(id_copy(XtoX), id_ua(id_copy(NOT)), id_copy(ID1)), id_base(ID_TRUE)), id_base(ID_TRUE)));
      ok_case("Id Bool (transp^(X->X)(ua not) not @ t) f",
              id_idty(id_base(ID_BOOL), id_app(id_transp(id_copy(XtoX), id_ua(id_copy(NOT)), id_copy(NOT)), id_base(ID_TRUE)), id_base(ID_FALSE)));
      /* one-sided families: codomain varies (Bool->X), domain varies (X->Bool) */
      ok_case("transp^(Bool->X)(ua not) id  [direct lambda]",
              id_transp(id_lam(id_arr(id_base(ID_BOOL), id_var(0))), id_ua(id_copy(NOT)), id_copy(ID1)));
      ok_case("transp^(X->Bool)(ua not) id  [direct lambda]",
              id_transp(id_lam(id_arr(id_var(0), id_base(ID_BOOL))), id_ua(id_copy(NOT)), id_copy(ID1)));
      /* GENUINE non-involutive 3-cycle on Bool+Unit (cyc != cyc_inv): transport id must be
       * the identity on every element -- this distinguishes correct inverse routing (a wrong
       * forward-in-contravariant-position would send inl t to inr * = cyc^2). */
      {
        id_node *BU = id_sum(id_base(ID_BOOL), id_base(ID_UNIT));
        /* cyc:  inl t -> inl f -> inr * -> inl t */
        id_node *CYC  = id_lam(id_case(id_var(0), id_lam(id_if(id_var(0), id_inl(id_base(ID_FALSE)), id_inr(id_base(ID_STAR)))), id_lam(id_inl(id_base(ID_TRUE)))));
        id_node *CYCi = id_lam(id_case(id_var(0), id_lam(id_if(id_var(0), id_inr(id_base(ID_STAR)), id_inl(id_base(ID_TRUE)))), id_lam(id_inl(id_base(ID_FALSE)))));
        ok_case("Id BU (transp^(X->X)(uae cyc cyc_inv) id @ inl t) inl t",
                id_idty(id_copy(BU), id_app(id_transp(id_copy(XtoX), id_uae(id_copy(CYC), id_copy(CYCi)), id_copy(ID1)), id_inl(id_base(ID_TRUE))), id_inl(id_base(ID_TRUE))));
        ok_case("Id BU (transp^(X->X)(uae cyc cyc_inv) id @ inr *) inr *",
                id_idty(id_copy(BU), id_app(id_transp(id_copy(XtoX), id_uae(id_copy(CYC), id_copy(CYCi)), id_copy(ID1)), id_inr(id_base(ID_STAR))), id_inr(id_base(ID_STAR))));
        /* transport cyc along its own univalence acts as cyc: inl t -> inl f */
        ok_case("Id BU (transp^(X->X)(uae cyc cyc_inv) cyc @ inl t) inl f",
                id_idty(id_copy(BU), id_app(id_transp(id_copy(XtoX), id_uae(id_copy(CYC), id_copy(CYCi)), id_copy(CYC)), id_inl(id_base(ID_TRUE))), id_inl(id_base(ID_FALSE))));
        id_free(BU); id_free(CYC); id_free(CYCi);
      }
      id_free(XtoX); id_free(NOT); id_free(ID1);
    }

    printf("[9c] SOUND REFUSAL -- a function family along a NON-univalence path -> refuse\n");
    /* the family is a function type but the path is not a ua equivalence (here a free var of
     * the equality type): the net has no inverse map to route, so it refuses (never guesses). */
    refuse_case("transp^(lX.X->X) (var path) id",
                id_transp(id_lam(id_arr(id_var(0), id_var(0))), id_var(0),
                          id_lam(id_var(0))));
  }

  printf("[10] TRANSPORT FUZZ -- random constant/product families + refl, and identity-family univalence\n");
  {
    int i, N = 60000, bad = 0, refused = 0;
    for (i = 0; i < N; i++) {
      id_node *P, *p, *x, *term, *spec, *net;
      unsigned pick = rnd() % 3u;
      if (pick == 0) {                          /* constant family, random path-irrelevant value */
        id_node *B = rand_type(2);
        P = id_lam(id_copy(B)); p = id_refl(id_base(ID_STAR)); x = rand_val(B, 2);
        id_free(B);
      } else if (pick == 1) {                   /* product family with constant components, refl */
        id_node *L = rand_type(1), *R = rand_type(1);
        P = id_lam(id_prod(id_copy(L), id_copy(R)));
        p = id_refl(id_base(ID_STAR));
        x = id_pair(rand_val(L, 1), rand_val(R, 1));
        id_free(L); id_free(R);
      } else {                                  /* identity family + ua not : univalence on a bool */
        id_node *b = (rnd() & 1u) ? id_base(ID_TRUE) : id_base(ID_FALSE);
        P = id_lam(id_var(0));
        p = id_ua(id_lam(id_if(id_var(0), id_base(ID_FALSE), id_base(ID_TRUE))));
        x = b;
      }
      term = id_transp(P, p, x);
      spec = id_nf(id_copy(term));
      net  = idnet_id_nf(term, (long *)0);
      if (!net) refused++;
      else if (!id_eq(net, spec)) { bad++; if (bad <= 3) { printf("      MISMATCH net="); id_print(net); printf(" spec="); id_print(spec); printf("\n"); } }
      id_free(term); id_free(spec); if (net) id_free(net);
    }
    if (bad || refused) { fails++; printf("    FAIL %d mismatches, %d unexpected refusals out of %d\n", bad, refused, N); }
    else printf("    ok   %d random transports (constant/product/univalence): net matches spec, 0 wrong, 0 refused\n", N);
  }

  printf("[11] DEPENDENT SIGMA -- Id over Sigma x:A. B x on the net (inductive first component)\n");
  {
    id_node *Bdep = id_if(id_var(0), id_base(ID_BOOL), id_base(ID_NAT));  /* B = lam x. if x then Bool else Nat */
    ok_case("Id (Sx:Nat.Nat) (2,5)(2,5)",
            id_idty(id_sigma(id_base(ID_NAT), id_base(ID_NAT)),
                    id_pair(id_nat_lit(2), id_nat_lit(5)), id_pair(id_nat_lit(2), id_nat_lit(5))));
    ok_case("Id (Sx:Nat.Nat) (2,5)(2,7)",
            id_idty(id_sigma(id_base(ID_NAT), id_base(ID_NAT)),
                    id_pair(id_nat_lit(2), id_nat_lit(5)), id_pair(id_nat_lit(2), id_nat_lit(7))));
    ok_case("Id (Sx:Nat.Nat) (2,5)(3,5)",   /* first components differ -> Empty */
            id_idty(id_sigma(id_base(ID_NAT), id_base(ID_NAT)),
                    id_pair(id_nat_lit(2), id_nat_lit(5)), id_pair(id_nat_lit(3), id_nat_lit(5))));
    /* GENUINELY DEPENDENT: second component's type depends on the (equal) first value */
    ok_case("Id (Sx:Bool. if x B N) (true,t)(true,f)",
            id_idty(id_sigma(id_base(ID_BOOL), id_copy(Bdep)),
                    id_pair(id_base(ID_TRUE), id_base(ID_TRUE)), id_pair(id_base(ID_TRUE), id_base(ID_FALSE))));
    ok_case("Id (Sx:Bool. if x B N) (false,2)(false,2)",
            id_idty(id_sigma(id_base(ID_BOOL), id_copy(Bdep)),
                    id_pair(id_base(ID_FALSE), id_nat_lit(2)), id_pair(id_base(ID_FALSE), id_nat_lit(2))));
    ok_case("Id (Sx:Bool. if x B N) (true,t)(false,2)",   /* first differ -> Empty */
            id_idty(id_sigma(id_base(ID_BOOL), id_copy(Bdep)),
                    id_pair(id_base(ID_TRUE), id_base(ID_TRUE)), id_pair(id_base(ID_FALSE), id_nat_lit(2))));
    /* PRODUCT first component: contractible iff every part is */
    ok_case("Id (S(Bool*Bool).Bool) ((t,f),t)((t,f),t)",
            id_idty(id_sigma(id_prod(id_base(ID_BOOL), id_base(ID_BOOL)), id_base(ID_BOOL)),
                    id_pair(id_pair(id_base(ID_TRUE), id_base(ID_FALSE)), id_base(ID_TRUE)),
                    id_pair(id_pair(id_base(ID_TRUE), id_base(ID_FALSE)), id_base(ID_TRUE))));
    ok_case("Id (S(Nat*Bool).Nat) ((2,t),9)((2,f),9)",
            id_idty(id_sigma(id_prod(id_base(ID_NAT), id_base(ID_BOOL)), id_base(ID_NAT)),
                    id_pair(id_pair(id_nat_lit(2), id_base(ID_TRUE)), id_nat_lit(9)),
                    id_pair(id_pair(id_nat_lit(2), id_base(ID_FALSE)), id_nat_lit(9))));
    id_free(Bdep);

    printf("[11b] SOUND REFUSAL -- Sigma with a non-inductive (U) first component -> neutral path -> refuse\n");
    refuse_case("Id (Sx:U. Bool) (Bool,t)(Nat,t)",
                id_idty(id_sigma(id_base(ID_U), id_base(ID_BOOL)),
                        id_pair(id_base(ID_BOOL), id_base(ID_TRUE)), id_pair(id_base(ID_NAT), id_base(ID_TRUE))));
  }

  printf("[12] SIGMA FUZZ -- random inductive A (incl. products), constant family B, biased equal/unequal first\n");
  {
    int i, N = 60000, bad = 0, refused = 0;
    for (i = 0; i < N; i++) {
      id_node *A = rand_inductive(2), *B = rand_type(2);   /* B closed -> constant family */
      id_node *a1 = rand_val(A, 2), *a2 = (rnd() & 1u) ? id_copy(a1) : rand_val(A, 2);
      id_node *b1 = rand_val(B, 2), *b2 = rand_val(B, 2);
      id_node *term = id_idty(id_sigma(id_copy(A), id_copy(B)), id_pair(a1, b1), id_pair(a2, b2));
      id_node *spec = id_nf(id_copy(term));
      id_node *net  = idnet_id_nf(term, (long *)0);
      if (!net) refused++;
      else if (!id_eq(net, spec)) { bad++; if (bad <= 3) { printf("      MISMATCH net="); id_print(net); printf(" spec="); id_print(spec); printf("\n"); } }
      id_free(A); id_free(B); id_free(term); id_free(spec); if (net) id_free(net);
    }
    if (bad || refused) { fails++; printf("    FAIL %d mismatches, %d unexpected refusals out of %d\n", bad, refused, N); }
    else printf("    ok   %d random Id (Sigma A. B) (a,b)(a',b'): net matches spec, 0 wrong, 0 refused\n", N);
  }

  printf("[13] NAT RECURSOR -- rec z s n on the net (CBV step-forcer); value steps AND if-in-step-body\n");
  ok_case("double 0",  id_rec(id_nat_lit(0), dbl_step(), id_nat_lit(0)));
  ok_case("double 3",  id_rec(id_nat_lit(0), dbl_step(), id_nat_lit(3)));
  ok_case("double 5",  id_rec(id_nat_lit(0), dbl_step(), id_nat_lit(5)));
  ok_case("add 2 3",   id_rec(id_nat_lit(2), add_step(), id_nat_lit(3)));
  ok_case("pred 0",    id_rec(id_nat_lit(0), prd_step(), id_nat_lit(0)));
  ok_case("pred 4",    id_rec(id_nat_lit(0), prd_step(), id_nat_lit(4)));
  ok_case("even 4",    id_rec(id_base(ID_TRUE), evn_step(), id_nat_lit(4)));   /* if in the step body */
  ok_case("even 3",    id_rec(id_base(ID_TRUE), evn_step(), id_nat_lit(3)));
  ok_case("double(double 2)", id_rec(id_nat_lit(0), dbl_step(), id_rec(id_nat_lit(0), dbl_step(), id_nat_lit(2))));
  /* recursor result feeding an Id observation */
  ok_case("Id Nat (double 3) 6",
          id_idty(id_base(ID_NAT), id_rec(id_nat_lit(0), dbl_step(), id_nat_lit(3)), id_nat_lit(6)));
  ok_case("Id Nat (add 2 3) 6",
          id_idty(id_base(ID_NAT), id_rec(id_nat_lit(2), add_step(), id_nat_lit(3)), id_nat_lit(6)));
  ok_case("Id Bool (even 4) true",
          id_idty(id_base(ID_BOOL), id_rec(id_base(ID_TRUE), evn_step(), id_nat_lit(4)), id_base(ID_TRUE)));
  ok_case("Id Bool (even 3) false",
          id_idty(id_base(ID_BOOL), id_rec(id_base(ID_TRUE), evn_step(), id_nat_lit(3)), id_base(ID_FALSE)));

  printf("[13b] SOUND REFUSAL -- recursor on a neutral (free-variable) scrutinee -> net waits, read-back refuses\n");
  refuse_case("rec _ dbl (var)", id_rec(id_nat_lit(0), dbl_step(), id_var(0)));
  refuse_case("rec _ evn (var)", id_rec(id_base(ID_TRUE), evn_step(), id_var(0)));

  printf("[14] RECURSOR FUZZ -- random closed double/add/even/pred programs: net result == spec, every time\n");
  {
    int i, N = 60000, bad = 0, refused = 0;
    for (i = 0; i < N; i++) {
      id_node *term = rand_recursor();
      id_node *spec = id_nf(id_copy(term));
      id_node *net  = idnet_id_nf(term, (long *)0);
      if (!net) refused++;
      else if (!id_eq(net, spec)) { bad++; if (bad <= 3) { printf("      MISMATCH net="); id_print(net); printf(" spec="); id_print(spec); printf("\n"); } }
      id_free(term); id_free(spec); if (net) id_free(net);
    }
    if (bad || refused) { fails++; printf("    FAIL %d mismatches, %d unexpected refusals out of %d\n", bad, refused, N); }
    else printf("    ok   %d random recursor programs (double/add/even/pred): net matches spec, 0 wrong, 0 refused\n", N);
  }

  printf("[15] LISTS -- Id over List E by observation on the net (spine + element-wise, nested products)\n");
  {
    id_node *bt = id_base(ID_BOOL), *nt = id_base(ID_NAT), *ut = id_base(ID_U);
    ok_case("Id (List Bool) [] []",      id_idty(id_list(id_base(ID_BOOL)), id_nil(), id_nil()));
    ok_case("Id (List Bool) [t] [t]",    id_idty(id_list(id_base(ID_BOOL)), id_cons(id_base(ID_TRUE), id_nil()), id_cons(id_base(ID_TRUE), id_nil())));
    ok_case("Id (List Bool) [t] [f]",    id_idty(id_list(id_base(ID_BOOL)), id_cons(id_base(ID_TRUE), id_nil()), id_cons(id_base(ID_FALSE), id_nil())));
    ok_case("Id (List Nat) [1,2] [1,2]", id_idty(id_list(id_base(ID_NAT)), id_cons(id_nat_lit(1), id_cons(id_nat_lit(2), id_nil())), id_cons(id_nat_lit(1), id_cons(id_nat_lit(2), id_nil()))));
    ok_case("Id (List Nat) [1,2] [1,3]", id_idty(id_list(id_base(ID_NAT)), id_cons(id_nat_lit(1), id_cons(id_nat_lit(2), id_nil())), id_cons(id_nat_lit(1), id_cons(id_nat_lit(3), id_nil()))));
    ok_case("Id (List Nat) [1] [1,2]",   id_idty(id_list(id_base(ID_NAT)), id_cons(id_nat_lit(1), id_nil()), id_cons(id_nat_lit(1), id_cons(id_nat_lit(2), id_nil()))));
    ok_case("Id (List U) [Bool] [Nat]",  id_idty(id_list(id_base(ID_U)), id_cons(id_base(ID_BOOL), id_nil()), id_cons(id_base(ID_NAT), id_nil())));
    id_free(bt); id_free(nt); id_free(ut);
  }

  printf("[15b] SOUND REFUSAL -- a neutral (free-variable) list spine -> net waits, read-back refuses\n");
  refuse_case("Id (List Nat) (var) []", id_idty(id_list(id_base(ID_NAT)), id_var(0), id_nil()));

  printf("[16] LIST-Id FUZZ -- random element type & random lists (biased equal/unequal): net == spec\n");
  {
    int i, N = 60000, bad = 0, refused = 0;
    for (i = 0; i < N; i++) {
      id_node *E  = rand_inductive(2);
      id_node *xs = rand_list(E, 2);
      id_node *ys = (rnd() & 1u) ? id_copy(xs) : rand_list(E, 2);
      id_node *term = id_idty(id_list(id_copy(E)), xs, ys);
      id_node *spec = id_nf(id_copy(term));
      id_node *net  = idnet_id_nf(term, (long *)0);
      if (!net) refused++;
      else if (!id_eq(net, spec)) { bad++; if (bad <= 3) { printf("      MISMATCH net="); id_print(net); printf(" spec="); id_print(spec); printf("\n"); } }
      id_free(E); id_free(term); id_free(spec); if (net) id_free(net);
    }
    if (bad || refused) { fails++; printf("    FAIL %d mismatches, %d unexpected refusals out of %d\n", bad, refused, N); }
    else printf("    ok   %d random Id (List E) xs ys: net matches spec, 0 wrong, 0 refused\n", N);
  }

  printf("[17] LIST RECURSOR (foldr) -- length / count / all / map on the net (CBV step-forcer)\n");
  {
    id_node *L = id_cons(id_nat_lit(1), id_cons(id_nat_lit(2), id_nil()));
    ok_case("length [1,2]",  id_listrec(id_nat_lit(0), llen_step(), id_copy(L)));
    ok_case("length []",     id_listrec(id_nat_lit(0), llen_step(), id_nil()));
    ok_case("count [t,f,t]", id_listrec(id_nat_lit(0), cnt_step(),  id_cons(id_base(ID_TRUE), id_cons(id_base(ID_FALSE), id_cons(id_base(ID_TRUE), id_nil())))));
    ok_case("all [t,t,t]",   id_listrec(id_base(ID_TRUE), lall_step(), id_cons(id_base(ID_TRUE), id_cons(id_base(ID_TRUE), id_cons(id_base(ID_TRUE), id_nil())))));
    ok_case("all [t,f,t]",   id_listrec(id_base(ID_TRUE), lall_step(), id_cons(id_base(ID_TRUE), id_cons(id_base(ID_FALSE), id_cons(id_base(ID_TRUE), id_nil())))));
    ok_case("map succ [1,2]", id_listrec(id_nil(), lmap_step(), id_copy(L)));
    ok_case("Id (List Nat) (map succ [1,2]) [2,3]",
            id_idty(id_list(id_base(ID_NAT)), id_listrec(id_nil(), lmap_step(), id_copy(L)),
                    id_cons(id_nat_lit(2), id_cons(id_nat_lit(3), id_nil()))));
    ok_case("Id Nat (length [1,2]) 2",
            id_idty(id_base(ID_NAT), id_listrec(id_nat_lit(0), llen_step(), id_copy(L)), id_nat_lit(2)));
    id_free(L);
  }

  printf("[18] FOLDR FUZZ -- random length/count/all/map programs over random lists: net == spec\n");
  {
    int i, N = 60000, bad = 0, refused = 0;
    for (i = 0; i < N; i++) {
      id_node *term = rand_listfold();
      id_node *spec = id_nf(id_copy(term));
      id_node *net  = idnet_id_nf(term, (long *)0);
      if (!net) refused++;
      else if (!id_eq(net, spec)) { bad++; if (bad <= 3) { printf("      MISMATCH net="); id_print(net); printf(" spec="); id_print(spec); printf("\n"); } }
      id_free(term); id_free(spec); if (net) id_free(net);
    }
    if (bad || refused) { fails++; printf("    FAIL %d mismatches, %d unexpected refusals out of %d\n", bad, refused, N); }
    else printf("    ok   %d random foldr programs (length/count/all/map): net matches spec, 0 wrong, 0 refused\n", N);
  }

  printf("[19] SUM TYPES -- Id over A+B by observation (the coproduct, dual to the product)\n");
  {
    id_node *BN  = id_sum(id_base(ID_BOOL), id_base(ID_NAT));
    ok_case("Id (Bool+Nat)(inl t)(inl t)", id_idty(id_copy(BN), id_inl(id_base(ID_TRUE)),  id_inl(id_base(ID_TRUE))));
    ok_case("Id (Bool+Nat)(inl t)(inl f)", id_idty(id_copy(BN), id_inl(id_base(ID_TRUE)),  id_inl(id_base(ID_FALSE))));
    ok_case("Id (Bool+Nat)(inl t)(inr 2)", id_idty(id_copy(BN), id_inl(id_base(ID_TRUE)),  id_inr(id_nat_lit(2))));
    ok_case("Id (Bool+Nat)(inr 2)(inr 2)", id_idty(id_copy(BN), id_inr(id_nat_lit(2)),     id_inr(id_nat_lit(2))));
    ok_case("Id (Bool+Nat)(inr 2)(inr 3)", id_idty(id_copy(BN), id_inr(id_nat_lit(2)),     id_inr(id_nat_lit(3))));
    ok_case("Id (U+Nat)(inl Bool)(inl Nat)", id_idty(id_sum(id_base(ID_U), id_base(ID_NAT)), id_inl(id_base(ID_BOOL)), id_inl(id_base(ID_NAT))));
    ok_case("Id ((Bool*Nat)+Unit)(inl(t,2))(inl(t,3))",
            id_idty(id_sum(id_prod(id_base(ID_BOOL), id_base(ID_NAT)), id_base(ID_UNIT)),
                    id_inl(id_pair(id_base(ID_TRUE), id_nat_lit(2))), id_inl(id_pair(id_base(ID_TRUE), id_nat_lit(3)))));
    id_free(BN);
  }

  printf("[19b] SOUND REFUSAL -- a neutral (free-variable) sum -> net waits, read-back refuses\n");
  refuse_case("Id (Bool+Nat) (var) (inl t)", id_idty(id_sum(id_base(ID_BOOL), id_base(ID_NAT)), id_var(0), id_inl(id_base(ID_TRUE))));

  printf("[20] SUM-Id FUZZ -- random component types & random injections (biased equal/unequal): net == spec\n");
  {
    int i, N = 60000, bad = 0, refused = 0;
    for (i = 0; i < N; i++) {
      id_node *A = rand_inductive(2), *B = rand_inductive(2);
      id_node *x = rand_inj(A, B, 2);
      id_node *y = (rnd() & 1u) ? id_copy(x) : rand_inj(A, B, 2);
      id_node *term = id_idty(id_sum(id_copy(A), id_copy(B)), x, y);
      id_node *spec = id_nf(id_copy(term));
      id_node *net  = idnet_id_nf(term, (long *)0);
      if (!net) refused++;
      else if (!id_eq(net, spec)) { bad++; if (bad <= 3) { printf("      MISMATCH net="); id_print(net); printf(" spec="); id_print(spec); printf("\n"); } }
      id_free(A); id_free(B); id_free(term); id_free(spec); if (net) id_free(net);
    }
    if (bad || refused) { fails++; printf("    FAIL %d mismatches, %d unexpected refusals out of %d\n", bad, refused, N); }
    else printf("    ok   %d random Id (A+B) x y: net matches spec, 0 wrong, 0 refused\n", N);
  }

  printf("[21] CASE -- the sum eliminator on the net: case (inl x) f g = f x ; case (inr y) f g = g y\n");
  {
    id_node *fF = id_lam(id_if(id_var(0), id_nat_lit(1), id_nat_lit(0)));  /* Bool branch: if x then 1 else 0 */
    id_node *gG = id_lam(id_succ(id_var(0)));                             /* Nat branch:  succ y            */
    ok_case("case (inl t) fF gG",  id_case(id_inl(id_base(ID_TRUE)),  id_copy(fF), id_copy(gG)));
    ok_case("case (inl f) fF gG",  id_case(id_inl(id_base(ID_FALSE)), id_copy(fF), id_copy(gG)));
    ok_case("case (inr 5) fF gG",  id_case(id_inr(id_nat_lit(5)),     id_copy(fF), id_copy(gG)));
    ok_case("Id Nat (case (inl t) fF gG) 1", id_idty(id_base(ID_NAT), id_case(id_inl(id_base(ID_TRUE)), id_copy(fF), id_copy(gG)), id_nat_lit(1)));
    ok_case("Id Nat (case (inr 5) fF gG) 6", id_idty(id_base(ID_NAT), id_case(id_inr(id_nat_lit(5)),    id_copy(fF), id_copy(gG)), id_nat_lit(6)));
    /* case that PRODUCES a sum, then observed: swap the tags */
    ok_case("Id (Nat+Bool)(case(inl t) inr inl)(inr t)",
            id_idty(id_sum(id_base(ID_NAT), id_base(ID_BOOL)),
                    id_case(id_inl(id_base(ID_TRUE)), id_lam(id_inr(id_var(0))), id_lam(id_inl(id_var(0)))),
                    id_inr(id_base(ID_TRUE))));
    id_free(fF); id_free(gG);
  }

  printf("[22] CASE FUZZ -- random injections through branches (x->(x,x)) / (y->succ y): net == spec\n");
  {
    int i, N = 60000, bad = 0, refused = 0;
    for (i = 0; i < N; i++) {
      id_node *term = rand_case();
      id_node *spec = id_nf(id_copy(term));
      id_node *net  = idnet_id_nf(term, (long *)0);
      if (!net) refused++;
      else if (!id_eq(net, spec)) { bad++; if (bad <= 3) { printf("      MISMATCH net="); id_print(net); printf(" spec="); id_print(spec); printf("\n"); } }
      id_free(term); id_free(spec); if (net) id_free(net);
    }
    if (bad || refused) { fails++; printf("    FAIL %d mismatches, %d unexpected refusals out of %d\n", bad, refused, N); }
    else printf("    ok   %d random case programs (inl/inr through branches): net matches spec, 0 wrong, 0 refused\n", N);
  }

  printf("[23] FUNCTION-FAMILY TRANSPORT FUZZ -- univalence in lX.D->C across variance,\n");
  printf("     involutive Bool equivalences AND the genuine non-involutive 3-cycle: net == spec\n");
  {
    int i, N = 60000, bad = 0, refused = 0;
    for (i = 0; i < N; i++) {
      id_node *term = rand_funtransp();
      id_node *spec = id_nf(id_copy(term));
      id_node *net  = idnet_id_nf(term, (long *)0);
      if (!net) refused++;
      else if (!id_eq(net, spec)) { bad++; if (bad <= 3) { printf("      MISMATCH net="); id_print(net); printf(" spec="); id_print(spec); printf("\n"); } }
      id_free(term); id_free(spec); if (net) id_free(net);
    }
    if (bad || refused) { fails++; printf("    FAIL %d mismatches, %d unexpected refusals out of %d\n", bad, refused, N); }
    else printf("    ok   %d random function-family transports (the inverse path): net matches spec, 0 wrong, 0 refused\n", N);
  }

  printf(fails ? "\n%d checks FAILED\n" : "\nidnet: Id AND transport by interaction -- inductive + universe + functions (funext) + transport incl. computational univalence + dependent Sigma + the Nat recursor + Lists (Id + foldr) + coproducts A+B (Id + case) + univalence in FUNCTION families along a genuine equivalence (forward + inverse, the inverse path; validated by a non-involutive 3-cycle), matching the spec over 780k fuzz; unsupported cases refused, never mis-computed\n", fails);
  return fails ? 1 : 0;
}
