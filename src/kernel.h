/* src/kernel.h — Trusted kernel for type checking (Track K).
 *
 * The kernel is the ~500 lines of code that MUST be correct for
 * soundness. It uses its own term representation, separate from
 * the AST used by the parser/expander. The kernel:
 *
 *   1. Checks that a term has a given type
 *   2. Reduces terms to weak head normal form
 *   3. Compares terms for definitional equality
 *
 * Everything else (inference, tactics, elaboration) produces terms
 * that the kernel re-checks.
 */
#ifndef LIZARD_KERNEL_H
#define LIZARD_KERNEL_H

#include "lizard_internal.h"
#include <stdlib.h>

/* ---- kernel term tags ---- */

typedef enum {
  KT_VAR,          /* bound variable (de Bruijn index) */
  KT_SORT,         /* universe Sort(level) */
  KT_PI,           /* Π (x : A) → B */
  KT_LAM,          /* λ (x : A) . body */
  KT_APP,          /* f a */
  KT_SIGMA,        /* Σ (x : A) × B */
  KT_PAIR,         /* (a, b) : Σ-type */
  KT_PROJ1,        /* π₁ p */
  KT_PROJ2,        /* π₂ p */
  KT_NAT,          /* the type Nat */
  KT_ZERO,         /* 0 : Nat */
  KT_SUCC,         /* succ n : Nat */
  KT_NAT_REC,      /* natrec C z s n */
  KT_ID,           /* Id A a b — identity type */
  KT_REFL,         /* refl a : Id A a a */
  KT_J,            /* J-eliminator */
  KT_LET,          /* let x = e in body */
  KT_ANNOT,        /* (e : A) — type annotation */
  KT_BOOL,         /* the type Bool */
  KT_TRUE,         /* true : Bool */
  KT_FALSE,        /* false : Bool */
  KT_BOOL_REC,     /* if-then-else / Bool eliminator */
  KT_UNIT,         /* the type Unit */
  KT_STAR,         /* * : Unit */
  KT_PARTIAL,      /* Partial (cofib r b) A — the type of PARTIAL ELEMENTS of A defined on the face.
                    * On a HELD face (r == b endpoints) a partial element is a total element, so it
                    * reduces to A; on an EMPTY face (r,b distinct endpoints) the only partial element
                    * is the trivial one, so it reduces to Unit; otherwise NEUTRAL.  This is the first
                    * stone of the partial-elements / systems layer (the CCHM Partial former). */
  KT_PSYS,         /* psys (cofib r b) A a — a single-face SYSTEM [phi -> a], the INTRODUCTION for
                    * Partial : (Partial (cofib r b) A).  On a HELD face it is the value a; on an EMPTY
                    * face it is * : Unit (the trivial partial element); otherwise NEUTRAL.  The element
                    * a need only be well-typed ON the face, so its typing uses the face-restricted
                    * context.  This is the systems-introduction stone of the partial-elements layer. */
  KT_META,         /* metavariable / hole ?n */
  KT_LIST,         /* List A — the type of lists of A */
  KT_NIL_K,        /* nil : List A */
  KT_CONS_K,       /* cons : A → List A → List A */
  KT_LIST_REC,     /* list recursor */
  KT_MAYBE,        /* Maybe A — option type */
  KT_NOTHING,      /* nothing : Maybe A */
  KT_JUST,         /* just : A → Maybe A */
  KT_MAYBE_REC,    /* maybe eliminator */
  KT_EMPTY,        /* Empty/Void — the type with no constructors */
  KT_ABSURD,       /* absurd : Empty → A — ex falso */
  KT_SUM_K,        /* Sum A B — coproduct/either */
  KT_INL,          /* inl : A → Sum A B */
  KT_INR,          /* inr : B → Sum A B */
  KT_SUM_REC,      /* case/sum eliminator */
  KT_INTERVAL,     /* the interval I (cubical) */
  KT_I0,           /* interval endpoint 0 */
  KT_I1,           /* interval endpoint 1 */
  KT_INEG,         /* ~ r — interval negation (de Morgan): ~i0=i1, ~i1=i0, ~~r=r */
  KT_IMEET,        /* r /\ s — interval meet (min): i0/\x=i0, i1/\x=x, x/\x=x (and symmetric) */
  KT_IJOIN,        /* r \/ s — interval join (max): i1\/x=i1, i0\/x=x, x\/x=x (and symmetric) */
  KT_PATH,         /* Path A a b — path type */
  KT_PLAM,         /* <i> t — path abstraction (binds an interval variable) */
  KT_PAPP,         /* p @ r — path application */
  KT_CONST,        /* opaque named global constant (axiom) */
  KT_EQUIV,        /* Equiv A B — type of equivalences between A and B (univalence) */
  KT_UA,           /* ua e — univalence: turns an equivalence into a path/identity in the universe */
  KT_ID_EQUIV,     /* id-equiv A — the identity equivalence : Equiv A A */
  KT_HCOMP,        /* hcomp A u0 — homogeneous composition, EMPTY face system only.  Carries just the
                    * type A and base u0 (no face-system field), so a non-empty system is structurally
                    * unrepresentable: hcomp A u0 : A and reduces to u0 (empty composition is the base).
                    * The full face-system hcomp (compatibility checking + Kan filling) is NOT provided. */
  KT_COFIB,        /* cofib r b — a single cofibration (r = b), the smallest face.  r,b : I. */
  KT_COFIB_OR,     /* cofib-or c1 c2 — the DISJUNCTION of two cofibrations (c1 held OR c2 held).  This
                    * is the cofibration-lattice join that multi-face systems are built on: a system
                    * over (cofib-or c1 c2) is given by a value on c1 and a value on c2 agreeing on the
                    * overlap c1 /\ c2.  whnf: HELD if either operand is held; EMPTY if both are empty;
                    * otherwise neutral.  c1, c2 : the cofibration sort (each a (cofib ..) or nested or). */
  KT_COFIB_FORALL, /* cofib-forall <i> phi — the FORALL-FACE quantifier ∀i.phi(i): the cofibration that
                    * holds when phi holds for EVERY i in the interval.  This is CCHM's "forallFace",
                    * the locus delta = ∀i.phi where a Glue line's gluing structure is stable along the
                    * whole composition direction — the prerequisite for comp over a Glue line whose
                    * gluing cofibration VARIES in i.  body is a plam <i> phi(i).  whnf decides
                    * CONSERVATIVELY (sound): HELD only when phi is certainly held for all i (phi
                    * independent of i and held, or reflexive (cofib r r)); EMPTY when phi certainly
                    * fails somewhere (phi decided-empty at an endpoint i0 or i1); otherwise neutral. */
  KT_HCOMP1,       /* hcomp1 A (cofib r b) u u0 — SINGLE-face homogeneous composition.  On the face
                    * (r=b) reduces to the partial element u; off the face (r=other endpoint) reduces to
                    * the base u0; else neutral.  Typing enforces the compatibility u=u0 when r=b. */
  KT_HCOMP2,       /* hcomp2 A (cofib r1 b1) u1 (cofib r2 b2) u2 u0 — TWO-face homogeneous composition.
                    * Reduces to u1 on face1, else u2 on face2, else u0 when both faces empty, else
                    * neutral.  Typing enforces the OVERLAP compatibility lattice: u1=u0 on face1,
                    * u2=u0 on face2, and u1=u2 where both faces hold (the overlap).  3+-face hcomp is
                    * NOT provided (it adds more pairwise overlaps but no new kind of check). */
  KT_GLUE,         /* Glue A (cofib r b) T e — single-face Glue type.  A,T : Sort n, e : Equiv T A.
                    * Boundary rule: on the face (r=b), (Glue A cof T e) = T; off the face / neutral
                    * stays Glue.  This is the type former univalence is built from. */
  KT_EQUIV_FUN,    /* equiv-fun e — the forward map of an equivalence : (Pi (_:T) A) when e:Equiv T A.
                    * Computes for the identity equivalence (equiv-fun (id-equiv A) = lam x.x); a
                    * general e stays neutral (the kernel's Equiv is otherwise opaque). */
  KT_EQUIV_INV,    /* equiv-inv e — the inverse map of an equivalence : (Pi (_:A) T) when e:Equiv T A.
                    * Computes for the identity equivalence (equiv-inv (id-equiv A) = lam x.x); a
                    * general e stays neutral.  The symmetric partner of equiv-fun. */
  KT_MK_EQUIV,     /* mk-equiv T A f g eta eps — a GENERAL equivalence from its four parts:
                    * f:T->A, g:A->T, eta:(x:T)Id T (g(f x)) x, eps:(y:A)Id A (f(g y)) y.  Typing
                    * demands all four (a real quasi-equivalence), so : Equiv T A.  equiv-fun/inv/
                    * eta/eps project from it (beta rules). */
  KT_EQUIV_ETA,    /* equiv-eta e — the retraction coherence : (Pi (x:T) Id T (g(f x)) x).  refl for
                    * id-equiv; projects from mk-equiv; else neutral. */
  KT_EQUIV_EPS,    /* equiv-eps e — the section coherence : (Pi (y:A) Id A (f(g y)) y).  refl for
                    * id-equiv; projects from mk-equiv; else neutral. */
  KT_GLUE_ELEM,    /* glue A (cofib r b) T e u a — Glue INTRODUCTION : Glue A (cofib r b) T e, where
                    * on the face u : T is the T-component, a : A is the base component, and the glue
                    * COHERENCE holds: on the face, (equiv-fun e) u == a.  Off the face it is a; on the
                    * face it is u (so unglue recovers (equiv-fun e) u = a).  This is the constructor the
                    * Glue-transport result is built from. */
  KT_UNGLUE,       /* unglue A (cofib r b) T e g — eliminator : A when g : Glue A (cofib r b) T e.
                    * On the face (r=b): unglue = (equiv-fun e) g (lands in A via the equivalence).
                    * Off the face: g is already in A, unglue g = g.  On-face computation requires
                    * equiv-fun to reduce; for a general (non-identity) e it stays neutral. */
  KT_COMP,         /* comp line (cofib r b) u u0 — heterogeneous composition along a type line A(i).
                    * GENERALISES transp (empty face) and hcomp1 (constant line).  Reduces to
                    * transp line u0 on an empty face; to hcomp1 A (cofib r b) u u0 on a constant
                    * line; else NEUTRAL (the genuine heterogeneous correction is the named frontier
                    * — it is never guessed). */
  KT_COMP2,        /* comp2 line (cofib r1 b1) u1 (cofib r2 b2) u2 u0 — two-face composition along a
                    * type line A(i) with i-VARYING line partials u1 = <i>u1(i), u2 = <i>u2(i).  On a
                    * DECIDED held face the result is that face's partial line at i1 (held-face Kan
                    * behavior); on an EMPTY system it is transp line u0; otherwise NEUTRAL (abstract A,
                    * undecided faces).  This is the disjunction-system Kan brick that Path-type-line
                    * transport is built from. */
  KT_GTRANSP,      /* gtransp A (cofib r b) T e g0 — Glue transport (the univalence computation),
                    * SOUND fragment only: for the identity equivalence (e = id-equiv) the Glue is A
                    * and transport is the identity, so gtransp = g0 (regularity).  A general e needs
                    * the equivalence's inverse + is-equiv coherences to build the correction hcomp,
                    * which the kernel does not carry, so gtransp stays NEUTRAL there (never guesses). */
  KT_TRANSP        /* transp line base — transport: line is a plam <i> A(i), base : A(i0); result : A(i1).
                    * TYPING is in the trusted kernel.  COMPUTATION is the single sound rule only: when the
                    * type line is constant (A(i0) ≡ A(i1)), transp reduces to base (transport along a constant
                    * path is the identity).  A non-constant line stays NEUTRAL — full transport across a varying
                    * line needs Glue/comp and is deliberately NOT computed here, only typed. */
} kterm_tag_t;

