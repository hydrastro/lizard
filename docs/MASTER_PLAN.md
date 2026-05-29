# Lizard — Master Plan

This is the single canonical roadmap. The older overlapping docs
(`ROADMAP.md`, `ARCHITECTURE_ROADMAP.md`, `LIZARD_EVOLUTION_PLAN.md`,
`NEXT_STEPS.md`, `COMPILER_RUNTIME_DEBUGGER_PLAN.md`, `FEATURE_BACKLOG.md`,
`OPTIONAL_PROOF_SCAFFOLDS.md`) are archived under `docs/archive/`. The
canonical doc set is: `MASTER_PLAN.md`, `CLAIMS_MATRIX.md`,
`LIMITATIONS.md`, `LIBRARIES.md`, `USAGE.md`, `CAS.md`, plus the C-API
and type-theory references.

---


### Phase 2X progress

- Fixed the `surface_term.h` public type include-order regression by requiring
  implementation headers that expose public API types to include `lizard_api.h`
  themselves.
- Added `make header-audit`, wired into `make ci`, so future syntax/report
  headers cannot accidentally depend on transitive include order.
- This protects the syntax-object/reporting boundary while keeping strict
  warning flags intact.


### Phase 2Y progress

- Added include-layer auditing after the public API/header-boundary audits.
  The tree now checks public header purity, implementation-header acyclicity,
  and selected leaf-tooling header independence.
- `make ci` now runs `include-audit` before the full tests/examples path.  This
  prevents include-order and layering regressions from reappearing as syntax and
  report tooling grow.


### Phase 3A progress

- Began the Phase 3 runtime hardening spine with an ownership model scaffold.
- Added object ownership and trace-policy metadata without changing allocator or
  GC behavior.
- Added `make ownership-audit`, which checks AST allocation type initialization,
  C-owned module boundaries, and report snapshot ownership rules.
- Added `docs/OWNERSHIP.md` as the baseline for the future object-level
  non-moving mark/sweep transition.


### Phase 3B progress (#7 — splitting the monsters)

- Eliminated the ~450-line duplicate `sexp_to_kterm` in `primitives.c`: all
  call sites now use the already-compiled `lizard_kernel_sexp_to_kterm`
  (`kernel_sexp.c`).  This was the duplication whose silent drift previously
  broke the proof/`kernel-reduce` flow; collapsing it removes the hazard.
- Removed the stale, uncompiled extraction scaffolds (`prims_kernel*.c`,
  `prims_kernel_util.h`, `tt_registry.c`, `tt_faces.c`, ~1.5k lines).  They
  pre-dated the current kernel-state isolation and bridge fixes, so wiring them
  up would have reintroduced old bugs; #7 must continue as a *fresh* extraction
  from current code, not by reviving these.
- Net: `primitives.c` 8787 → 8334 lines and ~1,989 dead/duplicate lines gone,
  with `make ci` fully green (93/93 C, 5/5 Lisp, examples, audits, smoke).
- **Next #7 target:** `tt_equality.c` (~6.9k lines) carries static
  `make_glue`/`make_ua`/`make_u_max`/`make_u_suc`/`make_u_min`/
  `make_couniverse_set` that parallel the compiled `lizard_tt_make_*` in
  `tt_glue.c`/`tt_lattice.c`.  Worth collapsing, but each helper needs
  byte-identity verification before repointing — the cubical core is delicate.


### Phase 3B progress, continued

- Collapsed 17 byte-identical static `make_*` helpers in `tt_equality.c`
  (`make_glue`/`make_glue_intro`/`make_unglue`/`make_ua`/`make_equiv_*`/
  `make_id_equiv`/`make_system_cons`/`make_system_nil`/`make_u_suc`/`make_u_max`/
  `make_u_min`/`make_universe_set`/`make_couniverse_set`/`make_co_max`/
  `make_co_min`) onto the compiled `lizard_tt_make_*` publics in
  `tt_glue.c`/`tt_lattice.c`.  `tt_equality.c` 6972 → 6819, cubical tests green.
- Removed the entire stale, uncompiled `prims_*.c` family (`prims_tt.c` 2756L,
  `prims_collections`, `prims_hits`, `prims_modules`, `prims_persistent`,
  `prims_lists`, `prims_logic`, `prims_trunc`, `prims_theory_ext`,
  `prims_bytecode`, `prims_gc`, `prims_syntax`, `prims_common.c/.h`) — ~5.46k
  lines of orphaned duplicate code.  No compiled TU referenced any of them.
