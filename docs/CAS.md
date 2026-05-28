# A Proof-Producing CAS in Lizard

## The dream

A computer algebra system where every result carries a proof. You ask
for `d/dx x²` and get back not just `2x` but a **derivation** — a
finite tree of named rules — and that tree can be unfolded all the way
down to the **ZFC axioms**. The integral, the simplification, the limit:
each is a theorem, and lizard can show you the theorem's foundations.

This is the architecture lizard now provides a foundation for, in two
modules:

- `lib/cas.lisp` — the symbolic engine (simplify, differentiate,
  integrate)
- `lib/cas-proof.lisp` — the justification layer (axiom database,
  derivation trees, unfolding to ZFC)

## Why lizard is a good host for this

lizard already has the two halves a verified CAS needs:

1. **A symbolic substrate.** Scheme s-expressions ARE the natural
   representation for math expressions and for proof terms. Bignums are
   exact. Pattern matching (`lib/pattern.lisp`) and term rewriting
   (`lib/rewrite.lisp`) give you the rewrite engine a CAS is built on.

2. **A trusted proof kernel.** The dependent-type-theory kernel
   (`kernel-check`, `kernel-infer`, tactics) is the place where a
   *checked* proof can ultimately live. A CAS rule can be stated as a
   kernel theorem and discharged by the kernel — turning a "cited" rule
   into a "verified" one.

The CAS sits in the **derived tower**; the kernel is the **trusted
core**. A result is trustworthy exactly to the degree its derivation
reaches checked kernel theorems (and, below them, the axioms).

## The layered foundation

`cas-proof.lisp` encodes the dependency layers every calculus fact
rests on:

```
  Layer 5  CAS rewrite / calculus rules   d/dx xⁿ = n xⁿ⁻¹
              │
  Layer 4  the derivative                 f' = lim (f(x+h)−f(x))/h
              │
  Layer 3  limits & continuity            ε–δ over ℝ
              │
  Layer 2  field & order axioms           assoc, comm, distrib, …
              │
  Layer 1  number constructions           ℕ → ℤ → ℚ → ℝ (Dedekind cuts)
              │
  Layer 0  ZFC axioms                     extensionality, infinity, …
```

Each rule names its dependencies. `unfold-to-axioms` walks the chain
to the leaves; `print-dep-layers` prints the whole tree; and
`print-foundations` summarizes which ZFC axioms a given rule rests on.

So `d/dx x² = 2x` cites `calc-power`, which rests on `calc-product` and
`nat-construction`, which rest on `derivative-def`, `limit-def`,
`field-distrib`, …, which rest on `real-construction`, which rests on
`zfc-infinity`, `zfc-power-set`, `zfc-separation`. The black box opens
all the way down.

## What's implemented vs. scaffolded

**Implemented (computes for real):**
- Symbolic simplification with the standard identities
- Differentiation: constant, variable, sum, difference, product,
  quotient, power, chain, and sin/cos/exp/ln
- Basic polynomial integration
- Derivation trees with rule citations
- The full ZFC→calculus dependency database and its unfolding

**Scaffolded (structure, not yet machine-checked):**
- The *statements* of the axioms/lemmas are informal strings, not
  kernel terms. The next step is to state each as a kernel proposition.
- The dependency edges assert "rule X follows from Y, Z" without a
  checked proof of that entailment. Discharging those with the kernel
  is the path to a genuinely verified CAS.

## Integrating with a Maxima-like CAS

If you are writing a Maxima-style CAS in lizard, the integration points
are:

1. **Share the expression representation.** Use the same s-expression
   convention (`(+ a b)`, `(* a b)`, `(^ a n)`, `(sin e)`, …) so your
   simplifier, your differentiator, and `cas-proof.lisp` all speak the
   same language. `cas.lisp` is deliberately small so you can replace
   its engine with yours.

2. **Emit a derivation alongside each transformation.** Wherever your
   CAS applies a rule, also record `(rule-name premises)`. The
   `deriv`/`diff-proof` shape in `cas-proof.lisp` shows the pattern:
   conclusion + rule + sub-derivations.

3. **Register new rules in `foundation-db`.** Add an entry
   `(your-rule "statement" (dep ...))`. As long as the deps bottom out
   in existing rules (eventually ZFC), `unfold-to-axioms` works for
   free on your rule.

4. **Promote rules to checked theorems.** For the rules you most care
   about, state them as kernel propositions and prove them with the
   tactic engine (`begin-proof` … `qed`). Replace the informal string
   with a reference to the checked proof. Now those branches of every
   derivation are machine-verified.

## Roadmap to a verified CAS

1. **Kernel-state the axioms.** Encode ZFC (or a working subset) and
   the field axioms as kernel terms.
2. **Construct ℝ in the kernel** (or assume it as a structure with the
   field+completeness axioms), so `real-construction` becomes a real
   object, not a label.
3. **Prove the derivative rules** from `derivative-def` using the
   tactic engine; attach the proof objects to the database entries.
4. **Make the CAS proof-carrying end to end:** every `derivative`
   call returns `(result . derivation)` where the derivation type-checks
   against the kernel.
5. **Certificate checking:** a `check-derivation` pass that walks a
   derivation and confirms each step against its kernel theorem — the
   CAS analogue of proof-checking.

At that point a result like `∫₀¹ x² dx = 1/3` would come with a
certificate the kernel accepts, and you could trace it down to the
construction of ℝ and the axioms of ZFC — exactly the dream.

## See also

- `examples/125-cas-symbolic.lisp` — the symbolic engine
- `examples/126-cas-proof-to-zfc.lisp` — unfolding a derivative to ZFC
- `lib/rewrite.lisp` — term rewriting + induction (the rewrite substrate)
- `lib/proof.lisp`, `lib/tactics-ext.lisp` — the kernel-facing proof tools
