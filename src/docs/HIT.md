# Lizard — Higher Inductive Types Layer

This document covers the **higher inductive types (HIT)** layer of
lizard (Phases H.1 and H.2). It builds on the cubical core (Path,
faces, systems, Kan composition) and the scaffold/checked discipline
introduced in `docs/OPTIONAL_PROOF_SCAFFOLDS.md`.

Read this if you want to:

- Understand what higher inductive types are and why they matter
- Use lizard's propositional truncation (`Trunc`, `trunc`, `trunc-elim`)
- Understand the scaffold/checked status of each HIT
- Know what's checked and what remains an honest gap
- Use the S¹ scaffold for exploration

---

## What's a higher inductive type?

An ordinary inductive type (like natural numbers) is defined by its
**point constructors** — generators that produce values:

```
Nat has:
  zero : Nat
  succ : Nat → Nat
```

A **higher** inductive type extends this with **path constructors** —
generators that produce equalities between values, not just values
themselves:

```
S¹ (the circle) has:
  base : S¹                       (point constructor)
  loop : Path S¹ base base        (path constructor)
```

The point constructor `base` says "S¹ has an element." The path
constructor `loop` says "there's a non-trivial path from `base` to
itself" — making S¹ topologically a circle, not just a point.

The key challenge: **computation rules for path constructors.**
Eliminating out of an ordinary inductive type is straightforward
(pattern-match on constructors, reduce). Eliminating out of a HIT
requires reducing on path constructors, which interact with the
interval, face restrictions, and cubical machinery generally. In a
non-cubical setting, how to compute on path constructors is genuinely
an open problem.

Lizard handles this by splitting HITs into two tiers:

- **Scaffolds**: AST nodes exist, constructors parse and print, minimal
  typing only. No recursors, no computation rules. Safe to experiment
  with; no soundness claims.
- **Checked**: real typing rules, computation rules, structural
  verification. Specific soundness claims documented in
  `docs/CLAIMS_MATRIX.md`.

---

## The two-tier discipline

Every HIT form in lizard is gated behind a **logic rule**:

| Toggle | What it gates |
|---|---|
| `truncations-enabled` | `Trunc`, `trunc`, `trunc-elim` |
| `cubical-s1-enabled` | `S1`, `base`, `loop` |
| `theory-extensions-enabled` | `theory-extension` |

Named bundles set these:

| Bundle | truncations | cubical-s1 | extensions |
|---|---|---|---|
| `truncations` | on | off | off |
| `cubical-S1` | off | on | off |
| `proof-scaffold` | on | on | on |

The migration path from scaffold to checked is documented in
`docs/OPTIONAL_PROOF_SCAFFOLDS.md`. The current status of each form
is tracked in `docs/CLAIMS_MATRIX.md`.

---

## Propositional truncation (checked)

Propositional truncation `‖A‖` is the simplest HIT with non-trivial
computation. It "forgets" the distinguishing structure of `A`: any
two elements of `‖A‖` are equal by construction.

### Why it matters

In HoTT, propositional truncation is used for:

- **"Merely exists"**: `‖Σ x:A. P(x)‖` says "there exists an `x`
  satisfying `P`" without committing to which one.
- **Set quotients**: quotienting a type by an equivalence relation
  uses truncation to ensure the quotient is a set.
- **Bracket types**: `‖A‖` is the bracket type `[A]` from the HoTT
  book — "A is inhabited" as a proposition.

### The forms

```lisp
(set-logic 'truncations)

;; Type former: (Trunc level A)
;; level is structural (e.g. -1 for propositional, 0 for set)
;; lizard doesn't enforce level semantics in this turn
(Trunc -1 (U 0))        ;; => (Trunc -1 (U 0))

;; Constructor: (trunc value)
;; Lifts a value of A into its truncation
(trunc 'x)              ;; => (trunc x)

;; Eliminator: (trunc-elim C h e) — 3-arg scaffold form
;; (trunc-elim C h prop e) — 4-arg checked form
```

### Typing rules

```
Γ ⊢ A : (U n)
──────────────────────────
Γ ⊢ (Trunc level A) : (U n)
```

The truncation lives at the same universe level as its argument.
The `level` parameter is structural — lizard records it but doesn't
enforce level-specific discipline (e.g., it doesn't verify that a
level-0 truncation actually produces a set).

