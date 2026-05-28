; -*- lisp -*-
; ============================================================
;  EXAMPLE 116 — Track Q: HIT recursors that compute
; ============================================================

(import "cubical.lisp")
(import "hit-recursors.lisp")

(display "=== CIRCLE RECURSOR (β-rules) ===") (newline)
; Map S¹ → X by giving a point and a loop. We map into "colors":
; base ↦ 'red, loop ↦ the constant path at red.
(define s1-to-color (s1-rec 'red (path-const 'red)))
(display "  S¹-rec(red, refl):") (newline)
(display "    base ↦ ") (display (s1-to-color 'base)) (newline)
(display "    loop@i0 ↦ ") (display (s1-to-color (loop-pt (i0)))) (newline)
(display "    loop@i1 ↦ ") (display (s1-to-color (loop-pt (i1)))) (newline)
(newline)

(display "=== COMPUTATION RULES VERIFIED ===") (newline)
(display "  s1-rec-check(p, refl p):") (newline)
(for-each (lambda (r) (display "    ") (display r) (newline))
          (s1-rec-check 'p (path-const 'p)))
(display "  (base↦p and both loop endpoints ↦p — the β-rules hold)") (newline)
(newline)

(display "=== SUSPENSION RECURSOR ===") (newline)
; ΣBool has north, south, and two meridians (one per Bool).
(define susp-to-x
  (susp-rec 'N 'S (lambda (a) (path-const (list 'merid-of a)))))
(display "  north ↦ ") (display (susp-to-x 'north)) (newline)
(display "  south ↦ ") (display (susp-to-x 'south)) (newline)
(display "  merid(true)@i0 ↦ ")
(display (susp-to-x (merid-pt 'true (i0)))) (newline)
(newline)

(display "=== TORUS RECURSOR ===") (newline)
(define torus-map
  (torus-rec 'center (path-const 'horiz) (path-const 'vert)))
(display "  pt ↦ ") (display (torus-map 'pt)) (newline)
(display "  loop-a@i0 ↦ ") (display (torus-map (torus-loop-a (i0)))) (newline)
(display "  loop-b@i0 ↦ ") (display (torus-map (torus-loop-b (i0)))) (newline)
(newline)

(display "=== PROPOSITIONAL TRUNCATION ===") (newline)
; ‖A‖ collapses A to a mere proposition. trunc-rec maps inj a ↦ f a.
(define trunc-len (trunc-rec (lambda (s) (string-length s))))
(display "  trunc-rec(string-length) on (inj \"hello\"): ")
(display (trunc-len (trunc-inj "hello"))) (newline)
(display "  (squash makes all elements equal — A becomes a prop)") (newline)
(newline)

(display "=== WINDING NUMBER (π₁(S¹) = ℤ) ===") (newline)
; The fundamental group of the circle is the integers: going around
; the loop n times gives winding number n.
(display "  loop iterated 0 times: ") (display (winding-number 0)) (newline)
(display "  loop iterated 3 times: ") (display (winding-number 3)) (newline)
(display "  loop iterated 7 times: ") (display (winding-number 7)) (newline)
(display "  (the loop generates ℤ — the deepest theorem about S¹)") (newline)
(newline)

(display "=== Track Q milestone: HIT recursors ✓ ===") (newline)
