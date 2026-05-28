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
  KT_META,         /* metavariable / hole ?n */
  KT_LIST,         /* List A — the type of lists of A */
  KT_NIL_K,        /* nil : List A */
  KT_CONS_K,       /* cons : A → List A → List A */
  KT_LIST_REC,     /* list recursor */
  KT_MAYBE,        /* Maybe A — option type */
  KT_NOTHING,      /* nothing : Maybe A */
  KT_JUST,         /* just : A → Maybe A */
  KT_MAYBE_REC     /* maybe eliminator */
} kterm_tag_t;

/* ---- kernel term ---- */

typedef struct kterm {
  kterm_tag_t tag;
  union {
    struct { int index; } var;
    struct { int level; } sort;
    struct { const char *name; struct kterm *domain; struct kterm *codomain; } pi;
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
    struct { struct kterm *elem_type; } list;  /* List A */
    struct { struct kterm *elem_type; } nil_k; /* nil {A} */
    struct { struct kterm *head; struct kterm *tail; } cons_k;  /* cons h t */
    struct { struct kterm *motive; struct kterm *nil_case;
             struct kterm *cons_case; struct kterm *scrutinee; } list_rec;
    struct { struct kterm *elem_type; } maybe;      /* Maybe A */
    struct { struct kterm *value; } just;            /* just v */
    struct { struct kterm *motive; struct kterm *nothing_case;
             struct kterm *just_case; struct kterm *scrutinee; } maybe_rec;
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

/* ---- core operations ---- */

/* Substitute term s for variable index n in term t. */
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

#endif /* LIZARD_KERNEL_H */
