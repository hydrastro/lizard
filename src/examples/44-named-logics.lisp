; -*- lisp -*-
; ============================================================
;  PHASE M.3 — Named logic bundles
; ============================================================
;
; M.2 wired the lambda cube axes to the Pi typing rule. M.3 wraps
; those toggles into named bundles so a single command configures
; the kernel for a given logic.
;
;   (set-logic 'STLC)    — disables all three cube axes
;   (set-logic 'F)       — System F
;   (set-logic 'LF)      — Edinburgh LF (= lambda-P)
;   (set-logic 'F-omega) — System Fω
;   (set-logic 'CoC)     — Calculus of Constructions
;
; Plus the intermediate corners: lambda-P2, lambda-P-omega,
; lambda-omega.
;
; (current-logic) does reverse lookup — returns the name of the
; bundle matching the current configuration, or 'custom if it
; doesn't match.
;
; This is sugar. It doesn't add expressive power over M.2; it
; makes the user-facing API readable.

; ============================================================
;  1. Default state
; ============================================================

(display "=== Default ===") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  ↑ no rules registered = everything default-allow = CoC") (newline)
(newline)

; ============================================================
;  2. The list of predefined bundles
; ============================================================

(display "=== Predefined bundles ===") (newline)
(display "  ") (display (list-logics)) (newline)
(newline)

; ============================================================
;  3. Tour through the cube corners
; ============================================================

(display "=== STLC — the simply-typed lambda calculus ===") (newline)
(set-logic 'STLC)
(display "  current: ") (display (current-logic)) (newline)
(display "  Pi (U 0)(U 0) [simple arrow]: ")
(display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(display "  Pi (U 1)(U 0) [polymorphism, rejected]: ")
(display (infer (context) (Pi 'x (U 1) (U 0)))) (newline)
(newline)

(display "=== System F — polymorphic ===") (newline)
(set-logic 'F)
(display "  current: ") (display (current-logic)) (newline)
(display "  Pi (U 1)(U 0) [polymorphism, accepted]: ")
(display (infer (context) (Pi 'x (U 1) (U 0)))) (newline)
(display "  Pi (U 1)(U 1) [Fω, rejected]: ")
(display (infer (context) (Pi 'x (U 1) (U 1)))) (newline)
(newline)

(display "=== LF — dependent types ===") (newline)
(set-logic 'LF)
(display "  current: ") (display (current-logic)) (newline)
(display "  (LF allows type-depends-on-term but not type-depends-on-type)") (newline)
(newline)

(display "=== Fω — type-level abstraction ===") (newline)
(set-logic 'F-omega)
(display "  current: ") (display (current-logic)) (newline)
(display "  Pi (U 1)(U 1) [accepted]: ")
(display (infer (context) (Pi 'x (U 1) (U 1)))) (newline)
(newline)

(display "=== CoC — full Calculus of Constructions ===") (newline)
(set-logic 'CoC)
(display "  current: ") (display (current-logic)) (newline)
(display "  Pi (U 0)(U 0): ") (display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(display "  Pi (U 1)(U 0): ") (display (infer (context) (Pi 'x (U 1) (U 0)))) (newline)
(display "  Pi (U 1)(U 1): ") (display (infer (context) (Pi 'x (U 1) (U 1)))) (newline)
(newline)

; ============================================================
;  4. Custom configurations and reverse lookup
; ============================================================

(display "=== Manual config → reverse lookup ===") (newline)
(logic-config-reset)
(logic-rule-enable 'term-depends-on-type)
(logic-rule-enable 'type-depends-on-term)
(logic-rule-disable 'type-depends-on-type)
(display "  Manually set (1, 1, 0):") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  ↑ lambda-P2 — the corner with polymorphism and") (newline)
(display "    dependent types but no Fω") (newline)
(newline)

; ============================================================
;  5. Custom configurations beyond the cube
; ============================================================

(display "=== Adding rules outside the cube ===") (newline)
(logic-rule-disable 'some-future-rule-not-in-cube)
(display "  After adding a non-cube rule:") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  ↑ still lambda-P2 — the cube classification doesn't") (newline)
(display "    change because the three cube axes are unchanged.") (newline)
(display "    Future bundles (M.4+) could give richer names.") (newline)
(newline)

; ============================================================
;  6. Unknown bundle handling
; ============================================================

(display "=== Unknown bundle name ===") (newline)
(display "  (set-logic 'NotALogic): ")
(display (set-logic 'NotALogic)) (newline)
(display "  current-logic after: ") (display (current-logic)) (newline)
(display "  ↑ returned #f and didn't change anything") (newline)
(newline)

; ============================================================
;  7. Reset
; ============================================================

(logic-config-reset)
(display "=== After reset ===") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  ↑ back to CoC (the default-allow default)") (newline)
(newline)

(display "=== Phase M.3 complete ===") (newline)
(newline)
(display "Next phases:") (newline)
(display "  M.4 — structural rules (weakening, contraction, exchange)") (newline)
(display "  M.5 — modalities") (newline)
(display "  M.6 — lattice and HIT features as configurable axes") (newline)
