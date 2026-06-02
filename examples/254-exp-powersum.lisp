; The complete exponential power-sum integrator at height two.  For theta2 = exp(u) with derivation
; D2 theta2 = u' theta2, integrate P = a_0 + Sum_{k>=1} a_k theta2^k.  The exponential derivation preserves
; the grading by theta2-degree, so the degrees do not mix and each is solved on its own: for k >= 1 the
; antiderivative coefficient solves the exponential Risch differential equation b_k' + k u' b_k = a_k, and
; the constant term a_0 in K1 = Q(x)(theta1) is integrated by the complete single-exponential integrator
; intexp.lisp -- a K1 element is exactly a ratio of polynomials in theta1 = e^x over Q(x).  With theta1 = e^x
; and theta2 = exp(e^x) (u' = theta1),
;     INT [ 1/(e^x+1) + (theta1+theta1^2) theta2 + 6 theta1 theta2^2 ] dx
;        = (x - log(e^x+1)) + theta1 theta2 + 3 theta2^2,
; with the theta2^1 and theta2^2 coefficients found by the RDE solver and the theta2^0 term, a genuine
; logarithmic integral, by the single-extension capstone.  Correctness factors along the grading: the
; power part is certified by D2, the constant term by the single-extension integrator's own check.
(import "cas/tower2expint.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define (nth l i) (if (= i 0) (car l) (nth (cdr l) (- i 1))))
(define EXP1 (list 'exp (list 0 1)))
(define t1  (list (list (rat-zero) (rat-one)) (list (rat-one))))            ; theta1 = e^x  (u' = theta1)
(define a0  (list (list (rat-one)) (list (rat-one) (rat-one))))             ; 1/(e^x+1)
(define a1  (k1-add t1 (k1-mul t1 t1)))                                     ; theta1 + theta1^2
(define a2  (k1-iscale 6 t1))                                              ; 6 theta1
(define P (list a0 a1 a2))
(display "INT [ 1/(e^x+1) + (theta1+theta1^2) theta2 + 6 theta1 theta2^2 ] dx   [theta2 = exp(e^x)]") (newline)
(must "the power sum is elementary" (t2e-ps-integrable? P t1 EXP1))
(define r (t2e-int-powersum P t1 EXP1))
(must "theta2^1 coefficient solves to theta1 (via the exponential RDE)" (tr-equal? (tr-reduce (nth (car (cdr r)) 1)) (tr-reduce t1)))
(must "theta2^2 coefficient solves to 3 (via the exponential RDE)" (tr-equal? (tr-reduce (nth (car (cdr r)) 2)) (tr-reduce (k1-iscale 3 (k1-one)))))
(must "the theta2^0 term 1/(e^x+1) integrates elementarily in K1 (= x - log(e^x+1))" (int-exp-rational-full-elementary? (car a0) (car (cdr a0))))
(must "FULL CERTIFICATE: power part by D2, constant term by the single-exponential integrator" (t2e-int-powersum-verify P t1 EXP1))
(newline) (display "all exponential power-sum checks passed.") (newline)
