; -*- lisp -*-
; ============================================================
;  PHASE M.5.2 (Turn 1) — Box intro and elim with beta rule
; ============================================================
;
; M.5.1 added the Box and Diamond type constructors.
; M.5.2 Turn 1 adds the operational layer for Box: how do you
; CONSTRUCT a Box value, and how do you USE one?
;
;   Introduction: (box e)
;     Wraps a term e to produce a Box value
;
;   Elimination: (unbox x b body)
;     Read as "let unbox x = b in body"
;     Pattern-matches b against (box ...) and binds the unboxed
;     contents as x within body
;
; The fundamental computational equation:
;
;   (unbox x (box e) body)  ⟶  body[e/x]
;
; This is the beta rule for Box, parallel to the beta rule for
; lambda: (λx.body) e → body[e/x].
;
; ----- Honest scope note -----
;
; Turn 1 uses PLACEHOLDER typing — it doesn't enforce the dual-
; context discipline of S4 modal type theory. That means: any
; well-typed e gives a well-typed (box e); any Box-typed b with
; an appropriate body gives a well-typed unbox. The strict S4
; rules (which require "valid contexts" with only modal hypotheses
; available during box-introduction) come in Turn 2.
;
; What works in Turn 1: computation. The beta rule is correct,
; substitution is correct, types are checked at the loose level.
; This means you can WRITE programs with modalities and EXECUTE
; them — you just can't yet rule out unsound uses statically.

; ============================================================
;  1. Construction
; ============================================================

(display "=== Construction ===") (newline)
(display "  (box (U 0)):") (newline)
(display "    ") (display (box (U 0))) (newline)
(display "  (unbox 'x (box (U 0)) 'x):") (newline)
(display "    ") (display (unbox 'x (box (U 0)) 'x)) (newline)
(newline)

; ============================================================
;  2. The beta rule
; ============================================================

(display "=== Beta rule ===") (newline)
(display "Equation: (unbox x (box e) body) → body[e/x]") (newline)
(newline)

(display "Identity body:") (newline)
(display "  (reduce (unbox 'x (box (U 7)) 'x))") (newline)
(display "    ↦ ") (display (reduce (unbox 'x (box (U 7)) 'x))) (newline)
(newline)

(display "Body uses the unboxed value:") (newline)
(display "  (reduce (unbox 'x (box (U 5)) (Pi 'y 'x 'y)))") (newline)
(display "    ↦ ") (display (reduce (unbox 'x (box (U 5)) (Pi 'y 'x 'y)))) (newline)
(display "    ↑ (U 5) substituted for x in the Pi") (newline)
(newline)

(display "Body ignores the unboxed value:") (newline)
(display "  (reduce (unbox 'x (box (U 5)) (U 1)))") (newline)
(display "    ↦ ") (display (reduce (unbox 'x (box (U 5)) (U 1)))) (newline)
(display "    ↑ body has no free x, so substitution is a no-op") (newline)
(newline)

; ============================================================
;  3. Typing
; ============================================================

(display "=== Typing ===") (newline)
(display "  (infer (context) (box (U 0))):") (newline)
(display "    ") (display (infer (context) (box (U 0)))) (newline)
(display "    ↑ (Box (U 1)) — box wraps the inferred type of (U 0)") (newline)
(newline)

(display "  (infer (context) (unbox 'x (box (U 0)) 'x)):") (newline)
(display "    ") (display (infer (context) (unbox 'x (box (U 0)) 'x))) (newline)
(display "    ↑ x has type (U 0), so body 'x has type (U 0),") (newline)
(display "      and (U 0)'s universe is (U 1)") (newline)
(newline)

(display "  Reject when scrutinee isn't Box-typed:") (newline)
(display "    (infer (context) (unbox 'x (U 0) 'x))") (newline)
(display "    ") (display (infer (context) (unbox 'x (U 0) 'x))) (newline)
(newline)

; ============================================================
;  4. Binder shadowing
; ============================================================

(display "=== Binder shadowing ===") (newline)
(display "  Nested unbox where inner binder shadows outer:") (newline)
(display "  (reduce (unbox 'x (box (U 0)) (unbox 'x (box (U 7)) 'x)))") (newline)
(display "    ↦ ") (display (reduce (unbox 'x (box (U 0)) (unbox 'x (box (U 7)) 'x)))) (newline)
(display "    ↑ inner x = (U 7); outer x = (U 0) is shadowed; result = (U 7)") (newline)
(newline)

; ============================================================
;  5. Composition with lizard's lattice
; ============================================================

(display "=== Composition with pi-fresh ===") (newline)
(display "  (infer (context) (box (pi-fresh 'a (U 0) (U 0)))):") (newline)
(display "    ") (display (infer (context) (box (pi-fresh 'a (U 0) (U 0))))) (newline)
(display "    ↑ the dimension-creating Pi gets boxed; lattice point") (newline)
(display "      survives through the modality") (newline)
(newline)

; ============================================================
;  6. The gate
; ============================================================

(display "=== Gate ===") (newline)
(logic-rule-disable 'modalities-enabled)
(display "  After disabling modalities-enabled:") (newline)
(display "    (infer ... (box ...)): ")
(display (infer (context) (box (U 0)))) (newline)
(display "    But beta still works at the AST level:") (newline)
(display "    (reduce (unbox 'x (box (U 0)) 'x)):") (newline)
(display "    ") (display (reduce (unbox 'x (box (U 0)) 'x))) (newline)
(display "    ↑ reduction is independent of the typing gate") (newline)
(logic-rule-enable 'modalities-enabled)
(newline)

; ============================================================
;  7. Scope
; ============================================================

(display "=== Scope of Turn 1 ===") (newline)
(display "") (newline)
(display "Working:") (newline)
(display "  ✓ AST nodes and constructors for box and unbox") (newline)
(display "  ✓ Predicates, accessors, printer") (newline)
(display "  ✓ Beta reduction: (unbox x (box e) body) → body[e/x]") (newline)
(display "  ✓ Substitution with correct binder shadowing") (newline)
(display "  ✓ Typing at the loose level") (newline)
(display "  ✓ Toggle integration with modalities-enabled") (newline)
(display "") (newline)
(display "Not yet (Turn 2):") (newline)
(display "  ✗ Dual context (Γ; Δ) for S4-style scoping") (newline)
(display "  ✗ Strict box-intro rule: e must typecheck against Δ only") (newline)
(display "  ✗ Strict unbox: bound variable goes to the Δ side") (newline)
(display "  ✗ Modal-logic-specific axioms (K, T, S4)") (newline)
(display "") (newline)
(display "The loose typing in Turn 1 is enough to write and execute") (newline)
(display "modal programs. Turn 2 will tighten the type discipline.") (newline)
(newline)

(display "=== Phase M.5.2 Turn 1 complete ===") (newline)
