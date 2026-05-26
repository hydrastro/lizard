# Lizard — Design

This document walks through how the pieces fit together. Read it after
`README.md` if you want to understand the architecture, modify the
implementation, or evaluate the design decisions.

## Source layout

```
include/lizard.h          — main public types and AST node enum
src/
  lizard.c                — entry point, REPL, file driver
  parser.c                — s-expression reader
  printer.c               — pretty-printer
  evaluator.c             — Scheme evaluator (apply/eval loop)
  env.c, env.h            — environment (lexical scope)
  heap.c                  — bump allocator with explicit free at boundaries
  primitives.c, .h        — all built-in functions (Scheme prims + TT ops)
  bignums.c               — GMP wrapper
  macros.c                — syntax-rules and transformer-lambda
  exceptions.c            — raise / with-handler
  tt_check.c              — bidirectional type checker (infer/check)
  tt_equality.c           — reduction engine + structural/alpha equality
tests/                    — C test programs (one per topic)
examples/                 — Lisp programs demonstrating each layer
scripts/                  — clean.sh, build helpers
```

The Scheme core and the type-theory layer share infrastructure (the
heap, the AST, the parser) but are otherwise loosely coupled. The
type-theory layer adds new AST node types and wires them through the
six engine descent operations described below.

## AST representation

A single tagged-union type `lizard_ast_node_t` holds all node kinds.
The enum `lizard_ast_node_type_t` is the discriminator; the `data`
union holds per-kind payloads. New constructs add an enum value, a
union field, a constructor primitive, a printer case, a type-of case,
and (typically) six engine descents.

This is verbose but uniform. Adding a new construct touches a known
list of places — there's no surprise interaction with the rest of the
codebase.

## The six descent operations

Every AST node kind that the engine sees needs to be handled by six
operations in `tt_equality.c`:

1. **`contains_free_var(t, name)`** — does `t` mention the free variable
   `name`? Used by capture-avoiding substitution and by the comp Pi
   non-dependence check.

2. **`alpha_equal_rec(a, b, ea, eb)`** — alpha-equivalence with
   binder environments. Two binders are equivalent if their bodies are
   equivalent under a consistent renaming.

3. **`structurally_equal(a, b)`** — strict structural equality, no
   binder accommodation. Used by the memoization table.

4. **`subst_rec(t, x, v, heap)`** — capture-avoiding substitution of
   value `v` for the term-level variable name `x` in `t`. Interval
   variables and term variables live in separate namespaces, so this
   doesn't affect interval variables.

5. **`subst_interval(t, x, v, heap)`** — substitute interval term `v`
   for interval variable `x`. Used by path-app beta.

6. **`normalize_rec(t, heap, memo)`** — descend through subterms,
   normalizing each, then apply head rewrites at this node. The
   memoization table caches structurally-equal subterms within one
   normalization call.

When you add a new AST kind that has subterms, you implement all six.
The head reduction rule (if any) goes in `head_rewrites` which is
called from `normalize_rec` after the subterms are normalized.

## The reduction engine

`tt_equality.c` is the main engine. The public entry is
`lizard_tt_reduce(t, heap)` which clears the memo table and calls
`normalize_rec`. Internally:

- Subterms are normalized first (bottom-up)
- Then `head_rewrites` is consulted for a head-level rule
- If a head rule fires, we re-normalize the result (until fixpoint)

Each head rule is gated on a flag (e.g. `reduce-path-beta`,
`reduce-comp-pi`). Flags default to enabled. Disabling a flag is
useful for experiments and for understanding which rule does what.

## The bidirectional type checker

`tt_check.c` has two main entries:

- **`lizard_tt_infer(ctx, t, heap)`** — given context and term,
  produce the type, or return an error node. Used for terms whose
  type is determined by their shape (variables look up the context,
  applications check the function then determine the result, etc).

- **`lizard_tt_check(ctx, t, T, heap)`** — given context, term, and
  expected type, return 1 if the term checks, 0 otherwise. Used for
  terms that need the type to disambiguate (Lambda checks against
  Pi, Pair checks against Sigma, Inl/Inr check against Sum, path-abs
  checks against Path, glue-intro checks against Glue).

The general check pattern: try the special check cases first
(`if (t->type == AST_TT_LAMBDA) { ... }` etc), fall through to
infer-then-compare. Conversion uses `lizard_tt_alpha_equal` after
reduction.

Cumulativity is handled by `lizard_tt_universe_leq` — if alpha
fails on two universes, try the ≤ check.

## The universe lattice

Universes are not just integers — we support:

- `(U n)` — concrete level n
- `(U-var 'i)` — abstract universe variable
- `(U-suc u)` — successor of any universe expression
- `(U-max u v)` — supremum

