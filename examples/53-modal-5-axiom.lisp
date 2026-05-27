; -*- lisp -*-
; ============================================================
;  PHASE M.5.5 Turn 2 — The 5-axiom (◇A → □◇A)
;  Distinguishing S5 from S4 at the type-checking level
; ============================================================
;
; M.5 recap:
;   M.5.1 — Box, Diamond as type constructors
;   M.5.2 — Box intro/elim with beta + strict S4 dual context
;   M.5.3 — Named bundles K/T/S4/S5/modal-STLC
;   M.5.4 — modal-4-axiom toggle distinguishes T from S4
;   M.5.5 Turn 1 — Diamond intro/elim with beta (placeholder typing)
;   M.5.5 Turn 2 (THIS FILE) — 5-axiom typing rule, S4 vs S5 distinct
;
; ----- The axiom -----
;
;   5-axiom:  ◇A → □◇A
;
; "If A is possibly true, then A is necessarily-possibly true."
; Diamond is "absolute" in S5 — being possible doesn't depend on
; the current world.
;
; ----- The encoding -----
;
; Pfenning-Davies–style: under S5, `let-diamond x b body` places
; x in the VALID context Δ (not the truth context Γ). This means
; x survives entry into subsequent boxes — exactly the 5-axiom's
; content: a possibility, once observed, is permanently a
; possibility.
;
; Under K, T, S4: let-diamond places x in TRUTH (Γ), dropped at
; the next box-introduction. A possibility is only "currently"
; possible.
;
; ----- Toggle -----
;
;   modal-5-axiom  (default-on per default-allow convention)
;
;     K, T, S4     → 0 (off)
;     S5           → 1 (on)
;     modal-STLC   → 0 (S4-flavored)

; ============================================================
;  1. S4 doesn't have the 5-axiom
; ============================================================

(display "=== S4: no 5-axiom ===") (newline)
(set-logic 'S4)
(display "  modal-5-axiom under S4: ")
(display (logic-rule-enabled? 'modal-5-axiom)) (newline)
(newline)

(display "  Single let-diamond + use outside box works:") (newline)
(display "    (let-diamond 'x (diamond (U 0)) 'x)") (newline)
(display "      ⟶ ") (display (infer (context) (let-diamond 'x (diamond (U 0)) 'x))) (newline)
(newline)

(display "  But put it inside a box — fails under S4:") (newline)
(display "    (let-diamond 'x (diamond (U 0)) (box 'x))") (newline)
(display "      ⟶ ") (display (infer (context) (let-diamond 'x (diamond (U 0)) (box 'x)))) (newline)
(display "      ↑ x is truth-typed under S4; dropped inside the box") (newline)
(newline)

; ============================================================
;  2. S5 has the 5-axiom
; ============================================================

(display "=== S5: 5-axiom enabled ===") (newline)
(set-logic 'S5)
(display "  modal-5-axiom under S5: ")
(display (logic-rule-enabled? 'modal-5-axiom)) (newline)
(newline)

(display "  Same expression now typechecks:") (newline)
(display "    (let-diamond 'x (diamond (U 0)) (box 'x))") (newline)
(display "      ⟶ ") (display (infer (context) (let-diamond 'x (diamond (U 0)) (box 'x)))) (newline)
(display "      ↑ x is valid-typed under S5; survives the box") (newline)
(newline)

; ============================================================
;  3. The headline case: ◇A ⊢ □◇A
; ============================================================

(display "=== The headline case: ◇A ⊢ □◇A ===") (newline)
(display "") (newline)
(display "Term: (let-diamond 'x (diamond (U 0)) (box (diamond 'x)))") (newline)
(display "Reading: \"given a diamond of (U 0), reconstruct it inside a box\"") (newline)
(newline)

(display "Under S5:") (newline)
(display "  ⟶ ")
(display (infer (context) (let-diamond 'x (diamond (U 0)) (box (diamond 'x))))) (newline)
(display "  ↑ this is the 5-axiom in action: ◇(U 0) → □◇(U 0)") (newline)
(newline)

(set-logic 'S4)
(display "Same expression under S4:") (newline)
(display "  ⟶ ")
(display (infer (context) (let-diamond 'x (diamond (U 0)) (box (diamond 'x))))) (newline)
(display "  ↑ S4 rejects — no 5-axiom") (newline)
(newline)

; ============================================================
;  4. K and T also lack 5-axiom (weaker than S5)
; ============================================================

(display "=== K and T also lack 5-axiom ===") (newline)
(set-logic 'K)
(display "  K modal-5-axiom: ") (display (logic-rule-enabled? 'modal-5-axiom)) (newline)
(set-logic 'T)
(display "  T modal-5-axiom: ") (display (logic-rule-enabled? 'modal-5-axiom)) (newline)
(display "  Under T: (let-diamond 'x (diamond (U 0)) (box 'x)) ⟶ ")
(display (infer (context) (let-diamond 'x (diamond (U 0)) (box 'x)))) (newline)
(newline)

; ============================================================
;  5. Independence: 4-axiom still works under S5
; ============================================================

(display "=== 4-axiom and 5-axiom are independent ===") (newline)
(set-logic 'S5)
(define delta_x (context-extend (context) (variable 'x (U 0))))
(display "  Under S5, nested box of valid hypothesis (4-axiom):") (newline)
(display "    (box (box 'x)) ⟶ ") (display (infer-modal delta_x (context) (box (box 'x)))) (newline)
(display "  ↑ both 4 and 5 axioms active") (newline)
(newline)

; ============================================================
;  6. Direct toggle control
; ============================================================

(display "=== Direct toggle ===") (newline)
(set-logic 'S4)
(display "  Started in S4 (5-axiom off).") (newline)
(logic-rule-enable 'modal-5-axiom)
(display "  After enabling 5-axiom manually:") (newline)
(display "    (let-diamond 'x (diamond (U 0)) (box 'x)) ⟶ ")
(display (infer (context) (let-diamond 'x (diamond (U 0)) (box 'x)))) (newline)
(display "  ↑ 5-axiom now active even outside S5 bundle") (newline)
(newline)

; ============================================================
;  7. Scope honesty
; ============================================================

(display "=== M.5 status after Turn 2 ===") (newline)
(display "") (newline)
(display "Operationally distinct modal logics:") (newline)
(display "  ✓ S4 — 4-axiom on, 5-axiom off (default behavior)") (newline)
(display "  ✓ S5 — both axioms on") (newline)
(display "  ✓ T  — neither (toggle modal-4-axiom off)") (newline)
(display "") (newline)
(display "Still aliased to T:") (newline)
(display "  - K — would need K-axiom distribution and/or removal of") (newline)
(display "        T-axiom (extraction via unbox). Architectural work") (newline)
(display "        flagged for a future turn.") (newline)
(display "") (newline)
(display "What's still missing for soundness:") (newline)
(display "  - Soundness, canonicity, termination, decidability proofs") (newline)
(display "  - Categorical correspondence (Box should be comonad,") (newline)
(display "    Diamond should be monad)") (newline)
(display "  - Symmetric S5: full duality Box ⟷ Diamond") (newline)
(display "") (newline)

(logic-config-reset)
(display "=== Phase M.5.5 Turn 2 complete ===") (newline)
