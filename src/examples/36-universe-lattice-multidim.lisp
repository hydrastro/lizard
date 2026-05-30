; -*- lisp -*-
; ============================================================
;  UNIVERSE LATTICE — Phase L.2: multi-dimensional indices
; ============================================================
;
; Phase L.1 added the meet operation to give us a linear lattice
; (totally ordered, like the natural numbers with min/max).
;
; Phase L.2 makes the lattice MULTI-DIMENSIONAL: universes are
; indexed by finite *sets* of natural numbers, with subset
; inclusion as the order. This gives a real partial order where
; some pairs of universes are genuinely INCOMPARABLE.
;
; This is the distinguishing feature of lizard's universe system.
;
; Notation:
;   (U-set 0 1)    — universe at dimensions {0, 1}
;   (U-set 0 2)    — universe at dimensions {0, 2}
;   (U-set)        — empty set, the lattice bottom
;   (U n)          — backward-compat; auto-lifts to {n} when mixed
;
; Lattice operations:
;   join = U-max = set union
;   meet = U-min = set intersection
;
; The order:
;   (U-set S) ≤ (U-set T)  iff  S ⊆ T

; ------------------------------------------------------------
; 1. Construction normalizes
; ------------------------------------------------------------

(display "=== Construction ===") (newline)
(display "(U-set 2 0 1) = ")
(display (U-set 2 0 1)) (newline)
(display "  ↑ sorted") (newline)

(display "(U-set 2 0 1 0 2) = ")
(display (U-set 2 0 1 0 2)) (newline)
(display "  ↑ deduped") (newline)

(display "(U-set) = ")
(display (U-set)) (newline)
(display "  ↑ empty set, the lattice bottom") (newline)
(newline)

; ------------------------------------------------------------
; 2. Join is set union
; ------------------------------------------------------------

(display "=== Join (U-max) is set union ===") (newline)
(display "{0,1} ∨ {0,2} = ")
(display (reduce (U-max (U-set 0 1) (U-set 0 2)))) (newline)

(display "{0,1,2} ∨ {} = ")
(display (reduce (U-max (U-set 0 1 2) (U-set)))) (newline)
(display "  ↑ join with bottom is identity") (newline)
(newline)

; ------------------------------------------------------------
; 3. Meet is set intersection
; ------------------------------------------------------------

(display "=== Meet (U-min) is set intersection ===") (newline)
(display "{0,1,2} ∧ {0,2,3} = ")
(display (reduce (U-min (U-set 0 1 2) (U-set 0 2 3)))) (newline)

(display "{0,1} ∧ {2,3} = ")
(display (reduce (U-min (U-set 0 1) (U-set 2 3)))) (newline)
(display "  ↑ disjoint sets meet at the empty set") (newline)

(display "{0,1,2} ∧ {} = ")
(display (reduce (U-min (U-set 0 1 2) (U-set)))) (newline)
(display "  ↑ meet with bottom is bottom (annihilation)") (newline)
(newline)

; ------------------------------------------------------------
; 4. The defining property: incomparable elements
; ------------------------------------------------------------

(display "================================================") (newline)
(display "  THE DISTINGUISHING FEATURE") (newline)
(display "================================================") (newline)
(display "Two universes that BOTH extend (U-set 0):") (newline)
(display "  (U-set 0 1) and (U-set 0 2)") (newline)
(display "") (newline)
(display "Neither is below the other:") (newline)
(display "  {0,1} ≤ {0,2}: ")
(display (universe-leq? (U-set 0 1) (U-set 0 2))) (newline)
(display "  {0,2} ≤ {0,1}: ")
(display (universe-leq? (U-set 0 2) (U-set 0 1))) (newline)
(newline)

(display "But both are ≤ their join:") (newline)
(display "  {0,1} ≤ {0,1,2}: ")
(display (universe-leq? (U-set 0 1) (U-set 0 1 2))) (newline)
(display "  {0,2} ≤ {0,1,2}: ")
(display (universe-leq? (U-set 0 2) (U-set 0 1 2))) (newline)
(newline)

(display "And both ≥ their meet:") (newline)
(display "  {0} ≤ {0,1}: ")
(display (universe-leq? (U-set 0) (U-set 0 1))) (newline)
(display "  {0} ≤ {0,2}: ")
(display (universe-leq? (U-set 0) (U-set 0 2))) (newline)
(newline)

(display "This forms a DIAMOND in the lattice:") (newline)
(display "        {0,1,2}") (newline)
(display "        /     \\") (newline)
(display "     {0,1}   {0,2}") (newline)
(display "        \\     /") (newline)
(display "         {0}") (newline)
(newline)

; ------------------------------------------------------------
; 5. Mixed (U n) and (U-set ...)
; ------------------------------------------------------------

(display "=== Backward compatibility ===") (newline)
(display "Old single-nat (U n) still works:") (newline)

(display "(U 3) ≤ (U-set 0 1 2 3): ")
(display (universe-leq? (U 3) (U-set 0 1 2 3))) (newline)

(display "(U-max (U 3) (U-set 0 2)) = ")
(display (reduce (U-max (U 3) (U-set 0 2)))) (newline)
(display "  ↑ (U 3) auto-lifts to {3}, then unions to {0,2,3}") (newline)
(newline)

; ------------------------------------------------------------
; 6. What this enables for the thesis
; ------------------------------------------------------------

(display "=== Phase L.2 complete ===") (newline)
(display "Universes are now a real partial-order lattice.") (newline)
(display "") (newline)
(display "What this enables:") (newline)
(display "  - Multi-dimensional size tracking (e.g., logical") (newline)
(display "    complexity and computational complexity as") (newline)
(display "    independent dimensions)") (newline)
(display "  - Universe libraries from different sources can be") (newline)
(display "    combined via join without one being 'above' the") (newline)
(display "    other (impossible in a tower)") (newline)
(display "  - Foundation for couniverse work (Phase L.4): dual") (newline)
(display "    lattice with order flipped") (newline)
