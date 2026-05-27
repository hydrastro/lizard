; -*- lisp -*-
; ============================================================
;  PHASE M.5.4 — The 4-axiom (□A → □□A)
;  Distinguishing T from S4 at the type-checking level
; ============================================================
;
; Recap of M.5 so far:
;   M.5.1 — Box and Diamond as type constructors
;   M.5.2 — (box e) and (unbox x b body) intro/elim with beta
;           Turn 2 added the dual context (Δ; Γ) for strict S4
;   M.5.3 — Named bundles K, T, S4, S5
;
; In M.5.3 I claimed K/T/S4/S5 were aliases. While writing M.5.4
; I found that was wrong in two ways:
;
; 1. A real bug in box-intro: the rule moved valid to truth at
;    each level, which lost the 4-axiom for nested boxes.
;
; 2. After the fix, lizard genuinely implements S4 (not just
;    aspirationally). The unbox rule encodes T-axiom (extraction),
;    the box-intro rule encodes 4-axiom (preservation through
;    nesting).
;
; M.5.4 adds a toggle `modal-4-axiom` that, when off, makes lizard
; behave like T (no 4-axiom). When on (default), lizard is S4.
;
; ----- The axiom under microscope -----
;
;   4-axiom:  □A → □□A
;
; Read: "if A is necessarily true, then it is necessarily
; necessarily true." Necessity is idempotent.
;
; Operationally in lizard: a variable bound by `unbox` lives in
; the valid context Δ. Under S4, Δ is preserved across nested
; box-introductions, so the variable remains accessible at
; arbitrary depths. Under T, Δ is consumed at the first box-
; introduction, so nested boxes lose access.

; ============================================================
;  1. S4: nested boxes work (default)
; ============================================================

(define delta_x (context-extend (context) (variable 'x (U 0))))

(display "=== S4 (default, 4-axiom on) ===") (newline)
(display "  (Δ=[x:U0], Γ=∅) ⊢ (box 'x):") (newline)
(display "    ") (display (infer-modal delta_x (context) (box 'x))) (newline)
(display "  (Δ=[x:U0], Γ=∅) ⊢ (box (box 'x)):") (newline)
(display "    ") (display (infer-modal delta_x (context) (box (box 'x)))) (newline)
(display "  (Δ=[x:U0], Γ=∅) ⊢ (box (box (box 'x))):") (newline)
(display "    ") (display (infer-modal delta_x (context) (box (box (box 'x))))) (newline)
(display "  ↑ valid hypothesis survives arbitrary nesting (4-axiom)") (newline)
(newline)

; ============================================================
;  2. T: nested boxes fail
; ============================================================

(display "=== T (4-axiom off) ===") (newline)
(set-logic 'T)
(display "  modal-4-axiom? ") (display (logic-rule-enabled? 'modal-4-axiom)) (newline)
(newline)

(display "  Single box still works:") (newline)
(display "    (box 'x) ⊢ ") (display (infer-modal delta_x (context) (box 'x))) (newline)

(display "  Nested fails — no 4-axiom:") (newline)
(display "    (box (box 'x)) ⊢ ") (display (infer-modal delta_x (context) (box (box 'x)))) (newline)
(display "  ↑ T doesn't preserve Δ through nested boxes") (newline)
(newline)

; ============================================================
;  3. S4 round-trip example
; ============================================================

(set-logic 'S4)
(display "=== S4 round-trip ===") (newline)
(display "  (unbox 'x (box (U 0)) (box (box 'x))):") (newline)
(display "  ↑ unbox binds x as valid, nested box uses it") (newline)
(display "  ") (display (infer (context) (unbox 'x (box (U 0)) (box (box 'x))))) (newline)
(display "  Same expression under T:") (newline)
(set-logic 'T)
(display "    ") (display (infer (context) (unbox 'x (box (U 0)) (box (box 'x))))) (newline)
(display "  ↑ T rejects — the nested box can't see the valid x") (newline)
(set-logic 'S4)
(newline)

; ============================================================
;  4. The honest naming
; ============================================================

(display "=== K / S5 (currently aliases) ===") (newline)
(display "") (newline)
(display "K modal logic differs from T at the ELIMINATION rule —") (newline)
(display "K's elim is distribution (Box A → B + Box A → Box B), not") (newline)
(display "extraction. Lizard's unbox rule encodes extraction (the") (newline)
(display "T-axiom), so K isn't truly implementable without redefining") (newline)
(display "unbox. The 'K bundle is currently an alias for T.") (newline)
(display "") (newline)
(display "S5 differs from S4 at the DIAMOND rule (◇A → □◇A). Lizard") (newline)
(display "doesn't yet have Diamond intro/elim. The 'S5 bundle is") (newline)
(display "currently an alias for S4.") (newline)
(newline)

(display "  (set-logic 'K), modal-4-axiom: ")
(set-logic 'K)
(display (logic-rule-enabled? 'modal-4-axiom)) (newline)
(display "  ↑ K = T (no 4-axiom)") (newline)
(newline)

(display "  (set-logic 'S5), modal-4-axiom: ")
(set-logic 'S5)
(display (logic-rule-enabled? 'modal-4-axiom)) (newline)
(display "  ↑ S5 = S4 (has 4-axiom)") (newline)
(newline)

; ============================================================
;  5. The toggle directly
; ============================================================

(display "=== Direct toggle control ===") (newline)
(logic-config-reset)
(display "  Default (everything on, S4 behavior):") (newline)
(display "    (box (box 'x)) ⊢ ")
(display (infer-modal delta_x (context) (box (box 'x)))) (newline)

(logic-rule-disable 'modal-4-axiom)
(display "  After (logic-rule-disable 'modal-4-axiom):") (newline)
(display "    (box (box 'x)) ⊢ ")
(display (infer-modal delta_x (context) (box (box 'x)))) (newline)
(display "  ↑ rejected — 4-axiom off") (newline)

(logic-rule-enable 'modal-4-axiom)
(display "  After re-enable: ")
(display (infer-modal delta_x (context) (box (box 'x)))) (newline)
(newline)

; ============================================================
;  6. Scope honesty
; ============================================================

(display "=== Phase M.5.4 status ===") (newline)
(display "") (newline)
(display "Implemented operationally:") (newline)
(display "  ✓ S4 (with 4-axiom)") (newline)
(display "  ✓ T  (without 4-axiom — via modal-4-axiom toggle)") (newline)
(display "  ✓ Backward compat: existing modal code defaults to S4") (newline)
(display "") (newline)
(display "Still aliases:") (newline)
(display "  - K  (would need different unbox rule — K's elim is") (newline)
(display "        distribution, not extraction)") (newline)
(display "  - S5 (would need Diamond intro/elim plus 5-axiom)") (newline)
(display "") (newline)
(display "What's missing for soundness:") (newline)
(display "  - The usual: canonicity, termination, decidability") (newline)
(display "  - Interaction of modalities with lattice features") (newline)
(display "  - Categorical correspondence (Box should be a comonad)") (newline)
(display "") (newline)

(logic-config-reset)
(display "=== Phase M.5.4 complete ===") (newline)
