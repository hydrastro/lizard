# Lizard

```
___                     .-*''*-.
 '.* *'.        .|     *       _*
  _)*_*_\__   \.`',/  * EVAL .'  *
 / _______ \  = ,. =  *.___.'    *
 \_)|@_@|(_/   // \   '.   APPLY.'
   ))\_/((    //        *._  _.*
  (((\V/)))  //            ||
 //-\\^//-\\--            /__\
```

A C89 Scheme interpreter with a small cubical type theory implementation
built on top. Three things in one codebase:

1. A working Scheme implementation: bignums (via GMP), tail-call
   optimization, lexical closures, syntax-rules macros with hygiene,
   exception handling, vectors, hash tables.

2. A bidirectional type checker for λΠ extended with most of Martin-Löf
   type theory's term formers (Sigma, Sum, Unit, Bot, Id) and a universe
   lattice with cumulativity.

3. A cubical type theory layer (CCHM-style) on top of that: interval,
   paths, faces, partial elements, Kan composition, Glue types, and `ua`
   — with computational univalence working end-to-end in the canonical
   case.

Six focused turns of cubical work, each adding a layer in the standard
CCHM pipeline. The result is a small but coherent cubical type checker.

For the precise scope of what works, what's incomplete, and what's
deliberately out of reach, see `LIMITATIONS.md`. For how the pieces
fit together, see `DESIGN.md`.

## Build

Requires GCC (or compatible C89-clean compiler) and GMP.

With Nix:
```
nix-shell --run make
```

Without Nix (assuming GMP installed at standard prefix):
```
make
```

If GMP is at a non-standard location:
```
CPPFLAGS="-I/path/to/gmp/include" LDFLAGS="-L/path/to/gmp/lib" make
```

Binary lands at `build/lizard`. Tests:
```
make test
```

## Run

```
./build/lizard              # REPL
./build/lizard file.lisp    # run a script
./build/lizard < file.lisp  # via stdin
```

## What's in the box

### Scheme core

Standard forms: `define`, `lambda`, `let`, `let*`, `letrec`, `cond`,
`if`, `begin`, `set!`, `quote`, `quasiquote`, `unquote`,
`unquote-splicing`.

Three lambda parameter shapes: fixed (`(lambda (x y) ...)`), varargs
(`(lambda args ...)`), and mixed (`(lambda (first . rest) ...)`).

Bignums via GMP — arithmetic on arbitrary-precision integers.

Tail-call optimization for direct self-recursion and mutual recursion.

Macros: lizard-original transformer lambdas via `define-syntax` with
explicit AST manipulation, plus standard `syntax-rules` with basic
hygiene. `gensym` for fresh symbols.

Exception handling: `raise`, `with-handler`, structured error objects.

Vectors, hash tables, string operations, file I/O, character predicates,
real-number primitives.

Reflection: `type-of`, environment introspection, AST inspection.

### Dependent type checker

A bidirectional checker (`infer` and `check`) for λΠ with:

- Variables, universes, Pi formation, lambdas, applications
- Annotation forms `(annot t T)`
- Sigma, Sum, Unit, Bot — with introduction and elimination rules
- Identity types `(Id A a b)` with `refl` and `J` (path induction)
- Universe lattice: concrete `(U n)`, variables `(U-var 'i)`,
  successor `(U-suc u)`, max `(U-max u v)`, with cumulativity
- Convertibility check that respects alpha-equivalence and runs full
  normalization

### Observational HoTT-fragment engine

Reduction rules for the observational fragment:
- Identity algebra: `sym`, `trans`, `transport`, all with their
  computation rules
- `ap` as a functor on paths (commutes with refl, sym, trans)
- `J` reducing on `refl`
- Id-on-type-formers: `Id (Pi x A B) f g → Pi x A (Id B (f x) (g x))`
  (functional extensionality at the computation level)
- Id on Sigma, Sum, Unit
- `xport` per type former

### Cubical layer

Six turns of CCHM-style cubical:

**Turn 6 — Interval and paths.** Interval pre-type `I` with endpoints
`i0`, `i1`, connection operations (`I-and`, `I-or`, `I-neg`) with their
equations, `Path` type with `path-abs` (`<i> body`) introduction and
`path-app` (`p @ i`) elimination. Path-app beta computes by
substituting the interval point into the body. Endpoint conditions on
`path-abs` are enforced at typing time.

**Turn 7 — Faces and partial elements.** Face formulas `F0`, `F1`,
`F-eq`, `F-and`, `F-or` with the CCHM connection-on-face equations
(`(F-eq (i ∧ j) i0) → (F-or (F-eq i i0) (F-eq j i0))` and friends).
Face entailment decision procedure (returns `#t`/`#f`/`unknown`
honestly). `Partial` and `Sub` type formers.

**Turn 8 — Kan composition.** `comp`, `hcomp`, `fill` with per-type-
former reduction rules: F1 boundary collapse, `comp Unit → tt`,
`comp Sigma` decomposing into Pair of comps, `comp Pi` (non-dependent
case) becoming Lambda over composed codomain, `comp Sum` pushing
through `inl`/`inr`, `comp Path` building a path-abs with multi-face
inner partial.

**Turn 9 — Glue types and equivalences.** `Equiv A B`, `id-equiv`,
`equiv-fun`, `equiv-inv`, `Glue A φ T e`, `glue-intro`, `unglue` with
their reduction and typing rules.

**Turn 10 — ua.** `(ua e) : Path U A B` when `e : Equiv A B`. The
canonical case `(ua (id-equiv A)) @ i → A` fully computes.

**Turn 11 — Systems and comp Glue.** Multi-clause partial elements
(`system-nil`, `system-cons`) with simplification rules,
`system-lookup` decision procedure, comp over Glue types producing
`glue-intro` of constituent comps, and the chain-collapse for the
identity equivalence that gives end-to-end canonical-case
computational univalence:

```
(comp <i>((ua (id-equiv A)) @ i) F0 [] x)  →  x
```

The identity equivalence gives the identity path which gives the
identity transport, all by reduction, no postulates.

## Pluggable rules

Every reduction rule is gated on a flag — about 60 of them, each
with a clear name. `(flag-set! 'reduce-path-beta #f)` disables
path-app beta; `(flag-set! 'reduce-comp-pi #f)` disables comp Pi.
This is useful for understanding which rule does what and for
disabling individual rules during debugging or thesis-related
experimentation. Flags default to enabled.

## Examples

Examples live in `examples/`, numbered roughly by topic:

- `01-quine.lisp` through `22-benchmarks.lisp` — Scheme programs
- `23-pi.lisp` through `29-universes.lisp` — λΠ and observational HoTT
- `30-cubical.lisp` — Turn 6 walkthrough (interval and paths)
- `31-faces.lisp` — Turn 7 (faces and entailment)
- `32-comp.lisp` — Turn 8 (Kan composition)
- `33-glue-ua.lisp` — Turns 9 & 10 (Glue and ua)
- `34-systems-univalence.lisp` — Turn 11 (canonical computational
  univalence end-to-end)

Run any of them: `./build/lizard < examples/34-systems-univalence.lisp`.

## What lizard isn't

It isn't Cubical Agda. It isn't Coq. It isn't a soundness-proven
proof assistant. It's a six-turn implementation that demonstrates the
cubical computation pipeline working in the canonical case, with
honest documentation of what it doesn't do. For the precise scope of
limitations, see `LIMITATIONS.md`.

## Honest scope statement

This codebase exists primarily to support thesis work on
couniverse-stratified type theory. It is not a competitor to mature
proof assistants. What it does demonstrate is that a working cubical
computation engine can be built incrementally and that the canonical
case of computational univalence — identity equivalence transports —
runs end-to-end by reduction.

For research-grade extensions (full per-type-former dependent comp,
HITs, soundness proofs, decidable typechecking) see `LIMITATIONS.md`,
which is candid about what would be needed and what is out of scope
for the present implementation.

## License

(Set as appropriate for your thesis context. The Scheme core inherits
from the project's original licensing; the type-theory additions are
the user's work.)