with reduction rules: concrete successor and max evaluate, idempotence
(`(U-max u u) → u`), zero absorption (`(U-max (U 0) u) → u` since
`(U 0)` is the bottom of the lattice... actually no, it's the *unit*
of max where any concrete level ≥ 0). The lattice is well-formed
enough to express thesis-level universe stratification.

## The cubical layer architecture

The cubical layer sits on top of λΠ. It adds:

- **Interval namespace** — interval variables are separate from term
  variables. `subst_interval` is a separate substitution operation;
  it descends through term-level binders without shadowing concerns.

- **Faces as a separate sort** — represented as inhabiting `(U 0)`
  for slotting into the universe hierarchy, but conceptually a
  decidable propositional sort over the interval.

- **Systems for multi-face partials** — linked list of (face, value)
  clauses with simplification rules. Each clause can be active when
  its face becomes F1, dropped when its face becomes F0.

- **Composition as the computational primitive** — `comp`, `hcomp`,
  `fill` with rules per type former. Each per-type-former rule pushes
  the comp inside, decomposing it into comps on the constituent types.

- **Glue as a type former** — `Glue A φ T e` is A outside φ and T
  (related to A via the equivalence e) on φ. The CCHM rule for comp
  Glue produces a `glue-intro` of two new comps (one in A, one in T)
  coordinated by `equiv-fun` and `unglue`.

- **ua as a constructor** — `(ua e)` is the term, with typing rule
  giving `Path U A B`. Reduction rule for the identity equivalence
  case gives the constant path on A.

## How computational univalence flows

The end-to-end chain for the canonical case:

```
transport along (ua (id-equiv A)) applied to x
= (comp <i>((ua (id-equiv A)) @ i) F0 (system-nil) x)
```

Reduction steps:

1. **path-app on `(ua (id-equiv A))`** fires the `reduce-ua-endpoints`
   rule, which checks the equivalence is `id-equiv` and returns the
   domain `A`. So the family becomes `<i>A` — a constant path-abs.

2. **comp with constant family + empty partial + F0 face** fires the
   `reduce-system-nil-comp` rule. The body of the family doesn't use
   the binder (constant), so the result is the base `x`.

Done. No further reduction. The chain collapses cleanly.

For a non-identity equivalence, step 1's rule doesn't fire (we only
have the identity case), so the reduction stops at the path-app term.
The *type* `(ua e) : Path U A B` is still correct — only the
*computation* of `(ua e) @ i` for arbitrary `e` is incomplete.

## Where to extend

If you want to add new term formers (e.g. for a thesis-specific
construct):

1. Add an enum value in `lizard_ast_node_type_t` and a data field in
   the union.
2. Constructor + predicate + accessors in `primitives.c`.
3. Register in the env-population function.
4. Type-of case for the reflection primitive.
5. Printer case.
6. Header prototype in `primitives.h`.
7. Engine descent: contains_free_var, alpha_equal_rec,
   structurally_equal, subst_rec, subst_interval, normalize_rec.
8. Head reduction rule(s) if any (gated on flags).
9. Typing rule(s) in `tt_check.c`.
10. Test in `tests/`, example in `examples/`.

The verbosity is intentional — each new construct touches a checklist
of places, so you don't miss one and end up with subtle bugs.

## Memoization

`normalize_rec` uses a per-call memo table keyed on AST node pointer.
Within one top-level reduce call, structurally-shared subterms
(common after substitution) only reduce once. The table is cleared
at the entry to each public reduce call.

## Honest design choices

- **Faces inhabit `(U 0)`.** Strictly, faces are a separate sort; we
  reuse `(U 0)` to avoid plumbing a new sort through the universe
  lattice. This is sound for type checking but blurs a sort
  distinction that a stricter implementation would preserve.

- **Equivalence is `(equiv-fun, equiv-inv)` not `Σ f. isEquiv f`.**
  Full CCHM uses the contractible-fibers definition; we treat
  equivalence as the simpler pair of forward and backward functions.
  For `id-equiv` this is correct; for general equivalences it omits
  the coherence proof that contractibility provides.

- **Single-face Glue, not Glue-with-system.** `Glue A φ T e` takes one
  face. Multi-face Glue (needed for the general `ua` interior) would
  require either an AST extension or rewriting the Glue head rules
  to consume systems.

- **comp Pi non-dependent only.** The dependent comp Pi rule requires
  transport on the argument across the type family's domain line.
  Not implemented.

- **Strict regularity not enforced.** Canonical equality for
  composition (`comp A [F0 ↦ ⊥] u0 ≡ u0` definitionally, not just up
  to reduction) is not explicitly enforced; we rely on reduction to
  reach the canonical form.

These choices are documented in `LIMITATIONS.md` with their
implications.
