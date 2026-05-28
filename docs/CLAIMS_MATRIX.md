# Claims matrix

This file keeps the public claims precise. It should be updated whenever a
feature changes from notation, to scaffold, to checked kernel feature.

| Area | Current status | Notes |
| --- | --- | --- |
| C89 Lisp evaluator | implemented | Tree-walking evaluator with tail-call support and tests. |
| Runtime/context C API | scaffold/usable | Public opaque API exists; some legacy process-global state remains. |
| Public/internal header split | partial | `lizard_api.h` is public; internals are moved behind `src/lizard_internal.h`; compatibility `lizard.h` remains. |
| Source positions | partial | Tokens and many parsed AST nodes carry line/column/offset metadata. |
| Structured diagnostics | basic | Errors from the evaluator carry source spans (line:column). The printer prepends location when available. `(error-location err)` returns the span as an alist. Six key eval error paths (unbound symbol, invalid apply, bad define, bad assignment) carry spans. |
| Hygienic macros | partial | `syntax-rules` exists; full syntax objects/ellipsis/source-aware expansion are future work. |
| Module system | basic | `(import "path")` with caching, search-path resolution, `(module-loaded?)`, `(add-module-path!)`. No namespacing/exports — imported definitions go into the current environment. |
| Garbage collector | segment-level collection | `(gc)` runs mark-from-roots then frees heap segments with zero live objects. `(gc-stats)` reports live/garbage/total counts. Environment chains and closure captures are fully traversed. **Limitation:** only entire dead segments are freed (no per-object sweep). A segment with even one live object is kept whole. Effective when large computations are discarded between `(gc)` calls. |
| Bytecode compiler/VM | basic | `(vm-eval expr)` compiles Scheme to bytecode and executes on a stack-based VM. `(disassemble expr)` prints the bytecode. Covers: constants, arithmetic, comparisons, variables, if/else, define/set!, lambda with closures, general function calls, begin, cons/car/cdr, display/newline. Complex forms (macros, call/cc, type theory) fall back to constants. |
| Profiler/coverage/debugger | build-tool level | Make targets exist; language-frame debugger/profiler are future work. |
| MLTT/λΠ type checking | experimental | Useful checker exists, but no proof of soundness/normalization/canonicity. |
| Cubical interval/faces/systems | experimental | Useful CCHM-inspired fragments exist. |
| Glue/univalence | canonical fragment | Demonstrations exist; not full Cubical Agda-level univalence. |
| Cubical S¹ | scaffold | `S1`, `base`, `loop` are opt-in nodes with minimal typing spine. No recursor, no computation rule for `loop`, no Kan composition for the motive yet. |
| Truncations (propositional) | scaffold + checked | `Trunc`, `trunc`, `trunc-elim` are opt-in nodes. Typing rules implemented: `Trunc` preserves universe; `trunc x` infers `(Trunc A)` from `x : A`; `trunc-elim C h e` requires `e : Trunc _ A`, `h : Π _:A. C`, result `C`. Primary computation rule: `(trunc-elim C h (trunc x)) ⟶ (@ h x)`, deterministic by LHS uniqueness. **Propositionality coherence (Turn 3):** the 4-arg form `(trunc-elim C h prop e)` structurally verifies `prop : Π x:C. Π y:C. Path C x y` — the standard is-prop condition. The 3-arg form `(trunc-elim C h e)` defers this obligation (scaffold behavior). |
| Modal logic layer (K, T, S4, S5) | checked | Box / Diamond constructors, intro/elim, K-axiom (`box-app`), monadic bind (`diamond-bind`). Toggles `modal-strict-typing`, `modal-4-axiom`, `modal-5-axiom`, `t-axiom-enabled` distinguish the four logics operationally. See `docs/MODAL.md`. |
| Symmetric S5 (Pfenning-Davies) | checked | `dia`, `poss-coerce` AST forms; judgment-kind tracking (TRUE / VALID / POSS); kind propagation through `let-diamond`; kind check in `box-intro`. Gated on `modal-symmetric`. **Honest gap:** the strict single-Ω-context invariant is encoded via kind propagation rather than enforced syntactically. |
| Generic proof extensions | scaffold | `theory-extension` nodes let users plug in experiments behind opt-in rules. |
| Proof assistant | not yet | Needs a trusted kernel boundary, elaborator, goals/holes, tactics, and library discipline. |
| Soundness story | documented as future work | No formal proof is claimed. |

## Phase 2+ additions

| Feature | Status | Notes |
|---|---|---|
| Kernel Sigma/Pair/Proj | checked | Full computation rules for dependent pairs: proj1(pair(a,b))→a, proj2(pair(a,b))→b. Type inference for Sigma types. |
| Kernel J-eliminator | checked | J C d A a b (refl a) → d a. The fundamental elimination principle for identity types. |
| Proof state / tactics | basic | begin-proof, tactic-intro, tactic-exact, tactic-refl, qed. Proofs produce kernel-checkable certificates. |
| Persistent hash maps | basic | phash-map, phash-get, phash-set, phash-keys, phash-count. Copy-on-write flat array (HAMT trie upgrade planned). |
| Standard library | basic | sort, zip, partition, flatten, take, drop, any, every, enumerate, range, compose, ->, alist-ref, alist-set. |
| Syntax objects | basic | datum->syntax, syntax->datum, syntax-e, syntax-source, syntax?. Foundation for hygiene. |
| Persistent vectors | basic | pvec, pvec-ref, pvec-set, pvec-push, pvec-count, pvec->list. Copy-on-write (trie upgrade planned). |

## Final session additions

| Feature | Status | Notes |
|---|---|---|
| Kernel Maybe type | checked | Maybe A, nothing, just v. maybe-rec with computation: maybe-rec _ n _ nothing → n, maybe-rec _ _ j (just v) → j v. |
| Kernel List type | checked | List A, nil, cons h t. listrec with computation: listrec _ n _ nil → n, listrec C n c (cons h t) → c h t (listrec C n c t). |
| Unification | basic | First-order flex-rigid unification. Solves metavars by structural matching. Works under WHNF. |
| Kernel tests | comprehensive | C-level tests for Nat, Bool, Pi/beta, Sigma/proj, Id/refl, unification, sort hierarchy, definitional equality. |
