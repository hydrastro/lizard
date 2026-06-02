; The exponential Risch differential equation at height two.  With theta1 = e^x and theta2 = exp(e^x)
; (so u' = theta1), integrating a power sum Sum a_k theta2^k reduces in each degree to solving
; b' + k u' b = a_k for b in K1 = Q(x)(theta1).  The exact-power case handled only constant b; this
; solves the full equation for b a polynomial in theta1.  The headline RDE b' + theta1 b = theta1 + theta1^2
; has the NONCONSTANT solution b = theta1, and the integrator then produces antiderivatives whose
; theta2-coefficients are themselves nonconstant functions -- here INT [(theta1+theta1^2) theta2 +
; 6 theta1 theta2^2] dx = theta1 theta2 + 3 theta2^2.  Each result is certified by the exponential
; derivation D2.
(import "cas/tower2exprde.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define EXP1 (list 'exp (list 0 1)))
(define t1   (list (list (rat-zero) (rat-one)) (list (rat-one))))             ; theta1 = e^x  (= u')
(define t1sq (k1-mul t1 t1))                                                  ; theta1^2
(define t1p  (k1-add t1 t1sq))                                               ; theta1 + theta1^2
(display "Exponential Risch differential equation at height two  [theta2 = exp(e^x), u' = theta1]") (newline)
(define b (exp-rde t1p 1 t1 EXP1))
(must "RDE b' + theta1 b = theta1 + theta1^2 is solvable in K1" (exp-rde-solvable? t1p 1 t1 EXP1))
(must "its solution is b = theta1 (a NONCONSTANT element of K1)" (tr-equal? (tr-reduce b) (tr-reduce t1)))
(must "RDE solution certified: b' + theta1 b = theta1 + theta1^2" (exp-rde-check t1p 1 t1 b EXP1))
(define P (list (k1-zero) t1p (k1-iscale 6 t1)))                              ; (theta1+theta1^2) theta2 + 6 theta1 theta2^2
(define Q (t2e-int-rde P t1 EXP1))
(must "power sum is integrable via the RDE solver" (t2e-rde-integrable? P t1 EXP1))
(must "antiderivative is theta1 theta2 + 3 theta2^2 (nonconstant theta2 coefficient)"
      (h2-equal? (h2-norm Q) (h2-norm (list (k1-zero) t1 (k1-iscale 3 (k1-one))))))
(must "CERTIFIED: D2(antiderivative) = integrand over K1[theta2]" (t2e-int-rde-verify P t1 EXP1))
(newline) (display "all exponential-RDE height-two checks passed.") (newline)