/* ---- kernel term ---- */

typedef struct kterm {
  kterm_tag_t tag;
  union {
    struct { int index; } var;
    struct { int level; } sort;
    struct { const char *name; struct kterm *domain; struct kterm *codomain; int implicit; } pi;
    struct { const char *name; struct kterm *domain; struct kterm *body; } lam;
    struct { struct kterm *fun; struct kterm *arg; } app;
    struct { const char *name; struct kterm *fst_type; struct kterm *snd_type; } sigma;
    struct { struct kterm *fst; struct kterm *snd; } pair;
    struct { struct kterm *target; } proj;
    /* nat: no data */
    /* zero: no data */
    struct { struct kterm *pred; } succ;
    struct { struct kterm *motive; struct kterm *zero_case;
             struct kterm *succ_case; struct kterm *scrutinee; } nat_rec;
    struct { struct kterm *type; struct kterm *a; struct kterm *b; } id;
    struct { struct kterm *value; } refl;
    struct { struct kterm *motive; struct kterm *base_case;
             struct kterm *type; struct kterm *a; struct kterm *b;
             struct kterm *proof; } j;
    struct { const char *name; struct kterm *value; struct kterm *body; } let;
    struct { struct kterm *term; struct kterm *type; } annot;
    struct { struct kterm *motive; struct kterm *true_case;
             struct kterm *false_case; struct kterm *scrutinee; } bool_rec;
    struct { int id; } meta;  /* metavariable ?id */
    struct { const char *name; } constant;  /* opaque global (axiom) */
    struct { struct kterm *elem_type; } list;  /* List A */
    struct { struct kterm *elem_type; } nil_k; /* nil {A} */
    struct { struct kterm *head; struct kterm *tail; } cons_k;  /* cons h t */
    struct { struct kterm *motive; struct kterm *nil_case;
             struct kterm *cons_case; struct kterm *scrutinee; } list_rec;
    struct { struct kterm *elem_type; } maybe;      /* Maybe A */
    struct { struct kterm *value; } just;            /* just v */
    struct { struct kterm *motive; struct kterm *nothing_case;
             struct kterm *just_case; struct kterm *scrutinee; } maybe_rec;
    struct { struct kterm *target_type; } absurd;  /* absurd {A} : Empty → A */
    struct { struct kterm *left_type; struct kterm *right_type; } sum_k;
    struct { struct kterm *value; struct kterm *right_type; } inl;
    struct { struct kterm *value; struct kterm *left_type; } inr;
    struct { struct kterm *motive; struct kterm *left_case;
             struct kterm *right_case; struct kterm *scrutinee; } sum_rec;
    struct { struct kterm *type; struct kterm *a; struct kterm *b; } path; /* Path A a b */
    struct { const char *name; struct kterm *body; } plam;  /* <i> body */
    struct { struct kterm *path; struct kterm *arg; } papp; /* p @ r */
    struct { struct kterm *a; struct kterm *b; } equiv; /* Equiv A B */
    struct { struct kterm *equiv; } ua; /* ua e */
    struct { struct kterm *type; } id_equiv; /* id-equiv A */
    struct { struct kterm *type; struct kterm *base; } hcomp; /* hcomp A u0 (empty system) */
    struct { struct kterm *var; struct kterm *endpoint; } cofib; /* cofib r b */
    struct { struct kterm *cof1; struct kterm *cof2; } cofib_or; /* cofib-or c1 c2 */
    struct { struct kterm *body; } cofib_forall; /* cofib-forall <i> phi(i) */
    struct { struct kterm *type; struct kterm *cof; struct kterm *partial; struct kterm *base; } hcomp1; /* hcomp1 A cof u u0 */
    struct { struct kterm *line; struct kterm *cof; struct kterm *partial; struct kterm *base; } comp; /* comp line cof u u0 */
    struct { struct kterm *line; struct kterm *cof1; struct kterm *partial1; struct kterm *cof2; struct kterm *partial2; struct kterm *base; } comp2; /* comp2 line cof1 u1 cof2 u2 u0 */
    struct { struct kterm *type; struct kterm *cof1; struct kterm *partial1; struct kterm *cof2; struct kterm *partial2; struct kterm *base; } hcomp2; /* hcomp2 A cof1 u1 cof2 u2 u0 */
    struct { struct kterm *base_type; struct kterm *cof; struct kterm *glue_type; struct kterm *equiv; } glue; /* Glue A cof T e */
    struct { struct kterm *cof; struct kterm *type; } partial; /* Partial (cofib r b) A */
    struct { struct kterm *cof; struct kterm *type; struct kterm *elem; } psys; /* psys (cofib r b) A a */
    struct { struct kterm *equiv; } equiv_fun; /* equiv-fun e */
    struct { struct kterm *equiv; } equiv_inv; /* equiv-inv e */
    struct { struct kterm *glue_type; struct kterm *base_type; struct kterm *fwd; struct kterm *inv; struct kterm *eta; struct kterm *eps; } mk_equiv; /* mk-equiv T A f g eta eps */
    struct { struct kterm *equiv; } equiv_eta; /* equiv-eta e */
    struct { struct kterm *equiv; } equiv_eps; /* equiv-eps e */
    struct { struct kterm *base_type; struct kterm *cof; struct kterm *glue_type; struct kterm *equiv; struct kterm *arg; } unglue; /* unglue A cof T e g */
    struct { struct kterm *base_type; struct kterm *cof; struct kterm *glue_type; struct kterm *equiv; struct kterm *member; struct kterm *base; } glue_elem; /* glue A cof T e u a */
    struct { struct kterm *base_type; struct kterm *cof; struct kterm *glue_type; struct kterm *equiv; struct kterm *base; } gtransp; /* gtransp A cof T e g0 */
    struct { struct kterm *line; struct kterm *base; } transp; /* transp line base */
    struct { struct kterm *arg; } ineg; /* ~ arg — interval negation */
    struct { struct kterm *left; struct kterm *right; } imeet; /* r /\ s — interval meet */
    struct { struct kterm *left; struct kterm *right; } ijoin; /* r \/ s — interval join */
  } data;
} kterm_t;

