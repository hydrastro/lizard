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
  `logic_*` code; it cannot be done by reviving a parked copy.


### Phase 3B progress — `primitives.c` tt split COMPLETED

- Performed the real #7 split: extracted the entire type-theory / cubical
  surface out of `primitives.c` into a new compiled module `src/prims_tt.c`.
  A pattern-driven, brace/paren-aware extractor moved all of it as a unit —
  the 120 explicit `lizard_primitive_tt_*` functions, the three generator
  macros (`TT_PREDICATE`/`TT_ACCESSOR`/`MAKE_COMP_FAMILY`) and their ~190
  invocations, and the tt-only helpers (`parse_dim_args`,
  `lizard_make_nullary_tt`, `tt_list_to_lisp_list`) — while leaving the
  interleaved `logic_*` primitives and all registration (`install_one`) in
  `primitives.c`.
- Introduced `src/prims_shared.h`: a small internal header for helpers shared
  across primitive modules (`no_args`/`single_arg`/`two_args`/`three_args`/
  `four_args`/`nth_arg`/`lizard_rule_on`), de-static-ed in `primitives.c`.
  This is the reusable foundation for extracting further clusters
  (collections, modules, persistent, …) the same clean way.
- Result: `primitives.c` 8334 → 6109 lines; `prims_tt.c` ≈ 2.2k lines compiled
  into the lib; `nm` confirms all 313 tt symbols now live in `prims_tt.o` and
  none in `primitives.o`.  `make ci` fully green (lint, all audits, C 93/93,
  Lisp 5/5, examples 62/62, smoke).
- **Next #7 clusters** (now straightforward via `prims_shared.h`): collections
  (vectors/hash-maps/atoms/transients), modules, persistent data.  Each is a
  regular-function cluster with no macro layer, so the same extractor applies.

### Phase 3B progress — #7 monolith split COMPLETE; Phase 1 CLOSED

- Extracted **every** remaining cohesive cluster out of `primitives.c` using
  the `prims_shared.h` foundation and a generalized, name-anchored, brace/
  paren-aware extractor.  New compiled modules (all in `LIB_OPTIONAL_SRCS`,
  all green, registration left in `primitives.c`):
  `prims_collections` (mutable vectors + hash maps),
  `prims_persistent` (pvec / phash / transients, with the HAMT typedefs),
  `prims_string` (16 string ops), `prims_kernel` (Track-K kernel/proof/tactic
  surface + the `current_proof` macro and proof-state helpers),
  `prims_logic`, `prims_modules`, `prims_lists`, `prims_gc`,
  `prims_bytecode` (vm/disassemble/profile), and `prims_syntax`.
- Shared helpers promoted into `prims_shared.h` as the compiler demanded:
  `make_vector`, `make_string`, `make_symbol`, `gc_make_stat`, `gc_cons`
  (plus the arg-validators and `lizard_rule_on` from the tt split).  Cluster-
  only types/constants/macros (`hamt_entry_t`/`hamt_flat_t`,
  `LIZARD_HASH_INIT_CAP`, the `logic_*_collect_t` typedefs, `current_proof`)
  travelled into their owning modules.
- **Result: `primitives.c` 8787 → 3301 lines (a 62% cut from the original
  monolith).**  What remains is genuinely *core*: arithmetic (the `NUM_PRED`
  family + `unary_number`/`binary_numbers`), comparison, boolean/bitwise,
  type predicates, control (`apply`/`eval`/`callcc`/`guard`/`try`/`raise`),
  core data (`car`/`cdr`/`cons`), I/O, and all primitive registration.
  Arithmetic was never split in the original parked design — confirming it is
  the core that stays.
- Updated `scripts/check-build-graph.py`: removed `prims_kernel`,
  `prims_modules`, `prims_bytecode` from the experimental `EXCLUDED` set —
  those were placeholders for the long-deleted incomplete scaffolds; the
  current modules are complete, compiled, and test-covered.
- **Phase 1 verified CLOSED.**  (a) Parser is recoverable: syntax errors record
  a located `lizard_diagnostic_t` and `longjmp` to the recovery frame
  established in `lizard_parse` (which returns `NULL`); the REPL survives and
  continues.  The single residual `exit(1)` is *required* — `lizard_parser_fail`
  is `LZ_NORETURN`, and the no-recovery branch (unreachable in normal use,
  since `lz_parse_active` is set for the whole extent of `lizard_parse`) must
  not return.  (b) REPL and embedding API share one eval path: `repl.c` has no
  direct eval/parse calls — `eval_source` is a presentation wrapper over
  `lizard_context_eval_string` (defined once in `runtime.c`), the same entry
  the embedding API uses.
- `make ci` fully green end to end: lint, api/header/include/ownership/
  build-graph audits, C 93/93, Lisp 5/5, examples 62/62, smoke.

**Status:** Phases 0 and 1 complete; #7 complete; Lizard 0.2 "Recoverable
Core" milestone **met**.  Next frontier is **Phase 2** (representation split:
Surface / Core / Kernel / Value — kernel sees only `KernelTerm`; an ill-typed
term is rejected even with a buggy elaborator).

### Phase 2 progress — kernel soundness: gap found + fixed

- Confirmed the kernel API is already `KernelTerm`-only: `kt_infer`,
  `kt_check`, `kt_define`, `kt_equal`, `kt_whnf` all take `kterm_t *`; the
  kernel never sees runtime AST.  AST reaches the kernel only via the
  `kernel_sexp.c` converter, after which the kernel re-checks.
- Added `tests/kernel_soundness_test.c`: an *adversarial* test that hand-builds
  ill-typed `KernelTerm`s (no surface syntax, no elaborator) and asserts the
  kernel rejects them — directly exercising Phase 2's done-when invariant
  ("an ill-typed term is rejected even with a buggy elaborator").  Cases:
  applying a non-function, applying a type, `succ` of a Bool, argument-type
  mismatch, `Pi`/`Lam` with a non-type domain, an unbound variable, and `refl`
  of an ill-typed value, plus well-typed sanity cases.
- This surfaced a real soundness gap: `kt_infer`'s `KT_PI` case required the
  domain to be a type (its inferred type is a `Sort`), but the `KT_LAM` case
  did **not** — so `lambda (x : 0). x` was accepted, producing a `Pi` with a
  non-type domain.  Fixed `kt_infer` to check the lambda's domain is a type,
  mirroring `Pi`.  The kernel now rejects all 8 adversarial terms; full suite
  green at C 94/94, Lisp 5/5, examples 62/62.
- This is the first concrete Phase 2 hardening of the trusted core (beyond the
  Phase 2A representation scaffolding in `term_boundary_test.c`).  Remaining
  Phase 2 work: route the evaluator/checker through the Core→Kernel converters
  end to end, and extend the adversarial suite as more eliminators land.

### Phase 2 progress — kernel soundness sweep across the term surface

- Systematically audited every implemented `kt_infer` case against the
  adversarial test and found **five more** soundness gaps where the kernel
  accepted ill-typed introduction forms.  All fixed in `src/kernel.c`:
  - `KT_CONS_K` ignored the tail entirely — `cons 0 true` typed as `List Nat`.
    Now the tail must be a `List` whose element type equals the head's type.
  - `KT_NIL_K` did not check its element annotation is a type — `nil {0}` was
    accepted.  Now the annotation must infer to a `Sort`.
  - `KT_INL` / `KT_INR` did not check the *other* summand annotation is a type
    (`inl 0 {0}` accepted).  Both now require it to be a `Sort`.
  - `KT_ID` checked only that the carrier is a type, never that the endpoints
    inhabit it — `Id Nat true false` was accepted as a type.  Now both
    endpoints are `kt_check`ed against the carrier.
- Implemented sound typing for the `absurd` eliminator (`absurd {A} : Empty →
  A`, requiring `A` to be a type) — the first eliminator the kernel types
  rather than rejects.  The dependent recursors (`nat_rec`/`bool_rec`/
  `list_rec`/`maybe_rec`/`sum_rec`/`J`) remain unimplemented in `kt_infer` and
  are therefore *rejected* (NULL = sound but incomplete); the suite locks this
  in so a future dependent-eliminator implementation must be added
  deliberately, with its own adversarial cases.
