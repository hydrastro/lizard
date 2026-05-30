# Lizard — Modal Type Theory Layer

This document covers the **modal logic** layer of lizard (Phases M.5.1
through M.5.9). It builds on the lambda cube (M.2), the named-logic
bundles (M.3), and the substructural toggles (M.4), and shares
infrastructure with the universe lattice (L.1–L.5) and HIT scaffolding
(H.1).

Read this if you want to:

- Use lizard's `Box` and `Diamond` type constructors
- Understand which modal logic (K / T / S4 / S5) is in effect
- Toggle modal axioms individually (`modal-4-axiom`, `modal-5-axiom`,
  `t-axiom-enabled`, `modal-symmetric`)
- Engage with the symmetric S5 (Pfenning-Davies) judgment-kind
  discipline
- Understand what's checked and what's still on the wishlist

---

## What's a modal type theory?

Modal logics extend propositional/intuitionistic logic with two
operators:

- **□A ("Box A", necessity)** — A is true under all (accessible)
  worlds
- **◇A ("Diamond A", possibility)** — A is true in some (accessible)
  world

The strength of the modal logic depends on which **axioms** you adopt:

| Axiom | Statement | Reading |
|---|---|---|
| K | □(A→B) → □A → □B | Necessity distributes over implication |
| T | □A → A | Necessary implies actual |
| 4 | □A → □□A | Necessity is necessarily necessary |
| 5 | ◇A → □◇A | Possibility is necessarily possible |

The standard four propositional modal logics combine these:

| Logic | T | 4 | 5 | Notes |
|---|---|---|---|---|
| K | no | no | no | The minimal modal logic |
| T | yes | no | no | Adds reflexive accessibility |
| S4 | yes | yes | no | Transitive: nested boxes collapse |
| S5 | yes | yes | yes | Symmetric: possibility is robust |

In lizard, all four are operationally distinct. Each is a named
bundle (`(set-logic 'K)` through `(set-logic 'S5)`) that wires the
right combination of toggles.

---

## The toggles