/* ---- metavariable context ---- */

typedef struct meta_entry {
  int id;
  kterm_t *type;        /* declared type of the hole */
  kterm_t *solution;    /* NULL until solved */
  struct meta_entry *next;
} meta_entry_t;

typedef struct {
  meta_entry_t *entries;
  int next_id;
} meta_ctx_t;

/* ---- kernel context ---- */

typedef struct kctx_entry {
  const char *name;
  kterm_t *type;
  kterm_t *value;       /* NULL if assumption, non-NULL if definition */
  struct kctx_entry *next;
} kctx_entry_t;

typedef struct {
  kctx_entry_t *entries;
  int depth;             /* for de Bruijn indexing */
  lizard_heap_t *heap;   /* allocation target */
  void *defs;            /* kdef_ctx_t* of global defs (NULL if none) */
} kctx_t;

/* ---- kernel result ---- */

typedef enum {
  KERNEL_OK,
  KERNEL_TYPE_ERROR,
  KERNEL_UNIVERSE_ERROR,
  KERNEL_UNBOUND_VAR,
  KERNEL_NOT_A_FUNCTION,
  KERNEL_NOT_A_TYPE,
  KERNEL_REDUCTION_STUCK
} kernel_result_t;

/* ---- constructors ---- */

kterm_t *kt_var(lizard_heap_t *heap, int index);
kterm_t *kt_sort(lizard_heap_t *heap, int level);
kterm_t *kt_pi(lizard_heap_t *heap, const char *name, kterm_t *domain, kterm_t *codomain);
kterm_t *kt_lam(lizard_heap_t *heap, const char *name, kterm_t *domain, kterm_t *body);
kterm_t *kt_app(lizard_heap_t *heap, kterm_t *fun, kterm_t *arg);
kterm_t *kt_nat(lizard_heap_t *heap);
kterm_t *kt_zero(lizard_heap_t *heap);
kterm_t *kt_succ(lizard_heap_t *heap, kterm_t *pred);
kterm_t *kt_id(lizard_heap_t *heap, kterm_t *type, kterm_t *a, kterm_t *b);
kterm_t *kt_refl(lizard_heap_t *heap, kterm_t *value);
kterm_t *kt_interval(lizard_heap_t *heap);
kterm_t *kt_i0(lizard_heap_t *heap);
kterm_t *kt_i1(lizard_heap_t *heap);
kterm_t *kt_path(lizard_heap_t *heap, kterm_t *type, kterm_t *a, kterm_t *b);
kterm_t *kt_plam(lizard_heap_t *heap, const char *name, kterm_t *body);
kterm_t *kt_papp(lizard_heap_t *heap, kterm_t *path, kterm_t *arg);
kterm_t *kt_equiv(lizard_heap_t *heap, kterm_t *a, kterm_t *b);
kterm_t *kt_ua(lizard_heap_t *heap, kterm_t *equiv);
kterm_t *kt_id_equiv(lizard_heap_t *heap, kterm_t *type);
kterm_t *kt_hcomp(lizard_heap_t *heap, kterm_t *type, kterm_t *base);
kterm_t *kt_cofib(lizard_heap_t *heap, kterm_t *var, kterm_t *endpoint);
kterm_t *kt_cofib_or(lizard_heap_t *heap, kterm_t *cof1, kterm_t *cof2);
kterm_t *kt_cofib_forall(lizard_heap_t *heap, kterm_t *body);
kterm_t *kt_hcomp1(lizard_heap_t *heap, kterm_t *type, kterm_t *cof, kterm_t *partial, kterm_t *base);
kterm_t *kt_comp(lizard_heap_t *heap, kterm_t *line, kterm_t *cof, kterm_t *partial, kterm_t *base);
kterm_t *kt_comp2(lizard_heap_t *heap, kterm_t *line, kterm_t *cof1, kterm_t *partial1,
                  kterm_t *cof2, kterm_t *partial2, kterm_t *base);
