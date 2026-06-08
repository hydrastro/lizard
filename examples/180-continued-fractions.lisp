; 180-continued-fractions.lisp -- continued fractions over the integers.
;
; The continued fraction of a rational is finite -- the Euclidean algorithm -- and its
; last convergent reconstructs the rational exactly.  For a non-square d the continued
; fraction of sqrt(d) is eventually periodic, and the convergent built from one period
; gives the FUNDAMENTAL solution of Pell's equation x^2 - d y^2 = +-1, which we check
; exactly.  Everything is exact over the bignums, so even the famously enormous solution
; for sqrt(991) is produced and verified.  `must` raises on failure.

(import "cas/contfrac.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'cf-check-failed)))

(display "Continued fractions") (newline) (newline)

(display "1. rationals -- finite expansion, exact reconstruction") (newline)
(display "    415/93 = ") (display (cf->string (cf-rational (/ 415 93)))) (newline)
(display "    355/113 = ") (display (cf->string (cf-rational (/ 355 113)))) (newline)
(must "415/93 reconstructs"            (cf-rational-ok? (/ 415 93)))
(must "355/113 reconstructs"           (cf-rational-ok? (/ 355 113)))
(must "22/7 is [3; 7]"                 (equal? (cf-rational (/ 22 7)) (list 3 7)))
(must "last convergent of [4;2,6,7] is 415/93" (= (cf->rational (list 4 2 6 7)) (/ 415 93)))
(newline)

(display "2. sqrt(d) -- periodic expansion certified by Pell's equation") (newline)
(display "    sqrt(2)  = ") (display (cf-sqrt->string 2))  (display "  Pell ") (display (pell-solution 2))  (newline)
(display "    sqrt(7)  = ") (display (cf-sqrt->string 7))  (display "  Pell ") (display (pell-solution 7))  (newline)
(display "    sqrt(13) = ") (display (cf-sqrt->string 13)) (display "  Pell ") (display (pell-solution 13)) (newline)
(must "sqrt(2) Pell certified"         (cf-sqrt-ok? 2))
(must "sqrt(2) solution is 1,1 with value -1" (and (equal? (pell-solution 2) (cons 1 1)) (= (pell-value 2) -1)))
(must "sqrt(7) solution is 8,3 with value 1"  (and (equal? (pell-solution 7) (cons 8 3)) (= (pell-value 7) 1)))
(must "sqrt(3) Pell certified"         (cf-sqrt-ok? 3))
(must "sqrt(13) Pell certified"        (cf-sqrt-ok? 13))
(must "sqrt(23) Pell certified"        (cf-sqrt-ok? 23))
(newline)

(display "3. exact bignum Pell solutions") (newline)
(display "    sqrt(61) fundamental solution ") (display (pell-solution 61)) (newline)
(must "sqrt(61) certified"             (cf-sqrt-ok? 61))
(display "    sqrt(991) fundamental solution x = ") (display (car (pell-solution 991))) (newline)
(must "sqrt(991) certified (30-digit solution)" (cf-sqrt-ok? 991))
(must "sqrt(991) value is exactly 1"   (= (pell-value 991) 1))
(newline)

(display "all continued-fraction checks passed.") (newline)
