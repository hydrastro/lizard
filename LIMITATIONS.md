# Lizard — Limitations

This document is candid about what lizard does, doesn't, and can't (in
this codebase's current state) do. It's organized from "small,
fixable" through "deliberately out of scope" to "would require a
research project."

If you're evaluating lizard for thesis support, this is the document
to read carefully. The capability statements in `README.md` are
accurate but optimistic in tone; this document is the corresponding
pessimistic accounting.

## Quick summary

| Category | Status |
|---|---|
| Scheme core | Solid. Standard features, tests pass, real-world usage in examples. |
| λΠ type checker | Solid in scope. Bidirectional, cumulativity works, decidability not proved. |
| Lambda cube (M.2) | All 8 corners + CoC available as bundles. STLC/F/LF/λ-P/F-omega/λ-omega/λ-P2/λ-P-omega/CoC. |
| Named-logic bundles (M.3) | 19 bundles spanning cube + substructural + features + modal logics. |
| Substructural rules (M.4) | weakening/contraction/exchange toggles + linear-STLC/affine-STLC/relevant-STLC bundles. |
| Universe lattice (L.1–L.5) | Pi-fresh / co-pi-fresh dimension-creating, lattice and couniverse, shared fresh-dim counter. |
| Observational HoTT engine | 23 reduction rules. Covers core Id manipulations + per-type-former Id and xport. |
| Cubical core (Path, faces) | Solid. Connection operations and face entailment work. |
| Kan composition | Working for the common per-type-former cases; some specific rules are partial (see below). |
| Glue types | Single-face Glue. Reduction and typing work. |
| ua | Type rule sound. Reduction for `id-equiv` is complete; general case is incomplete. |
| HIT scaffolding (H.1) | AST nodes + registry for higher inductive types. **No computation rules.** |
| Modal logic layer (M.5.1–M.5.9) | K, T, S4, S5 operationally distinct. Asymmetric forms (box/unbox/diamond/let-diamond/box-app/diamond-bind) and symmetric S5 forms (dia/poss-coerce) with Pfenning-Davies judgment-kind discipline. See MODAL.md. |
| Soundness proof | None. |
| Decidability of typechecking | Not proved. |
| Categorical laws (comonad/monad) | Operations definable; specific witnesses fire via beta; laws not generally proven. |
| Strict single-Ω invariant | Encoded via kind propagation, not enforced syntactically. |
| HITs with computation rules | Not yet (H.2 pending). |

## 1. Things deliberately out of scope

These are *design decisions* — not bugs, but real limitations:

**Faces are represented as inhabiting `(U 0)` rather than a separate
sort.** A stricter type theory would put faces in their own decidable
propositional sort. Our simplification is sound for typing purposes
(faces and types are disjoint in actual use), but it doesn't preserve
the sort distinction.

**Equivalences are `(equiv-fun, equiv-inv)`, not `Σ f. isEquiv f`.**
Real CCHM uses contractible-fibers equivalences, which carry coherence
data needed for the full comp Glue rule. Our representation:
- ✓ Sufficient for stating `(ua e) : Path U A B`.
- ✓ Correct for `id-equiv` (where forward and backward are obviously
  inverse).
- ✗ Doesn't capture that arbitrary `equiv-fun ∘ equiv-inv` is path-equal
  to identity.

**Single-face Glue.** `(Glue A φ T e)` takes one face only. The general
`ua` definition in CCHM uses Glue with a *system* of two clauses
(`(i=i0) ↦ (A, e)` and `(i=i1) ↦ (B, id-equiv B)`). We could express
the unary case but not the binary case as a single Glue term.

**HITs are not supported.** No support for higher inductive types
(S¹, suspension, truncation, pushouts). Adding HITs would require:
- New AST node forms for HIT introductions
- Per-HIT computation rules
- The interaction of comp with HIT constructors (this is where
  Cubical Agda spends substantial implementation effort)
- Generic HIT framework or per-HIT special-casing

## 2. Things that work but are incomplete

These are *capabilities that exist but have honest gaps*:

### comp Pi: non-dependent only

```c
comp^i (Pi x:A. B) [φ ↦ u] u0   -->   Lambda y. comp^i B [...] (u0 y)
```

We implement this rule only when `B` doesn't mention `x` (the
non-dependent case). The dependent case requires transporting the
argument `y` backward across the type family's domain — using `fill`
on the domain line — which the current `fill` doesn't compute.

**What works:** `comp^i (Pi x:A. B) ...` when B doesn't mention x.
**What doesn't:** general dependent comp Pi.

### comp Sum: empty partial only

```c
comp^i (Sum A B) [φ ↦ u] (Inl x)  -->  Inl (comp^i A [...] x)
```

We push comp through canonical sum constructors only when the partial
is empty (`system-nil` or `F0` face). The general case requires
case-analysis on the partial at each face.

### comp Path: works via systems

We build the inner system with three clauses correctly. But the inner
comp may not fully reduce — it depends on the specifics of the partial
element.

### comp Glue: simplified

We produce `(glue-intro φ T-comp A-comp)` with the right shape, but the
T-comp uses `equiv-fun ∘ u0` instead of the full CCHM construction via
contractibility. This is correct for `id-equiv` (where `equiv-fun = id`)
but doesn't construct the proper coherence proof for general
equivalences.

### `(ua e) @ i` for non-identity equivalences