kterm_t *kt_hcomp2(lizard_heap_t *heap, kterm_t *type, kterm_t *cof1, kterm_t *partial1, kterm_t *cof2, kterm_t *partial2, kterm_t *base);
kterm_t *kt_glue(lizard_heap_t *heap, kterm_t *base_type, kterm_t *cof, kterm_t *glue_type, kterm_t *equiv);
kterm_t *kt_partial(lizard_heap_t *heap, kterm_t *cof, kterm_t *type);
kterm_t *kt_psys(lizard_heap_t *heap, kterm_t *cof, kterm_t *type, kterm_t *elem);
kterm_t *kt_equiv_fun(lizard_heap_t *heap, kterm_t *equiv);
kterm_t *kt_equiv_inv(lizard_heap_t *heap, kterm_t *equiv);
kterm_t *kt_mk_equiv(lizard_heap_t *heap, kterm_t *glue_type, kterm_t *base_type, kterm_t *fwd, kterm_t *inv, kterm_t *eta, kterm_t *eps);
kterm_t *kt_equiv_eta(lizard_heap_t *heap, kterm_t *equiv);
kterm_t *kt_equiv_eps(lizard_heap_t *heap, kterm_t *equiv);
kterm_t *kt_unglue(lizard_heap_t *heap, kterm_t *base_type, kterm_t *cof, kterm_t *glue_type, kterm_t *equiv, kterm_t *arg);
kterm_t *kt_glue_elem(lizard_heap_t *heap, kterm_t *base_type, kterm_t *cof, kterm_t *glue_type, kterm_t *equiv, kterm_t *member, kterm_t *base);
kterm_t *kt_gtransp(lizard_heap_t *heap, kterm_t *base_type, kterm_t *cof, kterm_t *glue_type, kterm_t *equiv, kterm_t *base);
kterm_t *kt_transp(lizard_heap_t *heap, kterm_t *line, kterm_t *base);
kterm_t *kt_ineg(lizard_heap_t *heap, kterm_t *arg);
kterm_t *kt_imeet(lizard_heap_t *heap, kterm_t *left, kterm_t *right);
kterm_t *kt_ijoin(lizard_heap_t *heap, kterm_t *left, kterm_t *right);

