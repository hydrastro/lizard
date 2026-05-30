; -*- lisp -*-
; ============================================================
;  PHASE M.1 — Logic-rule configuration registry
;  Infrastructure for lizard-as-logical-framework
; ============================================================
;
; The long-term goal: lizard is a CONFIGURABLE type theory.
; The user picks which inference rules are active and gets a
; particular logic out the other side. The lambda cube is the
; archetype — its eight corners (STLC, F, F2, Fω, LF, λP, λPω,
; CoC) are obtained by toggling three orthogonal axes:
;
;   term-depends-on-type   (System F)
;   type-depends-on-term   (LF)
;   type-depends-on-type   (System Fω)
;
; Beyond the cube, modalities, structural rules (weakening,
; contraction, exchange), and lizard's lattice and HIT features
; can all become configuration axes. The intended end state is
; a system where the user describes a logic by enumerating its
; active rules — some configurations consistent, some not.
;
; ----- What M.1 delivers -----
;
; The INFRASTRUCTURE for that: a per-process registry of named
; rule toggles, with enable/disable/query/snapshot/restore. M.1
; deliberately does NOT pre-register any specific rules and does
; NOT wire any kernel-typing site to consult the registry.
;
; M.2 onwards will register specific rules and start consulting
; the configuration in type-checking. That work happens one rule
; at a time so each step is reviewable.

; ============================================================
;  1. Empty initial state
; ============================================================

(display "=== Empty registry ===") (newline)
(display "  size: ") (display (logic-config-size)) (newline)
(display "  config: ") (display (logic-config)) (newline)
(newline)

; ============================================================
;  2. Register the lambda cube axes
; ============================================================

(display "=== Lambda cube axes ===") (newline)
(logic-rule-register 'term-depends-on-type)
(logic-rule-register 'type-depends-on-term)
(logic-rule-register 'type-depends-on-type)

(display "After registering 3 cube axes:") (newline)
(display "  config: ") (display (logic-config)) (newline)
(display "  ↑ all #f (off by default)") (newline)
(newline)

; ============================================================
;  3. Build STLC: all three off
; ============================================================

(display "=== STLC corner of the cube ===") (newline)
(display "  All three axes disabled = simply-typed lambda calculus") (newline)
(display "  term-depends-on-type? ")
(display (logic-rule-enabled? 'term-depends-on-type)) (newline)
(display "  type-depends-on-term? ")
(display (logic-rule-enabled? 'type-depends-on-term)) (newline)
(display "  type-depends-on-type? ")
(display (logic-rule-enabled? 'type-depends-on-type)) (newline)
(newline)

; ============================================================
;  4. Switch to System F: enable term-depends-on-type
; ============================================================

(display "=== System F corner ===") (newline)
(logic-rule-enable 'term-depends-on-type)
(display "  Enabled term-depends-on-type → System F") (newline)
(display "  config: ") (display (logic-config)) (newline)
(newline)

; ============================================================
;  5. Calculus of Constructions: all three on
; ============================================================

(display "=== Calculus of Constructions ===") (newline)
(logic-rule-enable 'type-depends-on-term)
(logic-rule-enable 'type-depends-on-type)
(display "  All three axes enabled = CoC") (newline)
(display "  config: ") (display (logic-config)) (newline)
(newline)

; ============================================================
;  6. Structural rules (linear logic territory)
; ============================================================

(display "=== Structural rules ===") (newline)
(display "  Disabling weakening, contraction, or exchange") (newline)
(display "  would give linear / affine / relevant logic.") (newline)
(display "  (Toggles register; no kernel checks consult them yet.)") (newline)
(logic-rule-register 'weakening)
(logic-rule-register 'contraction)
(logic-rule-register 'exchange)
(display "  Registered: weakening, contraction, exchange") (newline)
(display "  size: ") (display (logic-config-size)) (newline)
(newline)

; ============================================================
;  7. The vision: arbitrary configurations
; ============================================================

(display "=== Configuration as data ===") (newline)
(display "The current configuration is a first-class value.") (newline)
(display "Future phases will let you:") (newline)
(display "") (newline)
(display "  - bundle configurations as named logics") (newline)
(display "    (set-logic 'STLC), (set-logic 'CoC)") (newline)
(display "  - save/restore configurations across checks") (newline)
(display "  - record which logic a derivation was made in") (newline)
(display "  - compose / extend configurations") (newline)
(newline)

; ============================================================
;  8. What M.1 doesn't do (and shouldn't, yet)
; ============================================================

(display "=== Boundaries of M.1 ===") (newline)
(display "M.1 is INFRASTRUCTURE ONLY. Specifically:") (newline)
(display "  - No rule is pre-registered.") (newline)
(display "  - No type-checking decision consults the registry.") (newline)
(display "  - Switching configurations does NOT yet change") (newline)
(display "    which expressions type-check.") (newline)
(display "") (newline)
(display "That wiring is M.2 and beyond — one rule at a time,") (newline)
(display "with each rule's behavior visible in tests.") (newline)
(newline)

; ============================================================
;  9. Cleanup
; ============================================================

(logic-config-reset)
(display "After reset:") (newline)
(display "  size: ") (display (logic-config-size)) (newline)
(newline)

(display "=== Phase M.1 complete ===") (newline)
