#ifndef LIZARD_H
#define LIZARD_H

#include <ds.h>
#include <gmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

/* lizard uses its own typedefs anchored on the struct *tags*
   `linked_list` and `list_node` rather than ds's typedef names.
   Both the old and the new ds expose these structs under the same
   tags (ds typedef names: `list_t` in older releases, `ds_list_t`
   in newer ones), so this compiles regardless of which ds version
   is installed and interoperates with either set of ds function
   signatures. */
struct linked_list;
struct list_node;
typedef struct linked_list lz_list_t;
typedef struct list_node lz_list_node_t;

/*
___                     .-*''*-.
 '.* *'.        .|     *       _*
  _)*_*_\__   \.`',/  * EVAL .'  *
 / _______ \  = ,. =  *.___.'    *
 \_)|@_@|(_/   // \   '.   APPLY.'
   ))\_/((    //        *._  _.*
  (((\V/)))  //            ||
 //-\\^//-\\--            /__\
*/

typedef enum {
  AST_STRING,
  AST_NUMBER,
  AST_SYMBOL,
  AST_BOOL,
  AST_PAIR,
  AST_NIL,
  AST_QUOTE,
  AST_QUASIQUOTE,
  AST_UNQUOTE,
  AST_UNQUOTE_SPLICING,
  AST_ASSIGNMENT,
  AST_DEFINITION,
  AST_IF,
  AST_LAMBDA,
  AST_BEGIN,
  AST_COND,
  AST_APPLICATION,
  AST_PRIMITIVE,
  AST_MACRO,
  AST_PROMISE,
  AST_CONTINUATION,
  AST_CALLCC,
  AST_ERROR,
  AST_VECTOR,
  AST_HASH,
  AST_SYNTAX_RULES,
  /* ----- Type-theory notation. NONE of these forms are checked. -----
   * They are surface syntax / opaque values, suitable for designing
   * the look of a foundational system but not for verifying anything
   * about it. */
  AST_TT_PI,           /* (Pi (x A) B) — dependent function type */
  AST_TT_SIGMA,        /* (Sigma (x A) B) — dependent pair type */
  AST_TT_APP,          /* (@ f a) — explicit application form */
  AST_TT_SUM,          /* (Sum A B) — coproduct type */
  AST_TT_UNIVERSE,     /* (U n) — universe at integer level */
  AST_TT_COUNIVERSE,   /* (Uco n) — couniverse at integer level */
  AST_TT_ID,           /* (Id A a b) — identity type */
  AST_TT_REFL,         /* (refl a) — reflexivity witness */
  AST_TT_INDUCTIVE,    /* (Inductive name ctors...) — inductive decl */
  AST_TT_COINDUCTIVE,  /* (Coinductive name dtors...) — coinductive decl */
  AST_TT_ANNOT,        /* (: term type) — type annotation, stored only */
  /* ----- Context layer (still no checking) -----
   * Stratified along the couniverse hierarchy from your proposal:
   *   Uco -2 : variables / binding sites
   *   Uco -1 : contexts (lists of variables)
   *   Uco  0 : substitutions / context morphisms
   *   Uco  n : higher contexts (not represented separately yet)
   * A `judgment` packages a context, a term, and a claimed type — no
   * verification happens; it's a data structure that LOOKS like an
   * inference-rule conclusion. */
  AST_TT_VARIABLE,
  AST_TT_CONTEXT,
  AST_TT_SUBSTITUTION,
  AST_TT_JUDGMENT,
  /* ----- Identity manipulation & equivalence (notation only) -----
   * For writing the surface of HOTT-style identity reasoning.
   * `equivalence` packages two maps claimed to be inverse.
   * `transport` is the action of an identity proof on a term.
   * `Id-sym` and `Id-trans` are the basic identity manipulations
   * (symmetry, transitivity). No verification happens; these are
   * tags with structure. */
  AST_TT_EQUIV,
  AST_TT_TRANSPORT,
  AST_TT_ID_SYM,
  AST_TT_ID_TRANS,
  /* TT-level lambda: (Lambda 'x body). Introduces a function-like
   * term that the engine can pi-beta-reduce when applied with @.
   * Distinct from Lisp's (lambda ...) — this lives in the TT layer
   * as an opaque carrier with a binder name, and the reduce engine
   * knows that (@ (Lambda 'x b) a) reduces to b[a/x]. */
  AST_TT_LAMBDA,
  /* (ap f p) — congruence of identity along a function.
   * The HOTT-flavored rule: ap(f, refl_a) reduces to refl_{f a}.
   * In a real type theory, ap also has a typing rule:
   *     If f : A -> B and p : Id A a a', then ap f p : Id B (f a) (f a').
   * We don't check the typing here; ap is an opaque carrier with a
   * specific computation rule. */
  AST_TT_AP,
  /* ----- More HOTT-fragment constructors -----
   * Introduction and elimination forms for Sigma, Sum, Unit; plus J. */
  AST_TT_PAIR,         /* (pair a b) — Sigma intro */
  AST_TT_FST,          /* (fst p) — first projection */
  AST_TT_SND,          /* (snd p) — second projection */
  AST_TT_INL,          /* (inl a) — sum intro left */
  AST_TT_INR,          /* (inr b) — sum intro right */
  AST_TT_CASE,         /* (case s f g) — sum elim */
  AST_TT_UNIT,         /* Unit — singleton type former */
  AST_TT_TT,           /* tt — the unique inhabitant of Unit */
  AST_TT_BOT,          /* Bot — empty type, target of contradictory Id */
  AST_TT_J,            /* (J P d p) — path induction. P motive, d refl-case, p path */
  AST_TT_XPORT,        /* (xport motive path value) — transport with explicit motive */
  /* ----- Universe-level expressions -----
   * (U n)         — concrete universe at integer level n
   * (U-var 'i)    — universe-level variable (for polymorphism)
   * (U-suc u)     — successor: one universe level above u
   * (U-max u v)   — supremum of two universe expressions
   * These compose: (U-max (U-suc (U-var 'i)) (U 2)) is "max(i+1, 2)".
   * The reducer simplifies on concrete arguments: (U-suc (U 3)) -> (U 4).
   * For polymorphism, `(U-var 'i)` stands for a universe variable bound
   * elsewhere (in a Lambda or judgment). For cumulativity, the typing
   * predicate `universe-leq?` decides u1 ≤ u2 when comparable. */
  AST_TT_U_VAR,
  AST_TT_U_SUC,
  AST_TT_U_MAX
} lizard_ast_node_type_t;

