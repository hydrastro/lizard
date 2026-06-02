; The exponential second monomial: theta2 = exp(e^x) over K1 = Q(x)(theta1), theta1 = e^x, so
; D theta2 = u' theta2 with u' = theta1 -- the derivation D2(Sum b_k theta2^k) = Sum (D b_k + k u' b_k)
; theta2^k keeps each monomial's degree.  This certifies the exponential derivation on a monomial and
; the exponential exact-power integration, each checked by differentiating the result with D2.
(import "cas/tower2exp.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define EXP1 (list 'exp (list 0 1)))
(define uprime (list (list (rat-zero) (rat-one)) (list (rat-one))))            ; u' = theta1 = e^x
(display "Exponential second monomial: theta2 = exp(e^x), so D theta2 = theta1 theta2") (newline)
(define th2sq (list (k1-zero) (k1-zero) (k1-one)))                             ; theta2^2
(must "D2(theta2^2) = 2 theta1 theta2^2"
      (h2-equal? (h2-norm (t2e-deriv th2sq uprime EXP1)) (h2-norm (list (k1-zero) (k1-zero) (k1-iscale 2 uprime)))))
(define P (list (k1-zero) (k1-iscale 5 uprime) (k1-iscale 6 uprime)))          ; 5 theta1 theta2 + 6 theta1 theta2^2
(must "integrand is an exact exponential power sum" (t2e-integrable? P uprime EXP1))
(define Q (t2e-int P uprime EXP1))
(must "antiderivative equals 5 theta2 + 3 theta2^2"
      (h2-equal? (h2-norm Q) (h2-norm (list (k1-zero) (k1-iscale 5 (k1-one)) (k1-iscale 3 (k1-one))))))
(must "CERTIFIED: D2(antiderivative) = integrand over K1[theta2]" (t2e-int-verify P uprime EXP1))
(newline) (display "all exponential-second-monomial checks passed.") (newline)