/* ---- core operations ---- */

/* Substitute term s for variable index n in term t. */
kterm_t *kt_shift(lizard_heap_t *heap, kterm_t *t, int cutoff, int delta);
kterm_t *kt_subst(lizard_heap_t *heap, kterm_t *t, int n, kterm_t *s);

/* Reduce to weak head normal form. */
kterm_t *kt_whnf(lizard_heap_t *heap, kctx_t *ctx, kterm_t *t);

/* Check definitional equality. */
int kt_equal(lizard_heap_t *heap, kctx_t *ctx, kterm_t *a, kterm_t *b);

/* Infer the type of a term in context. Returns NULL on type error. */
kterm_t *kt_infer(lizard_heap_t *heap, kctx_t *ctx, kterm_t *t);

/* Check that term has the expected type. */
kernel_result_t kt_check(lizard_heap_t *heap, kctx_t *ctx,
                          kterm_t *term, kterm_t *expected_type);

/* ---- context operations ---- */

kctx_t *kctx_create(lizard_heap_t *heap);
kctx_t *kctx_extend(lizard_heap_t *heap, kctx_t *ctx,
                     const char *name, kterm_t *type, kterm_t *value);
kctx_entry_t *kctx_lookup(kctx_t *ctx, int index);

/* ---- printing ---- */

