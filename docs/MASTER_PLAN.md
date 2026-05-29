# Lizard — Master Plan

This is the single canonical roadmap. The older overlapping docs
(`ROADMAP.md`, `ARCHITECTURE_ROADMAP.md`, `LIZARD_EVOLUTION_PLAN.md`,
`NEXT_STEPS.md`, `COMPILER_RUNTIME_DEBUGGER_PLAN.md`, `FEATURE_BACKLOG.md`,
`OPTIONAL_PROOF_SCAFFOLDS.md`) are archived under `docs/archive/`. The
canonical doc set is: `MASTER_PLAN.md`, `CLAIMS_MATRIX.md`,
`LIMITATIONS.md`, `LIBRARIES.md`, `USAGE.md`, `CAS.md`, plus the C-API
and type-theory references.

---


### Phase 1H progress

- Equiv / Glue / ua / system constructors have been split into `src/tt_glue.c` with `src/tt_glue.h` as the internal API.
- `tt_equality.c` still owns the actual reduction and judgmental equality rules; this phase only makes constructors reusable and removes another chunk of infrastructure from the monolith.
- The next split target is the Glue/ua reduction rules themselves, followed by HIT/truncation computation.


### Phase 1I progress

- The type-theory monolith has been split again. Runtime-owned registries now
  live in `src/tt_registry.c`, universe/couniverse lattice operations live in
  `src/tt_lattice.c` / `src/tt_lattice.h`, and cubical face entailment/system
  lookup live in `src/tt_faces.c`.
- Added direct regression tests for the moved registry, lattice, and face APIs.
  These are important because the next architectural step is separating runtime
  values from checked kernel terms; the existing theory behavior must stay
  pinned down while that representation boundary is introduced.
- `tt_equality.c` is now closer to its intended role: normalization,
  judgmental equality, and reducer orchestration. Remaining split targets are
  HIT/truncation computation and, later, the reducer switch itself.


### Phase 1J progress

- HIT construction/lookup primitives now live in `src/prims_hits.c` instead of
  the monolithic `src/primitives.c`.
- Propositional truncation primitives now live in `src/prims_trunc.c`.  This
  keeps primitive registration centralized while moving implementation bodies
  into theory-specific files.
- Added `tests/tt_primitive_split_test.c` to guard the moved HIT/truncation
  primitive entry points directly through normal evaluation.
- The next theory split should move HIT/truncation *normalization and equality*
  helper logic out of `tt_equality.c`; this phase deliberately avoided changing
  reducer semantics.


### Phase 1K progress

- Optional proof-theory scaffolds are no longer embedded in the monolithic
  primitive implementation: S¹ and user theory-extension constructors now live
  in `src/prims_theory_ext.c`.
- Named logic-bundle primitives now live in `src/prims_logic.c`; the registry
  implementation remains in `tt_registry.c`.
- Added a split-regression test for optional S¹/theory-extension and named
  logic-bundle entry points.
- The next primitive split target is the proof/tactic interface, but that first
  requires exposing or moving the kernel S-expression converter so the split
  does not create hidden cross-file coupling.


### Phase 1L progress

- `src/prims_kernel.c` now owns kernel/proof/tactic primitives and the
  S-expression-to-kernel-term bridge.  This keeps tactics as ordinary optional
  front-end functions and removes proof-facing code from the general primitive
  monolith.
- `src/primitives.c` still owns registration, but implementation bodies are now
  split enough to prepare the later `SurfaceTerm / CoreTerm / KernelTerm / Value`
  boundary.
- The build hotfix also removes the unused `no_args` helper left behind by the
  previous theory-extension split.


### Phase 1M progress

- Kernel S-expression conversion now lives in `src/kernel_sexp.c` /
  `src/kernel_sexp.h`, rather than inside the proof primitive file.
- Quoted pair-chain data such as `'(Id Nat 0 0)` is reparsed before kernel
  conversion, fixing the split-regression in `kernel_primitive_split_test`.
- Proof state, metavariable context, and kernel definition context are now
  runtime-owned fields, preparing the kernel/proof layer for future
  thread-safe contexts.
- `scripts/clean.sh` no longer recursively invokes itself from the usage
  block, fixing the shell-level explosion after CI-like command chains.


### Phase 1N progress

- Kernel/proof primitives are now split by family. `src/prims_kernel_core.c`
  owns infer/check/reduce/equality primitives; `src/prims_kernel_proof.c` owns
  proof state and tactic front-end primitives; `src/prims_kernel_meta.c` owns
  holes/metavariables/unification; and `src/prims_kernel_defs.c` owns named
  kernel definitions. Shared string formatting lives in `src/prims_kernel_util.c`.
