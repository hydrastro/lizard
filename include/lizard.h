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
  /* (pi-fresh 'x A B) and (sigma-fresh 'x A B) — Phase L.3.
   *
   * These are dimension-creating variants of Pi and Sigma. They have
   * the *same term-level semantics* as Pi/Sigma (binder, lambda,
   * application all unchanged), but their TYPING RULE introduces a
   * fresh dimension into the result universe:
   *
   *   A : U_S    B : U_T (under x:A)
   *   ─────────────────────────────────────────────
   *   (pi-fresh x A B) : U_{S ∪ T ∪ {fresh dim}}
   *
   * This reifies the thesis claim that "what generates a new universe
   * is quantifying on a previous universe": every dimension-creating
   * quantification produces a universe with a strictly larger dim
   * set than its parts.
   *
   * The fresh dim is a per-session counter (heap-resident), so two
   * uses of pi-fresh in the same session get distinct dimensions.
   */
  AST_TT_PI_FRESH,
  AST_TT_SIGMA_FRESH,
  /* (co-pi-fresh 'x A B) and (co-sigma-fresh 'x A B) — Phase L.5.
   *
   * The dual of pi-fresh / sigma-fresh. Same term shape, but the
   * typing rule produces a result in the COUNIVERSE lattice rather
   * than the universe lattice. The thesis claim:
   *
   *   "Quantifying over a context/observation generates a new
   *    couniverse dimension."
   *
   *   A : Co_S    B : Co_T (under x:A)
   *   ───────────────────────────────────────────────────
   *   (co-pi-fresh x A B) : (Co-max Co_S Co_T) ∨ (Co-set fresh)
   *
   * The fresh-dim counter is SHARED with pi-fresh — every binding
   * event gets a globally unique nat, and the *sort* (U-set vs
   * Co-set) tells you which lattice the dim belongs to.
   */
  AST_TT_CO_PI_FRESH,
  AST_TT_CO_SIGMA_FRESH,
  /* (Box A) and (Diamond A) — Phase M.5.1 modal type constructors.
   *
   * Box and Diamond are the necessity (□) and possibility (◇)
   * modalities of modal logic. In a modal type theory they appear
   * as type constructors: Box A is "necessarily A", Diamond A is
   * "possibly A".
   *
   * M.5.1 delivers only the type-constructor level — typing rules
   * for (Box A) and (Diamond A) — without intro/elim forms or
   * reduction interactions. Adding box-introduction / unbox-
   * elimination is M.5.2, and that step commits to a specific modal
   * logic (K, T, S4, S5, etc.). M.5.1 stays neutral on which logic.
   *
   * Typing rule: (Box A) : univ(A), (Diamond A) : univ(A). The
   * modality preserves the universe of its argument.
   *
   * Gated on `modalities-enabled` in the logic-rule registry
   * (default-on, like other M.6 feature toggles).
   */
  AST_TT_BOX,
  AST_TT_DIAMOND,
  /* (box e) and (unbox x b e) — Phase M.5.2 introduction and
   * elimination forms for the Box modality.
   *
   * Introduction: (box e) has type (Box T) when e has type T.
   * Elimination:  (unbox x b body) binds the unboxed contents of
   *               b (a Box value) as x within body. Read as
   *               "let unbox x = b in body".
   *
   * Beta rule: (unbox x (box e) body) → body[e/x].
   *
   * M.5.2 turn 1 uses PLACEHOLDER typing rules that don't enforce
   * a specific modal logic. Turn 2 will add the dual context
   * discipline that makes this concretely S4. Until then, these
   * forms compute correctly (via the beta rule) but don't restrict
   * what's typeable.
   *
   * Gated on `modalities-enabled` (same toggle as Box/Diamond).
   */
  AST_TT_BOX_INTRO,
  AST_TT_BOX_ELIM,
  /* (diamond e) and (let-diamond x b e) — Phase M.5.5 (Turn 1 of
   * "5-axiom for S5" work).
   *
   * The introduction and elimination forms for the Diamond
   * (possibility) modality, parallel to box and unbox.
   *
   * Introduction: (diamond e) constructs a Diamond value.
   * Elimination:  (let-diamond x b body) pattern-matches a Diamond
   *               value b, binding its content as x in body.
   *
   * Beta rule: (let-diamond x (diamond e) body) → body[e/x].
   *
   * M.5.5 Turn 1 uses PLACEHOLDER typing — no 5-axiom yet, no
   * dual-context discipline for Diamond. Turn 2 will add the
   * S5-distinguishing 5-axiom (◇A → □◇A) as a typing rule.
   *
   * Gated on `modalities-enabled`.
   */
  AST_TT_DIAMOND_INTRO,
  AST_TT_DIAMOND_ELIM,
  /* (box-app f a) — Phase M.5.6. The K-axiom realized as a term:
   *
   *   K-axiom:  □(A → B) → □A → □B
   *
   * Given f of type Box (Pi x A B) and a of type Box A, (box-app f a)
   * produces a term of type Box B. This is the "necessity distributes
   * over function application" form: from "necessarily f" and
   * "necessarily a", deduce "necessarily (f a)".
   *
   * Computation: (box-app (box f') (box a')) → (box (@ f' a')).
   *
   * The K-axiom is admissible (derivable) in all of K/T/S4/S5 — it's
   * a theorem in each. We add it as an explicit AST node primarily
   * for K's sake, since K lacks other elimination rules. In T/S4/S5
   * it's redundant but still usable.
   *
   * Gated on `modalities-enabled` (same toggle as Box). */
  AST_TT_BOX_APP,
  /* (diamond-bind f d) — Phase M.5.8. The Kleisli composition for
   * Diamond, dual to box-app:
   *
   *   Δ; Γ ⊢ f : Pi x A (Diamond B)    Δ; Γ ⊢ d : Diamond A
   *   ─────────────────────────────────────────────────────
   *   Δ; Γ ⊢ (diamond-bind f d) : Diamond B
   *
   * This is the "monadic bind" for Diamond: given a function that
   * lifts A into Diamond B, and a Diamond A value, produce Diamond B.
   *
   * Computation: (diamond-bind (Lambda x body) (diamond a))
   *              → body[a/x]   (where body : Diamond B)
   *
   * Like box-app, this is admissible in all S4+ logics — derivable
   * from let-diamond and Pi. We add it as a first-class form to
   * make the Diamond monad structure explicit and symmetric with
   * box-app's K-axiom realization.
   *
   * Gated on `modalities-enabled`. */
  AST_TT_DIAMOND_BIND,
  /* (dia e) — Phase M.5.9. Symmetric Diamond introduction.
   *
   * The Pfenning-Davies symmetric S5 rule:
   *
   *   Δ; Γ; · ⊢ e : A poss
   *   ────────────────────
   *   Δ; Γ; · ⊢ dia e : Diamond A true
   *
   * The body must be a POSS judgment. This is the dual of box-intro:
   * where box requires Δ-only with truth dropped, dia requires the
   * body to be a poss-judgment (which itself requires the Ω context
   * to be empty at the dia boundary).
   *
   * Different from the asymmetric (diamond e) — that wraps any true
   * judgment loosely. The symmetric (dia e) enforces the proper
   * scoping discipline.
   *
   * Gated on `modalities-enabled` AND `modal-symmetric`. */
  AST_TT_DIAMOND_INTRO_SYM,
  /* (poss-coerce e) — Phase M.5.9. Explicit shift from "true" to
   * "poss" judgments. In the symmetric calculus, any true judgment
   * can be coerced to a poss judgment by this form:
   *
   *   Δ; Γ; · ⊢ e : A true
   *   ────────────────────
   *   Δ; Γ; · ⊢ (poss-coerce e) : A poss
   *
   * Operationally a no-op (reduces to e). Semantically marks the
   * judgment kind shift for the symmetric typing rules.
   *
   * Gated on `modal-symmetric`. */
  AST_TT_POSS_COERCE,
  AST_TT_APP,          /* (@ f a) — explicit application form */
  AST_TT_SUM,          /* (Sum A B) — coproduct type */
  AST_TT_UNIVERSE,     /* (U n) — universe at integer level */
  AST_TT_COUNIVERSE,   /* (Uco n) — couniverse at integer level */
  /* (U-set d1 d2 ...) — multi-dimensional universe, indexed by a
   * finite set of natural numbers rather than a single nat.
   * Phase L.2 of the lattice work: this is the representation that
   * makes (U-set 0 1) and (U-set 0 2) genuinely incomparable.
   * The set is stored as a sorted array of distinct longs. */
  AST_TT_UNIVERSE_SET,
  /* (Co-set d1 d2 ...) — multi-dimensional COUNIVERSE — Phase L.4.
   *
   * The dual of UNIVERSE_SET. Universes and couniverses live in
   * SEPARATE lattices: there is no automatic conversion between
   * (U-set ...) and (Co-set ...), and operations like U-max only
   * combine values within one lattice or the other.
   *
   * The rationale: in the thesis framework, universes track *where
   * types live* (the syntactic side) and couniverses track *where
   * contexts/observations live* (the semantic-ish side). The two
   * have parallel lattice structure but are not interchangeable —
   * mixing them would erase the distinction the framework rests on.
   *
   * Representation is identical to UNIVERSE_SET: sorted-array of
   * distinct nats. The kinds are distinguished only by the AST tag.
   *
   * Lattice operations:
   *   Co-max — join (set union, dual to U-max)
   *   Co-min — meet (set intersection, dual to U-min)
   *   couniverse-leq? — subset within the couniverse lattice
   */
  AST_TT_COUNIVERSE_SET,
  /* (Co-max c1 c2) and (Co-min c1 c2) — couniverse lattice ops. */
  AST_TT_CO_MAX,
  AST_TT_CO_MIN,
  /* ===== Phase H.1 — Higher Inductive Types (HITs) =====
   *
   * Architecturally clean scaffold. HIT declarations are first-class
   * data — an AST_TT_HIT_DECL node holds a name and two lists of
   * constructor records (point constructors and path constructors).
   *
   * H.1 deliberately keeps semantics minimal: declarations are stored,
   * looked up by name, substituted through, equality-tested, and
   * pretty-printed. No computation rules for HIT instances yet —
   * those require per-HIT or per-shape work (and the comp interaction
   * with HIT path-constructors is the hard open research problem).
   *
   * The structure to grow into is uniform: every HIT is "a list of
   * point constructors plus a list of path constructors", which is
   * the shape that generic comp rules would later iterate over.
   *
   * Surface forms:
   *   (HIT 'name (constructors ...) (paths ...))    — declaration
   *   (HIT-constructor 'cname A1 A2 ...)            — point ctor record
   *   (HIT-path 'pname source target ...)           — path ctor record
   *   (HIT-ref 'name)                               — use the HIT as a type
   *   (HIT-app 'cname arg1 arg2 ...)                — apply a constructor
   *
   * A registered HIT can be looked up via (HIT-lookup 'name).
   */
  AST_TT_HIT_DECL,
  AST_TT_HIT_CONSTRUCTOR,
  AST_TT_HIT_PATH,
  AST_TT_HIT_REF,
  AST_TT_HIT_APP,
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
  AST_TT_U_MAX,
  /* (U-min u v) — the meet (greatest lower bound) of two universe
   * expressions. Dual to U-max. Added in Phase L.1 (Phase L is the
   * lattice work: universes are indexed by finite sets of naturals,
   * the partial order is subset inclusion, join is U-max (= set
   * union on indices), meet is U-min (= set intersection). With
   * both join and meet, universes form a proper lattice rather than
   * just a join-semilattice. */
  AST_TT_U_MIN,
  /* ----- Cubical type theory layer (CCHM-style) -----
   * The interval, paths, and the machinery that makes univalence
   * a computation rule rather than a postulate. These coexist with
   * the observational Id rules — they don't replace them.
   *
   * Interval pre-type:
   *   AST_TT_INTERVAL  — the type I itself
   *   AST_TT_I0        — the endpoint 0 of I
   *   AST_TT_I1        — the endpoint 1 of I
   *   AST_TT_I_VAR     — an interval variable bound by <i>
   *
   * Connection operations on interval terms:
   *   AST_TT_I_AND     — i ∧ j (min in [0,1] reading)
   *   AST_TT_I_OR      — i ∨ j (max)
   *   AST_TT_I_NEG     — ~ i  (1 - i)
   *
   * Path type and its intro/elim:
   *   AST_TT_PATH      — (Path A a b), the type of paths
   *   AST_TT_PATH_ABS  — (<i> body), path abstraction
   *   AST_TT_PATH_APP  — (p @ i), path application
   */
  AST_TT_INTERVAL,
  AST_TT_I0,
  AST_TT_I1,
  AST_TT_I_VAR,
  AST_TT_I_AND,
  AST_TT_I_OR,
  AST_TT_I_NEG,
  AST_TT_PATH,
  AST_TT_PATH_ABS,
  AST_TT_PATH_APP,
  /* ----- Faces and partial elements (Turn 7) -----
   * A face formula is a boolean combination of equations on the
   * interval. It describes a subset of the cube.
   *
   *   AST_TT_F0    — the always-false face (empty subset)
   *   AST_TT_F1    — the always-true face (whole cube)
   *   AST_TT_F_EQ  — atomic (i = endpoint) or (i = j); two interval terms
   *   AST_TT_F_AND — conjunction of faces
   *   AST_TT_F_OR  — disjunction of faces
   *
   * Partial and Sub:
   *   AST_TT_PARTIAL  — (Partial φ A) is the type of A-elements
   *                     defined only where φ holds
   *   AST_TT_SUB      — (Sub A φ u) is A-elements agreeing with the
   *                     partial element u on the face φ
   */
  AST_TT_F0,
  AST_TT_F1,
  AST_TT_F_EQ,
  AST_TT_F_AND,
  AST_TT_F_OR,
  AST_TT_PARTIAL,
  AST_TT_SUB,
  /* ----- Kan composition (Turn 8) -----
   *
   * Kan composition `comp` is the operation that makes cubical type
   * theory compute up to homotopy. It fills in a missing face of a
   * partial cube. The signature:
   *
   *   comp A [φ ↦ u] u0  :  A @ i1
   *
   * where:
   *   A     : interval-indexed type family (path-abs over types)
   *   φ     : face formula
   *   u     : partial element along the line, defined on φ
   *   u0    : the starting face (at i0), agreeing with u on φ
   *
   * `hcomp` is the homogeneous case where the type family is constant:
   *
   *   hcomp A [φ ↦ u] u0  :  A
   *
   * `fill` is comp's "whole line" version — gives a path-abs whose
   * body at any i is the comp up to that point. Useful for proofs. */
  AST_TT_COMP,         /* (comp A phi u u0) */
  AST_TT_HCOMP,        /* (hcomp A phi u u0) */
  AST_TT_FILL,         /* (fill A phi u u0) */
  /* ----- Equivalences, Glue, and ua (Turns 9 & 10) -----
   *
   * Equiv A B is the type of equivalences from A to B. In the full
   * CCHM development it's Σ(f : A → B). isEquiv f, where isEquiv f
   * says all fibers of f are contractible. Building that out fully
   * here would require contractibility, fibers, etc. — substantial
   * extra machinery that isn't load-bearing for the computational
   * content of ua. We treat Equiv as a primitive type former with
   * two accessors (equiv-fun and equiv-inv) plus a constructor
   * (id-equiv A). This is sound for what ua needs — the inverse is
   * what makes comp over ua compute.
   *
   *   AST_TT_EQUIV_TYPE  — (Equiv A B), the type of equivalences
   *   AST_TT_ID_EQUIV    — (id-equiv A), the identity equivalence
   *   AST_TT_EQUIV_FUN   — (equiv-fun e), forward direction
   *   AST_TT_EQUIV_INV   — (equiv-inv e), backward direction
   *
   * Glue A [φ ↦ (T, e)] is a type that's A outside φ and equivalent
   * to T (via e) on φ. Plus introduction `glue` and elimination
   * `unglue`. For the syntax we use a *single* face/T/e — the
   * multi-face case can be encoded by combining φ's with F-or.
   *
   *   AST_TT_GLUE        — (Glue A φ T e)
   *   AST_TT_GLUE_INTRO  — (glue φ t a)  — glues t along φ over a
   *   AST_TT_UNGLUE      — (unglue e g)  — extracts the A-element
   *
   * Finally:
   *   AST_TT_UA          — (ua e), a path in U built from equivalence e
   */
  AST_TT_EQUIV_TYPE,
  AST_TT_ID_EQUIV,
  AST_TT_EQUIV_FUN,
  AST_TT_EQUIV_INV,
  AST_TT_GLUE,
  AST_TT_GLUE_INTRO,
  AST_TT_UNGLUE,
  AST_TT_UA,
  /* ----- Systems and comp Glue (Turn 11) -----
   *
   * A "system" is a finite map from faces to values, used as the
   * partial-element argument to comp/hcomp when the partial is
   * defined on a disjunction of multiple atomic faces.
   *
   *   [φ_1 ↦ u_1, φ_2 ↦ u_2, ..., φ_n ↦ u_n]
   *
   * Side condition: u_i ≡ u_j on φ_i ∧ φ_j.
   *
   * Representation: a linked list of clauses. SYSTEM_NIL is the
   * empty system (acts like the empty partial element, defined
   * nowhere). SYSTEM_CONS wraps (face, value, rest).
   *
   * The single-face Partial of Turn 7 is the n=1 case; we keep
   * both representations and treat them interchangeably.
   */
  AST_TT_SYSTEM_NIL,
  AST_TT_SYSTEM_CONS,
  /* ----- Glue with system (Phase A.1) -----
   *
   * Generalizes Glue to take a face-system of (face, T, equiv) triples
   * instead of a single (face, T, equiv). The CCHM `ua` definition:
   *
   *   ua e = <i> Glue-sys B [(i=i0) ↦ (A, e), (i=i1) ↦ (B, id-equiv B)]
   *
   * needs this two-clause form. With Glue-sys we can construct the
   * proper interior for non-identity equivalences.
   *
   *   AST_TT_GLUE_SYS_NIL  — empty Glue-system, Glue degenerates to A
   *   AST_TT_GLUE_SYS_CONS — (face, T, equiv, next) clause cell
   *   AST_TT_GLUE_SYS      — (Glue-sys A system) the type former
   */
  AST_TT_GLUE_SYS_NIL,
  AST_TT_GLUE_SYS_CONS,
  AST_TT_GLUE_SYS
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
    /* Phase L.3: pi-fresh and sigma-fresh share the same data shape
     * as their non-fresh counterparts. The dimension-creating semantics
     * lives in the typing rule, not in the data. */
    struct {
      lizard_ast_node_t *binder;
      lizard_ast_node_t *domain;
      lizard_ast_node_t *codomain;
    } tt_pi_fresh;
    struct {
      lizard_ast_node_t *binder;
      lizard_ast_node_t *domain;
      lizard_ast_node_t *codomain;
    } tt_sigma_fresh;
    /* Phase L.5: dual co-pi-fresh and co-sigma-fresh. Same shape, but
     * the typing rule produces a couniverse-set instead. */
    struct {
      lizard_ast_node_t *binder;
      lizard_ast_node_t *domain;
      lizard_ast_node_t *codomain;
    } tt_co_pi_fresh;
    struct {
      lizard_ast_node_t *binder;
      lizard_ast_node_t *domain;
      lizard_ast_node_t *codomain;
    } tt_co_sigma_fresh;
    /* Phase M.5.1: modal type constructors. Both Box and Diamond
     * take a single subtype argument. */
    struct {
      lizard_ast_node_t *argument;
    } tt_box;
    struct {
      lizard_ast_node_t *argument;
    } tt_diamond;
    /* Phase M.5.2: introduction and elimination for Box.
     *
     * box_intro: (box e) — holds the term being boxed.
     * box_elim:  (unbox x b body) — binds x to the unboxed
     *            contents of b inside body. */
    struct {
      lizard_ast_node_t *body;
    } tt_box_intro;
    struct {
      lizard_ast_node_t *binder;     /* x — the variable for the unboxed value */
      lizard_ast_node_t *scrutinee;  /* b — the Box-typed term being eliminated */
      lizard_ast_node_t *body;       /* body — uses x freely */
    } tt_box_elim;
    /* Phase M.5.5: Diamond intro and elim, mirroring box / unbox. */
    struct {
      lizard_ast_node_t *body;
    } tt_diamond_intro;
    struct {
      lizard_ast_node_t *binder;
      lizard_ast_node_t *scrutinee;
      lizard_ast_node_t *body;
    } tt_diamond_elim;
    /* Phase M.5.6: K-axiom application. */
    struct {
      lizard_ast_node_t *fun;   /* Box (Pi x A B) */
      lizard_ast_node_t *arg;   /* Box A */
    } tt_box_app;
    /* Phase M.5.8: Diamond bind (Kleisli composition). */
    struct {
      lizard_ast_node_t *fun;   /* Pi x A (Diamond B) */
      lizard_ast_node_t *arg;   /* Diamond A */
    } tt_diamond_bind;
    /* Phase M.5.9: Symmetric Diamond intro and poss coercion. */
    struct {
      lizard_ast_node_t *body;  /* must be poss-typed */
    } tt_diamond_intro_sym;
    struct {
      lizard_ast_node_t *body;  /* must be true-typed */
    } tt_poss_coerce;
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
    /* Multi-dimensional universe (Phase L.2). The `dims` array holds
     * a sorted list of distinct natural-number dimensions; `count` is
     * its length. The set {0,2,5} is stored as dims=[0,2,5], count=3.
     * The empty set {} is count=0 with dims allowed to be NULL; this
     * represents the bottom of the lattice (the universe of nothing). */
    struct {
      long *dims;
      long count;
    } tt_universe_set;
    /* Phase L.4: COUNIVERSE_SET parallels UNIVERSE_SET in representation
     * but is a *distinct* lattice. */
    struct {
      long *dims;
      long count;
    } tt_couniverse_set;
    /* Co-max and Co-min: lattice operations within the couniverse
     * lattice. Same shape as tt_u_max / tt_u_min. */
    struct {
      lizard_ast_node_t *left;
      lizard_ast_node_t *right;
    } tt_co_max;
    struct {
      lizard_ast_node_t *left;
      lizard_ast_node_t *right;
    } tt_co_min;
    /* ===== Phase H.1 — HIT data layout =====
     *
     * Every HIT-related node carries its data as lz_list_t * lists of
     * child AST nodes (rather than fixed-arity slots) so the same shape
     * scales from nullary constructors to many-arg paths uniformly.
     */
    struct {
      lizard_ast_node_t *name;        /* AST_SYMBOL — HIT name */
      lz_list_t *constructors;        /* list of HIT_CONSTRUCTOR nodes */
      lz_list_t *paths;               /* list of HIT_PATH nodes */
    } tt_hit_decl;
    struct {
      lizard_ast_node_t *name;        /* AST_SYMBOL — constructor name */
      lz_list_t *arg_types;           /* list of AST nodes (each an arg type) */
    } tt_hit_constructor;
    struct {
      lizard_ast_node_t *name;        /* AST_SYMBOL — path name */
      lizard_ast_node_t *source;      /* source endpoint */
      lizard_ast_node_t *target;      /* target endpoint */
    } tt_hit_path;
    struct {
      lizard_ast_node_t *name;        /* reference name; resolves via registry */
    } tt_hit_ref;
    struct {
      lizard_ast_node_t *name;        /* constructor name */
      lz_list_t *args;                /* applied args */
    } tt_hit_app;
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
    struct {
      lizard_ast_node_t *left;        /* (U-min u v) */
      lizard_ast_node_t *right;
    } tt_u_min;
    /* Cubical nodes. INTERVAL, I0, I1 are nullary (no data).
     * I_VAR holds a name (interval variable). I_AND, I_OR are
     * binary, I_NEG unary on interval terms. */
    struct {
      const char *name;               /* (I-var 'i) */
    } tt_i_var;
    struct {
      lizard_ast_node_t *left;        /* (I-and i j) or (I-or i j) */
      lizard_ast_node_t *right;
    } tt_i_binop;
    struct {
      lizard_ast_node_t *operand;     /* (I-neg i) */
    } tt_i_neg;
    /* (Path A a b) — path type. */
    struct {
      lizard_ast_node_t *domain;      /* the ambient type A */
      lizard_ast_node_t *a;            /* endpoint at i0 */
      lizard_ast_node_t *b;            /* endpoint at i1 */
    } tt_path;
    /* (<i> body) — path abstraction. The binder is the interval var. */
    struct {
      lizard_ast_node_t *binder;      /* AST_SYMBOL for the interval var */
      lizard_ast_node_t *body;
    } tt_path_abs;
    /* (p @ i) — path application at interval point. */
    struct {
      lizard_ast_node_t *path;
      lizard_ast_node_t *point;       /* interval term */
    } tt_path_app;
    /* Face: (F-eq i j) where i,j are interval terms. */
    struct {
      lizard_ast_node_t *left;
      lizard_ast_node_t *right;
    } tt_f_eq;
    /* Face: (F-and φ ψ) and (F-or φ ψ). */
    struct {
      lizard_ast_node_t *left;
      lizard_ast_node_t *right;
    } tt_f_binop;
    /* (Partial φ A) — the type of A-elements defined on face φ. */
    struct {
      lizard_ast_node_t *face;
      lizard_ast_node_t *type;
    } tt_partial;
    /* (Sub A φ u) — A-elements agreeing with u on φ. */
    struct {
      lizard_ast_node_t *type;        /* the ambient A */
      lizard_ast_node_t *face;
      lizard_ast_node_t *partial;     /* the partial element u */
    } tt_sub;
    /* Shared shape for comp, hcomp, fill. */
    struct {
      lizard_ast_node_t *type_family; /* (path-abs i A_i) for comp/fill,
                                       * constant type for hcomp */
      lizard_ast_node_t *face;
      lizard_ast_node_t *partial;
      lizard_ast_node_t *base;
    } tt_comp;
    /* (Equiv A B) */
    struct {
      lizard_ast_node_t *domain;
      lizard_ast_node_t *codomain;
    } tt_equiv_type;
    /* (id-equiv A), (equiv-fun e), (equiv-inv e) — single operand */
    struct {
      lizard_ast_node_t *operand;
    } tt_equiv_op;
    /* (Glue A φ T e) */
    struct {
      lizard_ast_node_t *base;
      lizard_ast_node_t *face;
      lizard_ast_node_t *t;
      lizard_ast_node_t *equiv;
    } tt_glue;
    /* (glue φ t a) */
    struct {
      lizard_ast_node_t *face;
      lizard_ast_node_t *t;
      lizard_ast_node_t *a;
    } tt_glue_intro;
    /* (unglue e g) */
    struct {
      lizard_ast_node_t *equiv;
      lizard_ast_node_t *target;
    } tt_unglue;
    /* (ua e) */
    struct {
      lizard_ast_node_t *equiv;
    } tt_ua;
    /* System cons cell: (face, value, next-system). */
    struct {
      lizard_ast_node_t *face;
      lizard_ast_node_t *value;
      lizard_ast_node_t *next;
    } tt_system_cons;
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
