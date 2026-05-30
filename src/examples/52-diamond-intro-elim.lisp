; -*- lisp -*-
; ============================================================
;  PHASE M.5.5 Turn 1 — Diamond intro/elim with beta rule
; ============================================================
;
; Recap of M.5 so far:
;   M.5.1 — Box and Diamond as type constructors
;   M.5.2 — (box e) and (unbox x b body); strict S4 dual context
;   M.5.3 — Named bundles K / T / S4 / S5 / modal-STLC
;   M.5.4 — modal-4-axiom toggle distinguishes T from S4
;
; M.5.5 fills in Diamond's operational layer. Until now, Diamond
; was only a type constructor — no way to construct or eliminate
; a Diamond value. This phase adds:
;
;   (diamond e)             — introduction
;   (let-diamond x b body)  — elimination
;
; Parallel to box / unbox but for the possibility modality.
;
; Beta rule:
;   (let-diamond x (diamond e) body) → body[e/x]
;
; ----- Turn structure -----
;
; Turn 1 (this file): AST nodes, descents, beta, placeholder typing
; Turn 2: 5-axiom (◇A → □◇A) as actual typing rule + modal-5-axiom
;         toggle that distinguishes S5 from S4 operationally

; ============================================================
;  1. Construction
; ============================================================

(display "=== Construction ===") (newline)
(display "  (diamond (U 0))     ⟶ ") (display (diamond (U 0))) (newline)
(display "  (let-diamond 'x (diamond (U 0)) 'x) ⟶ ")
(display (let-diamond 'x (diamond (U 0)) 'x)) (newline)
(newline)

; ============================================================
;  2. The beta rule
; ============================================================

(display "=== Beta rule ===") (newline)
(display "Equation: (let-diamond x (diamond e) body) → body[e/x]") (newline)
(newline)

(display "Identity body:") (newline)
(display "  (reduce (let-diamond 'x (diamond (U 7)) 'x))") (newline)
(display "    ↦ ") (display (reduce (let-diamond 'x (diamond (U 7)) 'x))) (newline)
(newline)

(display "Body uses x in a Pi:") (newline)
(display "  (reduce (let-diamond 'x (diamond (U 5)) (Pi 'y 'x 'y)))") (newline)
(display "    ↦ ") (display (reduce (let-diamond 'x (diamond (U 5)) (Pi 'y 'x 'y)))) (newline)
(newline)

; ============================================================
;  3. Typing
; ============================================================

(display "=== Typing ===") (newline)
(display "  (diamond (U 0))         : ")
(display (infer (context) (diamond (U 0)))) (newline)
(display "    ↑ diamond wraps the inferred type") (newline)

(display "  (let-diamond 'x (diamond (U 0)) 'x): ")
(display (infer (context) (let-diamond 'x (diamond (U 0)) 'x))) (newline)
(display "    ↑ x has type (U 0), so body 'x has type (U 0)") (newline)
(display "      and (U 0)'s universe is (U 1)") (newline)
(newline)

; ============================================================
;  4. Cross-modal rejection
; ============================================================

(display "=== Cross-modal rejection ===") (newline)
(display "Box and Diamond are distinct modalities. unbox expects Box,") (newline)
(display "let-diamond expects Diamond, and neither accepts the other.") (newline)
(newline)
(display "  (unbox 'x (diamond (U 0)) 'x): ")
(display (infer (context) (unbox 'x (diamond (U 0)) 'x))) (newline)
(display "    ↑ unbox rejected — scrutinee is Diamond, not Box") (newline)

(display "  (let-diamond 'x (box (U 0)) 'x): ")
(display (infer (context) (let-diamond 'x (box (U 0)) 'x))) (newline)
(display "    ↑ let-diamond rejected — scrutinee is Box, not Diamond") (newline)
(newline)

; ============================================================
;  5. Composition
; ============================================================

(display "=== Composition ===") (newline)
(display "  (diamond (box (U 0))) : ")
(display (infer (context) (diamond (box (U 0))))) (newline)
(display "  (box (diamond (U 0))) : ")
(display (infer (context) (box (diamond (U 0))))) (newline)
(display "    ↑ note these are DIFFERENT types — modalities don't commute") (newline)
(newline)

; ============================================================
;  6. Scope notes
; ============================================================

(display "=== Turn 1 status ===") (newline)
(display "") (newline)
(display "Working:") (newline)
(display "  ✓ AST nodes for diamond and let-diamond") (newline)
(display "  ✓ Predicates, accessors, printer") (newline)
(display "  ✓ Beta reduction: (let-diamond x (diamond e) body) → body[e/x]") (newline)
(display "  ✓ Substitution with correct binder shadowing") (newline)
(display "  ✓ Typing at the loose level (placeholder)") (newline)
(display "  ✓ Cross-modal rejection (Box ≠ Diamond)") (newline)
(display "  ✓ Toggle integration with modalities-enabled") (newline)
(display "") (newline)
(display "Not yet (Turn 2):") (newline)
(display "  ✗ 5-axiom (◇A → □◇A) as a typing rule") (newline)
(display "  ✗ modal-5-axiom toggle distinguishing S5 from S4") (newline)
(display "") (newline)
(display "=== Phase M.5.5 Turn 1 complete ===") (newline)
