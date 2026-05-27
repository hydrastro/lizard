; -*- lisp -*-
; ============================================================
;  PHASE M.5.1 — Modal type constructors
;  Box (□) and Diamond (◇) as inert types
; ============================================================
;
; Modal logic adds two unary operators to a base logic:
;
;   □A "necessarily A"  — Box A
;   ◇A "possibly A"     — Diamond A
;
; In modal TYPE theory these appear as type constructors. A
; value of type Box A is a "necessary A"; a value of type
; Diamond A is a "possible A". What those mean computationally
; depends on which modal logic you choose (K, T, S4, S5, ...).
;
; ----- What M.5.1 delivers -----
;
; The TYPE CONSTRUCTOR level only. (Box A) and (Diamond A) are
; well-typed types when A is a type, with the rule:
;
;   A : U_n
;   ────────────────────
;   (Box A) : U_n        (and similarly for Diamond)
;
; The modality preserves the universe of its argument. This is
; the minimal commitment — it says NOTHING about which modal
; logic Box belongs to. That choice comes with intro/elim
; (M.5.2), since different modal logics have different rules.
;
; M.5.1 also wires `modalities-enabled` as a M.6-style toggle.
;
; ----- What M.5.1 does NOT deliver -----
;
;   - No (box e) introduction form
;   - No (unbox b) elimination form
;   - No K-axiom, T-axiom, etc.
;   - No reduction interaction with Pi or other types

; ============================================================
;  1. Construction
; ============================================================

(display "=== Constructors ===") (newline)
(display "  (Box (U 0))     : ") (display (Box (U 0))) (newline)
(display "  (Diamond (U 0)) : ") (display (Diamond (U 0))) (newline)
(newline)

; ============================================================
;  2. Predicates and accessors
; ============================================================

(display "=== Predicates ===") (newline)
(display "  (Box? (Box (U 0)))       : ") (display (Box? (Box (U 0)))) (newline)
(display "  (Box? (Diamond (U 0)))   : ") (display (Box? (Diamond (U 0)))) (newline)
(display "  (Diamond? (Box (U 0)))   : ") (display (Diamond? (Box (U 0)))) (newline)
(newline)

(display "=== Accessors ===") (newline)
(display "  (Box-arg (Box (U 0)))                 : ")
(display (Box-arg (Box (U 0)))) (newline)
(display "  (Diamond-arg (Diamond (Pi 'x (U 0) (U 0)))) : ")
(display (Diamond-arg (Diamond (Pi 'x (U 0) (U 0))))) (newline)
(newline)

; ============================================================
;  3. Typing rule: modality preserves universe
; ============================================================

(display "=== Typing ===") (newline)
(display "  (Box (U 0))   : ") (display (infer (context) (Box (U 0)))) (newline)
(display "  (Box (U 1))   : ") (display (infer (context) (Box (U 1)))) (newline)
(display "  (Diamond (U 2)) : ") (display (infer (context) (Diamond (U 2)))) (newline)
(display "  (Box (Pi 'x (U 0) (U 0))) : ")
(display (infer (context) (Box (Pi 'x (U 0) (U 0))))) (newline)
(newline)

(display "=== Nested modalities ===") (newline)
(display "  (Box (Box (U 0)))           : ")
(display (infer (context) (Box (Box (U 0))))) (newline)
(display "  (Box (Diamond (Box (U 0)))) : ")
(display (infer (context) (Box (Diamond (Box (U 0)))))) (newline)
(display "  ↑ all stay at the same universe (modality preserves level)") (newline)
(newline)

; ============================================================
;  4. Reduction descends into argument
; ============================================================

(display "=== Reduction descends ===") (newline)
(display "  (reduce (Box (U-max (U-set 0) (U-set 1)))) :") (newline)
(display "    ") (display (reduce (Box (U-max (U-set 0) (U-set 1))))) (newline)
(display "  ↑ Box is preserved; the argument reduces inside") (newline)
(newline)

; ============================================================
;  5. Composition with lizard's lattice
; ============================================================

(display "=== Box of dimension-creating Pi ===") (newline)
(display "  (Box (pi-fresh 'x (U 0) (U 0))) :") (newline)
(display "    ") (display (infer (context) (Box (pi-fresh 'x (U 0) (U 0))))) (newline)
(display "  ↑ the lattice point survives through the modality") (newline)
(newline)

(display "=== Diamond of co-pi-fresh ===") (newline)
(display "  (infer (context) (Diamond (co-pi-fresh 'x (Uco 0) (Uco 0)))) :") (newline)
(display "    ") 
(display (infer (context) (Diamond (co-pi-fresh 'x (Uco 0) (Uco 0))))) (newline)
(display "  ↑ the COUNIVERSE point survives the modality") (newline)
(newline)

; ============================================================
;  6. The toggle: modalities-enabled
; ============================================================

(display "=== Disabling modalities ===") (newline)
(logic-rule-disable 'modalities-enabled)
(display "  After (logic-rule-disable 'modalities-enabled):") (newline)
(display "  (infer (context) (Box (U 0))) : ")
(display (infer (context) (Box (U 0)))) (newline)
(display "  ↑ typing rejected, but constructor still works:") (newline)
(display "  (Box (U 0)) → ") (display (Box (U 0))) (newline)
(display "  ↑ only the TYPING is gated, not the AST construction") (newline)

(logic-rule-enable 'modalities-enabled)
(display "  After re-enable: ")
(display (infer (context) (Box (U 0)))) (newline)
(newline)

; ============================================================
;  7. Honest scope notes
; ============================================================

(display "=== Scope of M.5.1 ===") (newline)
(display "") (newline)
(display "What works:") (newline)
(display "  ✓ Box and Diamond as type constructors") (newline)
(display "  ✓ Predicates, accessors, printer") (newline)
(display "  ✓ Typing rule (modality preserves universe)") (newline)
(display "  ✓ Substitution, alpha-equality, structural equality") (newline)
(display "  ✓ Reduction descends into the argument") (newline)
(display "  ✓ Composition with all other type formers") (newline)
(display "  ✓ Toggle integration via modalities-enabled") (newline)
(display "") (newline)
(display "What's missing (M.5.2):") (newline)
(display "  ✗ Introduction form (box e) for constructing Box values") (newline)
(display "  ✗ Elimination form (unbox b) for using Box values") (newline)
(display "  ✗ K-axiom, T-axiom, S4-axiom, S5-axiom, etc.") (newline)
(display "  ✗ Commitment to a SPECIFIC modal logic") (newline)
(display "") (newline)
(display "The intro/elim forms commit to a particular modal logic,") (newline)
(display "since different logics have different rules. That choice") (newline)
(display "is deferred to M.5.2 where it can be made deliberately.") (newline)
(newline)

(logic-config-reset)
(display "=== Phase M.5.1 complete ===") (newline)
(newline)
(display "Next: M.5.2 (intro/elim forms + a chosen modal logic),") (newline)
(display "then M.5.3 (modal-logic bundles).") (newline)
