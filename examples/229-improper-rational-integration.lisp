; 229-improper-rational-integration.lisp -- complete rational-function integration for an
; ARBITRARY A/D, including improper fractions (deg A >= deg D) and constant denominators.
;
; rischrat.lisp's rat-integrate assumes a proper fraction; on an improper one Hermite reduction
; silently drops the polynomial part (e.g. for x^3/(x^2-1) it kept x/(x^2-1) but lost the
; quotient x).  rat-integrate-full divides A = Q D + Rem first, so INT A/D = INT Q + INT Rem/D,
; with INT Q the polynomial antiderivative and Rem/D proper.  Both defects this exercise guards
; against -- the dropped polynomial part and a division-by-zero on constant denominators -- were
; found by the randomized validator in example 230.  Every result is differentiation-certified.

(import "cas/ratfull.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'ratfull-check-failed)))

(display "Complete rational integration: arbitrary numerator degree") (newline) (newline)

(display "1. x^3/(x^2-1)  =  x^2/2 + (1/2) log(x^2-1)") (newline)
(define r (rat-integrate-full (list 0 0 0 1) (list -1 0 1)))
(display "    polynomial part = ") (display (rif-polypart r)) (display "  (= x^2/2)") (newline)
(must "x^3/(x^2-1) certified" (rat-integrate-full-verify (list 0 0 0 1) (list -1 0 1)))
(newline)

(display "2. further improper fractions") (newline)
(must "(5x^4+3)/(x^2-1) certified"   (rat-integrate-full-verify (list 3 0 0 0 5) (list -1 0 1)))
(must "x^5/(x^3-x) certified"        (rat-integrate-full-verify (list 0 0 0 0 0 1) (list 0 -1 0 1)))
(must "(2x^2+2)/(-x+2) certified"    (rat-integrate-full-verify (list 2 0 2) (list 2 -1)))
(newline)

(display "3. degenerate denominators") (newline)
(must "constant denom (x^2+2x+1)/(-2) certified"        (rat-integrate-full-verify (list 1 2 1) (list -2)))
(must "exact division (x^3-x)/(x-1) = x^2+x certified"  (rat-integrate-full-verify (list 0 -1 0 1) (list -1 1)))
(newline)

(display "4. proper fractions still handled exactly") (newline)
(must "1/(x^2(x-1)) certified"        (rat-integrate-full-verify (list 1) (poly-mul (list 0 0 1) (list -1 1))))
(must "(2x+1)/(x^2-1) certified"      (rat-integrate-full-verify (list 1 2) (list -1 0 1)))
(must "x^2/(x^2+1): rational part exact (residues algebraic)" (rat-integrate-full-rational-ok? (list 0 0 1) (list 1 0 1)))
(newline)

(display "all improper-rational-integration checks passed.") (newline)
