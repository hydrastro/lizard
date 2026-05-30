; -*- lisp -*-
; ============================================================
;  PHASE M.5.3 — Modal-logic bundles
; ============================================================
;
; M.5.2 Turn 2 wired the strict S4 dual-context discipline.
; M.5.3 adds named bundles so users can configure lizard for
; modal logic with a single command:
;
;   (set-logic 'K)         ; minimal modal logic
;   (set-logic 'T)         ; K + (□A → A)
;   (set-logic 'S4)        ; T + (□A → □□A)
;   (set-logic 'S5)        ; S4 + (◇A → □◇A)
;   (set-logic 'modal-STLC); STLC base + modalities
;
; ----- IMPORTANT HONEST SCOPE NOTE -----
;
; At lizard's current implementation depth, K/T/S4/S5 are
; OPERATIONALLY INDISTINGUISHABLE. The strict box-intro and
; unbox rules from M.5.2 Turn 2 are identical across these
; logics. What differs MATHEMATICALLY are AXIOMS:
;
;   K: no extra axioms beyond box-intro/elim
;   T: adds (□A → A) — the reflexivity axiom
;   S4: T's plus (□A → □□A) — transitivity / idempotence
;   S5: S4's plus (◇A → □◇A) — euclidean property
;
; These axioms are NOT YET wired as type-checkable rules.
; Selecting 'K vs 'S4 currently flips the same toggles and
; produces identical type-checking behavior.
;
; The bundle names are FORWARD-LOOKING: they declare intent
; and reserve the names for future axiom work. set-logic
; works, current-logic returns the cube-base name because
; the modal toggles match the default state and the matcher
; prefers earlier (more general) bundles in the table.

; ============================================================
;  1. Setting modal logics
; ============================================================

(display "=== set-logic 'S4 ===") (newline)
(set-logic 'S4)
(display "  modalities-enabled?  ") (display (logic-rule-enabled? 'modalities-enabled)) (newline)
(display "  modal-strict-typing? ") (display (logic-rule-enabled? 'modal-strict-typing)) (newline)
(display "  Box still works: ") (display (infer (context) (box (U 0)))) (newline)
(newline)

(display "=== K / T / S5 work the same way ===") (newline)
(set-logic 'K)
(display "  After (set-logic 'K), box: ") (display (infer (context) (box (U 0)))) (newline)
(set-logic 'T)
(display "  After (set-logic 'T), box: ") (display (infer (context) (box (U 0)))) (newline)
(set-logic 'S5)
(display "  After (set-logic 'S5), box: ") (display (infer (context) (box (U 0)))) (newline)
(newline)

; ============================================================
;  2. The strict S4 discipline is active under modal bundles
; ============================================================

(display "=== Strict S4 discipline under 'S4 ===") (newline)
(set-logic 'S4)
(define gamma_y (context-extend (context) (variable 'y (U 0))))
(display "  truth ctx = [y:(U 0)], valid ctx = empty") (newline)
(display "  (infer-modal (context) gamma_y (box 'y)):") (newline)
(display "    ") (display (infer-modal (context) gamma_y (box 'y))) (newline)
(display "    ↑ rejected: y is truth-only, dropped inside box") (newline)
(newline)

; ============================================================
;  3. modal-STLC combines modal with STLC restrictions
; ============================================================

(display "=== modal-STLC: STLC cube + modalities ===") (newline)
(set-logic 'modal-STLC)
(display "  Box typechecks:") (newline)
(display "    ") (display (infer (context) (box (U 0)))) (newline)
(display "  Simple arrow OK:") (newline)
(display "    ") (display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(display "  System F Pi rejected (STLC restriction):") (newline)
(display "    ") (display (infer (context) (Pi 'x (U 1) (U 0)))) (newline)
(newline)

; ============================================================
;  4. The reverse-lookup quirk
; ============================================================

(display "=== current-logic returns cube-name ===") (newline)
(set-logic 'S4)
(display "  After (set-logic 'S4):") (newline)
(display "    current-logic: ") (display (current-logic)) (newline)
(display "    ↑ returns 'CoC, not 'S4. Reason:") (newline)
(display "      - modal toggles default to 1 (on)") (newline)
(display "      - S4's bundle setting (modal=1) matches default state") (newline)
(display "      - CoC's don't-care for modal also matches default") (newline)
(display "      - CoC comes first in table, wins") (newline)
(display "") (newline)
(display "  This is honest: 'S4 and 'CoC are currently the same toggle") (newline)
(display "  configuration. Differentiation awaits modal axiom work.") (newline)
(newline)

; ============================================================
;  5. Full list of bundles
; ============================================================

(display "=== All predefined logics ===") (newline)
(display "  ") (display (list-logics)) (newline)
(newline)

; ============================================================
;  6. Path forward
; ============================================================

(display "=== Pending work ===") (newline)
(display "") (newline)
(display "Future M.5.x phases would add modal AXIOMS to differentiate") (newline)
(display "K / T / S4 / S5 at the type-checking level:") (newline)
(display "") (newline)
(display "  T-axiom (□A → A):") (newline)
(display "    A typing rule that allows (unbox b) (no binder) — extract") (newline)
(display "    the content of a Box without naming it. Sound in T+.") (newline)
(display "") (newline)
(display "  4-axiom (□A → □□A):") (newline)
(display "    The valid context Δ becomes 'self-similar' — terms of") (newline)
(display "    Box type at the valid level remain Box-typed inside") (newline)
(display "    nested boxes. Sound in S4+.") (newline)
(display "") (newline)
(display "  5-axiom (◇A → □◇A):") (newline)
(display "    Diamond-typed things at the valid level remain diamond-") (newline)
(display "    typed everywhere. Sound in S5.") (newline)
(display "") (newline)
(display "Each axiom is a new typing rule, each is small. The framework") (newline)
(display "is in place; the work is filling in the table.") (newline)
(newline)

(logic-config-reset)
(display "=== Phase M.5.3 complete ===") (newline)
