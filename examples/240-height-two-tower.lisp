; 240-height-two-tower.lisp -- first rung of the HEIGHT-TWO tower.
;
; Every integrator up to this point works over a single transcendental extension theta of Q(x).  A
; height-two tower stacks a second monomial theta2 on top of a height-one field K1 = Q(x)(theta1),
; so that D theta2 is an element of K1 rather than of Q(x).  This example exercises the differential
; structure at that level on two genuinely different towers and certifies the simplest height-two
; integrals -- the exact powers INT theta2^k (D theta2) dx = theta2^{k+1}/(k+1) -- by differentiating
; the proposed antiderivative with the two-level derivation (product rule on the K1 coefficients plus
; chain rule through theta2) and checking equality coefficient by coefficient.  `must` raises on
; failure.

(import "cas/tower2.lisp")
(define EXP1 (list 'exp (list 0 1)))
(define LOG  (list 'log))
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'tower2-check-failed)))

(display "Height-two tower: a second monomial whose derivative lives in a height-one field") (newline) (newline)

(display "1. theta1 = e^x, theta2 = log(e^x + 1); D theta2 = e^x/(e^x + 1) in Q(x, e^x)") (newline)
(define Dth2 (list (list (rat-zero) (rat-one)) (list (rat-one) (rat-one))))   ; theta1 / (1 + theta1)
(must "INT (D theta2) dx        = theta2       (k=0)" (t2-int-exact-power-verify 0 Dth2 EXP1))
(must "INT theta2 (D theta2) dx = theta2^2 / 2 (k=1)" (t2-int-exact-power-verify 1 Dth2 EXP1))
(must "INT theta2^2 (D theta2)  = theta2^3 / 3 (k=2)" (t2-int-exact-power-verify 2 Dth2 EXP1))
(must "INT theta2^4 (D theta2)  = theta2^5 / 5 (k=4)" (t2-int-exact-power-verify 4 Dth2 EXP1))
(newline)

(display "2. a different tower: theta1 = log x, theta2 = log(log x + 1); D theta2 = (1/x)/(log x + 1)") (newline)
(define Dth2b (list (list (rat-make (list 1) (list 0 1))) (list (rat-from-poly (list 1)) (rat-from-poly (list 1)))))
(must "INT theta2 (D theta2) dx = theta2^2 / 2 (k=1)" (t2-int-exact-power-verify 1 Dth2b LOG))
(must "INT theta2^3 (D theta2)  = theta2^4 / 4 (k=3)" (t2-int-exact-power-verify 3 Dth2b LOG))
(newline)

(display "3. the two-level derivation is exact: a wrong antiderivative is rejected") (newline)
(must "D(theta2^2/2) is NOT theta2^2 (D theta2)" (not (t2-equal? (t2-deriv (t2-monomial (t2-trrat (/ 1 2)) 2) Dth2 EXP1) (t2-monomial Dth2 2))))
(newline)

(display "all height-two-tower checks passed.") (newline)
