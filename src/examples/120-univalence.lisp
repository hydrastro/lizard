; -*- lisp -*-
; ============================================================
;  EXAMPLE 120 — Track Q: Glue, univalence & composition
; ============================================================

(import "cubical.lisp")
(import "univalence.lisp")

(display "=== EQUIVALENCES ===") (newline)
(display "  identity equivalence forward on x: ")
(display (equiv-id-fwd 'A 'x)) (newline)
(display "  identity equivalence backward on y: ")
(display (equiv-id-bwd 'A 'y)) (newline)
(display "  (id-equiv computes to the identity function)") (newline)
(newline)

(display "=== GLUE TYPES ===") (newline)
; Glue A φ T e is T where φ holds, A where it doesn't.
(display "  Glue A F1 T e (φ true)  → ")
(display (glue-on-true 'A 'T 'e)) (newline)
(display "  Glue A F0 T e (φ false) → ")
(display (glue-on-false 'A 'T 'e)) (newline)
(display "  (Glue interpolates between a base and an equivalent type)") (newline)
(newline)

(display "=== UNIVALENCE: equiv → path ===") (newline)
; ua turns an equivalence into a path in the universe.
(display "  ua (id-equiv A) reduced: ")
(display (ua-id 'A)) (newline)
(display "  (univalence makes A ≃ B yield A = B)") (newline)
(newline)

(display "=== KAN COMPOSITION ===") (newline)
; hcomp with an empty face (F0) returns the base unchanged.
(display "  hcomp A F0 (<i>base) base → ")
(display (comp-const 'A 'base)) (newline)
; comp along a constant type line.
(display "  transp <i>A base → ")
(display (transp (path 'i 'A) 'base)) (newline)
(newline)

(display "=== THE BIG PICTURE ===") (newline)
(display "  Before univalence: equivalence (≃) and equality (=)") (newline)
(display "  were separate. Cubical TT makes them interconvertible:") (newline)
(display "    ua    : (A ≃ B) → (A = B)") (newline)
(display "    Glue  : attaches an equivalent type along a face") (newline)
(display "    comp  : makes every type Kan-fibrant (paths compose)") (newline)
(display "  Together: transport across an equivalence is computation.") (newline)
(newline)

(display "=== Track Q milestone: Glue + univalence ✓ ===") (newline)