- The split keeps tactics optional and unprivileged: every tactic primitive still
  emits/checks kernel terms through the same public runtime path.
- The next architectural target is no longer more file splitting; it is the
  representation boundary: SurfaceTerm / CoreTerm / KernelTerm / Value.


### Phase 1N hotfix note

- `src/prims_kernel.c` is now a marker/documentation file only and is not compiled.
  This avoids an empty-translation-unit failure under strict C89/`-Wpedantic` while
  preserving the directory-level explanation of the kernel primitive family split.
- `scripts/clean.sh` now treats generated `lizard-*.zip` and `lizard-*.patch` files
  as removable release artifacts.


### Phase 2A progress

- Added the first explicit representation-boundary scaffold: `SurfaceTerm`,
  `CoreTerm`, existing `KernelTerm` (`kterm_t`), and runtime `Value` now have
  named paths in the codebase.
- This phase is deliberately non-invasive: the evaluator still uses the runtime
  AST, while the new wrappers make reader/macro/elaborator/kernel routing
  testable before changing semantics.
- Added `docs/REPRESENTATION_BOUNDARY.md` and a regression test proving that the
  four paths are distinct.


### Phase 2B progress

- Parser output can now be wrapped as `SurfaceTerm` values with source spans
  preserved from tokenizer/parser output.
- `lizard_surface_parse_source` is the first concrete bridge from the existing
  AST parser into the SurfaceTerm boundary.
- `lizard_core_from_surface` gives the future elaborator path a preferred name
  while preserving the old runtime-AST-backed CoreTerm scaffold.
- Evaluator semantics are unchanged; this is an integration boundary, not a
  new elaborator yet.


### Phase 2C progress

- SurfaceTerm now carries the first syntax-object scaffold: source span, phase,
  a compact lexical-scope marker, and an untrusted property table for future
  macro/debugger/tool metadata.
- Added a direct syntax-object scaffold regression test. Evaluator and macro
  semantics remain unchanged; this is a boundary/data-model step only.
- Next direction: replace the compact scope marker with a persistent scope-set
  object and start preserving syntax metadata through quote/quasiquote and
  macro expansion.


### Phase 2D progress

- `SurfaceTerm` now stores a small explicit scope-set object rather than a
  single unsigned-long placeholder.  The compatibility summary API remains,
  but future hygiene work can compare real scope sets.
- Added scope-set add/remove/contains/equal/count/summary APIs and a direct
  regression test for scope-set behavior.
- Evaluator and macro behavior remain unchanged; this is still scaffold for the
  future hygienic expander.


### Phase 2F progress

- SurfaceTerm now has untrusted transformation tracing for future macro/rewrite
  debugging.
- Metadata propagation copies trace chains, so rewritten surface syntax can
  preserve an explanation history.
- Evaluator and macro semantics remain unchanged; the trace chain is tooling
  metadata only.


### Phase 2G progress

- Added `src/expansion_context.c` / `.h`, a scaffold for future hygienic macro
  expansion contexts.
- Expansion contexts allocate fresh macro scopes, carry a phase/name, and attach
  `macro-introduce` / rewrite trace events to SurfaceTerm values.
- Added origin-chain debug formatting for SurfaceTerm traces.
- Evaluator and macro semantics remain unchanged; this is still syntax-tooling
  metadata only.


### Phase 2H progress

- Added a syntax-expander adapter API that accepts `SurfaceTerm` plus
  `ExpansionContext` and delegates semantics to the existing macro expander.
- Expansion adapter results preserve spans, phase, scopes, properties, and trace
  metadata while returning the old expanded runtime AST for evaluation.
- This is the first bridge from syntax-object metadata into the macro pipeline;
  it does not change macro semantics yet.


### Phase 2I progress

- Runtime contexts now have an opt-in traced expansion path.  By default,
  `lizard_context_eval_string` and `lizard_context_eval_file` keep the old
  parser/evaluator path.  When tracing is enabled, the same source flows through
  `SurfaceTerm` parsing and the syntax-expander adapter before evaluating the
  resulting runtime AST.
- The public API can enable/disable tracing and inspect the latest expansion
  trace count, stage, detail, and source span.  This makes syntax-object
  infrastructure observable through the runtime API without making the evaluator
  depend on it.
- Added regression coverage proving traced expansion and normal evaluation
  coexist in one context.