- **Why the real `primitives.c` split is non-trivial (verified by attempt):**
  `prims_tt.c` was a *complete, byte-identical* copy of the 120 explicit `tt`
  primitives, but the `tt` surface in `primitives.c` is mostly *macro-generated*
  — `TT_PREDICATE`, `TT_ACCESSOR`, and `MAKE_COMP_FAMILY` (~190 invocations)
  interleaved with non-`tt` `logic_*` primitives.  A clean extraction must move
  the macro layer + ~190 invocations as a unit and disentangle the interleaved
  `logic_*` code; it cannot be done by reviving a parked copy.  That is the
  bounded-but-careful next step for #7 (best done in Claude Code).

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

### Phase 2W progress

- Fixed the public report/type boundary by restoring the expansion trace event
  type in `lizard_api.h`.
- Added `make api-audit` and a script-level guard so future report/syntax
  refactors cannot accidentally remove public types required by internal syntax
  headers.
- Added strict compile coverage for public report/event type visibility.

### Phase 2Z progress

- Added canonical diagnostic/span construction helpers and refactored tokenizer,
  parser, runtime, and syntax-expansion report paths to use them.
- Diagnostic severity/category mapping now lives with diagnostic construction,
  reducing drift across runtime and report tooling.
- Added direct helper tests plus runtime-path tests for tokenizer/parser/IO
  diagnostic categories. Diagnostic report v2 remains stable.

### Phase 3B progress

- Added a per-heap GC metadata side table scaffold.
- Heap allocation now registers metadata for allocated objects without changing
  object layout, object addresses, mark traversal, or sweep behavior.
- Added metadata stats and lookup helpers so future GC work can audit object
  classes and trace policies before object-level collection is enabled.
- Added `gc_metadata_test.c` and ownership-audit checks for the side table.

### Phase 3C progress

- Added a build-graph closure audit after a linker failure showed that syntax
  scaffold tests could reference functions whose implementation objects were not
  archived into `liblizard.a`.
- `LIB_SRCS` now keeps its explicit core order but automatically closes over
  additional `src/*.c` implementation modules present in the tree. This protects
  syntax/report/gc scaffold files from being forgotten in the Makefile.
- `make ci` now includes `build-graph-audit` alongside API, header, include,
  and ownership audits.

### Phase 3D progress

- Recovered the build graph from the overly aggressive Phase 3C automatic
  `src/*.c` closure.
- The library now closes over an allowlisted optional module set, pulling in
  syntax/report/GC scaffolds when present while avoiding incomplete experimental
  modules.
- Added a recovery script and allowlist-aware build-graph audit so local trees
  with mixed Phase 2/Phase 3 patches can be repaired without weakening compiler
  strictness.

### Phase 3E progress

- Added a public API duplicate-definition audit after a local recovery merge
  produced two `lizard_expansion_trace_event_t` definitions in `lizard_api.h`.
- Hardened the recovery script so repeated runs de-duplicate public API struct
  blocks before adding missing declarations.

### Phase 3F progress

- Implemented `lizard_tokenize_source`, fixing a scaffold link failure in
  syntax/surface tests.
- Added explicit GC metadata classification through `lizard_heap_alloc_tagged`
  and started moving core constructors away from size-only object-kind
  inference.
- Extended metadata stats and tests while keeping allocator and collector
  semantics behavior-preserving.


### Phase 3G progress

- Recovered the context-level expansion trace API in the public header after the
  Phase 3 recovery patches dropped those declarations.
- Added an audit/recovery helper so syntax-object trace tests cannot regress to
  implicit declarations.

### Phase 3H progress

- Fixed the remaining surface-layer filename propagation gap from the current status recap: source filenames now survive tokenizer/source parsing, parser diagnostics, top-level AST spans, SurfaceTerm wrapping, syntax expansion, and expansion trace events.
- Reconciled the Phase 3 audit scripts with the current tree: `kernel_sexp` is treated as the live AST→kernel bridge, report snapshot modules are explicitly C-owned, and `surface_term.h` now declares its public API dependency directly.
- Cleaned the packaged tree of generated build artifacts and obvious wrong-project leftovers.
