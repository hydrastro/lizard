# lizard — Comprehensive Roadmap

This document maps the full vision for lizard: a C89 Scheme that is
simultaneously a practical programming language and a research vehicle
for type theory. It unifies four traditions ("tracks") under one
runtime.

## Vision

```
                        lizard
                          │
        ┌─────────────┬───┴───┬─────────────┐
        │             │       │             │
   Track R        Track C  Track K      Track Q
   (Racket)      (Clojure) (Lean/Coq) (Cubical)
   macros &      data &    proofs &   homotopy
   syntax        concurrency kernel    type theory
```

The goal: a single language where you can write a web server (Track C
data structures + concurrency), prove it correct (Track K kernel),
metaprogram its boilerplate away (Track R macros), and reason about
its equalities up to homotopy (Track Q).

## Status Summary (current)

| Track | Progress | C foundation | Lisp library |
|-------|----------|-------------|-------------|
| R (Racket) | 45% | syntax objects | syntax-tools.lisp |
| C (Clojure) | 70% | pvec, phash, atoms, transients | concurrent.lisp |
| K (Lean/Coq) | 93% | kernel, tactics, metas, unify | proof.lisp |
| Q (Cubical) | 40% | Path, comp, transport, Glue | hott.lisp |
| Engineering | 85% | VM, GC, modules, profiler | data/algo/matrix/streams |

## Phase Plan

### Phase 1 — Foundations ✅ COMPLETE
- [x] C89 Scheme core (closures, tail calls, macros)
- [x] Bignum arithmetic (GMP-style)
- [x] Tree-walking evaluator + REPL
- [x] Module loader with search paths
- [x] Garbage collector (mark + segment sweep)
- [x] Bytecode VM with TCO + profiler
- [x] Structured diagnostics (source locations)

### Phase 2 — Data & Proofs ✅ COMPLETE
- [x] Persistent vectors + hash maps
- [x] Atoms + transients
- [x] Lazy evaluation (delay/force)
- [x] Exceptions (raise/guard/try)
- [x] Trusted kernel: 10 types, 7 eliminators
- [x] Metavariables + first-order unification
- [x] Tactics: intro/exact/refl/assumption/simpl/split/left/right
- [x] Kernel definitions (type-checked)

### Phase 3 — Track Completion (IN PROGRESS)

#### Track K → 100% (nearest to done)
- [ ] **Inductive families** — Vec(A,n), Fin(n) indexed by Nat
  - Dependency: extend kernel term repr with index args
  - Effort: medium (new KT_ tags + computation rules)
- [ ] **Induction tactic** — generate subgoals from an eliminator
  - Dependency: motive inference
- [ ] **Rewrite tactic** — use an equality to rewrite the goal
  - Dependency: J-eliminator + motive synthesis
- [x] **Tactic combinators** — tseq, tfirst, ttry, trepeat (lib/tactics-ext.lisp)
  - Effort: small (Lisp-level over existing tactics)
- [ ] **Higher-order unification** — solve metas in function position
  - Effort: large (Miller patterns)

#### Track C → 100%
- [ ] **Futures** — (future expr) parallel thunk
  - Dependency: thread-safe heap OR cooperative scheduler
- [ ] **Agents** — async actors with message queues
  - Dependency: event loop
- [ ] **Channels** — CSP-style communication
- [ ] **STM** — software transactional memory over atoms

#### Track R → 100%
- [ ] **Ellipsis in syntax-rules** — (p ... ) pattern repetition
  - Dependency: pattern variable depth tracking
  - Effort: medium
- [ ] **syntax-case** — procedural macros with fenders
  - Dependency: ellipsis + hygiene
- [ ] **Hygiene** — automatic renaming via scope sets on syntax objects
  - Dependency: scope set marks (partially scaffolded)
- [ ] **provide/require** — module namespaces with selective export
  - Effort: medium (env filtering)
- [ ] **#lang** — per-file reader extensions

#### Track Q → 100%
- [ ] **Dependent composition** — full comp for all type formers
  - Dependency: face lattice + systems (scaffolded)
- [ ] **Complete face entailment** — decision procedure for I-faces
- [ ] **Glue/ua computation** — unglue reduction + univalence transport
- [ ] **HIT recursors** — eliminators for S¹, truncation, pushouts
- [ ] **Cubical kernel bridge** — connect AST cubical ops to kernel terms

### Phase 4 — Integration & Polish
- [ ] **Unified kernel** — one term representation for all four tracks
- [ ] **Self-hosting reader** — lizard reader written in lizard
- [ ] **Standard library freeze** — stable, documented API
- [ ] **Package manager** — fetch/install community libraries
- [ ] **FFI** — call C from lizard, embed lizard in C
- [ ] **LSP server** — editor integration (completion, type-on-hover)
- [ ] **Optimizing compiler** — bytecode → native via C codegen

### Phase 5 — Research Frontiers
- [ ] **Two-level type theory** — internal/external distinction
- [ ] **Cubical HITs** — circle, sphere, suspension, pushouts
- [ ] **Univalence as computation** — transport across equivalences
- [ ] **Observational equality** — alternative to cubical
- [ ] **Proof search** — automated tactic application (hammer)

## Dependency Graph (critical path)

```
syntax objects ──→ ellipsis ──→ syntax-case ──→ hygiene
                                                    │
kernel ──→ metas ──→ unification ──→ HO-unification │
   │                      │                         │
   │                      ▼                         ▼
   └──→ tactics ──→ induction ──→ rewrite ──→ proof search
   │
   └──→ inductive families ──→ Vec/Fin ──→ verified stdlib
   
cubical core ──→ face entailment ──→ dependent comp ──→ HITs ──→ univalence
```

## Design Principles

1. **Trusted core stays small** — the kernel (~1100 lines) is the only
   code that must be correct for soundness. Everything else builds on it.
2. **Scaffold then verify** — features land first as AST operations
   (scaffold), then get kernel verification (checked). The CLAIMS_MATRIX
   tracks which is which.
3. **C89 strict** — maximum portability. No C99/C11 features.
4. **Persistent by default** — data structures are immutable; mutation
   is opt-in (atoms, transients).
5. **Pure-Lisp libraries when possible** — features that can be
   expressed in lizard itself stay out of the trusted C core.

## How to Contribute

Pick an unchecked box above. Most Track-completion items have a clear
dependency chain. The lowest-effort high-impact items right now:

1. **Tactic combinators** (Track K) — pure Lisp over existing tactics
2. **provide/require** (Track R) — env filtering in the module loader
3. **Futures** (Track C) — cooperative scheduler, no threads needed
4. **HIT recursors** (Track Q) — follow the natrec/listrec pattern

See `docs/ARCHITECTURE_ROADMAP.md` for the original cross-paradigm
feature mapping and `docs/NEXT_STEPS.md` for the working task list.

## Performance Optimization (bignum power loops)

The multiply (`*`) and subtract (`-`) primitives now have a two-argument
fast path that skips copying the accumulator. For a tail-recursive power
loop like:

```scheme
(define (iter a b acc) (if (= b 0) acc (iter a (- b 1) (* acc a))))
(power-tail 123 12312)   ; 123^12312, a ~25,000-digit number
```

the old code copied the 25,000-digit accumulator (mpz_set) on every one
of the 12,312 iterations before multiplying. The fast path multiplies
directly into a fresh result (mpz_mul(result, acc, b)), eliminating that
per-iteration O(digits) copy. `+` was already optimal (accumulates into a
zero-initialized result).