### Phase 2J progress

- Expansion trace metadata is now reportable through the stable context API.
  Callers can retrieve indexed trace events with stage/detail/origin fields and
  can format individual events into caller-owned buffers.
- Fixed the primitive header guard so the type-theory prototype block is no
  longer outside the include guard, eliminating duplicate-declaration warnings
  under `-Wredundant-decls -Werror`.
- The evaluator remains unchanged; trace reporting is still opt-in tooling
  metadata on top of the SurfaceTerm adapter path.


### Phase 2K progress

- Expansion traces can now be snapshotted into an owned
  `lizard_expansion_trace_report_t`. Reports copy trace event strings and are
  valid after later evaluations or context destruction.
- Added a strictness contract document and `make strict`; warning/security
  flags remain part of the design, not optional comfort settings.
- The runtime still keeps tracing opt-in and evaluator semantics unchanged.


### Phase 2L progress

- Added REPL/CLI hooks for expansion tracing. `--trace-expansion` enables the
  traced SurfaceTerm path; `--print-expansion-trace` prints an owned trace report
  after each evaluation.
- Tracing is still off by default, and evaluation semantics remain unchanged.
- Added CLI regression coverage so future tooling work cannot accidentally print
  traces in normal execution mode.

### Phase 2M progress

- Expansion trace reports can now be exported to a stable line-oriented file
  format via `--trace-expansion-file PATH` and
  `lizard_expansion_trace_report_fprint`.
- The export path uses owned trace report snapshots and leaves normal evaluator
  output untouched unless tracing is explicitly requested.


### Phase 2P progress

- Added an owned diagnostic report layer for parser/tokenizer/expansion
  failures.  This keeps strict warning policy intact while giving tooling a
  stable text/JSON diagnostic surface.
- `--expand-only` failure reports now contain structured diagnostics alongside
  expansion metadata; evaluator and macro semantics are unchanged.


### Phase 2Q progress

- Shared report-format escaping now lives in `src/report_writer.c` /
  `src/report_writer.h`.  Expansion trace, syntax expansion, and diagnostic
  reports all use the same strict text/JSON escaping implementation.
- Public report formats are preserved; this is an internal hardening step so
  future editor/tooling output cannot drift across report types.
- Added direct report-writer tests for tab/newline/carriage-return/backslash,
  JSON quote/backslash/control-byte escaping, and NULL field behavior.


### Phase 2R progress

- Added a stable report schema registry in `src/report_schema.c` /
  `src/report_schema.h`. Report type names and version numbers for syntax
  expansion, expansion traces, and diagnostics now live in one place.
- Report writers still emit the same text/JSON formats, but no longer duplicate
  schema constants across files. This reduces editor/tooling drift risk.
- Added regression coverage for schema names, versions, validity checks, and JSON
  type-field printing.

## Current milestone: Lizard 0.2 — "Recoverable Core"

Do **not** start with the exciting features (native compiler,
concurrency, full Racket macros, CAS, cubical metatheory). Start with the
spine that makes them possible. The next release target is a core that is
**impossible to accidentally lie**: errors are reported (not hidden),
examples pass honestly (not cosmetically), the kernel checks terms (not
trusts AST), the REPL survives bad input.

**0.2 is done when:**

- `nix develop`, `make`, `make test`, `make examples` all work
- the repo has no generated junk (no `build/`, no `*.zip`, no stale docs)
- examples fail honestly (the manifest gates `make examples`)
- syntax errors do **not** kill the process; the REPL survives parse errors
- the embedding API returns diagnostics for bad input
- source locations exist from tokenizer through parser
- the REPL and the embedding API use the **same** context path
- `README` matches reality; `MASTER_PLAN.md` is canonical

## What already exists (verified this session — Phase 1 is mostly *wiring*)

A survey of the tree shows the Recoverable-Core infrastructure is largely
already present; the work is to connect it, not invent it:

- **Diagnostics types exist.** `include/lizard_api.h` already defines
  `lizard_status_t` (with `PARSE_ERROR`, `EVAL_ERROR`, `TYPE_ERROR`,
  `OUT_OF_MEMORY`), `lizard_source_span_t` (filename + line/col/offset
  spans), and `lizard_diagnostic_t` (status + span + message).
- **Context API exists.** `lizard_runtime_t` / `lizard_context_t` with
  `lizard_context_create`, `lizard_context_eval_string`,
  `lizard_context_eval_file`, `lizard_context_last_diagnostic`.