```
Γ ⊢ e : A
──────────────────────────────────
Γ ⊢ (trunc e) : (Trunc <level> A)
```

The constructor infers `A` from `e`'s type. The level is left
unspecified at inference time (NULL internally, prints as `(Trunc A)`
without a level). To get a specific level, construct the type
explicitly: `(Trunc -1 A)`.

```
Γ ⊢ C : (U n)                    (motive)
Γ ⊢ h : Π _:A. C                 (handler — point case)
Γ ⊢ e : (Trunc level A)          (scrutinee)
──────────────────────────────────────────────
Γ ⊢ (trunc-elim C h e) : C       (3-arg, scaffold)
```

The 3-arg form checks the motive, handler, and scrutinee but does
NOT verify that C is a proposition. This is the scaffold behavior —
documented as an honest gap.

```
additionally:
Γ ⊢ p : Π x:C. Π y:C. Path C x y    (propositionality witness)
──────────────────────────────────────────────────────────────────
Γ ⊢ (trunc-elim C h p e) : C         (4-arg, checked)
```

The 4-arg form structurally verifies `p`'s type has the full shape:
outer Π over C, inner Π over C, Path body with domain C. If any
part doesn't match, a clear error is returned identifying which
component failed.

### Computation rule

```
(trunc-elim C h [p] (trunc x))  ⟶  (@ h x)
```

The `[p]` is optional (present in the 4-arg form, absent in 3-arg).
Beta fires when the scrutinee is a literal `trunc`. The rule is
**deterministic**: the LHS pattern `(trunc-elim _ _ [_] (trunc _))`
is unique across all reductions in lizard. Variable, application,
and nested-eliminator scrutinees don't trigger — the recursor stays
as a normal form.

### What's checked vs. what's not

| Aspect | Status |
|---|---|
| Type former preserves universe | checked |
| Constructor infers underlying type | checked |
| Eliminator checks motive, handler, scrutinee types | checked |
| Eliminator checks handler domain = scrutinee's A | checked |
| Eliminator checks handler codomain = motive C | checked |
| Propositionality of C (4-arg form) | checked (structural shape) |
| Propositionality of C (3-arg form) | **not checked** (scaffold gap) |
| Beta on literal trunc | checked, deterministic |
| Level semantics (propositional vs. set vs. groupoid) | **not checked** |
| Soundness of combined cubical + modal + HIT system | **not proven** |
| Kan composition with truncation motive | **not implemented** |

---

## S¹ — the circle (scaffold only)

S¹ is the canonical non-trivial HIT. lizard has AST nodes for it
but no computation rules.

### The forms

```lisp
(set-logic 'cubical-S1)

(S1)        ;; => S1            (the type)
(base)      ;; => base          (the point constructor)
(loop)      ;; => loop          (the path constructor)
```

### What exists

- **AST nodes**: `AST_TT_S1`, `AST_TT_S1_BASE`, `AST_TT_S1_LOOP`.
- **Typing**: minimal spine only.
  - `S1 : (U 0)` — S¹ lives in the lowest universe.
  - `base : S1` — the base point.
  - `loop` — typed as a path `Path S1 base base` in the spine, but
    the interval-variable interaction is not implemented.
- **Printing**: works correctly.
- **Predicates/accessors**: `S1?`, `base?`, `loop?`.
- **Gated on** `cubical-s1-enabled`.

### What doesn't exist (yet)

- **No recursor.** There's no `S1-rec` or `S1-elim` form. You can't
  eliminate out of S¹.
- **No computation rule for `loop`.** This is the hard part: the
  `loop` case of the recursor needs to interact with the interval
  `i : I`, face restrictions, and Kan composition for the motive.
  In CCHM (Cohen-Coquand-Huber-Mörtberg 2018), this is handled by
  computing `ap` of the recursor along the path; in lizard's
  non-fully-cubical setting, the right approach isn't settled.
- **No Kan composition for S¹.** `comp` with S¹ as the type former
  would need to handle the `loop` case, which requires the recursor.

### What it takes to promote S¹

The `OPTIONAL_PROOF_SCAFFOLDS.md` migration path applies:

1. Add `S1-rec` AST node with point and path branches.
2. Implement the point-case beta: `S1-rec C cb cl base ⟶ cb`.
3. Implement the path-case reduction: `S1-rec C cb cl (loop @ i) ⟶ cl @ i`
   with appropriate interval-variable interaction.
4. Add Kan composition for S¹-typed terms.
5. Verify (or document gaps in) the interaction with the existing
   cubical layer (Path, faces, systems).

Steps 1–2 are mechanical. Step 3 is where the research starts.
Steps 4–5 are where it gets genuinely hard.

---

## Generic theory extensions (scaffold only)

The `theory-extension` node is a catch-all for user-defined
experimental forms:

```lisp
(set-logic 'proof-scaffold)
(theory-extension 'my-rule (S1) (Trunc 0 (S1)))
;; => (theory-extension my-rule S1 (Trunc 0 S1))
```

This has no typing rule (returns an explicit error), no computation
rule, and no elimination form. It exists so that users can explore
custom type-theory extensions without changing the AST enum — the
`name` and `args` fields carry arbitrary data.

Gated on `theory-extensions-enabled`.

---

## H.1 scaffolding (unchanged)

Phase H.1 added a generic HIT declaration registry:

```lisp
(declare-hit 'Circle
  (list (hit-constructor 'base '()))
  (list (hit-path 'loop 'Circle 'base 'base)))
```

This registry is separate from the built-in S¹ and truncation nodes.
It stores declaration metadata (name, constructors, paths) but has
no computation rules, no recursor, and no typing rules beyond
declaration validation. It exists for exploration and documentation.

The H.1 AST nodes (`AST_TT_HIT_DECL`, `AST_TT_HIT_CONSTRUCTOR`,
`AST_TT_HIT_PATH`, `AST_TT_HIT_REF`, `AST_TT_HIT_APP`) are always
available (not gated).

---

## Phase summary

| Phase | What landed |
|---|---|
| H.1 | Generic HIT declaration registry (AST nodes, no computation) |
| H.2 Turn 1+2 | Propositional truncation: type former, constructor, eliminator, beta rule |
| H.2 Turn 3 | Propositionality coherence: 4-arg `trunc-elim` with structural is-prop check |
| Scaffold | S¹ (`S1`, `base`, `loop`) with minimal typing spine |
| Scaffold | Generic `theory-extension` node |
| Infrastructure | Scaffold/checked discipline, `CLAIMS_MATRIX.md`, logic-rule gating |

---

## Examples to read

| Example | Topic |
|---|---|
| `examples/62-truncation.lisp` | Full propositional truncation walkthrough |
| `tests/tt_truncation_test.c` | C-level tests: typing, beta, determinism, 4-arg form |
| `tests/tt_optional_extensions.lisp` | Scaffold forms: S¹, truncation, theory-extension |
| `tests/tt_hit_test.c` | H.1 generic declaration registry |

For the scaffold/checked discipline:

| Document | What it covers |
|---|---|
| `docs/CLAIMS_MATRIX.md` | Per-feature status (implemented / scaffold / checked) |
| `docs/OPTIONAL_PROOF_SCAFFOLDS.md` | Migration path from scaffold to kernel |

---

## Honest gaps (full list)

These are not bugs — they're documented limitations:

1. **Propositionality of C in 3-arg `trunc-elim`**: not checked. Use
   the 4-arg form with an explicit witness for the checked version.

2. **Level semantics**: the `level` parameter in `(Trunc level A)` is
   structural. lizard doesn't verify that level=-1 means propositional
   or that level=0 means set. All levels behave identically at the
   computation level.

3. **Kan composition with truncation**: not implemented. `comp` with
   a `Trunc`-typed term as the type former doesn't fire.

4. **S¹ computation**: no recursor, no loop-case reduction, no Kan
   composition. Scaffold only.

5. **Soundness**: no proof of soundness for the combined system
   (cubical + modal + lattice + HIT + truncation). The rules are
   standard, but the metatheory is unproven.

6. **Canonicity**: closed terms of truncation type don't necessarily
   reduce to canonical form (i.e., a literal `trunc`). The normalizer
   is best-effort.

7. **Decidability of type-checking**: not proven. The kernel terminates
   on all programs we've tested, but no termination proof exists.

These gaps are tracked in `docs/CLAIMS_MATRIX.md` and updated
whenever a feature changes tier.
