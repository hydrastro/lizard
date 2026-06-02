; -*- lisp -*-
; lib/cas/tower2exp.lisp -- the exponential second monomial: derivation and exact-power integration.
;
; This begins the second branch of the height-two climb, in which the second monomial theta2 is an
; exponential rather than a primitive: theta2 = exp(u) with u in K1 = Q(x)(theta1), so its derivative
; D theta2 = u' theta2 is NOT an element of K1 -- it carries a factor of theta2 itself.  The two-level
; derivation therefore differs structurally from the primitive case: differentiating b_k theta2^k gives
; (D b_k) theta2^k + b_k k theta2^(k-1) (u' theta2) = (D b_k + k u' b_k) theta2^k, so each monomial keeps
; its degree instead of dropping by one.  Concretely D2(Sum b_k theta2^k) = Sum (D b_k + k u' b_k)
; theta2^k, where D b_k is the height-one derivation tr-deriv on the coefficient and u' is supplied as an
; element of K1.  The first integration case this exposes is the exponential exact-power case: a Laurent
; polynomial Sum a_k theta2^k (here with no theta2^0 term) is an exact derivative exactly when each a_k
; equals k u' times a constant of the tower, in which case the antiderivative is Sum (a_k/(k u')) theta2^k;
; the integrator forms b_k = a_k/(k u') over K1 and accepts when every b_k is a constant (its height-one
; derivative vanishes).  The general coefficient equation b_k' + k u' b_k = a_k is the exponential Risch
; differential equation, the next rung.  Every answer is certified by differentiating it with the
; exponential derivation D2 and checking equality with the integrand over K1[theta2].  With theta1 = e^x
; and theta2 = exp(e^x), so u' = theta1, the module certifies D2(theta2^2) = 2 theta1 theta2^2 and
; INT (5 theta1 theta2 + 6 theta1 theta2^2) dx = 5 theta2 + 3 theta2^2.  Builds on tower2int.lisp.

(import "cas/tower2int.lisp")

; ----- the exponential two-level derivation (D theta2 = uprime * theta2) -----
(define (t2e-deriv-go p uprime mono1 k)
  (if (null? p) '()
      (cons (k1-add (tr-deriv (car p) mono1) (k1-iscale k (k1-mul uprime (car p))))
            (t2e-deriv-go (cdr p) uprime mono1 (+ k 1)))))
(define (t2e-deriv p uprime mono1) (t2e-deriv-go p uprime mono1 0))

; fraction of two h2polys (height-two exponential), derivative by the quotient rule
(define (t2e-tr-deriv N D uprime mono1)
  (list (h2-sub (h2-mul (t2e-deriv N uprime mono1) D) (h2-mul N (t2e-deriv D uprime mono1))) (h2-mul D D)))

; ----- the exponential exact-power integration case -----
(define (t2e-int-go p uprime mono1 k)
  (if (null? p) '()
      (if (= k 0)
          (if (k1-zero? (car p)) (t2e-cons (k1-zero) (t2e-int-go (cdr p) uprime mono1 1)) 'notexact)
          (let ((bk (k1-div (car p) (k1-iscale k uprime))))
            (if (k1-constant? bk mono1) (t2e-cons bk (t2e-int-go (cdr p) uprime mono1 (+ k 1))) 'notexact)))))
(define (t2e-cons x rest) (if (equal? rest 'notexact) 'notexact (cons x rest)))
(define (t2e-int p uprime mono1) (t2e-int-go p uprime mono1 0))
(define (t2e-int-verify p uprime mono1)
  (let ((q (t2e-int p uprime mono1)))
    (if (equal? q 'notexact) #f (h2-equal? (h2-norm (t2e-deriv q uprime mono1)) (h2-norm p)))))
(define (t2e-integrable? p uprime mono1) (if (equal? (t2e-int p uprime mono1) 'notexact) #f #t))
