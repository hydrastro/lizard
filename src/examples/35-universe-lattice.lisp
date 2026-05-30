; -*- lisp -*-
; ============================================================
;  UNIVERSE LATTICE — Phase L.1: the meet operation
; ============================================================
;
; Lizard's distinguishing feature: universes form a *lattice*, not a
; tower. Conventional type theory has U_0 : U_1 : U_2 : ... in a
; linear order. Here we add the meet operation U-min, dual to the
; existing join U-max, giving universes a proper lattice structure.
;
; The lattice axioms hold by reduction:
;   - Idempotence: u ∧ u = u, u ∨ u = u
;   - Commutativity (built into the rules: max/min are symmetric)
;   - Associativity (chain reductions resolve to the concrete level)
;   - Absorption: u ∧ (u ∨ v) = u  and  u ∨ (u ∧ v) = u
;   - Bottom: 0 is the bottom; meet with 0 is 0, join with 0 is the
;     other arg.
;
; This is Phase L.1 of the lattice work. Next phases:
;   L.2 — multi-dimensional universe indices (sets of naturals)
;   L.3 — lattice generation primitive
;   L.4 — couniverses as a dual lattice

; ------------------------------------------------------------
; 1. Concrete meet
; ------------------------------------------------------------

(display "=== Concrete meet ===") (newline)
(display "(U-min (U 2) (U 5)) = ")
(display (reduce (U-min (U 2) (U 5)))) (newline)

(display "(U-min (U 5) (U 2)) = ")
(display (reduce (U-min (U 5) (U 2)))) (newline)
(display "  (commutative)") (newline)
(newline)

; ------------------------------------------------------------
; 2. Lattice axioms by reduction
; ------------------------------------------------------------

(display "=== Idempotence ===") (newline)
(display "(U-min u u) = ")
(display (reduce (U-min (U-var 'u) (U-var 'u)))) (newline)

(display "(U-max u u) = ")
(display (reduce (U-max (U-var 'u) (U-var 'u)))) (newline)
(newline)

(display "=== Absorption ===") (newline)
(display "u ∧ (u ∨ v) = ")
(display (reduce (U-min (U-var 'u) (U-max (U-var 'u) (U-var 'v))))) (newline)
(display "  (absorption: meet with a join containing me is me)") (newline)

(display "u ∨ (u ∧ v) = ")
(display (reduce (U-max (U-var 'u) (U-min (U-var 'u) (U-var 'v))))) (newline)
(display "  (dual absorption)") (newline)
(newline)

; ------------------------------------------------------------
; 3. Bottom annihilation (DUAL of bottom identity)
; ------------------------------------------------------------

(display "=== Bottom (0) behavior ===") (newline)
(display "U-max with bottom: identity (max picks the other)") (newline)
(display "(U-max (U 0) (U 7)) = ")
(display (reduce (U-max (U 0) (U 7)))) (newline)

(display "U-min with bottom: annihilator (min returns bottom)") (newline)
(display "(U-min (U 0) (U 7)) = ")
(display (reduce (U-min (U 0) (U 7)))) (newline)
(display "  ↑ this asymmetry is what makes 0 the lattice bottom") (newline)
(newline)

; ------------------------------------------------------------
; 4. Lattice ordering
; ------------------------------------------------------------

(display "=== Lattice ordering decision procedure ===") (newline)
(display "(U-min u v) ≤ u? ")
(display (universe-leq? (U-min (U-var 'u) (U-var 'v)) (U-var 'u)))
(newline)
(display "  (meet is below each component)") (newline)

(display "u ≤ (U-max u v)? ")
(display (universe-leq? (U-var 'u) (U-max (U-var 'u) (U-var 'v))))
(newline)
(display "  (each component is below the join)") (newline)

(display "(U 2) ≤ (U-min (U 5) (U 3))? ")
(display (universe-leq? (U 2) (U-min (U 5) (U 3)))) (newline)
(display "  (true: 2 ≤ 3, and 3 is the meet)") (newline)

(display "(U 4) ≤ (U-min (U 5) (U 3))? ")
(display (universe-leq? (U 4) (U-min (U 5) (U 3)))) (newline)
(display "  (false: 4 is not ≤ 3)") (newline)
(newline)

; ------------------------------------------------------------
; 5. Implications for cumulativity
; ------------------------------------------------------------

(display "=== Implications ===") (newline)
(display "Universes are now a lattice rather than a tower.") (newline)
(display "Cumulativity follows the lattice order: T : (U u)") (newline)
(display "can be promoted to T : (U v) whenever u ≤ v in the") (newline)
(display "lattice — and ≤ now means lattice-leq, not just integer ≤.") (newline)
(newline)

(display "This is the foundation for Phase L.2: multi-dimensional") (newline)
(display "universe indices (sets of naturals) with subset inclusion") (newline)
(display "as the order. That representation will let two universes") (newline)
(display "be incomparable: U_{0,1} and U_{0,2} both extend U_0 but") (newline)
(display "neither is below the other.") (newline)

(display "=== Phase L.1 complete ===") (newline)
