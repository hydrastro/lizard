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

    printf("[9b] SOUND REFUSAL -- transport over a function family (non-refl) needs the inverse path -> refuse\n");
    /* transport^(lam X. X->X) (ua not) (id) : function-family transport not implemented -> refuse */
    refuse_case("transp^(lX.X->X) (ua not) id",
                id_transp(id_lam(id_arr(id_var(0), id_var(0))),
                          id_ua(id_lam(id_if(id_var(0), id_base(ID_FALSE), id_base(ID_TRUE)))),
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

  printf(fails ? "\n%d checks FAILED\n" : "\nidnet: Id AND transport reduce by interaction -- inductive + universe + functions (funext) + transport incl. computational univalence (transport along ua f = f), matching the spec over 360k fuzz; unsupported cases refused, never mis-computed\n", fails);
  return fails ? 1 : 0;
}
