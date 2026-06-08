; The exponential Risch differential equation with a degree-2 logarithmic derivative.  The solver of
; tower2exprde.lisp handles any deg(u') >= 1: with theta1 = e^x and theta2 = exp(e^{2x}/2) over
; K1 = Q(x)(theta1), the second monomial has u' = theta1^2, of degree two in theta1.  The recursion
; (b' + k u' b)_j = b_j' + j b_j + k (u' * b)_j solves the coefficients from the top down by division by
; k u'_P with P = 2, so a polynomial solution has degree deg(a) - 2.  The equation
; b' + theta1^2 b = theta1 + theta1^3 has the nonconstant solution b = theta1, and the integrator built on
; the solver certifies INT (theta1 + theta1^3) theta2 dx = theta1 theta2.  Each result is checked by the
; exponential derivation D2.
(import "cas/tower2exprde.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define EXP1 (list 'exp (list 0 1)))
(define t1   (list (list (rat-zero) (rat-one)) (list (rat-one))))            ; theta1 = e^x
(define t1sq (k1-mul t1 t1))                                                 ; u' = theta1^2  (deg 2)
(define a1   (k1-add t1 (k1-mul t1sq t1)))                                  ; theta1 + theta1^3
(display "Exponential RDE with deg(u') = 2  [theta1 = e^x, theta2 = exp(e^{2x}/2), u' = theta1^2]") (newline)
(define b1 (exp-rde a1 1 t1sq EXP1))
(must "RDE b' + theta1^2 b = theta1 + theta1^3 is solvable (deg u' = 2)" (exp-rde-solvable? a1 1 t1sq EXP1))
(must "its solution is b = theta1 (a NONCONSTANT element of K1)" (tr-equal? (tr-reduce b1) (tr-reduce t1)))
(must "RDE solution certified: b' + theta1^2 b = theta1 + theta1^3" (exp-rde-check a1 1 t1sq b1 EXP1))
(define P (list (k1-zero) a1))
(must "INT (theta1 + theta1^3) theta2 dx is integrable via the RDE solver" (t2e-rde-integrable? P t1sq EXP1))
(must "antiderivative is theta1 theta2" (h2-equal? (h2-norm (t2e-int-rde P t1sq EXP1)) (h2-norm (list (k1-zero) t1))))
(must "CERTIFIED: D2(antiderivative) = integrand over K1[theta2]" (t2e-int-rde-verify P t1sq EXP1))
(newline) (display "all deg-2 exponential-RDE checks passed.") (newline)
