; -*- lisp -*-
; ============================================================
;  PHASE M.2 — The lambda cube as configuration
;  First real wiring of the logic-rule registry into the checker
; ============================================================
;
; M.1 landed the registry as infrastructure. M.2 wires the
; three lambda cube axes to the Pi typing rule. Now configuring
; the registry actually CHANGES what type-checks.
;
; ----- The three axes -----
;
;   term-depends-on-type : System F-style polymorphism
;   type-depends-on-term : LF / λP-style dependent types
;   type-depends-on-type : Fω-style type-level abstraction
;
; The cube's eight corners are products of which subset of these
; three is enabled:
;
;   {}                                  = STLC
;   {term-on-type}                      = System F
;   {type-on-term}                      = LF / λP
;   {type-on-type}                      = Fω
;   {term-on-type, type-on-type}        = F-omega-bar
;   {term-on-type, type-on-term}        = ...
;   {type-on-term, type-on-type}        = ...
;   {term-on-type, type-on-term, type-on-type} = CoC
;
; ----- Encoding -----
;
; lizard encodes the cube using universe levels rather than
; explicit sorts:
;
;   A : (U 0)  → A is a "type" (its inhabitants are values)
;   A : (U 1+) → A is a "kind" (its inhabitants are types)
;
; The classifier looks at dom_univ and cod_univ levels:
;
;   level(dom)=1, level(cod)=1: simple arrow OR type-depends-on-term
;   level(dom)≥2, level(cod)=1: term-depends-on-type (System F)
;   level(dom)≥2, level(cod)≥2: type-depends-on-type (Fω)
;
; This is a simplification of the strict cube formulation
; (which uses * and ☐ as explicit sorts). A refactor is possible
; in a future phase if desired.
;
; ----- Default behavior -----
;
; If a rule isn't registered, it's treated as ENABLED. So with
; no configuration changes, lizard behaves as it always has —
; full CoC-style flexibility.

; ============================================================
;  1. Baseline — no configuration, everything works
; ============================================================

(display "=== Baseline (default, no rules registered) ===") (newline)
(display "Simple arrow (Pi 'x (U 0) (U 0)):") (newline)
(display "  ") (display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(display "System F-style (Pi 'x (U 1) (U 0)):") (newline)
(display "  ") (display (infer (context) (Pi 'x (U 1) (U 0)))) (newline)
(display "Fω-style (Pi 'x (U 1) (U 1)):") (newline)
(display "  ") (display (infer (context) (Pi 'x (U 1) (U 1)))) (newline)
(newline)

; ============================================================
;  2. Configure as STLC — disable everything
; ============================================================

(display "=== Configure as STLC (disable all three axes) ===") (newline)
(logic-rule-disable 'term-depends-on-type)
(logic-rule-disable 'type-depends-on-term)
(logic-rule-disable 'type-depends-on-type)

(display "Simple arrow (Pi 'x (U 0) (U 0)) [STLC allows]:") (newline)
(display "  ") (display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)

(display "System F-style (Pi 'x (U 1) (U 0)) [STLC REJECTS]:") (newline)
(display "  ") (display (infer (context) (Pi 'x (U 1) (U 0)))) (newline)

(display "Fω-style (Pi 'x (U 1) (U 1)) [STLC REJECTS]:") (newline)
(display "  ") (display (infer (context) (Pi 'x (U 1) (U 1)))) (newline)

(display "  ↑ simple arrow still works because it requires no axis") (newline)
(newline)

; ============================================================
;  3. Configure as System F — enable just term-depends-on-type
; ============================================================

(display "=== Configure as System F ===") (newline)
(logic-rule-enable 'term-depends-on-type)

(display "Simple arrow [F allows]:") (newline)
(display "  ") (display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)

(display "System F-style [F allows]:") (newline)
(display "  ") (display (infer (context) (Pi 'x (U 1) (U 0)))) (newline)

(display "Fω-style [F REJECTS]:") (newline)
(display "  ") (display (infer (context) (Pi 'x (U 1) (U 1)))) (newline)
(newline)

; ============================================================
;  4. Upgrade to Fω
; ============================================================

(display "=== Upgrade to Fω ===") (newline)
(logic-rule-enable 'type-depends-on-type)

(display "Fω-style [Fω allows]:") (newline)
(display "  ") (display (infer (context) (Pi 'x (U 1) (U 1)))) (newline)
(newline)

; ============================================================
;  5. Full CoC — enable everything
; ============================================================

(display "=== Full CoC (all axes enabled) ===") (newline)
(logic-rule-enable 'type-depends-on-term)

(display "  Configuration: ") (display (logic-config)) (newline)
(display "  Simple arrow: ") (display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(display "  System F:     ") (display (infer (context) (Pi 'x (U 1) (U 0)))) (newline)
(display "  Fω:           ") (display (infer (context) (Pi 'x (U 1) (U 1)))) (newline)
(newline)

; ============================================================
;  6. Reset and verify back to defaults
; ============================================================

(display "=== Reset ===") (newline)
(logic-config-reset)
(display "After reset, all Pi forms accepted (default):") (newline)
(display "  (U 0)(U 0): ") (display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(display "  (U 1)(U 0): ") (display (infer (context) (Pi 'x (U 1) (U 0)))) (newline)
(display "  (U 1)(U 1): ") (display (infer (context) (Pi 'x (U 1) (U 1)))) (newline)
(newline)

; ============================================================
;  7. What M.2 doesn't yet do
; ============================================================

(display "=== Scope notes ===") (newline)
(display "M.2 wires only the Pi typing rule. Not yet wired:") (newline)
(display "  - Sigma (would mirror Pi)") (newline)
(display "  - pi-fresh / co-pi-fresh (the lattice rules)") (newline)
(display "  - HIT typing") (newline)
(display "  - Structural rules (weakening, contraction, exchange)") (newline)
(display "  - Modalities") (newline)
(display "Each of those is its own phase, M.3+.") (newline)
(newline)

(display "Also: the universe-level encoding is a simplification of") (newline)
(display "the strict lambda cube. A future phase could refactor to") (newline)
(display "explicit sorts (* and ☐) for stricter correspondence.") (newline)
(newline)

(display "=== Phase M.2 complete ===") (newline)
