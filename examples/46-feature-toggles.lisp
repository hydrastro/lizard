; -*- lisp -*-
; ============================================================
;  PHASE M.6 — Lizard-distinctive features as configurable axes
; ============================================================
;
; The lattice (L.1-L.5), HITs (H.1), and couniverse machinery
; are lizard's distinctive features. Until now they were ALWAYS
; available — there was no way to configure lizard as, say,
; "STLC with no lattice extras" or "CoC with lattice but no
; HITs."
;
; M.6 makes them toggleable rules in the M.1 registry:
;
;   pi-fresh-enabled     gates pi-fresh / sigma-fresh
;   co-pi-fresh-enabled  gates co-pi-fresh / co-sigma-fresh
;   HIT-enabled          gates HIT-ref / HIT-app typing
;   lattice-universes    gates (U-set ...) typing
;   couniverse-lattice   gates (Co-set ...) typing
;
; Plus two new bundles demonstrating the matrix:
;
;   STLC-strict        STLC with ALL lizard extras off
;   CoC-plus-lattice   CoC with lattice on, HITs off

; ============================================================
;  1. Default — everything available
; ============================================================

(display "=== Default (CoC, all features) ===") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  pi-fresh:    ") (display (infer (context) (pi-fresh 'x (U 0) (U 0)))) (newline)
(display "  U-set:       ") (display (infer (context) (U-set 0 1))) (newline)
(display "  Co-set:      ") (display (infer (context) (Co-set 0 1))) (newline)
(newline)

; ============================================================
;  2. Individual toggles
; ============================================================

(display "=== Disable pi-fresh-enabled ===") (newline)
(logic-rule-disable 'pi-fresh-enabled)
(display "  pi-fresh:        ")
(display (infer (context) (pi-fresh 'x (U 0) (U 0)))) (newline)
(display "  sigma-fresh:     ")
(display (infer (context) (sigma-fresh 'x (U 0) (U 0)))) (newline)
(display "  ordinary Pi OK:  ")
(display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(logic-rule-enable 'pi-fresh-enabled)
(newline)

(display "=== Disable lattice-universes ===") (newline)
(logic-rule-disable 'lattice-universes)
(display "  (U-set 0 1):     ")
(display (infer (context) (U-set 0 1))) (newline)
(display "  ordinary (U 1):  ")
(display (infer (context) (U 1))) (newline)
(logic-rule-enable 'lattice-universes)
(newline)

(display "=== Disable HIT-enabled ===") (newline)
(define s1 (HIT 'S1 (HIT-constructor 'base)))
(display "  HIT declaration itself still creates metadata —") (newline)
(display "  only HIT-ref / HIT-app typing is gated.") (newline)
(logic-rule-disable 'HIT-enabled)
(display "  (HIT-ref 'S1):   ")
(display (infer (context) (HIT-ref 'S1))) (newline)
(logic-rule-enable 'HIT-enabled)
(newline)

; ============================================================
;  3. The new bundles
; ============================================================

(display "=== Bundle: STLC-strict ===") (newline)
(set-logic 'STLC-strict)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  This is STLC with ALL lizard extras turned off.") (newline)
(display "  Cube axes:") (newline)
(display "    term-on-type? ") (display (logic-rule-enabled? 'term-depends-on-type)) (newline)
(display "    type-on-term? ") (display (logic-rule-enabled? 'type-depends-on-term)) (newline)
(display "    type-on-type? ") (display (logic-rule-enabled? 'type-depends-on-type)) (newline)
(display "  Feature toggles:") (newline)
(display "    pi-fresh?           ") (display (logic-rule-enabled? 'pi-fresh-enabled)) (newline)
(display "    co-pi-fresh?        ") (display (logic-rule-enabled? 'co-pi-fresh-enabled)) (newline)
(display "    HIT?                ") (display (logic-rule-enabled? 'HIT-enabled)) (newline)
(display "    lattice-universes?  ") (display (logic-rule-enabled? 'lattice-universes)) (newline)
(display "    couniverse-lattice? ") (display (logic-rule-enabled? 'couniverse-lattice)) (newline)
(display "  Plain Pi still works:") (newline)
(display "    ") (display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(display "  But lattice forms rejected:") (newline)
(display "    (pi-fresh ...): ")
(display (infer (context) (pi-fresh 'x (U 0) (U 0)))) (newline)
(display "    (U-set 0 1):    ")
(display (infer (context) (U-set 0 1))) (newline)
(newline)

(display "=== Bundle: CoC-plus-lattice ===") (newline)
(set-logic 'CoC-plus-lattice)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  This is CoC with lattice ON but HITs OFF.") (newline)
(display "  pi-fresh:") (newline)
(display "    ") (display (infer (context) (pi-fresh 'x (U 0) (U 0)))) (newline)
(display "  HIT-ref:") (newline)
(display "    ") (display (infer (context) (HIT-ref 'S1))) (newline)
(display "  (note: HIT declaration data still exists in the registry,") (newline)
(display "   it's just that HIT-ref typing is gated)") (newline)
(newline)

; ============================================================
;  4. What this enables
; ============================================================

(display "=== Why this matters ===") (newline)
(display "") (newline)
(display "Before M.6: lizard's lattice/HIT features were always") (newline)
(display "available, intermixed with whatever cube configuration you") (newline)
(display "chose. STLC-with-lattice was indistinguishable from") (newline)
(display "CoC-with-lattice if you only used Pi.") (newline)
(display "") (newline)
(display "After M.6: the lizard-distinctive features are explicitly") (newline)
(display "part of the configuration matrix. The user can declare:") (newline)
(display "") (newline)
(display "  - pure STLC (no extras)") (newline)
(display "  - CoC with lattice for size-tracking research") (newline)
(display "  - CoC with HITs but no lattice (more like Cubical Agda)") (newline)
(display "  - CoC with everything (the default / full thesis logic)") (newline)
(display "  - custom: pick any subset") (newline)
(display "") (newline)
(display "The lizard-distinctive features have become FIRST-CLASS") (newline)
(display "configuration axes alongside the cube. This is what makes") (newline)
(display "the 'multiple logics with few commands' vision concrete.") (newline)
(newline)

; ============================================================
;  5. The matrix view
; ============================================================

(display "=== The full bundle list ===") (newline)
(display "  ") (display (list-logics)) (newline)
(newline)

(logic-config-reset)
(display "=== After reset ===") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  pi-fresh still works: ")
(display (infer (context) (pi-fresh 'x (U 0) (U 0)))) (newline)
(newline)

(display "=== Phase M.6 complete ===") (newline)
(newline)
(display "Pending: M.5 (modalities), H.2 (specific HITs), and") (newline)
(display "the broader research questions about lattice metatheory.") (newline)
