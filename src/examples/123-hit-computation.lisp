; -*- lisp -*-
; ============================================================
;  EXAMPLE 123 — Track Q: HIT computation & canonicity
; ============================================================

(import "match.lisp")
(import "cubical.lisp")
(import "hit-recursors.lisp")
(import "hit-compute.lisp")

(display "=== π₁(S¹) ≅ ℤ : loops ARE integers ===") (newline)
(display "  loop⁰ (refl) = ") (display (loop-id)) (newline)
(display "  loop³ · loop²  = ") (display (loop-compose 3 2)) (newline)
(display "  (loop⁵)⁻¹      = ") (display (loop-invert 5)) (newline)
(display "  loop² · loop⁻² = ") (display (loop-compose 2 (loop-invert 2)))
(display "  (back to refl)") (newline)
(newline)

(display "=== GROUPOID LAWS (computed) ===") (newline)
(display "  identity (refl · loop³ = loop³): ")
(display (loop-law-identity 3)) (newline)
(display "  inverse (loop⁴ · loop⁻⁴ = refl): ")
(display (loop-law-inverse 4)) (newline)
(display "  associativity ((1·2)·3 = 1·(2·3)): ")
(display (loop-law-assoc 1 2 3)) (newline)
(newline)

(display "=== UNIVERSAL COVER (transport adds winding) ===") (newline)
(display "  transport along loop³ from 0: ")
(display (cover-transport 3 0)) (newline)
(display "  transport along loop⁻²from 10: ")
(display (cover-transport (loop-invert 2) 10)) (newline)
(display "  encode/decode loop⁷: ")
(display (decode-winding (encode-loop 7))) (newline)
(display "  (the cover's fiber over base IS ℤ)") (newline)
(newline)

(display "=== CIRCLE RECURSOR ON LOOP POWERS ===") (newline)
(display "  image of loop⁰ in ℤ: ") (display (s1-rec-loop-power 0)) (newline)
(display "  image of loop⁵ in ℤ: ") (display (s1-rec-loop-power 5)) (newline)
(display "  image of loop⁻³in ℤ: ") (display (s1-rec-loop-power -3)) (newline)
(newline)

(display "=== DEGREE OF SELF-MAPS [S¹,S¹] ≅ ℤ ===") (newline)
(display "  degree of identity: ") (display (degree-of-identity)) (newline)
(display "  degree of constant: ") (display (degree-of-constant)) (newline)
(display "  compose degree 2 ∘ degree 3: ")
(display (compose-degrees 2 3)) (newline)
(display "  (winding multiplies under composition)") (newline)
(newline)

(display "=== CANONICITY ===") (newline)
; Every closed term of a base type reduces to a canonical form.
(display "  (if true 0 1) canonical?: ")
(display (check-canonicity '(if true 0 1) canonical-nat?)) (newline)
(display "  S¹ form of 'base: ") (display (s1-canonical-form 'base)) (newline)
(display "  S¹ form of (loop-pt i0): ")
(display (s1-canonical-form (loop-pt (i0)))) (newline)
(display "  (closed terms always reach canonical constructors)") (newline)
(newline)

(display "=== Track Q COMPLETE: HIT computation + canonicity ✓ ===") (newline)
