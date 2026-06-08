; 237-complete-exp-integration.lisp -- the COMPLETE integrator for a rational function of a single
; exponential: INT A/D dx with A, D polynomials in theta = e^x over Q(x).  The exponential capstone,
; mirror of example 236.
;
; A rational function of theta = e^x splits into a Laurent polynomial part (positive and negative
; powers, since theta is a unit) and a proper part coprime to theta.  Writing D = theta^j D0 and
; S theta^j + T D0 = 1 by Bezout, A/D = A T/theta^j + A S/D0; dividing A S = Q' D0 + R yields the
; proper part R/D0, and the entire Laurent part is (A T + Q' theta^j)/theta^j -- a single polynomial
; in theta shifted down by j.  The Laurent part is integrated by expoly.lisp and the proper part by
; expnrt.lisp; by linearity, certifying each part certifies the whole.  `must` raises on failure.

(import "cas/intexp.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'intexp-check-failed)))

(display "Complete integration of rational functions of e^x") (newline) (newline)

(display "1. a Laurent case: negative power plus a proper part") (newline)
(display "    INT 1/(e^x (e^x + 1)) dx = -e^(-x) - x + log(e^x + 1)") (newline)
(define A1 (rf-const (rat-one)))
(define D1 (list (rat-zero) (rat-one) (rat-one)))                 ; theta(theta+1) = theta^2 + theta
(define d1 (iexp-decompose A1 D1))
(display "    theta-content j = ") (display (iexp-j d1)) (display ",  proper denominator D0 = e^x + 1") (newline)
(must "certified" (int-exp-rational-full-verify A1 D1))
(newline)

(display "2. a mixed integrand: polynomial part e^(2x) plus a two-residue proper part") (newline)
(define D2 (list (rat-from-poly (list 2)) (rat-from-poly (list 3)) (rat-one)))   ; e^(2x)+3e^x+2
(define A2 (rfpoly-add (rfpoly-mul (rfpoly-monomial (rat-one) 2) D2) (list (rat-from-poly (list -6)) (rat-from-poly (list -5)))))
(must "certified"  (int-exp-rational-full-verify A2 D2))
(must "elementary" (int-exp-rational-full-elementary? A2 D2))
(newline)

(display "3. a pure proper part and a pure Laurent polynomial") (newline)
(must "INT -1/(e^x+1) dx certified"      (int-exp-rational-full-verify (list (rat-from-poly (list -1))) (list (rat-one) (rat-one))))
(must "INT (e^x + 2 e^(-x)) dx certified" (int-exp-rational-full-verify (list (rat-from-poly (list 2)) (rat-zero) (rat-one)) (list (rat-zero) (rat-one))))
(newline)

(display "4. non-elementarity of the proper part propagates") (newline)
(define D4 (list (rat-zero) (rat-from-poly (list 0 1)) (rat-one)))   ; theta(theta + x)
(must "INT 1/(e^x (e^x + x)) dx reported non-elementary" (not (int-exp-rational-full-elementary? (rf-const (rat-one)) D4)))
(newline)

(display "all complete-exp-integration checks passed.") (newline)