void kt_fprint(FILE *fp, kterm_t *t);

/* ---- metavariables ---- */

meta_ctx_t *meta_ctx_create(lizard_heap_t *heap);
kterm_t *meta_fresh(lizard_heap_t *heap, meta_ctx_t *mctx, kterm_t *type);
meta_entry_t *meta_lookup(meta_ctx_t *mctx, int id);
int meta_solve(meta_ctx_t *mctx, int id, kterm_t *solution);
kterm_t *meta_zonk(lizard_heap_t *heap, meta_ctx_t *mctx, kterm_t *t);
int meta_unsolved_count(meta_ctx_t *mctx);
void meta_ctx_fprint(FILE *fp, meta_ctx_t *mctx);

/* ---- unification ---- */

/* Try to make two terms definitionally equal by solving metavariables.
 * Returns 1 on success, 0 on failure. On success, some metas in mctx
 * may be solved. */
int kt_unify(lizard_heap_t *heap, kctx_t *ctx, meta_ctx_t *mctx,
             kterm_t *a, kterm_t *b);

/* ---- global definitions ---- */

/* A named kernel definition: name, type, and value. */
typedef struct kdef {
  const char *name;
  kterm_t *type;
  kterm_t *value;
  struct kdef *next;
} kdef_t;

typedef struct {
  kdef_t *defs;
  void *inds;            /* kind_ctx_t* of user inductive declarations */
} kdef_ctx_t;

