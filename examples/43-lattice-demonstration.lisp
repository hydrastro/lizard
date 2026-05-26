; -*- lisp -*-
; ============================================================
;  LATTICE DEMONSTRATION
;  What lizard does that tower-based universe systems can't
; ============================================================
;
; This example walks through a worked scenario where lizard's
; universe lattice (Phases L.1-L.5) produces type-checking
; behavior that is *structurally impossible* in a tower-based
; system like Coq, Agda, or Lean.
;
; The point is not "lizard does more"; it's that lizard does
; something DIFFERENT — captures a structural property of
; quantification that towers flatten away.
;
; ----- The thesis claim being demonstrated -----
;
; "What generates a new universe is quantifying on a previous
;  universe."  (Phase L.3)
;
; In a tower-based system, every universe is comparable to
; every other — they're linearly ordered. Two independent
; quantifications either stay at the same level or one has
; to be artificially lifted.
;
; In lizard's lattice, two independent `pi-fresh` quantifications
; produce universes that are *incomparable*. Neither extends
; the other. Their join is a strictly larger universe that
; contains both dimensions.
;
; This isn't a feature checkbox. It's a different account of
; what universes are: a partial order that tracks WHERE size
; growth came from, not just HOW MUCH growth happened.

; ============================================================
;  PART 1 — Two parallel "libraries"
; ============================================================
;
; Imagine two independent developments. Library A works in one
; mathematical domain (call it geometric structures), library B
; in another (algebraic structures). Each library uses pi-fresh
; to define its own quantified types.

(display "============================================================") (newline)
(display "  PART 1 — Two parallel libraries") (newline)
(display "============================================================") (newline)
(newline)

