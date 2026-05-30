# lizard v5 — Status & Next Steps

## Current Status

### Codebase Metrics
| Metric | Value |
|--------|-------|
| Lines of C (src/) | ~29,000 |
| Trusted kernel | ~1,100 lines |
| Registered primitives | 513+ |
| Examples | 82+ |
| Library files | 3 |
| Documentation | 10+ docs |
| Total files | 200+ |

### Track Status

#### Track R (Racket-style macros) — 40%
**Done:** Syntax objects (datum->syntax, syntax->datum, syntax-e, syntax-source, syntax?)

**Next priorities:**
1. **Ellipsis in syntax-rules** — `(syntax-rules () ((name p ...) body))` with pattern variable binding
2. **syntax-case** — procedural macros with pattern matching
3. **Module namespaces** — `(provide name ...)` and `(require "module")` with selective import
4. **Macro phases** — compile-time vs runtime separation
5. **Source-aware errors** — macro expansion errors point to original source
6. **#lang** — per-file language declarations

#### Track C (Clojure-style data + concurrency) — 55%
**Done:** Persistent vectors (7 ops), persistent hash maps (8 ops), atoms (5 ops), lazy eval (3 ops)

**Next priorities:**
1. **Transients** — mutable batch-mode for building persistent structures efficiently
2. **Futures/promises** — `(future expr)` for parallel computation
3. **Agents** — async state management with message passing
4. **Thread-safe runtime** — protect global state with mutexes

#### Track K (Lean/Coq-style proofs) — 90%
**Done:** 10 types (Nat Bool Unit Empty List Maybe Sum Pi Sigma Id), 7 eliminators with computation, metavariables, first-order unification, 8 tactics (intro exact refl assumption simpl split left right), kernel definitions, universe hierarchy

**Next priorities:**
1. **Inductive families** — Vec(A,n), Fin(n) indexed by naturals
2. **Higher-order unification** — solve metas in function position
3. **Termination checker** — ensure recursive defs terminate
4. **More tactics** — induction, rewrite, auto, omega
5. **Proof scripts** — `(proof-script (intro "x") (simpl) (refl))` as a single command
6. **Tactic combinators** — `try`, `repeat`, `all`, `first`

#### Track Q (Cubical type theory) — 30%
**Done:** Path types, faces, systems, comp (basic), Glue, transport, ua, modal logic (K/T/S4/S5), propositional truncation, S¹ scaffold, HIT registry

**Next priorities:**
1. **Dependent composition** — full comp rule for all type formers
2. **Face entailment** — complete algorithm for face lattice
3. **Glue/ua computation** — unglue reduction rules
4. **HIT recursors** — elimination principles for HITs
5. **Higher inductive families** — indexed HITs

#### Engineering — 85%
**Done:** Module loader, GC (mark + segment sweep), bytecode VM (30+ opcodes), TCO, profiler, structured diagnostics, exceptions, 50+ stdlib functions, comprehensive tests

**Next priorities:**
1. **Per-object GC** — sweep individual objects, not just segments
2. **VM coverage** — cond now compiled; let/letrec next
3. **REPL** — tab completion, readline, history
4. **FFI** — call C functions from lizard
5. **Package manager** — fetch and install libraries

## Architecture

```
                    ┌─────────────────┐
                    │   REPL / CLI    │
                    └────────┬────────┘
                             │
                    ┌────────▼────────┐
                    │  Parser/Tokenizer│
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
     ┌────────▼───────┐ ┌───▼────┐  ┌──────▼──────┐
     │  Tree-walking   │ │Bytecode│  │   Kernel    │
     │  Evaluator      │ │ VM+TCO │  │ Type Checker│
     └────────┬───────┘ └───┬────┘  │  (trusted)  │
              │              │       │  1068 lines  │
              └──────┬───────┘       └──────┬──────┘
                     │                      │
              ┌──────▼──────┐        ┌──────▼──────┐
              │ Runtime     │        │  Tactics    │
              │ (env, heap, │        │  (intro,    │
              │  modules,   │        │   exact,    │
              │  GC, atoms) │        │   refl, ... │
              └─────────────┘        └─────────────┘
```

## How to Run

```bash
make clean && make && make test
./build/lizard examples/80-grand-finale.lisp
./build/lizard examples/81-cheat-sheet.lisp
./build/lizard -e '(display (+ 1 2))'
```