- **AST nodes already carry a `span`** (set from token line/col/offset in
  `parser.c`), and `runtime.c` already has `set_diagnostic` helpers.

What's missing (the actual 0.2 work):

1. **The parser still `exit(1)`s** — 42 sites in `parser.c`, each an
   `fprintf(stderr, "Error: …")` followed by `exit(1)`. It never returns
   failure, so `lizard_parse` never returns `NULL`, so the existing
   recovery path in the REPL's `eval_source` (which already checks
   `ast_list == NULL`) can never fire. Fixing this is the single
   highest-impact change.
2. **The REPL bypasses the context API.** `repl.c:main` builds its own
   heap/env and calls a local `eval_source`, instead of going through
   `lizard_context_eval_string`. Unify them.
3. **Diagnostics aren't populated by the parser** — the structs exist but
   the parser writes to `stderr` instead of filling a `lizard_diagnostic_t`.

---

## 0. Principles (decide these once, hold them everywhere)

1. **Small trusted core, large derived tower.** Only the kernel and the
   constructors of *kernel terms* are trusted. Reader, macros,
   elaborator, tactics, CAS, libraries — all untrusted. If any of them
   produces a bad term, the kernel rejects it. This is already the
   project's instinct; we make it structural.

2. **Term-first proving; tactics are optional and never privileged.**
   You write proof terms (with holes); elaboration solves implicits and
   unification; the kernel checks. Tactics, if present, are just library
   functions that *emit* core terms — one optional front-end among
   several, not the primary interface. (This honors the stated dislike
   of tactics: they become removable, not foundational.)

3. **Immutable by default; mutation is explicit and checkable.** The
   Clojure model. Persistent structures are the shared substrate;
   mutation happens only through atoms / refs / transients with rules
   the runtime can enforce. This is what makes the concurrent runtime
   tractable.

4. **Non-moving GC.** Object addresses stay stable. Keeps the embedding
   API and any FFI sane, and removes read barriers / forwarding from the
   concurrency story.

5. **Pragmatic native path: compile to C.** Native speed without a
   backend to maintain, fully in keeping with the C89 ethos and the
   trusted-core principle. (Argued in §3.6.)

6. **Every phase ships standalone value and de-risks the next.** No
   phase is "refactor with nothing to show." Each ends with something
   runnable and tested.

---

## 1. The two lists, reconciled

The engineering "hygiene" list is the foundation course for the feature
vision. Mapping:

| Hygiene step | Unblocks (vision) |
|---|---|
| 1. Repo hygiene | everything (clean base, CI possible) |
| 2. Honest examples / CI | tooling, safe refactoring of all of the below |
| 3. Recoverable diagnostics + **source locations** | Racket syntax objects, macro debugging, REPL/IDE tooling |
| 4. Unified runtime/context API (REPL == embedding) | **concurrent runtime**, API hardening, thread-safe contexts |
| 5. **Runtime AST vs checked kernel terms** | the *entire* theorem-proving pipeline, cubical metatheory work |
| 6. Strict / fallback-marked bytecode | native compiler |
| 7. Split huge source files | maintainability of every subsystem below |
| 8. Object-level non-moving mark-sweep GC | concurrent runtime, native compiler |
| 9. Real module exports/imports/namespaces | Racket module infra, contracts, #lang, **CAS merge** |
| 10. Merge CAS as libraries | depends on #9; closes the CAS loop |

Read top-to-bottom, the hygiene list is already in roughly dependency
order. The vision features hang off it.

---

## 2. Subsystem dependency graph

```
                    [F1 repo hygiene]
                           |
        +------------------+------------------+
        |                  |                  |
 [F4 split files]   [F2 srcloc+diag]   [I3 honest examples/CI]
        |                  |                  |
        |          +-------+--------+         |
        |          |                |         |
        v          v                v         |
 [F3 context API]  |        [MM2 syntax objs] |
        |          |                |         |
   +----+----+     |        [MM3 phases]------+
   |         |     |        [MM4 #lang]
   v         v     |        [MM5 contracts]
[M1 obj GC] [R2    |        [MM6 macro debug/tooling]
   |        bytecode strict]            ^
   v              |                     |
[M2 threads/      |               [MM1 modules/namespaces]
 concurrent       |                     |
 runtime]         |          +----------+----------+
   |              |          |                     |
   v              v          v                     v
[I2 concurrency  [C1 native [I1 CAS as libs]  [R1 runtime AST
 DS = your DS]    compiler]                     vs kernel terms]
                     ^                                |
                     |                    +-----------+-----------+
                     +--------------------|                       |
                                   [TT1 elaboration        [TT2 cubical
                                    surface->core->kernel]  metatheory + TT]
                                          |
                                   [TT3 term-mode proving]
```