typedef struct lizard_ast_node lizard_ast_node_t;
typedef struct lizard_env lizard_env_t;
typedef struct lizard_heap lizard_heap_t;

typedef lizard_ast_node_t *(*lizard_primitive_func_t)(lz_list_t *args,
                                                      lizard_env_t *env,
                                                      lizard_heap_t *heap);
typedef lizard_ast_node_t *(*lizard_callcc_func_t)(
    lz_list_t *args, lizard_env_t *env, lizard_heap_t *heap,
    lizard_ast_node_t *(*current_cont)(lizard_ast_node_t *, lizard_env_t *,
                                       lizard_heap_t *));

struct lizard_ast_node {
  lizard_ast_node_type_t type;
  union {
    bool boolean;
    const char *string;
    mpz_t number;
    const char *variable;
    struct lizard_ast_node *quoted;
    struct {
      struct lizard_ast_node *variable;
      struct lizard_ast_node *value;
    } assignment;
    struct {
      struct lizard_ast_node *variable;
      struct lizard_ast_node *value;
    } definition;
    struct {
      struct lizard_ast_node *pred;
      struct lizard_ast_node *cons;
      struct lizard_ast_node *alt;
    } if_clause;
    struct {
      lz_list_t *parameters;
      lizard_env_t *closure_env;
    } lambda;
    lz_list_t *begin_expressions;
    lz_list_t *cond_clauses;
    lz_list_t *application_arguments;
    struct {
      lizard_ast_node_t *car;
      lizard_ast_node_t *cdr;
    } pair;
    lizard_primitive_func_t primitive;
    struct {
      lizard_ast_node_t *expr;
      lizard_env_t *env;
      lizard_ast_node_t *value;
      bool forced;
    } promise;
    struct {
      lizard_ast_node_t *variable;
      lizard_ast_node_t *transformer;
    } macro_def;
    struct {
      lizard_ast_node_t *(*captured_cont)(lizard_ast_node_t *result,
                                          lizard_env_t *env,
                                          lizard_heap_t *heap);
    } continuation;
    struct {
      int code;
      lz_list_t *data;
    } error;
    struct {
      size_t size;
      lizard_ast_node_t **elements;
    } vector;
    struct {
      /* Open-addressed hash table with linear probing.
       * `keys[i] == NULL` marks an empty slot. */
      size_t size;       /* number of live entries */
      size_t cap;        /* allocated capacity (always a power of two) */
      lizard_ast_node_t **keys;
      lizard_ast_node_t **values;
    } hash;
    struct {
      /* (syntax-rules (literals...) (pattern1 template1) ...) */
      lz_list_t *literals;   /* list of AST_SYMBOL nodes */
      lz_list_t *clauses;    /* list of (pattern, template) pairs;
                                each clause is itself a 2-element list */
    } syntax_rules;
    /* ----- Type-theory carriers (no semantic checking) ----- */
    struct {
      lizard_ast_node_t *binder;     /* AST_SYMBOL, or NULL for ->/non-dep */
      lizard_ast_node_t *domain;
      lizard_ast_node_t *codomain;
    } tt_pi;
    struct {
      lizard_ast_node_t *binder;
      lizard_ast_node_t *domain;
      lizard_ast_node_t *codomain;
    } tt_sigma;
    struct {
      lizard_ast_node_t *fun;
      lizard_ast_node_t *arg;
    } tt_app;
    struct {
      lizard_ast_node_t *left;
      lizard_ast_node_t *right;
    } tt_sum;
    struct {
      long level;
    } tt_universe;
    struct {
      long level;
    } tt_couniverse;
    struct {
      lizard_ast_node_t *domain;
      lizard_ast_node_t *a;
      lizard_ast_node_t *b;
    } tt_id;
    struct {
      lizard_ast_node_t *value;
    } tt_refl;
    struct {
      lizard_ast_node_t *name;
      lz_list_t *constructors;
    } tt_inductive;
    struct {
      lizard_ast_node_t *name;
      lz_list_t *destructors;
    } tt_coinductive;
    struct {
      lizard_ast_node_t *term;
      lizard_ast_node_t *type;
    } tt_annot;
    /* Context layer. */
    struct {
      lizard_ast_node_t *name;       /* AST_SYMBOL */
      lizard_ast_node_t *type;       /* any type expression */
    } tt_variable;
    struct {
      lz_list_t *bindings;           /* list of tt_variable nodes,
                                        order: leftmost = outermost */
    } tt_context;
    struct {
      lizard_ast_node_t *source;     /* source context */
      lizard_ast_node_t *target;     /* target context */
      lz_list_t *mappings;           /* list of (name . term) pairs */
    } tt_substitution;
    struct {
      lizard_ast_node_t *context;
      lizard_ast_node_t *term;
      lizard_ast_node_t *type;
    } tt_judgment;
    struct {
      lizard_ast_node_t *left;        /* type A */
      lizard_ast_node_t *right;       /* type B */
      lizard_ast_node_t *fwd;         /* claimed forward map A -> B */
      lizard_ast_node_t *bwd;         /* claimed inverse B -> A */
    } tt_equiv;
    struct {
      lizard_ast_node_t *path;        /* an Id proof */
      lizard_ast_node_t *value;       /* the thing being transported */
    } tt_transport;
    struct {
      lizard_ast_node_t *path;        /* (sym p) reverses an Id proof */
    } tt_id_sym;
    struct {
      lizard_ast_node_t *p;           /* (trans p q) composes two Id proofs */
      lizard_ast_node_t *q;
    } tt_id_trans;
    struct {
      lizard_ast_node_t *binder;
      lizard_ast_node_t *body;
    } tt_lambda;
    struct {
      lizard_ast_node_t *fn;          /* (ap f p) — congruence */
      lizard_ast_node_t *path;
    } tt_ap;
    struct {
      lizard_ast_node_t *fst;         /* (pair a b) — Sigma intro */
      lizard_ast_node_t *snd;
    } tt_pair;
    struct {
      lizard_ast_node_t *target;      /* (fst p) / (snd p) */
    } tt_proj;
    struct {
      lizard_ast_node_t *value;       /* (inl a) or (inr b) */
    } tt_inj;
    struct {
      lizard_ast_node_t *scrutinee;   /* (case s f g) */
      lizard_ast_node_t *left_branch;
      lizard_ast_node_t *right_branch;
    } tt_case;
    struct {
      lizard_ast_node_t *motive;      /* (J P d p) */
      lizard_ast_node_t *refl_case;
      lizard_ast_node_t *path;
    } tt_j;
    struct {
      lizard_ast_node_t *motive;      /* (xport motive path value) — the
                                       * motive is a TT-level Lambda
                                       * (Lambda 'x T) whose body T tells
                                       * the engine which type-former rule
                                       * to apply. */
      lizard_ast_node_t *path;
      lizard_ast_node_t *value;
    } tt_xport;
    struct {
      const char *name;               /* (U-var 'i) */
    } tt_u_var;
    struct {
      lizard_ast_node_t *operand;     /* (U-suc u) */
    } tt_u_suc;
    struct {
      lizard_ast_node_t *left;        /* (U-max u v) */
      lizard_ast_node_t *right;
    } tt_u_max;
  } data;
};