Modal axioms in lizard are individually controllable. Each is a logic
rule that defaults to ON (per lizard's default-allow convention) and
can be enabled/disabled directly.

| Toggle | Default | What it controls |
|---|---|---|
| `modalities-enabled` | on | Master switch for Box, Diamond, and all modal forms |
| `modal-strict-typing` | on | Strict S4-style context discipline for box-intro |
| `modal-4-axiom` | on | The 4-axiom in box-intro: valid hypotheses survive nested boxes |
| `modal-5-axiom` | on | The 5-axiom in let-diamond: unboxed diamond content is valid |
| `t-axiom-enabled` | on | The T-axiom in unbox: extraction is allowed |
| `modal-symmetric` | on | Pfenning-Davies symmetric S5 judgment-kind discipline |

The named bundles set these explicitly:

| Bundle | strict | 4 | 5 | T-ax | sym |
|---|---|---|---|---|---|
| K | on | off | off | off | off |
| T | on | off | off | on | off |
| S4 | on | on | off | on | off |
| S5 | on | on | on | on | on |
| modal-STLC | on | on | off | on | off |

---

## The forms

Lizard provides modal forms at three layers: type constructors,
asymmetric intro/elim, and symmetric intro/coercion.

### Type constructors

```lisp
(Box T)       ;; type of necessarily-T values
(Diamond T)   ;; type of possibly-T values
```

Both live at the same universe level as T.

### Asymmetric forms (always available; M.5.2–M.5.8)

These work the way the M.5.* phases through M.5.8 introduced them.
They're available in all modal logics.

```lisp
(box e)                    ;; intro: produces a Box value
(unbox x b body)           ;; elim: binds x and evaluates body
(diamond e)                ;; intro: produces a Diamond value
(let-diamond x d body)     ;; elim: binds x and evaluates body

(box-app f a)              ;; K-axiom application: Box (A→B) → Box A → Box B
(diamond-bind f d)         ;; Kleisli composition: (A → Diamond B) → Diamond A → Diamond B
```

Beta rules:

```
(unbox x (box e) body)              → body[e/x]
(let-diamond x (diamond e) body)    → body[e/x]
(box-app (box f) (box a))           → (box (@ f a))
(diamond-bind f (diamond a))        → (@ f a)
```

### Symmetric S5 forms (M.5.9; gated on `modal-symmetric`)

These implement the Pfenning-Davies three-judgment discipline. They
require `modal-symmetric` on (i.e. the S5 bundle, or explicit enable).

```lisp
(poss-coerce e)            ;; shift judgment from TRUE to POSS
(dia e)                    ;; symmetric Diamond intro: requires POSS body
```

Operational behavior:

```
(reduce (poss-coerce e))   → reduces to e (it's a judgment-level no-op)
(reduce (dia e))           → (dia e_reduced) (the dia form preserves shape)
```

---

## The Pfenning-Davies symmetric calculus

The intuitive reading: in the asymmetric form, you can write
`(let-diamond x d x)` to "extract" a diamond's contents, and the
result is just a regular value. That's loose — you've thrown away the
modality.

The symmetric form makes you track this. The judgment shifts:

| Form | Judgment kind | Reading |
|---|---|---|
| `e : A`  | TRUE | A is true in the current world |
| `(poss-coerce e) : A` | POSS | A is possibly true |
| `(dia (poss-coerce e)) : Diamond A` | TRUE | The Diamond value is true in the current world |

In lizard, the judgment kind is tracked in the kernel but isn't
visible from Lisp — you see only the result type. The kind controls
which compositions typecheck:

- `(dia e)` requires e's kind to be POSS
- `(poss-coerce e)` requires e's kind to be TRUE
- `(box e)` under `modal-symmetric` requires e's kind to be TRUE
  (rejects boxing of poss values)
- `(let-diamond x d body)` propagates body's kind to the result

The result: a clean opt-in discipline. Old asymmetric code
(`(let-diamond x d x)` and friends) continues to work because TRUE
propagates through. New symmetric code engages by writing
`(poss-coerce e)` or `(dia e)`.

### The full symmetric pipeline

```lisp
(set-logic 'S5)

(dia (let-diamond x (diamond (U 0)) (poss-coerce x)))
;; => (Diamond (U 1))

;; Step by step:
;;   (diamond (U 0))               : Diamond (U 1)    kind TRUE
;;   (let-diamond x (diamond ...)  : binds x in valid context (5-axiom)
;;     (poss-coerce x))            : (U 1)            kind POSS
;;   let-diamond returns           : (U 1)            kind POSS (propagated)
;;   (dia ...)                     : (Diamond (U 1))  kind TRUE
```

This is the canonical symmetric S5 computation: a possibility is
opened (let-diamond), its content is shifted to poss (poss-coerce),
and re-closed under Diamond (dia). The kinds flow through the chain.

---

## Implementation details

### The three-context kernel API

The kernel exposes both two-context and three-context entry points,
plus kind-aware variants:

```c
/* Two-context: legacy and most-common path. */
lizard_ast_node_t *lizard_tt_infer2(valid_ctx, truth_ctx, t, heap);

/* Three-context: includes poss context (Ω). */
lizard_ast_node_t *lizard_tt_infer3(valid_ctx, truth_ctx, poss_ctx, t, heap);

/* Kind-aware: writes the judgment kind through out_kind (optional). */
lizard_ast_node_t *lizard_tt_infer2_kind(
    valid_ctx, truth_ctx, t, heap,
    lizard_judgment_kind_t *out_kind);   /* NULL-able */

lizard_ast_node_t *lizard_tt_infer3_kind(
    valid_ctx, truth_ctx, poss_ctx, t, heap,
    lizard_judgment_kind_t *out_kind);
```

The kind enum:

```c
typedef enum {
  LIZARD_KIND_TRUE  = 0,   /* default for all existing rules */
  LIZARD_KIND_VALID = 1,
  LIZARD_KIND_POSS  = 2
} lizard_judgment_kind_t;
```

By design, the kind defaults to TRUE if a rule doesn't set it
explicitly. This matches the implicit assumption of all pre-M.5.9
code — every old rule produces TRUE-kind without needing to know.

### How Ω is encoded

Pfenning-Davies puts at most one "poss hypothesis" in Ω. Lizard
doesn't enforce this structurally — instead, the kind propagation
through `let-diamond` and the kind check at `dia`'s boundary together
encode the same discipline at the *kind* level rather than the
*context* level.

This means lizard's symmetric S5 is sound for well-formed programs
but doesn't enforce the strict single-Ω invariant syntactically. For
the type-checking decisions, the two are equivalent; for a strict
metatheoretic treatment, they're not. See LIMITATIONS.md.

### Backward compatibility

Symmetric S5 was added without breaking any existing code:

- All pre-M.5.9 AST nodes keep their pre-M.5.9 typing rules
- The new symmetric forms (`dia`, `poss-coerce`) are entirely new AST
  nodes — they don't shadow anything
- `box-intro` under `modal-symmetric` adds a kind check, but every
  existing rule produces TRUE so the check is a no-op for old code
- `let-diamond` now propagates the body's kind, but TRUE bodies still
  give TRUE results (M.5.5 contract)

---

## Phase summary

| Phase | What landed |
|---|---|
| M.5.1 | Box and Diamond as type constructors with universe-preserving typing |
| M.5.2 Turn 1 | Box intro/elim AST nodes, beta reduction |
| M.5.2 Turn 2 | Dual-context kernel (Δ; Γ) for strict S4 |
| M.5.3 | Named bundles K, T, S4, S5, modal-STLC |
| M.5.4 | `modal-4-axiom` toggle distinguishes T from S4 |
| M.5.5 Turn 1 | Diamond intro/elim with beta `(let-diamond x (diamond e) body) → body[e/x]` |
| M.5.5 Turn 2 | `modal-5-axiom` toggle distinguishes S5 from S4 |
| M.5.6 | K's distinguished elim: `t-axiom-enabled` toggle + `box-app` (K-axiom) |
| M.5.7 | Reverse-lookup memory (`current-logic` remembers explicit set-logic) + dependent Pi in box-app |
| M.5.8 | Hygiene fix for box-app's fresh-name + `diamond-bind` (Diamond Kleisli) |
| M.5.9 Turn 1 | Symmetric S5 infrastructure: AST nodes for `dia`/`poss-coerce`, toggle, three-context API wrappers |
| M.5.9 Turn 2a | Judgment-kind tracking infrastructure (Approach A: pair return via optional out parameter) |
| M.5.9 Turn 2b | Symmetric S5 typing rules: `poss-coerce`, `dia`, kind propagation in `let-diamond`, kind check in `box-intro` |
| M.5.9 Turn 3 | This documentation, comprehensive worked example, polish |

---

## What's not checked

Lizard checks types but doesn't prove metatheoretic properties. The
modal layer specifically does not establish:

- **Soundness** of any modal logic. The rules implement the standard
  presentations but no soundness proof has been formalized for the
  combined system (lattice + modal + HIT + lambda cube).

- **Canonicity** — closed terms of inductive type don't necessarily
  reduce to canonical form.

- **Termination** of the reduction relation. The normalize routine
  doesn't have a termination proof; pathological terms could loop.

- **Decidability of type-checking**. The kernel is a decidable
  algorithm in practice (it terminates on all programs we've tried),
  but no proof of decidability exists.

- **Categorical laws** for the Box-as-comonad and Diamond-as-monad
  structure. The operations are definable at the right types and
  specific equational witnesses fire via beta reduction (see example
  55), but the laws aren't proven generally.

- **Strict Pfenning-Davies single-Ω invariant**. Encoded via kind
  propagation rather than syntactic restriction (see above).

These are honest gaps. For thesis-grade work that depends on any of
the above, lizard's modal layer is best viewed as a testbed for
exploring the syntactic discipline, not as a verified type checker.

---

## Examples to read

| Example | Topic |
|---|---|
| `examples/47-modalities.lisp` | Box and Diamond constructors (M.5.1) |
| `examples/48-box-intro-elim.lisp` | Box intro/elim and beta |
| `examples/49-strict-s4.lisp` | Dual-context discipline |
| `examples/51-modal-4-axiom.lisp` | T vs S4 via the 4-axiom |
| `examples/52-diamond-intro-elim.lisp` | Diamond intro/elim |
| `examples/53-modal-5-axiom.lisp` | S4 vs S5 via the 5-axiom |
| `examples/54-k-distinguished-elim.lisp` | K's restricted elim, `box-app` |
| `examples/55-comonad-monad.lisp` | Comonad/monad witnesses for Box/Diamond |
| `examples/56-s5-lookup-and-dep-pi.lisp` | Reverse-lookup memory + dependent Pi |
| `examples/57-symmetric-s5.lisp` | Diamond's structural dual (`diamond-bind`) |
| `examples/58-symmetric-s5-turn1.lisp` | Symmetric S5 infrastructure |
| `examples/59-symmetric-s5-turn2a.lisp` | Judgment-kind tracking |
| `examples/60-symmetric-s5-turn2b.lisp` | Symmetric typing rules |
| `examples/61-symmetric-s5-walkthrough.lisp` | Full M.5 walkthrough |

For C-level usage, see `tests/tt_modal*_test.c`,
`tests/tt_symmetric_s5*_test.c`.
