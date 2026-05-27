# Claims matrix

This file keeps the public claims precise. It should be updated whenever a
feature changes from notation, to scaffold, to checked kernel feature.

| Area | Current status | Notes |
| --- | --- | --- |
| C89 Lisp evaluator | implemented | Tree-walking evaluator with tail-call support and tests. |
| Runtime/context C API | scaffold/usable | Public opaque API exists; some legacy process-global state remains. |
| Public/internal header split | partial | `lizard_api.h` is public; internals are moved behind `src/lizard_internal.h`; compatibility `lizard.h` remains. |
| Source positions | partial | Tokens and many parsed AST nodes carry line/column/offset metadata. |
| Structured diagnostics | partial | API exposes last diagnostic; parser still has some legacy fatal paths. |
| Hygienic macros | partial | `syntax-rules` exists; full syntax objects/ellipsis/source-aware expansion are future work. |
| Module system | not implemented | Needs import/export and library loading semantics. |
| Garbage collector | not implemented | Arena allocation remains the runtime memory model. |
| Bytecode compiler/VM | not implemented | Planned after runtime/context and syntax pipeline hardening. |
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
