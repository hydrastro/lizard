; -*- lisp -*-
; ============================================================
;  UNIVERSE LATTICE + POLYMORPHISM + CUMULATIVITY
; ============================================================
;
; lizard now has a small but real universe-expression layer:
;
;   (U n)          — concrete universe at integer level n
;   (U-var 'i)     — universe variable (for polymorphism)
;   (U-suc u)      — one level above u
;   (U-max u v)    — supremum of u and v
;
; Universe expressions reduce when they can:
;   (U-suc (U n))         → (U n+1)
;   (U-max (U n) (U m))   → (U max(n,m))
;   (U-max u u)           → u           (idempotence)
;   (U-max (U 0) u)       → u           (0 is bottom)
;
; And there's a cumulativity predicate:
;   (universe-leq? u v)   → #t, #f, or 'unknown
;
; The lattice operations are partial when variables are involved
; — that's the honest answer for a system without unification.

; ------------------------------------------------------------
; 1. Concrete universe arithmetic
; ------------------------------------------------------------

(display "=== Concrete arithmetic ===") (newline)
(display "(U-suc (U 0)) = ") (display (reduce (U-suc (U 0)))) (newline)
(display "(U-suc (U-suc (U 0))) = ") (display (reduce (U-suc (U-suc (U 0))))) (newline)
(display "(U-suc (U-suc (U-suc (U 4)))) = ")
(display (reduce (U-suc (U-suc (U-suc (U 4)))))) (newline)

(display "(U-max (U 2) (U 5)) = ") (display (reduce (U-max (U 2) (U 5)))) (newline)
(display "(U-max (U 9) (U 3)) = ") (display (reduce (U-max (U 9) (U 3)))) (newline)
(newline)

; ------------------------------------------------------------
; 2. Symbolic universes (the polymorphism part)
; ------------------------------------------------------------

(display "=== Symbolic universes ===") (newline)
(display "(U-var 'i) = ") (display (U-var 'i)) (newline)
(display "(U-suc (U-var 'i)) = ") (display (reduce (U-suc (U-var 'i)))) (newline)
(display "  (cannot simplify symbolic suc)") (newline)

(display "(U-max (U-var 'i) (U-var 'i)) = ")
(display (reduce (U-max (U-var 'i) (U-var 'i)))) (newline)
(display "  (idempotence kicks in)") (newline)

(display "(U-max (U-var 'i) (U-var 'j)) = ")
(display (reduce (U-max (U-var 'i) (U-var 'j)))) (newline)
(display "  (two different variables — stays symbolic)") (newline)

(display "(U-max (U 0) (U-var 'i)) = ")
(display (reduce (U-max (U 0) (U-var 'i)))) (newline)
(display "  (0 is absorbed)") (newline)

(display "(U-max (U-var 'i) (U 0)) = ")
(display (reduce (U-max (U-var 'i) (U 0)))) (newline)
(display "  (works on either side)") (newline)
(newline)

; ------------------------------------------------------------
; 3. The lattice operation composes
; ------------------------------------------------------------

(display "=== Composing ===") (newline)
(display "(U-suc (U-max (U 3) (U 7))) = ")
(display (reduce (U-suc (U-max (U 3) (U 7))))) (newline)
(display "  (inner reduces to (U 7), then U-suc to (U 8))") (newline)

(display "(U-max (U-suc (U 2)) (U-suc (U 5))) = ")
(display (reduce (U-max (U-suc (U 2)) (U-suc (U 5))))) (newline)
(display "  (both sucs concrete-reduce, then max kicks in)") (newline)
(newline)

; ------------------------------------------------------------
; 4. Cumulativity decisions
; ------------------------------------------------------------

(display "=== Cumulativity ===") (newline)
(display "0 ≤ 0:  ") (display (universe-leq? (U 0) (U 0))) (newline)
(display "0 ≤ 5:  ") (display (universe-leq? (U 0) (U 5))) (newline)
(display "5 ≤ 0:  ") (display (universe-leq? (U 5) (U 0))) (newline)
(display "5 ≤ 5:  ") (display (universe-leq? (U 5) (U 5))) (newline)

(display "(suc (suc (U 3))) ≤ (U 5):  ")
(display (universe-leq? (U-suc (U-suc (U 3))) (U 5))) (newline)

(display "i ≤ i (same var):  ")
(display (universe-leq? (U-var 'i) (U-var 'i))) (newline)
(display "i ≤ j (diff vars): ")
(display (universe-leq? (U-var 'i) (U-var 'j))) (newline)

(display "i ≤ (suc i): ")
(display (universe-leq? (U-var 'i) (U-suc (U-var 'i)))) (newline)
(display "(max 1 i) ≤ (suc i):  ")
(display (universe-leq? (U-max (U 1) (U-var 'i)) (U-suc (U-var 'i)))) (newline)
(display "  (above is genuinely undecidable without knowing i)") (newline)
(newline)

; ------------------------------------------------------------
; 5. Cumulativity meets type checking
; ------------------------------------------------------------

(display "=== Cumulativity in the type checker ===") (newline)
(display "(check (context) (U 0) (U 1)): ")
(display (check (context) (U 0) (U 1))) (newline)
(display "  (U 0 inferred-type is (U 1); cumulativity gives (U 1) ≤ (U 1))")
(newline)

(display "(check (context) (U 0) (U 100)): ")
(display (check (context) (U 0) (U 100))) (newline)
(display "  (cumulativity all the way up)") (newline)

(display "(check (context) (U 5) (U 4)): ")
(display (check (context) (U 5) (U 4))) (newline)
(display "  (no, cumulativity is monotone-up only)") (newline)
(newline)

; ------------------------------------------------------------
; 6. Flags control universe simplification
; ------------------------------------------------------------

(display "=== Pluggability ===") (newline)
(flag-set! 'reduce-u-suc-concrete #f)
(display "with U-suc-concrete OFF:") (newline)
(display "  (U-suc (U 3)) = ") (display (reduce (U-suc (U 3)))) (newline)
(flag-set! 'reduce-u-suc-concrete #t)

(flag-set! 'reduce-u-max-idem #f)
(display "with U-max-idem OFF:") (newline)
(display "  (U-max (U-var 'i) (U-var 'i)) = ")
(display (reduce (U-max (U-var 'i) (U-var 'i)))) (newline)
(flag-set! 'reduce-u-max-idem #t)
(newline)

(display "=== Scope ===") (newline)
(display "  Implemented: concrete max/suc, idempotence, 0-absorption,") (newline)
(display "    cumulativity predicate with #t/#f/unknown for partial decidability") (newline)
(display "  Not yet:") (newline)
(display "    universe-level Λ-binder (true polymorphism)") (newline)
(display "    Coq-style universe constraint solver") (newline)
(display "    inductive types whose universe depends on parameters") (newline)