- `tests/kernel_soundness_test.c` now covers **29** cases (introduction forms,
  applications, projections, annotations, the Id carrier/endpoints, list and
  sum constructors, `absurd`, and the rejected recursors), all green.  Full
  suite: C 94/94, Lisp 5/5, examples 62/62, all audits.
- **Net across the two Phase 2 turns: six real soundness gaps in the trusted
  kernel found and fixed** (lambda domain; cons tail; nil/inl/inr type
  annotations; Id endpoints), plus one eliminator now soundly typed.  The
  kernel's entire *introduction-form* surface is now adversarially verified to
  reject ill-typed terms regardless of how they were produced — Phase 2's
  central invariant, now regression-guarded.

### Phase 2/7 progress — kernel now types every dependent eliminator

- The kernel previously *reduced* the eliminators (via `kt_whnf`) but `kt_infer`
  rejected them (`default → NULL`: sound but incomplete), so it could not check
  a proof by induction.  Implemented sound dependent type inference for **all
  six** eliminators in `kt_infer`, each rule derived directly from the kernel's
  own reduction semantics so typing and computation agree:
  - `bool_rec C t f b : C b` (motive `Bool → Sort`).
  - `nat_rec C z s n : C n` with `s : Π(k:Nat).Π(_:C k). C (succ k)`.
  - `maybe_rec` / `sum_rec` with case functions `Π(a:A). C (just a)` /
    `Π(a:A). C (inl a)`, `Π(b:B). C (inr b)`, the summand/element types read
    from the scrutinee's inferred type.
  - `list_rec C n c xs : C xs` with
    `c : Π(h:A).Π(t:List A).Π(_:C t). C (cons h t)`.
  - `J C d A a b p : C a b p` (Martin-Löf path induction; from the reduction
    `J … a a (refl a) → d a`, the motive ranges over both endpoints and the
    path, and `d : Π(x:A). C x x (refl x)`).
