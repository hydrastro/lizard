; 176-pade.lisp — Pade approximants of a power series over Q.
;
; The [m/n] approximant P/Q (deg P <= m, deg Q <= n, Q(0)=1) matches the series S to
; order x^(m+n+1): S*Q - P = O(x^(m+n+1)).  The denominator solves an exact rational
; linear system; the numerator is a convolution.  Each approximant is verified by
; expanding S*Q - P as a series and checking it vanishes to order m+n+1 -- so a wrong
; approximant is never reported.  Pade both condenses a series into a rational function
; and recovers a rational function exactly from its expansion.  `must` raises on failure.

(import "cas/pade.lisp")
(import "cas/series.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'pade-check-failed)))
(define (cert ans c m n) (pade-ok? c (car ans) (car (cdr ans)) m n))

(display "Pade approximants over Q") (newline) (newline)

(display "1. exp(x): the classic diagonal approximant") (newline)
(define expc (list 1 1 (/ 1 2) (/ 1 6) (/ 1 24)))
(define e22 (pade-approx expc 2 2))
(display "    [2/2] = ") (display (pade->string e22 "x")) (newline)
(must "matches exp to order x^5"       (cert e22 expc 2 2))
(must "numerator  = 1 + x/2 + x^2/12"  (equal? (car e22) (list 1 (/ 1 2) (/ 1 12))))
(must "denominator = 1 - x/2 + x^2/12" (equal? (car (cdr e22)) (list 1 (/ -1 2) (/ 1 12))))
(define e11 (pade-approx (list 1 1 (/ 1 2)) 1 1))
(must "[1/1] exp matches to order x^3" (cert e11 (list 1 1 (/ 1 2)) 1 1))
(newline)

(display "2. recovering a rational function from its series") (newline)
(must "[1/1] of 1+x+x^2+... is 1/(1-x)"
      (equal? (pade-approx (list 1 1 1) 1 1) (list (list 1) (list 1 -1))))
(define ratser (ser-trunc (ser-mul (list 1 1) (ser-inverse (list 1 -1 1) 6) 6) 6))
(display "    series of (1+x)/(1-x+x^2) = ") (display ratser) (newline)
(define r22 (pade-approx ratser 2 2))
(display "    [2/2] recovers ") (display (pade->string r22 "x")) (newline)
(must "numerator recovered = 1 + x"        (equal? (car r22) (list 1 1)))
(must "denominator recovered = 1 - x + x^2" (equal? (car (cdr r22)) (list 1 -1 1)))
(must "and it is certified"                 (cert r22 ratser 2 2))
(newline)

(display "3. log(1+x)") (newline)
(define logc (list 0 1 (/ -1 2) (/ 1 3) (/ -1 4)))
(define l22 (pade-approx logc 2 2))
(display "    [2/2] = ") (display (pade->string l22 "x")) (newline)
(must "matches log(1+x) to order x^5" (cert l22 logc 2 2))
(newline)

(display "all Pade checks passed.") (newline)
