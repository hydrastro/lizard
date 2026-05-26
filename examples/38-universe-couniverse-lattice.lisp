; -*- lisp -*-
; ============================================================
;  PHASE L.4 — The dual lattice
;  Universes (U-set) and couniverses (Co-set) as separate sorts
; ============================================================
;
; The thesis intuition: types live in universes; contexts and
; observations live in couniverses. These are not just two ways
; to view the same thing — they're genuinely distinct sorts with
; parallel-but-separate lattice structure.
;
; Phase L.4 makes this concrete:
;
;   (U-set 0 1)    universe at dims {0, 1}
;   (Co-set 0 1)   COuniverse at dims {0, 1}
;
;   (U-max u v)    join in the U-lattice
;   (Co-max c d)   join in the Co-lattice — refuses to mix with U
;
;   (U-min u v)    meet in the U-lattice
;   (Co-min c d)   meet in the Co-lattice
;
;   (universe-leq? u v)     subset in the U-lattice
;   (couniverse-leq? c d)   subset in the Co-lattice — mixed = unknown
;
; Mixing across lattices is a structural error: Co-max on a U-set
; stays unreduced, couniverse-leq? on mixed input returns 'unknown.

(display "============================================================") (newline)
(display "  Parallel structure") (newline)
(display "============================================================") (newline)
(newline)

(display "Universe and couniverse lattices have parallel ops:") (newline)
(display "  (U-max (U-set 0 1) (U-set 0 2))  = ")
(display (reduce (U-max (U-set 0 1) (U-set 0 2)))) (newline)
(display "  (Co-max (Co-set 0 1) (Co-set 0 2)) = ")
(display (reduce (Co-max (Co-set 0 1) (Co-set 0 2)))) (newline)
(display "  ↑ same shape, separate sorts") (newline)
(newline)

(display "  (U-min (U-set 0 1 2) (U-set 0 2 3))  = ")
(display (reduce (U-min (U-set 0 1 2) (U-set 0 2 3)))) (newline)
(display "  (Co-min (Co-set 0 1 2) (Co-set 0 2 3)) = ")
(display (reduce (Co-min (Co-set 0 1 2) (Co-set 0 2 3)))) (newline)
(newline)

(display "============================================================") (newline)
(display "  Lattice axioms hold in the Co-lattice too") (newline)
(display "============================================================") (newline)
(newline)

(display "Idempotence:") (newline)
(display "  (Co-max c c) = ")
(display (reduce (Co-max (Co-set 0 1) (Co-set 0 1)))) (newline)
(display "  (Co-min c c) = ")
(display (reduce (Co-min (Co-set 0 1) (Co-set 0 1)))) (newline)
(newline)

(display "Absorption (defining lattice axiom):") (newline)
(display "  (Co-min c (Co-max c d)) = ")
(display (reduce (Co-min (Co-set 0) (Co-max (Co-set 0) (Co-set 1))))) (newline)
(display "  (Co-max c (Co-min c d)) = ")
(display (reduce (Co-max (Co-set 0) (Co-min (Co-set 0) (Co-set 1))))) (newline)
(newline)

(display "============================================================") (newline)
(display "  Couniverse ordering — parallel to universe ordering") (newline)
(display "============================================================") (newline)
(newline)

(display "(Co-set 0 1) ≤ (Co-set 0 1 2): ")
(display (couniverse-leq? (Co-set 0 1) (Co-set 0 1 2))) (newline)
(display "(Co-set 0 1) ≤ (Co-set 0 2): ")
(display (couniverse-leq? (Co-set 0 1) (Co-set 0 2))) (newline)
(display "  ↑ incomparable: same lattice property as U") (newline)
(newline)

(display "============================================================") (newline)
(display "  CRUCIAL — the two lattices stay separate") (newline)
(display "============================================================") (newline)
(newline)

(display "Mixing U-set and Co-set under Co-max stays unreduced:") (newline)
(display "  (Co-max (Co-set 0 1) (U-set 0 1)) = ")
(display (reduce (Co-max (Co-set 0 1) (U-set 0 1)))) (newline)
(display "  ↑ neither lattice reduction fires — sort mismatch") (newline)
(newline)

(display "Same for U-max with a Co-set arg:") (newline)
(display "  (U-max (U-set 0 1) (Co-set 0 1)) = ")
(display (reduce (U-max (U-set 0 1) (Co-set 0 1)))) (newline)
(newline)

(display "Cross-lattice ordering returns 'unknown:") (newline)
(display "  (couniverse-leq? (Co-set 0) (U-set 0)) = ")
(display (couniverse-leq? (Co-set 0) (U-set 0))) (newline)
(display "  (universe-leq? (U-set 0) (Co-set 0)) = ")
(display (universe-leq? (U-set 0) (Co-set 0))) (newline)
(display "  ↑ 'unknown' is the right answer — they're different sorts") (newline)
(newline)

(display "============================================================") (newline)
(display "  What this enables") (newline)
(display "============================================================") (newline)
(newline)
(display "With two separate lattices we can now ENCODE the distinction") (newline)
(display "between universes-of-types and couniverses-of-contexts/") (newline)
(display "observations as a sort distinction in the type system itself.") (newline)
(display "") (newline)
(display "A judgment that asserts something at a universe level vs. a") (newline)
(display "couniverse level is now structurally different — neither can") (newline)
(display "accidentally substitute for the other.") (newline)
(display "") (newline)
(display "Whether and how the two lattices SHOULD interact in the") (newline)
(display "framework (e.g., via explicit conversion operators, or via") (newline)
(display "a duality functor between them) is a design question for") (newline)
(display "future phases.") (newline)

(display "=== Phase L.4 complete ===") (newline)