(display "Library A — quantifies over base types:") (newline)
(define libA_quant1 (infer (context) (pi-fresh 'X (U 0) (U 0))))
(display "  ") (display libA_quant1) (newline)
(define libA_quant2 (infer (context) (pi-fresh 'Y (U 0) (U 0))))
(display "  ") (display libA_quant2) (newline)
(newline)

(display "Library B — also quantifies over base types,") (newline)
(display "developed independently:") (newline)
(define libB_quant1 (infer (context) (pi-fresh 'Z (U 0) (U 0))))
(display "  ") (display libB_quant1) (newline)
(define libB_quant2 (infer (context) (pi-fresh 'W (U 0) (U 0))))
(display "  ") (display libB_quant2) (newline)
(newline)

(display "Notice: every pi-fresh got a DIFFERENT fresh dim.") (newline)
(display "The four quantifications are at four different lattice") (newline)
(display "points, even though they all started from (U 0).") (newline)
(newline)

; ============================================================
;  PART 2 — These universes are INCOMPARABLE
; ============================================================

(display "============================================================") (newline)
(display "  PART 2 — Genuine incomparability") (newline)
(display "============================================================") (newline)
(newline)

(display "Are libA_quant1 and libB_quant1 in any order?") (newline)
(display "  libA_quant1 ≤ libB_quant1: ")
(display (universe-leq? libA_quant1 libB_quant1)) (newline)
(display "  libB_quant1 ≤ libA_quant1: ")
(display (universe-leq? libB_quant1 libA_quant1)) (newline)
(newline)

(display "Neither. They're at INCOMPARABLE points in the lattice.") (newline)
(display "") (newline)
(display "This is structurally impossible in a tower. In Coq/Agda/Lean,") (newline)
(display "two universes (Type i) and (Type j) always have i ≤ j or") (newline)
(display "j ≤ i (or both, if i = j). Universes form a chain.") (newline)
(display "") (newline)
(display "Lizard's lattice tracks *which derivation* produced each") (newline)
(display "universe — the fresh dim records the binding event. Two") (newline)
(display "independent bindings yield incomparable universes because") (newline)
(display "the lattice records WHERE the growth came from, not just") (newline)
(display "how much growth happened.") (newline)
(newline)

; ============================================================
;  PART 3 — The join: combining the two libraries
; ============================================================

(display "============================================================") (newline)
(display "  PART 3 — The join combines both") (newline)
(display "============================================================") (newline)
(newline)

(display "When code uses results from BOTH libraries, the universe") (newline)
(display "of the combined thing is the lattice JOIN — strictly larger") (newline)
(display "than either library's universe.") (newline)
(newline)

(display "Computing the join (U-max libA_quant1 libB_quant1):") (newline)
(define combined (reduce (U-max libA_quant1 libB_quant1)))
(display "  ") (display combined) (newline)
(newline)

(display "And the combined universe is ≥ both inputs:") (newline)
(display "  libA_quant1 ≤ combined: ")
(display (universe-leq? libA_quant1 combined)) (newline)
(display "  libB_quant1 ≤ combined: ")
(display (universe-leq? libB_quant1 combined)) (newline)
(newline)

(display "But neither library was forced to be 'lifted' to the other's") (newline)
(display "level. The composition happens at a point that strictly") (newline)
(display "extends BOTH branches without privileging either.") (newline)
(newline)

; ============================================================
;  PART 4 — The contrast with tower-based systems
; ============================================================

(display "============================================================") (newline)
(display "  PART 4 — What a tower would have required") (newline)
(display "============================================================") (newline)
(newline)

(display "Imagine the same scenario in a tower system (Coq-like):") (newline)
(display "") (newline)
(display "Library A defines quantified types at level i₀.") (newline)
(display "Library B defines quantified types at level i₁.") (newline)
(display "") (newline)
(display "If i₀ = i₁, the libraries 'conflict' at the same level —") (newline)
(display "their content has to share the level even if it's logically") (newline)
(display "independent. Universe polymorphism papers over this with") (newline)
(display "implicit level variables, but the polymorphism is over a") (newline)
(display "single nat dimension.") (newline)
(display "") (newline)
(display "If i₀ ≠ i₁, one library has to be ABOVE the other in the") (newline)
(display "tower. This privileges one branch — the choice of which is") (newline)
(display "above is artificial; the math doesn't dictate it.") (newline)
(display "") (newline)
(display "In lizard's lattice, no choice is forced. The two libraries") (newline)
(display "live at parallel-but-distinct points; their composition") (newline)
(display "lives at their join.") (newline)
(newline)

; ============================================================
;  PART 5 — Nested quantification accumulates dimensions
; ============================================================

(display "============================================================") (newline)
(display "  PART 5 — Nested quantifications accumulate") (newline)
(display "============================================================") (newline)
(newline)

(display "When pi-fresh is nested, each level contributes a") (newline)
(display "dimension to the result:") (newline)
(newline)

(define nested1 (infer (context) (pi-fresh 'X (U 0) (U 0))))
(display "  pi-fresh once:   ") (display nested1) (newline)

(define nested2 (infer (context) (pi-fresh 'X (U 0) (pi-fresh 'Y (U 0) (U 0)))))
(display "  pi-fresh twice:  ") (display nested2) (newline)

(define nested3
  (infer (context)
         (pi-fresh 'X (U 0)
           (pi-fresh 'Y (U 0)
             (pi-fresh 'Z (U 0) (U 0))))))
(display "  pi-fresh thrice: ") (display nested3) (newline)
(newline)

(display "Each binding event added its own dim to the result set.") (newline)
(display "A tower would have collapsed all three to the same level.") (newline)
(newline)

; ============================================================
;  PART 6 — The dual side: couniverses
; ============================================================

(display "============================================================") (newline)
(display "  PART 6 — The dual lattice") (newline)
(display "============================================================") (newline)
(newline)

(display "Phase L.4 introduced a parallel COUNIVERSE lattice — meant") (newline)
(display "to house contexts/observations rather than types.") (newline)
(newline)

(define co_quant1 (infer (context) (co-pi-fresh 'A (Uco 0) (Uco 0))))
(display "  co-pi-fresh once:  ") (display co_quant1) (newline)
(define co_quant2 (infer (context) (co-pi-fresh 'B (Uco 0) (Uco 0))))
(display "  co-pi-fresh twice: ") (display co_quant2) (newline)
(newline)

(display "Same lattice structure, but in a SEPARATE sort.") (newline)
(display "Mixing across sorts stays unreduced:") (newline)
(display "  (Co-max <co_quant_pieces> (U-set 1 1000)) =") (newline)
(display "  ") (display (reduce (Co-max (Co-set 0 1) (U-set 0 1)))) (newline)
(display "  ↑ kept literal; sort mismatch") (newline)
(newline)

(display "Cross-lattice comparison returns 'unknown:") (newline)
(display "  (couniverse-leq? (Co-set 0) (U-set 0)) = ")
(display (couniverse-leq? (Co-set 0) (U-set 0))) (newline)
(newline)

(display "This is the thesis-distinctive piece: the universe-couniverse") (newline)
(display "distinction is built into the type structure, not derived") (newline)
(display "from external annotation. A judgment that asserts something") (newline)
(display "at universe level cannot accidentally substitute for one at") (newline)
(display "couniverse level.") (newline)
(newline)

; ============================================================
;  PART 7 — Combining with the cube configuration (M.2)
; ============================================================

(display "============================================================") (newline)
(display "  PART 7 — Lattice and cube are independent axes") (newline)
(display "============================================================") (newline)
(newline)

(display "The lattice work (L) and the cube configuration (M.2) are") (newline)
(display "independent dimensions of lizard's flexibility. You can") (newline)
(display "combine them.") (newline)
(newline)

(display "Example: configure as STLC (no dependencies), but keep") (newline)
(display "the lattice operations available:") (newline)
(logic-rule-disable 'term-depends-on-type)
(logic-rule-disable 'type-depends-on-term)
(logic-rule-disable 'type-depends-on-type)

(display "  Simple arrow (Pi 'x (U 0) (U 0)):") (newline)
(display "    ") (display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(display "  Lattice op (U-max (U-set 0 1) (U-set 0 2)):") (newline)
(display "    ") (display (reduce (U-max (U-set 0 1) (U-set 0 2)))) (newline)
(display "  Lattice ordering:") (newline)
(display "    (U-set 0 1) ≤ (U-set 0 2) → ")
(display (universe-leq? (U-set 0 1) (U-set 0 2))) (newline)
(newline)

(display "The lattice operations work even with STLC's Pi restrictions.") (newline)
(display "These are orthogonal concerns: WHAT Pi-types are allowed is a") (newline)
(display "different question from HOW universes are structured.") (newline)
(newline)

(logic-config-reset)

; ============================================================
;  PART 8 — What this is, and what it isn't
; ============================================================

(display "============================================================") (newline)
(display "  PART 8 — Honest scope of this demonstration") (newline)
(display "============================================================") (newline)
(newline)

(display "What this demonstration shows:") (newline)
(display "  ✓ Universes can be genuinely incomparable") (newline)
(display "  ✓ Quantification events are tracked structurally, not") (newline)
(display "    just as level bumps") (newline)
(display "  ✓ Independent developments compose via lattice join") (newline)
(display "    without privileging one over the other") (newline)
(display "  ✓ The universe/couniverse distinction is a sort distinction,") (newline)
(display "    not a derived annotation") (newline)
(display "  ✓ Lattice structure and configurable rule sets are") (newline)
(display "    independent axes of lizard's design") (newline)
(newline)

(display "What this demonstration does NOT show:") (newline)
(display "  ✗ Soundness of the resulting type theory") (newline)
(display "  ✗ Canonicity / decidability properties") (newline)
(display "  ✗ Any specific mathematical result that requires this") (newline)
(display "    machinery") (newline)
(display "  ✗ HIT computation rules (deferred to H.2+)") (newline)
(display "  ✗ Interaction with cubical comp on lattice universes") (newline)
(display "") (newline)
(display "Those are open research questions, not feature gaps —") (newline)
(display "they require theory work alongside implementation. This") (newline)
(display "demonstration shows the IMPLEMENTATION SUPPORTS the") (newline)
(display "machinery; it does not claim the theory is complete.") (newline)
(newline)

(display "============================================================") (newline)
(display "  Lattice demonstration complete") (newline)
(display "============================================================") (newline)
