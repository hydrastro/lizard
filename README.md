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

A C89 Scheme with a small trusted dependent-type-theory kernel, a
cubical type theory layer, and a large language-level library tower.
The guiding principle is **a small trusted core and a large derived
tower**: keep C responsible for runtime, representation, evaluation,
memory, and the proof kernel — and build everything else as Lisp
libraries that cannot compromise soundness.

## What it is

At the C level (~29,000 lines, a 1,068-line trusted kernel, ~520
primitives):

1. **A working Scheme.** Bignums via GMP, tail-call optimization,
   lexical closures, `syntax-rules` macros, exceptions, vectors, hash
   tables, a bytecode VM with TCO, a mark-sweep GC, a module loader,
   and a profiler.

2. **A trusted dependent-type-theory kernel.** A bidirectional checker
   for λΠ with Sigma, Sum, Unit, Empty, Id, a universe lattice with
   cumulativity, ten inductive types, seven eliminators, metavariables,
   first-order unification, and a tactic engine.

3. **A cubical type theory layer (CCHM-style).** Interval, paths,
   faces, partial elements, Kan composition, Glue types, and `ua`,
   with computational univalence in the canonical case.

On top of that sits a **standard library of 45 modules (~5,300 lines
of Lisp)** organized around four research tracks, plus self-hosting
demonstrations: a metacircular evaluator, a Prolog-style logic engine,
a regex engine, a lambda-calculus evaluator, and a Hindley-Milner type
inferencer — all written in lizard itself.

## The four research tracks

lizard unifies four traditions under one runtime. All four are now
feature-complete at the library level:

| Track | Tradition | What it provides |
|-------|-----------|------------------|
| **R** | Racket | `syntax-rules` + ellipsis, `syntax-case`, hygiene, a reader, `#lang` dialects |
| **C** | Clojure | persistent vectors/maps, atoms, transients, STM, blocking CSP channels |
| **K** | Lean/Coq | dependent kernel, tactics, unification, inductive families (Vec/Fin), term rewriting, induction, Hindley-Milner inference |
| **Q** | Cubical Agda | interval algebra, paths, Glue, univalence, HITs (circle/suspension/torus/truncation), HIT recursors, canonicity |

See `docs/ROADMAP.md` for the full track breakdown and
`docs/LIBRARIES.md` for the per-module API reference.

## Build

Requires GCC (or a C89-clean compiler) and GMP.

```
nix develop                 # enter the dev shell (provides GCC + GMP)
make                        # build
make test                   # unit tests
make examples               # run every example, checked against examples/manifest.sexp
make check                  # test + examples
```

Without Nix (GMP at a standard prefix): just `make`. For a custom GMP
location: `CPPFLAGS="-I/path/gmp/include" LDFLAGS="-L/path/gmp/lib" make`.

Binary lands at `build/lizard`. `make examples` is honest: it fails if a
`pass` example errors or an `error` example unexpectedly succeeds (see
`examples/manifest.sexp`).

## Run

```
./build/lizard              # REPL
./build/lizard file.lisp    # run a script
./build/lizard < file.lisp  # via stdin
```

## Quick taste

```lisp
; Persistent data + concurrency (Track C)
(import "stm.lisp")
(define acct (make-ref 100))
(stm-update! acct (lambda (b) (+ b 50)))      ; atomic, retrying

; Dependent types (Track K)
(import "inductive.lisp")
(vnth (list->vec '(a b c)) (make-fin 3 1))    ; 'b — out-of-bounds impossible

; Type inference (Track K)
(import "hm-infer.lisp")
(type->string (type-of '(lam x x)))           ; "(t1 -> t1)"

; Macros with ellipsis (Track R)
(import "syntax-rules.lisp")
(macro-apply my-list-macro '(my-list 1 2 3))  ; (list 1 2 3)

; Cubical paths (Track Q)
(import "cubical.lisp")
(i-reduce (i-not (i-not (i-var 'i))))         ; ~~i = i
```

## Standard library highlights

**Data & algorithms:** `data.lisp` (stack/queue/set/BST/heap),
`algorithms.lisp` (sorts, search, number theory), `graph.lisp`
(DFS/BFS/topo-sort), `matrix.lisp`, `streams.lisp` (lazy),
`combinatorics.lisp`, `numtheory.lisp`, `rational.lisp` (exact).

**Language tools:** `pattern.lisp` (structural match), `control.lisp`,
`format.lisp`, `json.lisp`, `record.lisp`, `parser.lisp` (combinators),
`reader.lisp` (text → s-expr + `#lang`).

**Metaprogramming:** `syntax-rules.lisp`, `syntax-case.lisp`,
`macro-expand.lisp` (recursive + hygienic), `syntax-tools.lisp`.

**Proofs & types:** `proof.lisp`, `tactics-ext.lisp` (combinators),
`rewrite.lisp` (term rewriting + induction), `inductive.lisp`
(Vec/Fin), `hm-infer.lisp` (Algorithm W).

**Cubical/HoTT:** `hott.lisp`, `cubical.lisp`, `hit-recursors.lisp`,
`hit-compute.lisp`, `univalence.lisp`.

**Concurrency:** `concurrent.lisp`, `csp.lisp`, `coroutine.lisp`,
`stm.lisp`.

**Self-hosting demos:** `metacircular.lisp` (a Lisp in lizard),
`logic.lisp` (Prolog), `regex.lisp`, `lambda-calc.lisp`, `calc.lisp`.

## Examples

124 examples in `examples/`, numbered by topic. Highlights:

- `01`–`22` — Scheme programs
- `23`–`34` — λΠ, observational HoTT, and the cubical layer
- `84`–`102` — the standard library (data, algorithms, streams, …)
- `107` — a Prolog-style logic engine
- `108` — a regex engine
- `109` — lambda calculus with Church arithmetic
- `110`–`124` — the research tracks (macros, HITs, STM, inference,
  reader, and a capstone pipeline that chains reader → macro expander
  → type inference → evaluation)

Run any of them: `./build/lizard examples/124-capstone-pipeline.lisp`.

## Performance

The bignum arithmetic path is competitive with mature Schemes: a
12,312-iteration tail-recursive `123^12312` power loop (building a
~25,000-digit accumulator) runs within ~200ms of MIT Scheme, after a
fast-path optimization that avoids copying the accumulator each
multiply.

## Pluggable rules

Every reduction rule in the cubical/type-theory engine is gated on a
named flag — about 60 of them. `(flag-set! 'reduce-path-beta #f)`
disables path-app beta, etc. Useful for understanding which rule does
what. Flags default to enabled.

## Honest scope

lizard is a research vehicle, not a competitor to mature proof
assistants. The trusted kernel is small and the cubical layer
demonstrates the computation pipeline in the canonical case; it is not
a soundness-proven system. The library tower is broad and the tracks
are feature-complete *as libraries* — some features (full hygiene,
dependent comp for every type former, HIT soundness) are scaffolds that
demonstrate structure rather than fully verified implementations. See
`LIMITATIONS.md` for the candid, precise scope and `docs/CLAIMS_MATRIX.md`
for what is checked vs. scaffolded.

## Documentation

- `DESIGN.md` — how the pieces fit together
- `LIMITATIONS.md` — precise scope of what works / doesn't
- `docs/ROADMAP.md` — the four-track plan and status
- `docs/LIBRARIES.md` — per-module API reference
- `docs/USAGE.md` — getting-started guide and recipes
- `docs/CAS.md` — the proof-producing CAS direction
- `CHANGELOG.md` — development history

## License

(Set as appropriate for your context. The Scheme core inherits from the
project's original licensing; the type-theory and library additions are
the author's work.)