- The implementation uses an "apply the motive and check" strategy (build the
  expected case type by applying the motive to the relevant constructor and
  `kt_check` the case against it) rather than destructuring the motive's `Pi`,
  which keeps the de Bruijn bookkeeping localized to the case-function types.
  Each rule is locked by a positive case (a well-typed elimination with a
  constant motive — e.g. a length-shaped `list_rec` step) plus adversarial
  cases (mismatched case/branch types, wrong scrutinee, non-`Id` proof, missing
  motive).  A motive-less recursor is still rejected (no regression: the
  surface converter's non-dependent `bool_rec` was never kernel-typed anyway).
- Also typed the `absurd` eliminator (`absurd {A} : Empty → A`).
- `tests/kernel_soundness_test.c` now spans **47** cases; full suite green at
  C 94/94, Lisp 5/5, examples 62/62, all audits.  The trusted kernel is now
  both **sound and complete across its entire term surface** — it can check
  proofs by induction and path induction, the foundation Phase 7 (term-first
  proving) builds on.

### Phase 3B progress — #7 second monster + the checker god-function

- **#7's second named monster, `tt_equality.c`, is split.**  6819 → 5706
  lines.  The cohesive conversion/normalization engine (substitution,
  alpha-equality, term constructors, memoization, head rewrites) stays
  together — fragmenting it would be the wrong move — but the self-contained
  appended *registries* came out into modules: `tt_logic.c` (844: the HIT
  registry, logic-rule configuration + named bundles, and universe ordering)
  and `tt_faces.c` (292: the face-entailment decision procedure, system
  lookup, and the TT/flag Lisp primitives).  A new internal header
  `tt_internal.h` shares the three engine helpers the registries genuinely
  need (`contains_free_var`, `universe_set_subset`, `couniverse_set_subset`)
  plus the flag type/accessor.  The engine never touched the registries' state
  directly (only their header-declared accessors), so the cut was clean.  With
  `primitives.c` (done earlier) this means **#7 is complete on both named
  monsters.**
- **`tt_check.c`'s 2150-line god-function is decomposed.**  `infer2_kind_impl`
  was a single switch over 74 node kinds.  Because the cases share no
  function-level locals and recurse through the *public* `lizard_tt_infer2`
  entry, contiguous theme-clusters could be lifted into well-named dispatched
  helpers without changing behaviour, each a coherent sub-theory:
  `tt_check_modal.c` (S4 modal: Box/Diamond intro/elim/app/bind),
  `tt_check_cubical.c` (interval, paths, faces, partial elements,
  comp/hcomp/fill, equivalences/Glue/ua), `tt_check_fresh.c` (dimension- and
  couniverse-creating fresh binders, Phase L.3/L.5), and `tt_check_hit.c`
  (the circle S1, propositional truncation with its coherence rule, and the
  general HIT forms).  The main switch dispatches those node kinds to the
  helpers; shared checker helpers (`ctx_lookup`, `ctx_extend`, `type_error`,
  `is_error`, `make_path_app_local`) live in a new `tt_check_internal.h`.
  `tt_check.c` 2762 → 1359, and the god-function itself **2150 → 742 lines**.
  What remains in it is exactly the core dependent-type theory (universes,
  Pi/Sigma, App, Id/Refl/J, projections, Sum, …) which is genuinely cohesive
  and stays together.  This is a real quality fix — a 2000-line function is a
  defect — not just code-moving.
- Net this phase: **seven** new focused modules across the two splits, the
  three biggest files all meaningfully reduced, full suite green throughout
  (C 94/94, Lisp 5/5, examples 62/62, all audits including ownership/
  build-graph updated for the relocations).

### Phase 7 progress — the surface-to-kernel proving loop is closed

- The kernel became sound *and* complete (it types every eliminator), but
  that power was unreachable: the surface→KernelTerm converter
  (`kernel_sexp.c`) had no syntax for most eliminators, so no one could feed an
  inductive proof to the trusted core.  Closed that gap.  The converter now
  accepts dependent `boolrec`, `listrec`, `mayberec`, `sumrec`, `absurd`, and
  `let`, plus an annotated empty list `(nil A)` so list eliminations are
  typeable, and a reader-safe de Bruijn form `(var N)`.
- (The bare `#N` notation the converter originally expected is a dead branch:
  the Lisp reader consumes `#0` as the boolean `#f` before it ever reaches the
  converter.  `(var N)` is the reader-safe replacement; the demonstration
  example and the new test use it.)
- **End to end, verified:** `tests/kernel_surface_test.c` drives the *whole*
  user-facing pipeline — a theorem in concrete surface syntax is tokenized,
  parsed, converted to a KernelTerm, and `kt_check`ed via the `kernel-check`
  primitive.  Positive cases for all six eliminators check (#t); adversarial
  cases (wrong branch type, false equation) are rejected (#f), so the surface
  path is demonstrably sound, not just expressive.
- **The headline theorem:** reflexivity proven *by induction*.  With motive
  `C n := Id Nat n n`, base `refl 0`, and step `refl (succ k)`, the `nat_rec`
  term inhabits `C 2`, and the trusted kernel verifies it against
  `Id Nat 2 2` — while rejecting the same proof offered for the false equation
  `Id Nat 2 0`.  This is the Phase 7 "done-when": a non-trivial theorem stated
  in surface TT, elaborated, and kernel-checked.  Shipped as a passing
  demonstration (`examples/127-kernel-induction.lisp`).  Suite: C 95/95,
  Lisp 5/5, examples 63/63, all audits.

### Phase 7 progress — a referenceable proof library

- Named binders already resolved (a free-binder environment in the converter
  maps a bound name to its de Bruijn index, so proofs read with `n`/`k`/`ih`
  rather than raw indices).  The remaining gap was *reuse*: `kernel-define`
  type-checked and stored a definition, but the converter had no way to
  *reference* a stored name in a later term, so every proof started from
  scratch.
- Closed it.  The per-runtime kernel definition context is now exposed
  (`lizard_kernel_defs`, made null-safe for runtime-less heaps), and the
  converter resolves a free name — after local binders, which shadow it — to
  its stored, closed definition value, inlining it.  So a library of verified
  lemmas accumulates: `(kernel-define 'two '(succ (succ zero)) 'Nat)`,
  `(kernel-define 'refl-all '(lam (n Nat) (refl n)) '(Pi (n Nat) (Id Nat n n)))`,
  then `(kernel-check '(app refl-all two) '(Id Nat two two))` checks, and a
  further definition `two-eq` can be built on top and reused downstream.
- Soundness is preserved across the library: the same lemma is rejected when
  offered for the false equation `Id Nat two zero`.  Shipped as a
  self-checking demonstration (`examples/128-kernel-library.lisp`, which
  `raise`s on a wrong verdict so it doubles as a regression guard).  Suite:
  C 95/95, Lisp 5/5, examples 64/64, all audits.

### Phase 7 progress — axioms (postulates) via opaque kernel constants

- Definitions are inlined, which can't express a *postulate* (an axiom has no
  value to inline).  Added a first-class opaque constant to the trusted
  kernel: a new `KT_CONST` tag carrying just a name.  It is closed (identity
  under shift/subst), opaque under whnf (never reduces), equal only to a
  same-named constant, and `kt_infer` types it by looking its declared type up
  in the definition context.  To reach that context from inside the kernel,
  `kctx_t` gained a `defs` pointer (a `void *` to avoid a header cycle) that
  `kctx_extend` now propagates through binders — the one real bug found and
  fixed along the way (a `Pi` codomain under a binder couldn't see the defs).
- New primitive `(kernel-assume name type)`: checks the postulated type is a
  well-formed type, then stores it with a NULL value.  The converter resolves
  a free name to an inlined value for a definition, or to an opaque `KT_CONST`
  for an axiom; local binders shadow both.
- This makes proving *from* postulates work: with `P Q R : Sort 0`,
  `imp : P → Q`, `qr : Q → R`, and `p : P` assumed, the kernel checks modus
  ponens `(app imp p) : Q` and the composition
  `(lam (x P) (app qr (app imp x))) : (Pi (x P) R)` (transitivity of
  implication), while rejecting `p : Q` and the mis-ordered composition —
  soundness is intact, the axioms are simply trusted at their stated types (as
  axioms are, by definition).
- Guarded by a defensive kernel-soundness case (an unknown `KT_CONST` with no
  definition context is rejected, not a crash) and a self-checking example
  (`examples/129-kernel-axioms.lisp`).  Suite: C 95/95 (soundness now 49
  cases), Lisp 5/5, examples 65/65, all audits.

### Phase 7 progress — delta-transparent definitions

- Definitions had been *inlined* at parse time (a wart): the kernel never saw
  the name, only its expansion, so error messages and proof terms lost the
  abstraction.  Unified the constant story instead.  The converter now
  resolves *every* defined name — definition or axiom — to the same opaque
  `KT_CONST`; the difference lives entirely in the kernel's `kt_whnf`, which
  delta-unfolds a constant whose definition has a value (returning the whnf of
  its body) and leaves an axiom (no value) opaque.  This mirrors how `kt_whnf`
  already delta-unfolds a local let-bound variable.
- The effect: a definition is now both *abstract* (it shows as its name, and
  the kernel reasons about `two` rather than `(succ (succ zero))`) and
  *definitionally equal to its body*.  So `(refl two)` checks against
  `(Id Nat two (succ (succ zero)))` — which requires unfolding `two` — while
  `(Id Nat two (succ zero))` is rejected.  No infinite unfolding is possible:
  a definition's value is checked before it is added, so it cannot reference
  itself, and circular definitions cannot both be formed.
- Demonstrated and guarded in `examples/128-kernel-library.lisp` (the existing
  proof-library demo now also exercises delta).  Suite: C 95/95, Lisp 5/5,
  examples 65/65, all audits.

### Phase 7 progress — interactive tactics that actually build (and verify) proofs

- A tactic engine already existed (`tactics.c`: `intro`/`exact`/`refl`/`apply`/
  `assumption`/`simpl`/`split`/`left`/`right`, with a goal list), but it was
  hollow: it tracked goal *closure* correctly yet never *assembled the proof
  term*.  `intro` built `lam(x, A, <hole>)` and `apply` built `app(f, <hole>)`,
  but nothing ever filled the holes, so `qed` returned skeletons like
  `(lam (x : P) ?)` — `P → Q → P` came out as just `(lam (x : P) ?)`, missing
  the inner lambda and body entirely.  No real proof term, nothing checkable.
- Fixed it with a *slot* mechanism: each goal carries `kterm_t **slot`, a
  pointer to the hole in its parent term.  A helper `goal_solve` writes a
  goal's solution through its slot, and tactics that introduce structure point
  their subgoals' slots at the skeleton's holes (`&lam->data.lam.body`,
  `&app->data.app.arg`, the pair fields, the inl/inr value).  Because holes are
  shared pointers, solving a subgoal updates the parent in place, so the term
  assembles bottom-up automatically.  `P → P` now yields `(lam (x : P) #0)` and
  `P → Q → P` yields `(lam (x : P) (lam (y : Q) #1))` — correct de Bruijn.
- **`qed` is now the LCF trust anchor.**  Tactics merely assemble a term; `qed`
  has the kernel independently re-check it against the original goal
  (`kt_check`), so a bug in any tactic cannot mint a false theorem.  And
  `(qed name)` stores the verified theorem as a reusable library lemma — so an
  interactive proof becomes a delta-transparent constant usable in later proofs
  (`(app id-P p) : P`), unifying tactics with the proof library.
- Demonstrated/guarded by `examples/130-tactic-proofs.lisp` (three theorems
  proved interactively, verified, stored, reused, with soundness checks).
  Suite: C 95/95, Lisp 5/5, examples 66/66, all audits.

### Phase 7 progress — eliminator tactics (cases / induction) and a latent kernel bug

- Added `tactic-cases` (Bool, via `bool_rec`) and `tactic-induction` (Nat, via
  `nat_rec`).  Both act on the innermost hypothesis (de Bruijn #0).  The motive
  is synthesised by abstracting the goal over that variable —
  `motive = \y. shift(goal, 1, 1)` (cutoff 1 keeps the variable's own
  occurrences for the new binder while lifting the rest of the context past
  it).  Subgoal types come from `whnf(app(motive, constructor))` — the kernel
  does the de Bruijn arithmetic, not the tactic.  `cases` yields one subgoal
  per constructor; `induction` yields a base subgoal and a step subgoal
  `Pi k. motive k -> motive (succ k)` carrying the induction hypothesis.  The
  assembled eliminator is re-checked by the kernel at `qed`, as always.
- The headline is boolean involution, `not (not b) = b`, proved interactively
  by `(intro b)(cases)(refl)(refl)`.  It genuinely needs case analysis: with
  `b` a variable, `not (not b)` is stuck and is not definitionally `b`; only
  after casing does each branch compute.
- **Latent kernel bug surfaced and fixed (one class, three sites).**  The
  eliminator tactics are the first code path to push *variable-scrutinee*
  eliminators through the kernel's structural operations, which exposed that
  `kt_subst`, `kt_shift`, and `kt_equal` all lacked cases for the eliminator
  nodes (`nat_rec`/`bool_rec`/`list_rec`/`maybe_rec`/`sum_rec`/`J` and the
  small constructors) and fell through to a `default`.  The result: `subst`/
  `shift` silently left an eliminator's scrutinee untouched (so a definition
  like `not = \b. boolrec(.., b)` got stuck when unfolded and applied), and
  `kt_equal` reported a *neutral* eliminator unequal to itself.  All three
  functions now recurse into every eliminator field.  Three adversarial checks
  were added to `kernel_soundness_test.c` (now 52 cases): a neutral `bool_rec`
  equals an identical copy, differs from a branch-swapped one, and reduces
  correctly when its scrutinee variable is substituted.
- Demonstrated/guarded by `examples/131-eliminator-tactics.lisp`.  Suite:
  C 95/95, Lisp 5/5, examples 67/67, all audits.

### Phase 7 progress — the elaborator: term-first proving with holes

- This puts the Phase 7 "done-when" in reach: a theorem stated as a surface
  term, elaborated, and kernel-checked **with no tactics**.  Added
  `src/elaborator.c` — a bidirectional elaborator over the existing
  metavariable layer (`KT_META`, already accepted by the converter as `?0`,
  `?1`, ...).  It pushes the expected type *inward* (checking mode) so a hole
  in a checking position learns its *goal* (the type it must inhabit) and its
  local *context* (the hypotheses in scope).  A hole-free subterm is handed
  straight to the trusted kernel; the elaborator only descends where a hole
  lives, so it stays small and the kernel stays the single source of truth.
  When every hole is filled, the finished term is confirmed by `kt_check`.
- New primitive `(check-holes term type)` elaborates `term` against `type` and
  prints each open hole as `?id : goal` with its hypotheses; it returns #t only
  when no holes remain *and* the kernel verifies the term, #f when holes remain
  or the term does not fit.  Bidirectional rules cover lambda (against a Pi),
  application (the argument is checked against the domain, so an argument hole
  gets the domain as its goal), and the `bool_rec`/`nat_rec` eliminators (each
  branch is checked against the motive applied to its constructor — so a branch
  hole gets exactly the case it must prove, and Nat induction surfaces the base
  goal plus the step goal with its hypothesis).
- Headline: boolean involution written as `(lam (b Bool) (boolrec M ?0 ?1 b))`
  reports `?0 : (Id Bool (not (not true)) true)` and
  `?1 : (Id Bool (not (not false)) false)`; filling both with `(refl true)` /
  `(refl false)` yields "No holes remaining; kernel-verified."  A wrong branch
  is rejected.  The goals come from the term's structure, not from a tactic.
- Demonstrated/guarded by `examples/132-elaborator-holes.lisp`.  Suite:
  C 95/95, Lisp 5/5, examples 68/68, all audits.

### Phase 7 progress — unification-driven elaboration (the elaborator now solves holes)

- The elaborator graduated from *reporting* hole goals to *solving* them.  It
  now threads a metavariable context (`meta_ctx`) and, in its checking step,
  unifies the inferred type against the expected type with `kt_unify` instead
  of merely comparing with `kt_equal`.  A hole whose value is *determined* by
  its surroundings is solved automatically; a hole that is a genuine free
  choice (a proof obligation) stays open and is reported.  After elaboration
  the term is zonked (solved metas substituted) and the finished term is handed
  to the trusted kernel — unification is only a heuristic, the kernel is judge.
- Each surface hole `?i` is registered in the meta context with a fresh
  type-meta, so it can be *inferred* (e.g. `refl ?0` infers `Id τ ?0 ?0`) and
  then *unified* against the goal.  Fresh metas are numbered above the highest
  user hole so they never collide.  `(check-holes ...)` now prints
  `?i := <value> (inferred)` for solved holes and only lists genuinely open
  ones.  Effects: `(refl ?0) : Id Nat 2 2` solves `?0 := 2`;
  `(refl-all ?0) : Id Nat 2 2` infers the argument `?0 := 2` through the return
  type; the involution skeleton's two branches stay open; `(refl ?0) : Id Nat
  1 2` is rejected (no consistent `?0`).
- **Latent kernel bug surfaced and fixed (the same fall-through trap, a fourth
  site).**  `kt_unify` lacked structural cases for the eliminators
  (`nat_rec`/`bool_rec`/...), `J`, the projections, and `absurd`, so it could
  not compare the neutral `bool_rec` inside the involution goal and failed
  where `kt_equal` succeeded.  Added all the missing cases (mirroring
  `kt_equal`).  Two regression checks in `kernel_soundness_test.c` (now 54):
  a metavariable unifies with a concrete term, and two identical neutral
  `bool_rec` unify.
- Demonstrated/guarded by `examples/133-elaborator-unification.lisp`.  Suite:
  C 95/95, Lisp 5/5, examples 69/69, all audits.

### Phase 7 progress — implicit arguments (TT1 complete: holes + unification + implicits)

- A function type may now mark leading binders **implicit** with the surface
  form `IPi`: `(IPi (A (Sort 0)) (Pi (x A) A))` is `{A : Type} -> A -> A`.
  When such a function is applied, the elaborator inserts a fresh metavariable
  for each leading implicit Pi and solves it by unification against the
  explicit argument and the result type.  `(id (succ zero))` elaborates to
  `((id Nat) (succ 0))`; `(const2 true (succ zero))` infers both implicits to
  `((((const2 Bool) Nat) true) (succ 0))`.  All elaborated terms are zonked and
  kernel-checked.
- Kernel change is additive and ignored by the trusted checker: `pi` gained an
  `int implicit` flag, preserved through `kt_subst`/`kt_shift`/`meta_zonk` and
  ignored by `kt_equal`/`kt_unify`/`kt_whnf`/`kt_infer` (so `{A}->B` and
  `(A)->B` are the same type for conversion).  `kt_fprint` shows implicit
  binders with braces.  The elaborator was rewritten to **produce** an
  elaborated core term (not just check), so it can insert the implicit
  applications; a new `(elaborate term type)` primitive prints that term.
- **Two latent kernel bugs surfaced and fixed by this work:**
  - `kt_infer(VAR)` returned a variable's type **without shifting** it into the
    occurrence's context.  This is invisible whenever a variable's type is
    closed (Nat, Bool, a global constant) — which was every prior proof — but
    wrong under polymorphism, where a variable's type is itself a bound
    variable.  The polymorphic identity `\A.\x.x : (A:*)->(x:A)->A` was rejected
    (its body type came out as the value `#0` instead of the type `#1`).  Fixed
    with `kt_shift(e->type, 0, index+1)`.  Locked by two soundness checks.
  - `kt_unify` was missing the `KT_CONST` case (it shared the missing-default
    bug class), so `unify(A, A)` between identical constants returned 0 once an
    application routed through the elaborator instead of `kt_check`.  Added the
    name comparison; locked by two soundness checks.
- **The recurring "missing-default fall-through" bug class was audited to
  closure.**  Every structural `switch` over `kterm_tag_t` was swept.  The
  fifth and final instance was `meta_zonk`, which lacked the eliminator/`J`/
  `Sigma`/pair/projection/sum/`let` cases and so would never substitute a
  solved metavariable sitting inside an eliminator — exactly the path implicit
  insertion exercises.  All five structural functions (`kt_subst`, `kt_shift`,
  `kt_equal`, `kt_unify`, `meta_zonk`) now cover the whole term language;
  `kt_whnf` (default `return t` = neutral) and `kt_infer` (default `NULL` =
  safe-reject) are correct by design.
- Soundness suite now 58 (added polymorphic-identity accept/reject and
  unify-constant pairs).  Demonstrated/guarded by
  `examples/134-implicit-arguments.lisp`.  Suite: C 95/95, Lisp 5/5, examples
  70/70, all audits.

### Phase 7 progress — user-defined inductive types

- `(data '(Name (params) Sort (ctor (argtypes)) ...))` declares a parameterized
  inductive.  The kernel strict-positivity-checks it and registers the type
  former, every constructor, and an automatically generated **dependent
  eliminator** `Name-rec` carrying a real iota-reduction rule.  Worked end to
  end: recursive types (`Nat2`, with `add2` proving `1+1=2` by `refl`), finite
  enumerations (`Color`, with case analysis computing), and parameterized types
  with multiple recursive subterms (`Bush A`, whose eliminator has two
  induction hypotheses and computes `size (fork leaf 0 leaf) = 3`), plus
  parameterized lists with length.
- **Representation: constants + application, no new term tags.**  The type
  former and each constructor are opaque `KT_CONST`s whose types live in the
  definition context; `Name p1..pn` and `c a1..ak` are ordinary `KT_APP`
  spines.  The eliminator is a constant with an iota rule applied in `kt_whnf`:
  `D-rec p.. M m.. (c_i p.. a..)` reduces to `m_i a.. (D-rec .. a_rec)..`, one
  recursive call per recursive argument.  Because everything reuses
  `APP`/`CONST`/`PI`, `kt_subst`/`kt_shift`/`kt_equal`/`kt_unify`/`meta_zonk`
  are entirely unchanged, and typing falls out of the registered constant types
  for free.
- **The soundness surface is exactly two things:** the strict-positivity check
  (`kind_declare` → `classify_arg`, which accepts an argument that is either
  free of the inductive or *exactly* `Name <params>`, and rejects every other
  occurrence — in particular a negative occurrence in a Pi domain, blocking
  `data Bad = mkBad (Bad -> Empty)`) and the iota rule (`try_iota`).  As a guard
  against synthesis bugs, each generated type (former, constructors, recursor)
  is sanity-checked with `kt_infer` at declaration time and rejected if it is
  not a well-formed type; the eliminator type itself was derived from the
  worked-out de Bruijn telescope and verified by inspection against `Nat` and
  the binary-recursive `Bush`.
- The surface→kernel converter gained `lizard_kernel_sexp_to_kterm_in`, which
  binds a list of parameter names so constructor/parameter types convert in the
  parameter telescope without manual de Bruijn bookkeeping.
- Limitations (honest next steps): parameters only, **no indices** (no `Vec`,
  `Fin`, or `Id`-as-inductive yet); constructor arguments may reference the
  parameters but not earlier arguments (a uniform-shift simplification); a
  recursive argument must be a direct `Name <params>` (no nested or
  higher-order recursion such as `A -> Name`); and the motive eliminates into
  `Sort sort_level` (no large elimination into a higher universe).
- Soundness suite now 60 (added strictly-positive-accepted and
  negative-occurrence-rejected).  Demonstrated/guarded by
  `examples/135-inductive-types.lisp`.  Suite: C 95/95, Lisp 5/5, examples
  71/71, all audits.

### Phase 7 progress — TT3: term-mode proving (the "no tactics" done-when)

- `(def name type term)` and `(theorem name type term)` state a definition or
  theorem in surface type theory and prove it by giving a **term** — no
  tactics.  Each elaborates the term against the type (inserting implicit
  arguments, solving holes by unification), reports any open goals with their
  local context and goal, and on success kernel-checks the elaborated term and
  stores it as a reusable δ-transparent definition.  The trusted kernel
  re-checks every result, so the (untrusted) elaborator cannot make an unsound
  statement go through.
- This closes Phase 7's own done-when: *a non-trivial theorem stated in surface
  TT, elaborated, and kernel-checked with no tactics.*  Demonstrated end to end
  in `examples/136-term-mode-proving.lisp`:
  - `comp : {A B C} (B->C) -> (A->B) -> A -> C` — a definition using implicit
    arguments, stored and reusable;
  - `refl-thm : {A} (x:A) -> Id A x x` proved by `\x. refl x`;
  - `neg-neg : (b:Bool) -> Id Bool (neg (neg b)) b` proved by the Bool
    eliminator as a term, then reused as a lemma (`neg-neg true`);
  - `add2-right-zero : (n:Nat2) -> Id Nat2 (add2 n z2) n` — a genuine
    **induction**, written as `Nat2-rec` applied to a motive, the base proof,
    and a successor method that is congruence `cong s2 ih` (congruence proved
    once via `J`); all implicit arguments to `cong` are inferred, and the
    successor goal closes because `add2 (s2 k) z2` ι-reduces to `s2 (add2 k z2)`;
  - an incomplete proof (`\b. ?0`) reports its open goal and local context
    instead of being accepted.
- To make real proof terms elaborable, the elaborator's bidirectional rules now
  cover the rest of the core type language: `Id`, `Pi`, `Sigma`, the
  projections, and the `List`/`Maybe`/`Sum` formers in inference position, plus
  pair-against-`Sigma` in checking position.  Type-former and projection
  subterms are elaborated and then typed by the kernel directly, which keeps the
  elaborator consistent with the kernel by construction.  (`J` in a bare term is
  not yet elaborated — proofs that need it wrap it in a raw definition, as
  `cong` does.)
- No kernel change was required; soundness stays at 60.  Suite: C 95/95, Lisp
  5/5, examples 72/72, all audits.  Remaining Phase 7 work is the cubical
  metatheory track (TT2: Path types, the interval, `transp`/`hcomp`), which is
  the other half of the phase's done-when.

### Phase 7 progress — TT2: the cubical layer (interval, Path types, ap)

- The kernel now has a cubical fragment: an interval `I : Sort 0` with endpoints
  `i0`/`i1` (and no eliminator — the interval is not inductive, which keeps it
  sound), path types `Path A a b`, path abstraction `(plam i body)` binding an
  interval variable, and path application `(papp p r)`.  Surface syntax: the
  symbols `I`/`i0`/`i1` and the forms `(Path A a b)`, `(plam i body)`,
  `(papp p r)`.
- Two computation rules, both in `kt_whnf`:
  - **beta:** `(plam i. t) @ r` reduces to `t[i := r]`;
  - **boundary:** `p @ i0` reduces to the left endpoint and `p @ i1` to the
    right.  Crucially this fires even when `p` is *neutral* (a variable or stuck
    term): the endpoints are recovered by inferring `p`'s type, which must be a
    `Path`.  `kt_infer(PLAM)` reads the endpoints off the body by substituting
    `i0`/`i1` and verifies the resulting `Path` is well-formed, which rejects
    dependent paths (PathP), unsupported in this fragment.
- Making the neutral boundary work required `kt_equal` and `kt_unify` to **extend
  the typing context as they descend `Pi`/`Lam`/`Sigma` binders** (previously
  they compared bodies in the outer context, relying on de Bruijn alone).  This
  is what typed conversion does and is sound — it only adds type information,
  never δ-reductions (the entries carry `value = NULL`) — and it is what lets the
  boundary's `kt_infer` find a bound path variable's type under binders.  The
  cubical tags are threaded through `subst`/`shift`/`whnf`/`equal`/`unify`/
  `infer`/`zonk`/`fprint`/`const_occurs`, and `Path`/`plam`/`papp` are in the
  elaborator too, so term-mode `def`/`theorem` can use paths.
- Demonstrated in `examples/137-cubical-paths.lisp` (kernel level) and in
  term mode: `plam i. x : Path A x x` (refl as the constant path); an ill-typed
  path is rejected; the neutral boundary `p @ i0 = x` / `p @ i1 = y` for an
  assumed `p : Path A x y`; beta `(plam i. i) @ i1 = i1`; and the general
  `ap : {A B} (f:A->B) {a b} -> Path A a b -> Path B (f a) (f b)
     := \A B f a b p. plam i. f (p @ i)`,
  whose lifted path computes (`(ap f p) @ i0 = f x`).  `ap-refl-thm` proves
  `Path B (f x) (f x)` as the term `ap f (plam i. x)` with all implicit
  arguments inferred at the call site.
- Soundness gains four cubical regressions (`plam i.0 : Path Nat 0 0` accepted /
  `Path Nat 0 1` rejected; the neutral boundary; path beta), now 64.  Suite:
  C 95/95, Lisp 5/5, examples 73/73, all audits.
- **This closes both halves of Phase 7's done-when:** a non-trivial theorem
  stated in surface TT, elaborated and kernel-checked with no tactics (TT3), and
  ≥1 cubical milestone (TT2).  The next cubical milestones are the Kan
  operations (`transp`/`hcomp`) and ultimately `Glue`/univalence; this fragment
  has none of those and is sound on its own (no transport, no `PathP`, no
  path-eta).

### Phase 7 progress — named variables make the proving surface human-writable

- The proving surface still forced raw de Bruijn indices (`(var 0)`,
  `(var 1)`), which is expert-only and error-prone.  Added a name-resolution
  pass to the converter: binders (`lam`, `Pi`, `Sigma`, and the new `klet`)
  push their binder name onto a small environment around the conversion of
  their body, and a symbol that matches a binder resolves to its de Bruijn
  depth (innermost = 0).  Implemented with a stack-allocated frame per binder
  and an unconditional push/pop, so it needs no rewrite of the ~40 recursive
  call sites and stays balanced even on error returns.  `(var N)` remains as
  an explicit escape hatch.
- Now `(lam (x Nat) x)`, `(lam (x Nat) (lam (y Nat) x))` (cross-binder
  resolution), and shadowing (`(lam (x Nat) (lam (x Bool) x))` — the inner
  binder wins) all check; the induction theorem reads as
  `(natrec (lam (n Nat) (Id Nat n n)) (refl zero) (lam (k Nat) (lam (ih (Id Nat k k)) (refl (succ k)))) 2)`
  instead of a wall of indices.
- Added a `klet` form for definitional let — `let` itself was unusable as a
  kernel keyword because the Lisp reader's `let` special form claims the
  quoted datum (and its parser even segfaults on the non-standard binding
  shape, a latent robustness bug worth fixing separately).  Also gave the
  kernel a `KT_LET` typing rule in `kt_infer` (type the body under the
  binding, substitute the value back so dependent body-types stay correct),
  matching the existing reduction rule; guarded by adversarial soundness
  cases.
- Verified: `tests/kernel_surface_test.c` checks the named identity, K, and
  shadowing terms, named `J`, named `klet`, the named induction theorem, and
  rejects an unbound name; `kernel_soundness_test.c` is up to 49 cases (with
  the `KT_LET` accept/reject).  Suite green: C 95/95, Lisp 5/5,
  examples 63/63, all audits.

### Phase 3 progress — real module namespaces (module / export / import)

- Until now `import` only re-evaluated a file in the *caller's* environment (with
  load-once caching and a search path), so two libraries defining the same name
  would clobber each other — Phase 3's "colliding names don't interfere" was not
  met.  There is now a real module system layered on top of that loader.
- **Model.**  A *module* is evaluated in a hermetic environment: a child of a
  pristine primitives-only base env (`module_base_env`, built once per runtime),
  so a module's private definitions are invisible to the importer and to other
  modules.  `export` names the public bindings; with no `export` form, every
  top-level definition is public.  A loaded module is recorded as a
  `lizard_namespace_t { name; env; exports; }` in the runtime's namespace list.
- **Surface.**  `(module NAME body...)` defines a hermetic namespace.
  `(export a b ...)` marks exports.  `(import SRC ...)` brings a namespace's
  exports into the caller — qualified `alias:name` by default, or unqualified
  with `:only (a b)` / `:all`; `:as alias` renames the qualifier.  `SRC` is a
  registered namespace name (symbol) or a file path (string); a file is loaded
  as a hermetic module named by its basename (or `:as` alias).  Qualified names
  like `cas:simplify` need no reader changes — `:` is an ordinary symbol
  constituent, and the qualified binding is just `lizard_env_define` of the
  string `"prefix:name"`.
- **Legacy preserved.**  A lone-string `(import "match.lisp")` (no options) keeps
  its historic "load unqualified into the caller" behaviour by delegating to the
  original import primitive — all 131 existing import sites are unaffected.
- **Architecture.**  module/export/import are rewritten in `lizard_expand_macros`
  (the macro expander) into calls to three primitives, `lz:module` / `lz:export`
  / `lz:import`, with their payloads quoted.  This is done structurally in C
  rather than via `syntax-rules` (whose ellipsis-into-`quote` expansion is
  unreliable for 3+-element bodies) and needs no bootstrap, so it works the same
  inside a hermetic module body as at top level.  The module body is passed as a
  pair-chain of the *parsed* body forms; `lz:module` re-expands and evaluates each
  in the module's own environment, which is what makes `export`/nested `import`
  inside a module resolve correctly.  No new AST node kinds, so the bytecode
  path, printer, and GC are untouched.  Kernel definitions
  (`kernel-define`/`theorem`/`data`/axioms) remain global by design — modules
  namespace the Lisp environment, not the proof context.
- **Done-when met**, demonstrated in `examples/138-module-namespaces.lisp`
  (self-checking): two modules both exporting `foo` coexist as `alpha:foo`=1 /
  `beta:foo`=2; a private `helper` stays unbound in the importer; export/import
  round-trips a value; `:as`, `:only`, `:all`, and the export-all default all
  hold; and `cas.lisp` loads as a namespaced library (`cas:simplify (+ x 0)` =
  `x`), its own internal `(import "match.lisp")` resolved hermetically.  Suite
  green: C 95/95, Lisp 5/5, examples 74/74, soundness 64/64, all audits.
- Not done here (future Phase 3 polish): splitting the CAS into namespaced
  `lib/cas/{core,poly,...}.lisp` (item #10); and the load-once cache is keyed
  globally, so a file imported hermetically after it was already imported flatly
  returns the cached value rather than re-binding — fine for the demo, worth
  revisiting when modules and the legacy loader are unified.

### Phase 8 progress — proof-carrying CAS (kernel-checked differentiation)

- `lib/cas-proof.lisp` already attaches an *informal* justification (a chain of
  named rules bottoming out at ZFC) to each step, and `docs/CAS.md` names the
  next move: "state each [rule] as a kernel proposition ... a checked proof the
  kernel accepts."  That is now done for differentiation: a derivative emits a
  CERTIFICATE the trusted kernel type-checks.
- **Encoding — a typed derivative judgment, not equational rewriting.**  Over an
  abstract commutative ring `R` we postulate one relation
  `Der : (R -> R) -> (R -> R) -> Sort 0` ("g is the derivative of f"), and the
  differentiation rules as postulated *constructors* of it: `der_id` (the
  variable), `der_const` (a constant, `(c:R) -> Der (\x.c) (\x.0)`), `der_add`
  (`Der f f' -> Der g g' -> Der (\x. f x + g x) (\x. f' x + g' x)`), and `der_mul`
  (the product rule).  These four axioms *are* the "cited rules" of `docs/CAS.md`,
  now expressed as kernel propositions.  A derivative's certificate is simply a
  nested application of them; the kernel checks it because β-reduction lines the
  dependent types up — so none of the fragile `sym`/`trans`/`cong` equational
  plumbing is needed, and congruence-under-operators is avoided entirely by the
  judgment design.  Because the proof term must literally inhabit
  `Der (\x. f) (\x. f')`, a *wrong* derivative cannot be certified — the kernel
  refuses it.
- **The differentiator** (`diff`) recurses over a ring term in `x`, returning the
  derivative term *and* its proof, mirroring the four rules; `certify` then calls
  `kernel-check` on the proof against `Der (\x. e) (\x. e')`.  The trust anchor is
  the kernel: the differentiator is ordinary (untrusted) Lisp, but a derivation
  is only believed once the kernel has accepted the proof.  The fragment is
  polynomials — variable, constants, sums, products, and powers as repeated
  products (`x*x*x`); a dedicated power/scalar/chain rule would be more postulated
  constructors in the same style.
- **Packaged as a namespaced library** (`lib/cas/diff-cert.lisp`, the first
  resident of the `lib/cas/` tree from Phase 3 item #10), imported with
  `(import "cas/diff-cert.lisp" :as dc)`.  Caveat: `kernel-assume` is *global*
  kernel state, so importing the module postulates the ring + rules into the one
  shared proof context (the Lisp helpers are namespaced as usual) — proofs are
  global facts, which is the intended reading.
- **Done-when met.**  `examples/139-cas-certificates.lisp` (self-checking)
  certifies the polynomial cases (d/dx(x), d/dx(a), d/dx(x·x), d/dx(x·x·x), …)
  and the elementary/chain-rule cases (d/dx(sin x)=cos x, d/dx(exp x)=exp x,
  d/dx(ln x)=1/x, d/dx(sin(x·x)), d/dx(ln(sin x)), d/dx(x·sin x)), and shows the
  kernel *rejecting* two wrong claims
  (d/dx(x·x)=x and d/dx(x)=0).  Combined with Phase 3 (the CAS loads as a
  namespaced library), this satisfies Phase 8's done-when — "the CAS loads as
  libraries; a derivative emits a certificate the kernel accepts."  Suite green:
  C 95/95, Lisp 5/5, examples 75/75, soundness 64/64, all audits.
- Reserved-symbol caveat worth recording: the kernel converter reserves `I` (the
  cubical interval) and `zero`/`succ` (Nat constructors), so the ring's units are
  named `oneR`/`zeroR` to avoid silently mis-typing as `Interval`/`Nat`.

### Phase 4 — hygienic macros (COMPLETE)

All three done-when clauses are met: an imported macro expands hygienically; the
macro stepper shows located expansion; and a contract violation names the blaming
party.  `lib/macros.lisp`, `lib/macro-stepper.lisp`, `lib/contract.lisp`, and
`examples/140`–`142` exercise the whole phase.

- `syntax-rules` now supports the **ellipsis** (`...`), the feature that makes
  macros genuinely usable, plus the existing lightweight hygiene.  `lib/macros.lisp`
  (my-or, my-and, unless, swap!) and `examples/140-hygienic-macros.lisp` exercise it.
- **Matching.**  A pattern element immediately followed by `...` matches a prefix
  of the form: the sub-pattern consumes `len(form) − len(rest-pattern)` leading
  elements, and each pattern variable beneath the ellipsis binds to the *sequence*
  of its per-element matches at depth one greater.  The forms after the ellipsis
  match the remaining suffix.  Variables are collected structurally first, so an
  ellipsis matching **zero** elements still binds them (to empty sequences).
- **Expansion.**  A template element followed by `...` is expanded once per
  iteration: the iteration variables (those in the sub-template with depth ≥ 1)
  are found, the copy count is the shortest of their sequence lengths (lockstep),
  and for each `i` the variables are shadowed by their `i`-th element at depth−1
  before expanding the sub-template; the results are spliced in, then the rest of
  the template follows.  Because a depth-`d` value is a pair-chain of depth-`(d−1)`
  values, this generalises to **nested ellipsis** with no special case — verified
  on `(rows (x ...) ...)` and per-row `(+ x ...) ...`.
- **Hygiene.**  Template-introduced binders in `let`/`let*`/`letrec`/`lambda`
  positions are renamed to fresh gensyms (`name.hN`), so a macro's temporaries
  can't capture caller identifiers.  `examples/140` checks the classic cases —
  `(my-or #f t)` returns the caller's `t`, and `(swap! tmp other)` works even when
  the caller's variable is literally named `tmp`.
- **Imported macros.**  A macro defined in a file and brought in with
  `(import "macros.lisp")` expands hygienically in the importing code (done-when
  clause 1), on top of Phase 3's module system.
- **Macro stepper (done-when clause 2).**  Three new primitives support tooling:
  `macroexpand-1` performs ONE hygienic `syntax-rules` step on a datum (reusing the
  same matcher/expander as the full expander); `read-syntax` tokenises and parses a
  string into a datum WITH source spans (a one-line fix made `lizard_parse_datum`
  attach spans like the main parser); and `form-location` reads the span of a form's
  head.  `lib/macro-stepper.lisp` iterates `macroexpand-1` to a fixed point, printing
  each rewrite with the source location of the call it expanded — and the trace shows
  hygiene at work (renamed binders) and even traces template-introduced forms back to
  their definition site.  `examples/141-macro-stepper.lisp`.
- **Contracts with blame (done-when clause 3).**  `lib/contract.lisp` is a pure-Lisp
  Findler–Felleisen contract system: a flat (predicate) contract blames whoever
  produced the bad value, and `(-> dom rng)` checks the domain with the parties
  *swapped* (a bad argument blames the caller) and the range as-is (a bad result
  blames the function); since `rng` can itself be `(-> …)`, contracts are curried and
  higher-order.  Lizard's exceptions are values (`raise` yields an error object), so
  `protect` threads them through explicitly and a violation surfaces as an error
  object whose payload is `(contract-violation party name value)`.
  `examples/142-contracts-blame.lisp` checks that a bad argument blames the client, a
  bad result blames the implementation, and a bad *second* argument of a curried
  function still blames the client.
- **Beyond the done-when (future).**  The full Racket-style infrastructure would add
  proper syntax objects with scope-set hygiene replacing the gensym pass (the
  `AST_SYNTAX` type with `scopes`/`scope_count` already exists, used by
  `datum->syntax`/`syntax-source`) and `#lang` reader extensions (MM4).

### Language — exact rational arithmetic (numeric tower over ℚ)
- **What landed.**  Lizard now has a real numeric tower: integers stay exact
  `mpz` (`AST_NUMBER`), and any operation that produces a fraction yields an
  exact, fully-reduced rational backed by GMP `mpq` (a new `AST_RATIONAL`).  The
  representation is canonical — rationals always have denominator > 1, and a
  result that reduces to a whole number (`6/3`, `1/2 + 1/2`, `4/2`) automatically
  **demotes** back to an integer, so "integers are always `AST_NUMBER`" is an
  invariant the rest of the system can rely on.
- **`/` is now exact.**  Previously `(/ 7 2)` truncated to `3` (`mpz_tdiv_q`); it
  now returns `7/2`.  `+ - * expt ^` accept and produce rationals (the fast
  integer paths are unchanged when no operand is rational), and `< <= > >= =`
  and `zero?/positive?/negative?` compare exactly across integers and rationals.
- **Reader + printer.**  Rational literals are written `n/m` (the tokenizer
  consumes the `/m` suffix, canonicalizes, and demotes `4/2` → `2` at read time);
  the printer shows reduced fractions via `%Qd`.
- **New primitives.**  `rational?`, `real?`, `integer?`, `exact?`, `numerator`,
  `denominator`.  Rationals also work as keys in persistent maps/sets and the
  collection hash (equal rationals like `2/4` and `1/2` collide/merge correctly).
- **Scope.**  This is a *runtime/value* feature; it stays out of the trusted
  kernel term language (the kernel still uses `mpz` for `Nat`), so soundness is
  unaffected (`kernel_soundness_test` 64/64 still green).  Covered by
  `examples/144-exact-rationals.lisp` (40 checks) and `tests/rationals_test.c`.

### Phase 5 — per-object conservative GC (the hard gate)
- **What landed.**  `(gc)` is now a per-object, **non-moving, fully
  conservative** mark & sweep.  It reclaims *individual* dead objects (not just
  whole segments as before) into per-size free lists that the bump allocator
  reuses on the next allocation; the addresses of surviving objects never move.
- **Why it is sound.**  Because the collector never relocates objects, treating
  a non-pointer as a pointer can only *retain* garbage, never corrupt — so a
  conservative scan is safe.  Roots are discovered **root-completely by
  construction**: the C stack, the callee-saved registers (spilled with
  `setjmp`), and the entire data/BSS segment are scanned, so there is no manual
  root list that could be missing an entry.  (An audit confirmed the only
  file-scope AST pointers are `callcc_value_fallback`, `lizard_jump_value`, and
  the runtime struct, and that there is no symbol-intern table; the runtime
  env / module results / namespace frames are also marked explicitly as
  insurance.)  Live objects are traced by scanning their own bytes for tracked
  pointers, which needs no per-type tracer.
- **One real bug, fixed.**  The first cut segfaulted intermittently (and under
  valgrind) in the stack scan: deriving the stack top by parsing
  `/proc/self/maps` `[stack]` is wrong — its reserved upper bound need not be
  mapped, and under valgrind it is valgrind's stack.  Switching to glibc's
  `__libc_stack_end` (the client's true initial SP, always within committed
  stack) fixed it; valgrind then reports no invalid reads/writes.
- **Tests.**  `tests/gc_objects_test.c` checks reclamation > 0, that diverse
  live data (lists, HAMT map, set, rational, closure) survives 5 cycles of
  *collect-then-reuse*, and that a survivor's address is unchanged (non-moving).
  `examples/145-object-gc.lisp` does the same end-to-end over 6 cycles.
- **Honest limitations.**  (1) The sweep does not `mpz_clear`/`mpq_clear` swept
  numbers (the size-inferred `kind` is not exact enough to do so safely against
  a same-sized string buffer), so GMP limbs of dead numbers leak while the
  object chunk is reclaimed — a future exact-tagging pass closes this.  (2) GC
  is **explicit** via `(gc)`; no automatic collect-on-allocation yet.  (3) Being
  conservative, it can retain a little genuine garbage.  Net: Phase 5's done-when
  (reclaims individual objects, addresses stable, no `make ci` regressions) is
  met, and the real-threads half of Phase 6 is unblocked.

### Phase 6 — persistent vector becomes a bit-partitioned trie
- **What landed.**  The persistent **vector** was upgraded from a flat
  copy-on-write array (whose `pvec-set`/`pvec-push` copied the whole backing
  store, O(n)) to a **32-way bit-partitioned trie with a tail buffer** — the
  Clojure PersistentVector design (`src/pvector.c`).  `pvec-push` is now O(1)
  amortized, `pvec-ref`/`pvec-set` are O(log32 n), and an update **path-copies**:
  it allocates only the O(log32 n) nodes along one root-to-leaf path and shares
  every other node with the original.
- **Why it matters.**  Vectors now have the same structural-sharing guarantee
  the HAMT maps and sets got earlier, so the whole immutable-data half of
  Phase 6 is consistent and cheap to share.  Building/​updating large vectors is
  no longer quadratic.
- **Surface.**  Existing `pvec`, `pvec-ref`, `pvec-set`, `pvec-push`,
  `pvec-count`, `pvec->list`, `pvec?` keep working; added `pvec-pop` and
  `list->pvec`.  The mutable-vector bridge (`transient!` / `persistent!`)
  continues to work over the new representation.
- **Tests.**  `tests/pvector_test.c` checks correctness across the 32 / 1024
  trie-growth boundaries, persistence (the original is unchanged after an
  update), pop across a boundary, and — at the node level — that off-path leaves
  are pointer-identical between an original and its update while the on-path leaf
  is a fresh copy (i.e. genuine structural sharing).  `examples/146` mirrors this
  end-to-end over a 3000-element vector.

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

### Phase 4 — Syntax objects + hygienic macros (Racket infrastructure)  ✅ COMPLETE
- **Goal:** macros carry source + scope, not raw symbols.
- **Items:** MM2 syntax objects (datum + scope-set + srcloc + phase,
  reusing Phase 1 locations); MM3 hygienic expansion + macro phases; MM4
  `#lang` language declarations; MM5 contracts; MM6 macro stepper /
  debugger / tooling. **Order matters: syntax objects first, `#lang`
  last.**
- **Depends:** Phase 1 (srcloc), Phase 3 (modules). **Risk:** med-high.
- **Done when:** an imported macro expands hygienically; the stepper shows
  located expansion; a contract violation names the blaming party.
- **Status:** DONE — `syntax-rules` ellipsis (basic/recursive/parallel/nested)
  + lightweight hygiene; imported hygienic macros (`examples/140`); a located
  macro stepper via `macroexpand-1`/`read-syntax`/`form-location`
  (`examples/141`); and Findler–Felleisen contracts with blame
  (`examples/142`).  Remaining for a full Racket port (scope-set hygiene, `#lang`)
  is tracked as future work in the Phase 4 completion note above.

### Phase 5 — Object-level GC  ✅ COMPLETE (conservative · non-moving · explicit)
- **Goal:** per-object non-moving mark-sweep (§3.5).
- **Items:** #8 object-level mark bit; root registration from the context
  (envs, stacks, modules, globals, VM frames); per-object sweep; size-class
  free lists. Stay non-moving. Stop-the-world + safepoints for now.
- **Depends:** Phase 1 (contexts as root sources). **Risk:** high.
- **Done when:** GC reclaims individual objects; addresses stay stable;
  no regressions in `make check`.
- **Status:** DONE — `(gc)` now performs a per-object, non-moving, **fully
  conservative** mark & sweep that reclaims individual dead objects into
  per-size free lists which the bump allocator reuses; survivor addresses never
  change. Roots are discovered conservatively and **root-completely by
  construction**: the C stack, the callee-saved registers (spilled via
  `setjmp`), and the whole data/BSS segment are scanned, so no manual root
  enumeration can be incomplete (the only file-scope AST pointers are
  `callcc_value_fallback`, `lizard_jump_value`, and the runtime, all covered;
  there is no symbol-intern table). The runtime env / module results /
  namespace frames are additionally marked explicitly as insurance. Tracing is
  conservative too — each live object's own bytes are scanned for anything that
  looks like a pointer — which needs no per-type tracer and is **sound precisely
  because the collector never moves objects** (a false positive can only retain
  garbage, never corrupt). The stack top is taken from glibc's
  `__libc_stack_end` (correct and always mapped, unlike parsing `/proc/self/maps`).
  Validated by `tests/gc_objects_test.c` (reclamation > 0, diverse live data
  intact across 5 cycles of collect-then-reuse, **survivor address stable**) and
  `examples/145-object-gc.lisp` (lists, nested data, HAMT map, set, rational,
  closure, string survive 6 cycles); valgrind reports **no invalid reads/writes**
  on the stress path. **Honest limitations:** (1) the sweep does *not*
  `mpz_clear`/`mpq_clear` swept numbers — the metadata `kind` is inferred from
  size and a same-sized string buffer could be mis-tagged, so GMP limbs of dead
  numbers leak (the object chunk itself is reclaimed); a future exact-tagging
  pass would fix this. (2) Collection is **explicit** via `(gc)`; there is no
  automatic collect-on-allocation yet. (3) Being conservative, it may retain a
  little genuine garbage (values on the stack that merely look like pointers).
  This unblocks the real-threads half of Phase 6.

### Phase 6 — Persistent data + concurrency  ⏳ IN PROGRESS (persistent structures landed)
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
- **Status:** the immutable-structures half has landed, single-threaded. The
  persistent maps and sets are now backed by a **real Hash Array Mapped Trie**
  (`src/hamt.c`): 32-way branching, bitmap-compressed nodes, collision nodes,
  and **path-copying** so an update copies only the nodes on one root-to-leaf
  path (O(log32 n)) and shares the rest with the original.  This replaces the
  previous flat array whose `phash-set` copied the whole table (O(n)); a
  10000-entry build that was effectively quadratic is now instant
  (`examples/143`).  New surface: `phash-remove`, and a full persistent **set**
  API (`pset`, `pset-add`, `pset-contains?`, `pset-remove`, `pset-count`,
  `pset->list`, `pset?`) sharing the same HAMT (`is_set` distinguishes sets from
  maps).  `transient!`/`conj!`/`persistent!` continue to work over the new
  representation.  **Still pending (now unblocked — Phase 5 GC has landed):** real
  thread-safe contexts + thread pool, cross-thread transient rejection, and
  atomic STM.  The persistent **vector** has also been upgraded from
  copy-on-write to a **32-way bit-partitioned trie with a tail** (Clojure's
  PersistentVector): O(1) amortized `pvec-push`, O(log32 n) `pvec-ref`/`pvec-set`,
  and path-copying so an update shares every node off the updated path (proven
  at the node level in `tests/pvector_test.c`; `examples/146`).  New surface:
  `pvec-pop`, `list->pvec`.  So vectors, maps, and sets now all have the same
  structural-sharing guarantee — the immutable-data half of Phase 6 is complete.
  Also future: upgrade the persistent *vector* from copy-on-write to a
  bit-partitioned trie for the same sharing guarantee.

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