/* ---- user-defined inductive types ---- */
/* A constructor of an inductive: its constant name, argument count, and which
 * arguments are recursive (a direct occurrence of the inductive being
 * defined).  Recursive arguments receive an induction hypothesis. */
typedef struct {
  const char *name;
  int n_args;
  int *recursive;        /* recursive[j] = 1 if arg j is a recursive occurrence */
} kind_ctor_t;

/* An inductive declaration: type-former name, its recursor name, number of
 * parameters, and its constructors.  Used by kt_whnf to perform iota-reduction
 * of a recursor applied to a constructor. */
typedef struct kind {
  const char *name;
  const char *rec_name;
  int n_params;
  int n_ctors;
  kind_ctor_t *ctors;    /* array of n_ctors */
  struct kind *next;
} kind_t;

typedef struct {
  kind_t *inds;
} kind_ctx_t;

/* Inputs to a `data` declaration (pieces already converted to kernel terms by
 * the surface layer; kind_declare does positivity, eliminator synthesis, and
 * registration). */
typedef struct {
  const char *name;
  const char *rec_name;
  int n_params;
  const char **param_names;
  kterm_t **param_types;        /* param_types[k] in context [p_0..p_{k-1}] */
  int sort_level;
  int n_ctors;
  const char **ctor_names;
  int *ctor_nargs;
  kterm_t ***ctor_argtypes;     /* ctor_argtypes[i][j] in context [params] */
  int **ctor_recflags;          /* filled by kind_declare (positivity) */
} kind_decl_t;

/* Positivity-check, synthesize (type former, constructors, dependent
 * eliminator), and register an inductive.  Returns 1 on success, 0 on a
 * positivity violation or malformed synthesis (with a message to stderr). */
kterm_t *kind_former_type(lizard_heap_t *heap, kind_decl_t *decl);
int kind_declare(lizard_heap_t *heap, kdef_ctx_t *dctx, kind_decl_t *decl);
kind_t *kind_find_rec(kind_ctx_t *kc, const char *name);
kind_t *kind_find_ctor(kind_ctx_t *kc, const char *name, int *index_out);

kdef_ctx_t *kdef_ctx_create(lizard_heap_t *heap);
void kdef_add(lizard_heap_t *heap, kdef_ctx_t *dctx,
              const char *name, kterm_t *type, kterm_t *value);
kdef_t *kdef_lookup(kdef_ctx_t *dctx, const char *name);

/* The current runtime's kernel definition context (created on demand).
 * Used by the surface converter to resolve free names to definitions. */
kdef_ctx_t *lizard_kernel_defs(lizard_heap_t *heap);

#endif /* LIZARD_KERNEL_H */
