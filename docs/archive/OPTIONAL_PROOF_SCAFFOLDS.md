# Optional proof and cubical scaffolds

This repository intentionally separates two things:

1. the checked Lizard/Lisp runtime and the existing type-theory core; and
2. experimental proof-theory constructors that are useful while designing the
   next theory.

The experimental constructors in this snapshot are **opt-in scaffolds**. They are
not claimed to implement a full CCHM model, a complete higher-inductive-type
kernel, or a sound truncation theory. They give users a stable surface syntax and
runtime representation that future kernels can refine.

## Logic bundles

The new bundles are enabled with `set-logic`:

```scheme
(set-logic 'cubical-S1)       ; existing cubical core + S¹ constructors
(set-logic 'truncations)      ; existing core + truncation constructors
(set-logic 'proof-scaffold)   ; S¹ + truncations + generic extension nodes
```

The same features can also be toggled through rules:

```scheme
(logic-rule-enable 'cubical-s1-enabled)
(logic-rule-enable 'truncations-enabled)
(logic-rule-enable 'theory-extensions-enabled)
```

The constructors deliberately fail unless the corresponding rule is enabled.
This prevents experimental proof objects from silently entering stricter logics
such as `STLC`, `LF`, or `CoC`.

## Cubical S¹ scaffold

The S¹ scaffold provides the canonical surface nodes:

```scheme
(S1)      ; the circle type constructor
(base)    ; base point
(loop)    ; path from base to base
```

The type checker recognizes the following minimal typing spine when the
`cubical-s1-enabled` rule is active:

```text
S1    : U0
base  : S1
loop  : Path S1 base base
```

This is not yet a full higher-inductive type. In particular, there is no
recursor, induction principle, computation rule for the loop, or integration
with a general HIT framework yet.

## Truncation scaffold

The truncation scaffold provides the following surface nodes:

```scheme
(Trunc level A)
(trunc value)
(trunc-elim motive handler value)
```

Introspection helpers are provided for tests, printers, and future elaboration:

```scheme
(Trunc? x)
(Trunc-level x)
(Trunc-type x)
(trunc? x)
(trunc-value x)
(trunc-elim? x)
```

**As of Phase H.2, this scaffold has been partially promoted to checked
status.** The typing and reduction rules are now real (not stub errors):

- `(Trunc level A) : Universe-of-A` — universe-preserving type former.
- `(trunc x) : (Trunc A)` — infers `A` from `x : A`. Level is left
  unspecified at inference time (NULL in the AST; prints as `(Trunc A)`).
- `(trunc-elim C h e)` requires `C : Universe`, `e : (Trunc _ A)` for
  some `A`, and `h : Π _:A. C`. Result type is `C`.
- Primary computation rule: `(trunc-elim C h (trunc x)) ⟶ (@ h x)`,
  deterministic — the LHS pattern is unique and doesn't overlap with
  any other reduction.

**Honest gap (per CLAIMS_MATRIX.md):** the propositionality obligation
on the motive `C` — that any two values of `C` are path-equal — is
not structurally enforced. A future "propositionality coherence"
turn could tighten this. The current rule is sound for typing
decisions on the eliminator itself but accepts elims that may not
be justified by the propositional-truncation principle.

Higher-level truncations (n ≥ 0: set truncation, groupoid truncation,
etc.) have the same surface syntax but no additional discipline.

## Generic theory-extension nodes

The generic extension node lets experiments attach syntax without changing the
AST again:

```scheme
(theory-extension 'my-rule arg1 arg2 ...)
```

and provides:

```scheme
(theory-extension? x)
(theory-extension-name x)
(theory-extension-args x)
```

Use this for experimental logics, not for trusted kernel terms. A future kernel
should elaborate extension nodes into checked internal terms or reject them.

## Intended migration path

The current scaffolds are meant to evolve in this order:

1. keep constructors opt-in and visibly experimental;
2. attach source spans and diagnostics to failed checks;
3. add a separate TT kernel term representation;
4. elaborate surface scaffolds into kernel terms;
5. add actual recursors/eliminators and computation rules;
6. only then document any soundness/canonicity claims.

Until that happens, these features should be described as **experimental proof
scaffolding**, not as a complete proof assistant or a complete CCHM
implementation.