typedef struct lizard_ast_list_node {
  lz_list_node_t node;
  lizard_ast_node_t *ast;
} lizard_ast_list_node_t;

typedef struct lizard_heap_segment {
  char *start;
  char *top;
  char *end;
  struct lizard_heap_segment *next;
} lizard_heap_segment_t;

struct lizard_heap {
  lizard_heap_segment_t *head;
  lizard_heap_segment_t *current;
  size_t initial_size;
  size_t max_segment_size;
};

typedef struct lizard_env_entry {
  const char *symbol;
  lizard_ast_node_t *value;
  struct lizard_env_entry *next;
} lizard_env_entry_t;

struct lizard_env {
  lizard_env_entry_t *entries;
  struct lizard_env *parent;
};

typedef lizard_ast_node_t *(*lizard_continuation_t)(lizard_ast_node_t *result,
                                                    lizard_env_t *env,
                                                    lizard_heap_t *heap);

lizard_ast_node_t *lizard_convert_list_literal(lizard_ast_node_t *node,
                                               lizard_heap_t *heap);
lizard_ast_node_t *lizard_make_promise(lizard_heap_t *heap,
                                       lizard_ast_node_t *expr,
                                       lizard_env_t *env);
lizard_ast_node_t *lizard_force(lizard_ast_node_t *node, lizard_heap_t *heap);

lizard_ast_node_t *lizard_primitive_delay(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);
lizard_ast_node_t *lizard_primitive_force(lz_list_t *args, lizard_env_t *env,
                                          lizard_heap_t *heap);

lizard_ast_node_t *lizard_eval(lizard_ast_node_t *node, lizard_env_t *env,
                               lizard_heap_t *heap, lizard_continuation_t cont);
lizard_ast_node_t *lizard_apply(lizard_ast_node_t *func, lz_list_t *args,
                                lizard_env_t *env, lizard_heap_t *heap,
                                lizard_continuation_t cont);
lizard_ast_node_t *lizard_expand_macros(lizard_ast_node_t *node,
                                        lizard_env_t *env, lizard_heap_t *heap);
lizard_ast_node_t *lizard_expand_quasiquote(lizard_ast_node_t *node,
                                            lizard_env_t *env,
                                            lizard_heap_t *heap);

#endif /* LIZARD_H */