```c
(ua (id-equiv A)) @ i  -->  A         ✓ works
(ua e) @ i              -->  (stays unreduced for non-id e)
```

The general case would emit a Glue type with a two-clause system, which
needs the multi-face Glue extension above.

### `fill` is now reducing

`(fill A φ u u0)` now has a reduction rule. The CCHM rule:

```
fill A [φ ↦ u] u0  =  <k> comp A_narrowed [φ ↦ u_narrowed,
                                            (k=i0) ↦ <i>u0] u0
```

is implemented: fill produces a path-abs binding a fresh `k`, with the
type family and partial narrowed by `(i ∧ k)` and an extra system
clause pinning the `k = i0` face to the base. For constant families
and empty partials this collapses to `<k> u0`. For non-trivial inputs
the inner comp chains through the comp rules of the underlying type.

### Face entailment is sound but incomplete

`face-entails? φ ψ` returns one of:
- `#t` — definitely entails
- `#f` — definitely does not
- `'unknown` — can't decide from the structure

For example, `(φ ∧ ψ) ⊨ φ` returns `#t` (left-projection of `F-and`),
but more complex cases like distributing over disjunctions can return
`unknown` even when the entailment holds.

This is intentional — sound but incomplete, which is the right
behavior for a decision procedure that can't be complete.

### `path-abs` alpha-equality requires same binder name

Two path-abstractions `(<i> body1)` and `(<j> body2)` are alpha-equal
only if `i = j` as strings. The proper alpha-equivalence under
interval-variable renaming is not implemented — we'd need to extend
the binder environment machinery to track interval variables
separately.

In practice this means: use consistent binder names. Most code does
anyway.

## 3. Things lizard *cannot* claim

These are *non-claims* that should be flagged:

**Lizard does NOT have a soundness proof.** No formal verification of
the rules. No proof that the typing rules and reduction rules are
mutually consistent in the sense that well-typed terms don't reduce
to ill-typed ones. The rules are *intended* to be sound (they
transcribe published CCHM rules), but no proof is attached.

**Decidability of type checking is NOT proved.** The checker is
correct on the cases it handles — but we don't have a proof that
every well-formed term either type-checks or fails in bounded time.
For some pathological inputs the normalization could in principle
loop (though in practice on realistic terms it doesn't).

**Canonicity is NOT proved.** A central correctness property for a
type theory is: every closed term of base type reduces to a canonical
form. We demonstrate this property *holds for specific examples* (the
end-to-end ua-id-equiv transport), but we don't have a general proof.

**Normalization termination is NOT proved.** No proof that reduction
always terminates. We rely on structural decrease in practice but
nothing prevents a cycle in principle.

**Strict regularity is NOT enforced.** CCHM requires that
`comp [F0 ↦ ⊥] u0 ≡ u0` *definitionally* — not just up to reduction.
We implement it as a reduction rule; the *definitional* version
requires that the equality is built into convertibility checking at
a deeper level than rule firing.

## 4. Things lizard is honestly not, despite the cubical layer

It is not Cubical Agda. It is not RedTT. It is not cubicaltt. Those
systems have:

- Thousands of lines of code (lizard's cubical layer is hundreds)
- Years of development by full-time research engineers
- Published soundness proofs
- Decidable typechecking proven
- Full HIT support
- Polished surface syntax (lizard's is s-expressions)
- Industrial-scale proof developments using them

Lizard's cubical layer is a six-turn implementation of the core CCHM
pipeline. It demonstrates the *mechanism* but is not a substitute for
production proof assistants.

## 5. What lizard *is* good for

Despite all the above, lizard is honestly useful for:

- **Understanding cubical type theory** by reading and running small
  examples that show each rule firing.
- **Experimenting with rule variations** via the flag system — every
  reduction rule is toggleable, so you can disable parts of the engine
  and see how the rest behaves.
- **Pedagogy** — the cubical layer is small enough to read in a sitting,
  with each rule clearly labeled and documented inline.
- **Thesis support** for type-theory work that doesn't need full
  industrial features — like couniverse-stratified frameworks where the
  implementation supports the framework rather than being the
  contribution.
- **A platform for further development** — the architecture is
  consistent enough that adding a new construct is a checklist of
  10 places, not an open-ended refactor.

## 6. If you want to push further

The natural next-turn items, in increasing order of effort:

1. **Sharper face entailment.** Improve the decision procedure to
   handle more cases of `(φ_1 ∨ φ_2) ⊨ ψ` and similar. Bounded work.

2. **Glue-with-system AST extension.** Replace single-face Glue with
   one that takes a system of (face, T, e) clauses. Would let the
   general `(ua e) @ i` rule construct the proper interior Glue.
   Maybe 2-3 turns.

3. **Dependent comp Pi.** Implement `fill` on the domain line and use
   it to transport the argument backward. Then update comp Pi.
   Probably 2 turns.

4. **HIT framework.** Generic mechanism for declaring higher
   inductive types and their computation rules. Significant — multiple
   turns, with subtle interactions with comp.

5. **Soundness proof.** Would require either implementing the type
   theory in a proof assistant or building lizard atop a verified
   reduction-rule kernel. Months of work, not a single push.

6. **Decidability of typechecking proof.** Same magnitude as
   soundness.

For all of (4)-(6), the realistic statement is: these are research
projects, not implementation tasks. Lizard is an artifact suitable
for pushing through (1) and (2), arguably (3), and not designed to
support (4)-(6) within its current architecture.
