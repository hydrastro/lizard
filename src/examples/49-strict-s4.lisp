; -*- lisp -*-
; ============================================================
;  PHASE M.5.2 Turn 2 — Strict S4 modal type theory
;  Dual context (Δ; Γ) infrastructure
; ============================================================
;
; M.5.2 Turn 1 added (box e) and (unbox x b body) with placeholder
; typing rules — any well-typed term could be boxed and any Box
; value could be unboxed. The beta rule (unbox x (box e) body) →
; body[e/x] was correct, but the typing didn't enforce the
; discipline that makes Box specifically a NECESSITY modality.
;
; M.5.2 Turn 2 implements strict S4 typing.
;
; ----- The dual context -----
;
; The kernel now threads TWO contexts through every typing rule:
;
;   Δ (valid)  — hypotheses that are necessarily true; survive
;                entry into a box
;   Γ (truth)  — ordinary hypotheses; dropped inside a box
;
; All existing typing rules (Pi, Sigma, App, etc.) thread both
; contexts unchanged. Only the modal rules manipulate them:
;
;   Box-intro:  Δ; · ⊢ e : T  ⊢  Δ; Γ ⊢ box e : Box T
;               (drops Γ when checking e)
;
;   Unbox:      Δ; Γ ⊢ b : Box T,  Δ, x:T; Γ ⊢ body : U
;               ⊢ Δ; Γ ⊢ unbox x b body : U
;               (extends Δ, not Γ, with x:T)
;
; ----- API -----
;
; (infer ctx expr)              — single-ctx, backward compat
;                                 (passes empty Δ)
; (infer-modal Δ Γ expr)        — new two-ctx form
; (check ctx expr type)         — single-ctx, backward compat
; (check-modal Δ Γ expr type)   — new two-ctx form
;
; Toggle: modal-strict-typing (default on). When off, modal rules
; revert to Turn 1 placeholder typing.

; ============================================================
;  1. Backward compatibility — existing code works unchanged
; ============================================================

(display "=== Backward compatibility ===") (newline)
(display "  (infer (context) (box (U 0))):") (newline)
(display "    ") (display (infer (context) (box (U 0)))) (newline)
(display "  (infer (context) (unbox 'x (box (U 0)) 'x)):") (newline)
(display "    ") (display (infer (context) (unbox 'x (box (U 0)) 'x))) (newline)
(display "  ↑ same as Turn 1 — single-ctx API still works") (newline)
(newline)

; ============================================================
;  2. The discipline: truth-only vars don't survive box
; ============================================================

(display "=== Strict box rejects truth-only variable ===") (newline)
(define gamma_y (context-extend (context) (variable 'y (U 0))))

(display "  truth ctx = [y:(U 0)], valid ctx = empty") (newline)
(display "  Outside box, y is visible:") (newline)
(display "    (infer-modal (context) gamma_y 'y) → ")
(display (infer-modal (context) gamma_y 'y)) (newline)

(display "  Inside box, y is dropped:") (newline)
(display "    (infer-modal (context) gamma_y (box 'y)) → ")
(display (infer-modal (context) gamma_y (box 'y))) (newline)
(display "    ↑ strict S4 reject: truth context dropped inside box") (newline)
(newline)

; ============================================================
;  3. But valid-context vars DO survive
; ============================================================

(display "=== Valid-context variable survives box ===") (newline)
(define delta_y (context-extend (context) (variable 'y (U 0))))

(display "  valid ctx = [y:(U 0)], truth ctx = empty") (newline)
(display "  (infer-modal delta_y (context) (box 'y)) → ")
(display (infer-modal delta_y (context) (box 'y))) (newline)
(display "  ↑ y is valid, so it survives the box-introduction") (newline)
(newline)

; ============================================================
;  4. The round-trip: unbox-then-box
; ============================================================

(display "=== The round-trip: unbox then box ===") (newline)
(display "  (unbox 'x (box (U 0)) (box 'x)):") (newline)
(display "    1. unbox binds x as VALID (not truth)") (newline)
(display "    2. inner (box 'x) checks x against the valid context") (newline)
(display "    3. x is there — accepted") (newline)
(display "  Result:") (newline)
(display "    ") (display (infer (context) (unbox 'x (box (U 0)) (box 'x)))) (newline)
(display "  ↑ this is why valid/truth distinction matters: only the") (newline)
(display "    valid placement lets x survive into the inner box") (newline)
(newline)

; ============================================================
;  5. Comparison with Turn 1 (loose) typing
; ============================================================

(display "=== With strict mode off, reverts to Turn 1 ===") (newline)
(logic-rule-disable 'modal-strict-typing)
(display "  (infer-modal (context) gamma_y (box 'y)) → ")
(display (infer-modal (context) gamma_y (box 'y))) (newline)
(display "  ↑ now accepts: Turn 1 loose typing doesn't distinguish") (newline)
(display "    truth from valid") (newline)
(logic-rule-enable 'modal-strict-typing)
(newline)

; ============================================================
;  6. The architecture choice
; ============================================================

(display "=== Implementation: Approach A ===") (newline)
(display "") (newline)
(display "Two-context kernel:") (newline)
(display "  lizard_tt_infer2(valid_ctx, truth_ctx, t, heap)") (newline)
(display "  lizard_tt_check2(valid_ctx, truth_ctx, t, T, heap)") (newline)
(display "") (newline)
(display "Every typing rule (Pi, Sigma, App, ...) threads both") (newline)
(display "contexts unchanged. Only box-intro drops truth, only") (newline)
(display "unbox extends valid. ALL 80 internal call-sites updated.") (newline)
(display "") (newline)
(display "Variable lookup checks truth first, then valid — so a") (newline)
(display "valid hypothesis is visible everywhere, but only ordinary") (newline)
(display "(truth) hypotheses are filtered at the box boundary.") (newline)
(newline)

; ============================================================
;  7. What's still pending
; ============================================================

(display "=== Pending ===") (newline)
(display "") (newline)
(display "Working in Turn 2:") (newline)
(display "  ✓ Dual-context kernel (Δ; Γ)") (newline)
(display "  ✓ Strict S4 box-intro (drops Γ)") (newline)
(display "  ✓ Strict S4 unbox (extends Δ)") (newline)
(display "  ✓ Variable lookup consults both contexts") (newline)
(display "  ✓ Beta rule unchanged from Turn 1") (newline)
(display "  ✓ Toggle modal-strict-typing for loose mode") (newline)
(display "  ✓ Backward-compat single-ctx API") (newline)
(display "") (newline)
(display "Still to do:") (newline)
(display "  M.5.3 — modal-logic bundles") (newline)
(display "    (set-logic 'S4)") (newline)
(display "    (set-logic 'K)") (newline)
(display "    (set-logic 'T)") (newline)
(display "    etc.") (newline)
(display "  Stricter rules: T-axiom (□A → A), S5-axiom, etc.") (newline)
(display "    would each be small additions to specific bundles") (newline)
(display "") (newline)
(display "=== Phase M.5.2 Turn 2 complete ===") (newline)