Critical spine (build these in order, everything else hangs off them):
**F1 → F2/F4 → F3 → R1 → MM1 → (then fan out).**

---

## 3. Key architecture decisions

### 3.1 Three term representations + one value representation (resolves #5)

The single most important refactor. Four distinct types, three trust
levels:

```
SurfaceTerm   syntax object: datum + scope-set + srcloc      [untrusted]
CoreTerm      elaborated: explicit binders, types, implicits  [untrusted]
              resolved, holes/metavars allowed
KernelTerm    trusted: de Bruijn indices, no names, no srcloc, [TRUSTED]
              minimal constructors only
Value         runtime: closures, primitives, boxed data,       [untrusted]
              persistent structures
```

- **Reader** → SurfaceTerm (carries srcloc from byte 1).
- **Macro expander** → SurfaceTerm → SurfaceTerm (scopes added per
  expansion for hygiene; srcloc propagated).
- **Elaborator** → SurfaceTerm → CoreTerm (name resolution, implicit
  insertion, unification, hole-filling). *Untrusted.*
- **Erase/index** → CoreTerm → KernelTerm (drop names/locs, de Bruijn).
- **Kernel** checks KernelTerm only. Accept or reject. Nothing else is
  trusted.
- **Evaluator** runs Values, produced from CoreTerm (interpreted) or
  from compiled bytecode/native.

Why this shape: it makes the proving pipeline you described literal —
*surface syntax → elaborated TT → checked kernel terms → trusted
kernel* — and it makes tactics optional (a tactic is just a function
`ProofState -> CoreTerm`). The kernel never sees a tactic; it sees a
finished term. Soundness depends only on the kernel + KernelTerm
constructors, which stay tiny.

Migration: today runtime AST and "type-theory terms" are tangled. Step
1 is to introduce `KernelTerm` as a separate C type and a converter,
*without* changing the evaluator. Then move the checker onto KernelTerm.
Then introduce CoreTerm + elaborator. SurfaceTerm/syntax-objects come
with the Racket work (MM2) and reuse the same srcloc infra.

### 3.2 Source locations & diagnostics (resolves #3, feeds Racket infra)

One mechanism, three payoffs. A `srcloc` = (source-id, start, end,
line, col). Introduced by the reader, attached to every SurfaceTerm,
**preserved through macro expansion**. Then:

- **Diagnostics** (#3): replace all 51 `exit()` calls with a
  `Diagnostic` value (severity, srcloc, message, hint) accumulated in
  the context. The reader/parser returns partial results + diagnostics
  instead of dying. The REPL prints them and continues.
- **Hygiene** (Racket): scopes live next to srcloc on the syntax object.
- **Macro debugging** (Racket): an expansion step can show "this form
  at L:C expanded via macro M to that form" because srcloc survived.

Build srcloc *once*, before both #3 and the Racket work, and both get
cheaper.

### 3.3 Runtime context API (resolves #4, foundation of concurrency)

A `lizard_context` owns: the heap + GC, the global environment, the
module registry, the reader state, the flag table, and a diagnostics
sink. **The REPL and the embedding API both go through it** — no
globals. Sketch:

```c
lizard_context *lz_context_new(lz_config *cfg);
void            lz_context_free(lizard_context *ctx);
lz_value        lz_eval_string(lizard_context *ctx, const char *src, lz_diagnostics *out);
lz_value        lz_eval_term  (lizard_context *ctx, lz_value core_term);
lz_module      *lz_load_module(lizard_context *ctx, const char *name, lz_diagnostics *out);
```

Once state lives in a context (not globals), the concurrent runtime is
"many contexts" rather than "rewrite everything." This is why #4 must
precede the concurrency work.

### 3.4 Concurrent runtime + your data structures (the 1:1 mapping)

Your DS list maps directly onto roles in a Clojure-style runtime. This
is the "map one-to-one to the DS I wrote" you hoped for:

| Your structure | Runtime role | Sharing rule |
|---|---|---|
| persistent vector | shared immutable substrate | read from any thread, **no sync** |
| persistent HAMT map/set | shared immutable substrate | read from any thread, **no sync** |
| transient | thread-local fast mutation | **owned by one context**; freeze before sharing (enforce with an owner-id checked on every mutating op) |
| atom | single-cell lock-free mutation | CAS; independent state |
| ref + STM | coordinated multi-cell mutation | your `stm.lisp` semantics (versioned refs, validate-and-commit) promoted to the C runtime |
| future/promise | task on a thread pool | write-once cell others await |
| thread-safe context | per-OS-thread runtime | shares only immutable substrate + explicit cells |

Key de-risking move: **finalize and test the concurrent *semantics*
single-threaded first** (the libraries already do this), then flip on
real threads once GC (#8) and contexts (#4) support it. The persistent
structures are correct and shareable the moment they're immutable; the
parallelism is a later, separable switch.

"Clear immutable/mutable semantics" becomes a *typed* distinction at the
value level: a value is either persistent (shareable) or a cell
(atom/ref/transient, with rules). The runtime can check transient
ownership and reject cross-thread misuse with a real diagnostic.

### 3.5 GC: segment-level → object-level non-moving mark-sweep (#8)

Move from segment granularity to per-object marking. Keep it
**non-moving** (stable addresses → simple FFI, no read barriers). For
concurrency, start with **stop-the-world + safepoints**: threads poll a
safepoint flag, rendezvous, one thread marks-and-sweeps over all
contexts' roots, then release. Concurrent/parallel GC is a later
optimization, explicitly deferred. Honest: this is medium-high
difficulty and must land before real threads (#2 concurrency).

### 3.6 Native compiler: compile to C (#6 → native)

Recommendation, stated as an opinion: **don't hand-roll a native
backend or take an LLVM dependency.** Compile to C and use the system
compiler. Revised pipeline (per review): bytecode and C are **two
backends from a shared ANF-style IR**, not one generated from the other:

```
CoreTerm → ANF / low-level IR ─┬→ bytecode  (interpreted)
                               └→ C emission → cc → .so/.a  (native)
```

A small ANF (administrative-normal-form) IR pays off four times over:
optimization passes, source maps, profiling, and native codegen all hang
off the one IR instead of being reinvented per backend.

- Closures → C structs + function pointers; environments → heap records.
- TCO → tail calls lowered to loops / trampoline.
- GC → calls into the object-level collector's API (§3.5).
- Numbers → GMP, as today.

Why: native speed, zero backend maintenance, maximum portability, and
it preserves the trusted-core/C89 character. LLVM is noted but
discouraged (heavy dep, slow builds, overkill here). This is the last
big phase because it depends on a strict bytecode (#6), a clean value
model (R1), and the GC API (#8).

### 3.7 Cubical metatheory + the TT itself (honest framing)

This is the one item that is **research, not finite engineering**. Treat
it as an ongoing track with concrete, checkable milestones rather than a
checkbox:

- M-a: full per-type-former Kan composition (beyond canonical cases).
- M-b: HITs in the *kernel*, not just at library level.
- M-c: normalization-by-evaluation core for conversion checking
  (faster, more obviously correct than the current rule-set).
- M-d: a written (eventually mechanized) canonicity argument for the
  implemented fragment.
- M-e: decide and document regularity / definitional-vs-typal choices.

Each milestone is publishable-adjacent and independently valuable. None
is a prerequisite for the engineering phases, so this track runs in
parallel once R1 (term split) exists.

---

## 4. Phased roadmap

Each phase: **goal · items · depends-on · risk · done-when.**

Reordered per review: **modules/macros move ahead of concurrency**,
because CAS, proof libraries, contracts, and language-oriented
programming all depend on modules, whereas concurrency can wait. And
**persistent immutable data structures are split from the concurrent
runtime** — the immutable structures can land early (they're correct and
shareable the moment they're immutable); real threads wait for GC +
context hardening.

### Phase 0 — Hygiene & honest examples (do now)
- **Goal:** clean, trustworthy base; CI that catches regressions.
- **Items:** #1 repo hygiene (`.gitignore`; `git rm -r --cached build`;
  stop committing zips; archive stale roadmap docs); #2 `examples/
  manifest.sexp` + `scripts/run-examples.sh` gating `make examples`;
  README → `nix develop`; begin #7 (split the monsters: `primitives.c`
  348KB, `tt_equality.c` 297KB).
- **Depends:** none. **Risk:** low.
- **Done when:** clean `git status`, no artifacts tracked, `make examples`
  goes red on any unexpected error.

### Phase 1 — The spine: diagnostics + source locations + context API
- **Goal:** no `exit()`, locations everywhere, one eval path. (Mostly
  *wiring* — the types already exist; see "What already exists" above.)
- **Items:** #3 make the parser recoverable — replace the 42
  `exit(1)` sites with diagnostics + non-fatal failure so `lizard_parse`
  returns `NULL` and the REPL's existing recovery fires; populate
  `lizard_diagnostic_t` from the token span; #4 route `repl.c:main`
  through `lizard_context_eval_string`; finish #7 file splitting.
- **Depends:** Phase 0. **Risk:** medium (parser + startup).
- **Done when:** a syntax error prints a located diagnostic and the REPL
  survives; REPL and embedding API share the context path.

### Phase 2 — Representation split: Surface / Core / Kernel / Value (§3.1)
- **Goal:** runtime values vs trusted kernel terms separated.
- **Items:** #5 introduce `KernelTerm` + converter; move the checker onto
  it; introduce `CoreTerm` + a minimal elaborator skeleton; #6 strict /
  fallback-marked bytecode.
- **Depends:** Phase 1. **Risk:** high (central). Mitigate: converter
  first, evaluator untouched, migrate the checker behind tests.
- **Done when:** the kernel only ever sees `KernelTerm`; a deliberately
  ill-typed term is rejected even when the elaborator is buggy.

### Phase 3 — Module namespaces
- **Goal:** real `module` / `import` / `export` with isolated namespaces.
- **Items:** #9 module system; then #10 merge CAS as libraries once
  loading is stable (`lib/cas/core.lisp`, `lib/cas/poly.lisp`, …).
- **Depends:** Phase 1 (context holds the module registry), Phase 2
  (clean value model). **Risk:** medium.
- **Done when:** two modules with colliding names don't interfere; an
  export/import round-trips; the CAS loads as a namespaced library.

### Phase 4 — Syntax objects + hygienic macros (Racket infrastructure)
- **Goal:** macros carry source + scope, not raw symbols.
- **Items:** MM2 syntax objects (datum + scope-set + srcloc + phase,
  reusing Phase 1 locations); MM3 hygienic expansion + macro phases; MM4
  `#lang` language declarations; MM5 contracts; MM6 macro stepper /
  debugger / tooling. **Order matters: syntax objects first, `#lang`
  last.**
- **Depends:** Phase 1 (srcloc), Phase 3 (modules). **Risk:** med-high.
- **Done when:** an imported macro expands hygienically; the stepper shows
  located expansion; a contract violation names the blaming party.

### Phase 5 — Object-level GC
- **Goal:** per-object non-moving mark-sweep (§3.5).
- **Items:** #8 object-level mark bit; root registration from the context
  (envs, stacks, modules, globals, VM frames); per-object sweep; size-class
  free lists. Stay non-moving. Stop-the-world + safepoints for now.
- **Depends:** Phase 1 (contexts as root sources). **Risk:** high.
- **Done when:** GC reclaims individual objects; addresses stay stable;
  no regressions in `make check`.

### Phase 6 — Persistent data + concurrency
- **Goal:** your DS library becomes the runtime substrate; then threads.
- **Items:** promote persistent vector / HAMT map+set / transients /
  atoms / refs+STM / futures into the runtime with §3.4 sharing rules
  (immutable = shareable; transient = single-owner; cells = explicit).
  **Persistent immutable structures can land first**, tested
  single-threaded; #2 real thread-safe contexts + thread pool come after
  GC (Phase 5) lands.
- **Depends:** Phase 2 (values), Phase 5 (GC) for *real threads*.
- **Risk:** high. Mitigate: semantics single-threaded first.
- **Done when:** two contexts on two threads share an immutable map with
  no locks; a cross-thread transient is rejected with a diagnostic; an STM
  transfer is atomic under contention.

### Phase 7 — Elaborator / proof mode (term-first)
- **Goal:** term-first proving end to end; cubical milestones in parallel.
- **Items:** TT1 mature elaborator (implicits, unification, holes);
  TT3 term-mode proving — `(theorem … :term (lambda (n) ?hole))` reports
  the hole's context + goal; tactics demoted to optional hole-fillers
  that emit core terms; TT2 cubical metatheory M-a..M-e in parallel.
- **Depends:** Phase 2 (term split). TT2 can start as soon as R1 lands.
- **Risk:** high / partly research.
- **Done when:** a non-trivial theorem is stated in surface TT,
  elaborated, and kernel-checked with **no tactics**; ≥1 cubical milestone.

### Phase 8 — CAS as proof-aware libraries
- **Goal:** the CAS becomes namespaced libraries; optionally proof-carrying.
- **Items:** restructure into `lib/cas/{core,poly,rational,risch,control,
  dynamics}.lisp`; wire derivations to the kernel (per `docs/CAS.md`) so
  simplification/integration can emit kernel-checkable certificates.
- **Depends:** Phase 3 (modules), Phase 7 (kernel-checkable terms).
- **Risk:** medium (engine) / high (certificates).
- **Done when:** the CAS loads as libraries; a derivative emits a
  certificate the kernel accepts.

### Phase 9 — Native compiler
- **Goal:** native performance via shared-IR compile-to-C (§3.6).
- **Items:** C1 Core → ANF IR → {bytecode, C} → system cc; closures, TCO,
  GC integration, GMP numbers.
- **Depends:** Phase 2 (values), Phase 5 (GC API), #6 (strict bytecode).
- **Risk:** high.
- **Done when:** a compiled program runs natively and passes the same
  example suite as the interpreter, with a measured speedup.

---

## 5. How we execute (working method)

- **One subsystem at a time, as its own focused effort.** These are not
  "massive drops"; they are surgical changes to a trusted C core.
- **Grep the real structs / function names / patterns before any C
  edit.** Never assume members or arities. (Established project rule.)
- **C89 strict** (`-Werror -pedantic -Wconversion -Wstrict-prototypes`):
  declarations at block start, no C99/C11, no function↔object pointer
  casts.
- **The build happens on your machine** (Claude's sandbox lacks `ds.h`).
  Workflow: propose the change → you `make clean && make && make test`
  → paste any errors → iterate. Apply the grep-first rule to every fix.
- **Tests gate every phase.** Phase 0's example runner is the safety net
  for all later refactors.
- **Pure-Lisp work (libraries) stays risk-free** and is cross-checked
  against the primitive registry before landing, as before.

---

## 6. Immediate next actions (Phase 0, ready to apply)

`.gitignore` (replace the current one):

```gitignore
# build artifacts
/build/
*.o
*.a
*.so
*.dylib
build/lizard

# never commit release bundles
*.zip
*.tar
*.tar.gz

# editor / OS
*.swp
*~
.DS_Store

# logs / scratch
*.log
*.bak
/scratch/
```

Stop tracking artifacts already committed:

```sh
git rm -r --cached build 2>/dev/null || true
git rm --cached *.zip 2>/dev/null || true
git add .gitignore && git commit -m "repo hygiene: ignore build artifacts and bundles"
```

Doc consolidation:

```sh
mkdir -p docs/archive
git mv docs/ROADMAP.md docs/ARCHITECTURE_ROADMAP.md \
       docs/LIZARD_EVOLUTION_PLAN.md docs/NEXT_STEPS.md \
       docs/COMPILER_RUNTIME_DEBUGGER_PLAN.md docs/archive/
# this file (docs/MASTER_PLAN.md) is now canonical
```

Then: build the example test runner (#2) and start splitting
`primitives.c` and `tt_equality.c` (#7).

---

## 7. Risk register / honest scope

- **Engineering, finite, sequenced:** #1–#10, the context API, the
  representation split, modules, GC, compile-to-C. Hard but bounded.
- **Subtle, needs care:** hygiene + macro phases, STM/transient
  ownership under real threads, the runtime/kernel migration.
- **Research, partly open-ended:** cubical metatheory (TT2). Milestoned,
  but not a checkbox; runs in parallel.
- **Explicitly deferred:** concurrent/parallel GC (start stop-the-world);
  LLVM backend (use compile-to-C instead).

The throughline: keep the trusted core tiny, push everything else into
the untrusted tower, and let each phase stand on its own.

### Phase 2E progress

- SurfaceTerm now has explicit metadata propagation helpers for future reader
  and macro-expansion transformations.
- Quoted datum can be reparsed and wrapped as a SurfaceTerm while preserving the
  origin span, phase, scopes, and optional properties.
- Added debug-string support for syntax-object tooling; it is intentionally
  untrusted and ignored by evaluator/kernel paths.

### Phase 2N progress

- Added owned syntax expansion reports for tooling and macro-stepper work.
- Added `--expand-only`, which parses and macro-expands input but does not
  evaluate it.
- Reports expose source span, phase, scope summary, expanded AST summary, and
  trace events while preserving the old evaluator path.



### Phase 2O progress

- Added JSON writers for expansion trace reports and syntax expansion reports.
- Added `--expand-only-format text|json`; the text format remains the default.
- Added CLI/API tests for JSON report output and invalid format diagnostics.
- Evaluation and macro semantics remain unchanged.
