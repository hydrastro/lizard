; 212-hensel-lifting.lisp -- Hensel lifting of a polynomial factorization.
;
; A coprime factorization f = g h modulo a prime p lifts uniquely to a factorization modulo
; every power p^k -- the bridge from factoring over a small field to factoring over the
; integers.  Each step corrects g and h using fixed mod-p Bezout cofactors so that the
; product matches f to one higher power of p while g stays monic.  Certified by
; reconstruction (the lifted product equals f mod p^k) and that the lift reduces to the
; original factor mod p.  `must` raises on failure.

(import "cas/hensel.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'hensel-check-failed)))

(display "Hensel lifting of factorizations") (newline) (newline)

(display "1. x^2 + 1 = (x+2)(x+3) mod 5, lifted to higher powers") (newline)
(display "    mod 25  : ") (display (lift->string (list 1 0 1) (list 2 1) (list 3 1) 5 2)) (newline)
(display "    mod 125 : ") (display (lift->string (list 1 0 1) (list 2 1) (list 3 1) 5 3)) (newline)
(must "Bezout identity s g + t h = 1 mod 5"  (bezout-ok? (list 2 1) (list 3 1) 5))
(must "reconstruction mod 25"   (hensel-ok? (list 1 0 1) (list 2 1) (list 3 1) 5 2))
(must "reconstruction mod 125"  (hensel-ok? (list 1 0 1) (list 2 1) (list 3 1) 5 3))
(must "reconstruction mod 5^6"  (hensel-ok? (list 1 0 1) (list 2 1) (list 3 1) 5 6))
(newline)

(display "2. a cubic split: x^3 - 1 = (x-1)(x^2+x+1) mod 5") (newline)
(display "    mod 25 : ") (display (lift->string (list -1 0 0 1) (list -1 1) (list 1 1 1) 5 2)) (newline)
(must "Bezout identity for the cubic split" (bezout-ok? (list -1 1) (list 1 1 1) 5))
(must "reconstruction mod 25"  (hensel-ok? (list -1 0 0 1) (list -1 1) (list 1 1 1) 5 2))
(must "reconstruction mod 625" (hensel-ok? (list -1 0 0 1) (list -1 1) (list 1 1 1) 5 4))
(newline)

(display "3. lifting a high power of 3: x^2 - 10 = (x+2)(x+1) mod 3") (newline)
(display "    mod 81 : ") (display (lift->string (list -10 0 1) (list -1 1) (list 1 1) 3 4)) (newline)
(must "reconstruction mod 81"  (hensel-ok? (list -10 0 1) (list -1 1) (list 1 1) 3 4))
(must "reconstruction mod 3^8" (hensel-ok? (list -10 0 1) (list -1 1) (list 1 1) 3 8))
(newline)

(display "all Hensel-lifting checks passed.") (newline)
