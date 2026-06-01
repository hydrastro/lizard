; 224-hermite-reduction.lisp -- the rational part of integration (Hermite reduction).
;
; For a proper rational A/D, Hermite reduction produces INT A/D dx = (rational part) +
; INT (numerator)/(radical of D) dx, where the radical (squarefree part) has only simple roots
; so the remaining integral is purely logarithmic.  No factorization into irreducibles is
; needed: D is squarefree-factorized (Yun), split by the polynomial CRT, and each piece is
; reduced one power at a time by integration by parts using S V + T V' = 1.  Everything is exact
; rational arithmetic, certified by differentiation: d/dx(rational part) + residual/radical = A/D.
; `must` raises on failure.

(import "cas/hermite.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'hm-check-failed)))

(display "Hermite reduction") (newline) (newline)

(display "1. INT 1/(x^2 (x-1)) dx -- rational part plus a logarithmic remainder") (newline)
(define D1 (poly-mul (list 0 0 1) (list -1 1)))
(display "    D = x^2(x-1), squarefree structure ") (display (square-free (poly-monic D1))) (newline)
(display "    reduction (ratnum ratden resnum resden) = ") (display (hermite (list 1) D1)) (newline)
(display "    radical of D = ") (display (hermite-radical D1)) (newline)
(must "the reduction is certified by differentiation" (hermite-verify (list 1) D1))
(newline)

(display "2. INT 1/(x-1)^3 dx is purely rational (no logarithm)") (newline)
(define D2 (hm-pow (list -1 1) 3))
(display "    reduction = ") (display (hermite (list 1) D2)) (newline)
(must "1/(x-1)^3 certified"  (hermite-verify (list 1) D2))
(must "its residual numerator is zero (no log part)" (poly-zero? (car (cdr (cdr (hermite (list 1) D2))))))
(newline)

(display "3. higher multiplicities, including denominators irreducible over Q") (newline)
(must "x/(x^2+1)^2 certified"            (hermite-verify (list 0 1) (hm-pow (list 1 0 1) 2)))
(must "(x^2+1)/(x^2(x-1)^2) certified"   (hermite-verify (list 1 0 1) (poly-mul (hm-pow (list 0 1) 2) (hm-pow (list -1 1) 2))))
(must "5/(x^2-2)^3 certified"            (hermite-verify (list 5) (hm-pow (list -2 0 1) 3)))
(must "(2x^3+x+5)/(x^2+x+1)^2 certified" (hermite-verify (list 5 1 0 2) (hm-pow (list 1 1 1) 2)))
(newline)

(display "all Hermite-reduction checks passed.") (newline)
