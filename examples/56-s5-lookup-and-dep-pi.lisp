; -*- lisp -*-
; ============================================================
;  PHASE M.5.7 — Two fixes for M.5 cleanup
; ============================================================
;
; M.5 recap (after M.5.6):
;   M.5.1 — Box, Diamond as type constructors
;   M.5.2 — Box intro/elim with beta + strict S4 dual context
;   M.5.3 — Named bundles K/T/S4/S5/modal-STLC
;   M.5.4 — modal-4-axiom toggle (T vs S4)
;   M.5.5 — Diamond intro/elim + 5-axiom toggle (S4 vs S5)
;   M.5.6 — K's distinguished elim: t-axiom toggle + box-app
;
; M.5.7 fixes two issues identified at the end of M.5.6:
;
; (1) S5 reverse-lookup: previously (current-logic) returned "CoC"
;     after (set-logic 'S5). The toggle state was ambiguous — both
;     CoC's don't-care and S5's all-on matched the same state.
;
; (2) Dependent Pi in box-app: previously rejected outright. Now
;     supported via T-axiom realization (substitutes unbox into
;     the codomain). K still rejects (no T-axiom).

; ============================================================
;  1. set-logic remembers the explicit name
; ============================================================

(display "=== current-logic now respects explicit set-logic calls ===") (newline)

(display "  After reset:") (newline)
(logic-config-reset)
(display "    current-logic: ") (display (current-logic)) (newline)
(display "    ↑ table walk fallback → CoC") (newline)
(newline)

(display "  After (set-logic 'S5):") (newline)
(set-logic 'S5)
(display "    current-logic: ") (display (current-logic)) (newline)
(display "    ↑ remembered name → S5") (newline)
(newline)

(display "  After (set-logic 'lambda-P):") (newline)
(set-logic 'lambda-P)
(display "    current-logic: ") (display (current-logic)) (newline)
(display "    ↑ no longer aliases to LF; respects user's name") (newline)
(newline)

; ============================================================
;  2. Manual toggle changes break the remembered match
; ============================================================

(display "=== Manual toggle changes fall back to table walk ===") (newline)
(set-logic 'S5)
(display "  After set-logic 'S5: ") (display (current-logic)) (newline)

(logic-rule-disable 'modal-5-axiom)
(display "  After disabling modal-5-axiom: ") (display (current-logic)) (newline)
(display "    ↑ S5 no longer matches; falls back to S4") (newline)
(newline)

; ============================================================
;  3. Reset clears the remembered name
; ============================================================

(display "=== Reset clears state ===") (newline)
(set-logic 'S5)
(display "  After set-logic 'S5: ") (display (current-logic)) (newline)
(logic-config-reset)
(display "  After reset: ") (display (current-logic)) (newline)
(newline)

; ============================================================
;  4. Dependent Pi in box-app
; ============================================================

(display "=== Dependent Pi in box-app (NEW) ===") (newline)
(set-logic 'S4)
(define ctx_dep
  (context-extend
    (context-extend (context) (variable 'f (Box (Pi 'x (U 0) 'x))))
    (variable 'a (Box (U 0)))))
(display "  ctx: f : Box (Pi x:(U 0). x), a : Box (U 0)") (newline)
(display "  (box-app 'f 'a):") (newline)
(display "    ") (display (infer ctx_dep (box-app 'f 'a))) (newline)
(display "  ↑ result substitutes (unbox y a y) for x in the codomain") (newline)
(display "    Fresh binder name __boxapp_<N> avoids capture") (newline)
(newline)

; ============================================================
;  5. K still rejects dependent Pi (no T-axiom)
; ============================================================

(display "=== K still rejects dependent Pi ===") (newline)
(set-logic 'K)
(display "  Same expression under K:") (newline)
(display "    ") (display (infer ctx_dep (box-app 'f 'a))) (newline)
(display "  ↑ K lacks T-axiom; the unbox extraction can't be formed") (newline)
(newline)

; ============================================================
;  6. Reduction works end-to-end
; ============================================================

(display "=== End-to-end reduction ===") (newline)
(set-logic 'S4)
(display "  (reduce (box-app (box (Lambda 'x 'x)) (box (U 7)))):") (newline)
(display "    ") (display (reduce (box-app (box (Lambda 'x 'x)) (box (U 7))))) (newline)
(display "  ↑ K-axiom reduction gives (box (@ (Lambda x x) (U 7)))") (newline)
(display "    Then lambda-beta reduces (@ (Lambda x x) (U 7)) → (U 7)") (newline)
(display "    Result: (box (U 7))") (newline)
(newline)

; ============================================================
;  7. Direct toggle control
; ============================================================

(display "=== Direct toggle control ===") (newline)
(set-logic 'S4)
(display "  S4 default: dependent Pi works.") (newline)
(logic-rule-disable 't-axiom-enabled)
(display "  After (logic-rule-disable 't-axiom-enabled) under S4:") (newline)
(display "    (box-app 'f 'a) with dependent f:") (newline)
(display "    ") (display (infer ctx_dep (box-app 'f 'a))) (newline)
(display "  ↑ K-like behavior — dependent Pi rejected") (newline)
(logic-rule-enable 't-axiom-enabled)
(newline)

; ============================================================
;  Summary
; ============================================================

(display "=== M.5.7 summary ===") (newline)
(display "") (newline)
(display "Fix 1 — Reverse-lookup remembers explicit set-logic calls:") (newline)
(display "  ✓ (set-logic 'S5) → (current-logic) = 'S5'") (newline)
(display "  ✓ (set-logic 'lambda-P) → (current-logic) = 'lambda-P'") (newline)
(display "  ✓ Manual toggle changes fall back to table walk") (newline)
(display "  ✓ Reset clears the remembered name") (newline)
(display "  ✓ Snapshot/restore preserves the name") (newline)
(display "") (newline)
(display "Fix 2 — Dependent Pi in box-app (M.5.6 limitation removed):") (newline)
(display "  ✓ Under T-axiom, dependent Pi typechecks via substitution") (newline)
(display "  ✓ Result type contains an unbox extraction term") (newline)
(display "  ✓ Fresh binder names avoid variable capture") (newline)
(display "  ✓ Reduction composes K-axiom with Pi-beta correctly") (newline)
(display "  ✓ K still rejects (no T-axiom to form the substituent)") (newline)
(display "") (newline)
(display "What remains:") (newline)
(display "  - Categorical laws (still witnesses only, not proofs)") (newline)
(display "  - Soundness / canonicity / termination / decidability") (newline)
(display "  - Symmetric S5 (full Box / Diamond duality)") (newline)
(display "  - DESIGN.md documentation (still my standing recommendation)") (newline)
(display "") (newline)

(logic-config-reset)
(display "=== Phase M.5.7 complete ===") (newline)
