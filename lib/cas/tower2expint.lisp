; -*- lisp -*-
; lib/cas/tower2expint.lisp -- the complete exponential power-sum integrator at height two.
;
; For theta2 = exp(u) over K1 = Q(x)(theta1) with derivation D2 theta2 = u' theta2, integrate
; P = a_0 + Sum_{k>=1} a_k theta2^k.  The exponential derivation preserves the grading by theta2-degree
; (D2 of b theta2^k stays in degree k, and D2 of a K1 element stays in degree 0), so the degrees do not
; mix and each is solved independently: for k >= 1 the antiderivative coefficient b_k solves the
; exponential Risch differential equation b_k' + k u' b_k = a_k (tower2exprde.lisp), and the constant
; term a_0 in K1 is integrated by the complete single-exponential integrator (intexp.lisp), because a K1
; element is exactly a ratio of polynomials in theta1 = e^x over Q(x) -- the very representation intexp
; consumes.  The integrand is elementary precisely when every degree is, and correctness factors along
; the grading: the power part Sum b_k theta2^k is certified by differentiating with D2 and matching P
; above degree zero, while the constant term is certified by the single-extension integrator's own check.
; This composes the height-two RDE solver with the single-extension exponential capstone to integrate,
; e.g., INT [ 1/(e^x+1) + (theta1+theta1^2) theta2 + 6 theta1 theta2^2 ] dx
;       = (x - log(e^x+1)) + theta1 theta2 + 3 theta2^2.
; Builds on tower2exprde.lisp and intexp.lisp.

(import "cas/intexp.lisp")
(import "cas/tower2exprde.lisp")

(define (t2e-ps-go p uprime mono1 k)               ; solve RDE for k>=1; theta2^0 slot integrated separately
  (if (null? p) (quote ())
      (if (= k 0)
          (t2e-cons (k1-zero) (t2e-ps-go (cdr p) uprime mono1 1))
          (let ((bk (exp-rde-laurent (car p) k uprime mono1)))
            (if (equal? bk (quote nosolution)) (quote notexact)
                (t2e-cons bk (t2e-ps-go (cdr p) uprime mono1 (+ k 1))))))))
(define (t2e-ps-const p) (if (null? p) (k1-zero) (car p)))
(define (t2e-ps-zero-const p) (if (null? p) (quote ()) (cons (k1-zero) (cdr p))))
(define (t2e-int-powersum p uprime mono1)
  (let ((Q (t2e-ps-go p uprime mono1 0)))
    (if (equal? Q (quote notexact)) (quote notelementary)
        (let ((a0 (k1-normalize (t2e-ps-const p))))
          (if (int-exp-rational-full-elementary? (car a0) (car (cdr a0)))
              (list (quote ok) Q a0)
              (quote notelementary))))))
(define (t2e-ps-integrable? p uprime mono1) (if (equal? (t2e-int-powersum p uprime mono1) (quote notelementary)) #f #t))
(define (t2e-int-powersum-verify p uprime mono1)
  (let ((r (t2e-int-powersum p uprime mono1)))
    (if (equal? r (quote notelementary)) #f
        (let ((Q (car (cdr r))) (a0 (car (cdr (cdr r)))))
          (if (h2-equal? (h2-norm (t2e-deriv Q uprime mono1)) (h2-norm (t2e-ps-zero-const p)))
              (int-exp-rational-full-verify (car a0) (car (cdr a0)))
              #f)))))
