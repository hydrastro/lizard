; -*- lisp -*-
; lib/cas/tower2.lisp -- first rung of the HEIGHT-TWO tower: a second monomial theta2 whose
; derivative lives in a height-one tower field, not merely in Q(x).
;
; Everything before this point integrates a single transcendental extension theta over Q(x).  A
; height-two tower stacks a second monomial theta2 on top of a height-one field K1 = Q(x)(theta1):
; for instance theta1 = e^x and theta2 = log(e^x + 1), so that D theta2 = e^x/(e^x + 1) is an element
; of K1 = Q(x, e^x) rather than of Q(x).  This module builds the differential structure at that level
; -- polynomials in theta2 with coefficients in K1 (tower-rationals over theta1), and the derivation
; that applies the product and chain rules through BOTH tower levels:
;   D( sum_k b_k theta2^k ) = sum_k (D b_k) theta2^k + sum_k k b_k (D theta2) theta2^{k-1},
; where D b_k differentiates the height-one coefficient and D theta2 is supplied as a K1 element.  It
; then certifies the simplest genuinely height-two integrals, the exact powers
;   INT theta2^k (D theta2) dx = theta2^{k+1} / (k+1),
; by differentiating the proposed antiderivative with the two-level derivation and checking equality
; coefficient by coefficient.  This is the first rung: it demonstrates that the chain rule composes
; correctly across two transcendental levels.  Full height-two integration -- Hermite reduction and
; Rothstein-Trager with the coefficient field itself a tower -- is the larger remaining lift.
; Builds on tower.lisp.

(import "cas/tower.lisp")

; ----- K1 = tower-rational (tr) field arithmetic missing from tower.lisp -----
(define (t2-trmul a b) (list (rfpoly-mul (car a) (car b)) (rfpoly-mul (car (cdr a)) (car (cdr b)))))
(define (t2-trinv a) (list (car (cdr a)) (car a)))
(define (t2-trdiv a b) (t2-trmul a (t2-trinv b)))
(define (t2-trone) (list (rf-const (rat-one)) (rf-const (rat-one))))
(define (t2-trrat q) (list (rf-const (rat-from-poly (list q))) (rf-const (rat-one))))   ; rational q as K1 element
(define (t2-triscale k a) (list (rfpoly-cscale (rat-from-poly (list k)) (car a)) (car (cdr a))))

; ----- height-two polynomials: lists of K1 coefficients (tr), low -> high in theta2 -----
(define (t2-zero) '())
(define (t2-add P Q) (cond ((null? P) Q) ((null? Q) P) (else (cons (tr-add (car P) (car Q)) (t2-add (cdr P) (cdr Q))))))
(define (t2-shift P k) (if (= k 0) P (cons (tr-zero) (t2-shift P (- k 1)))))
(define (t2-monomial e k) (t2-shift (list e) k))                       ; e theta2^k
(define (t2-equal? P Q) (cond ((null? P) (t2-allzero? Q)) ((null? Q) (t2-allzero? P)) (else (if (tr-equal? (car P) (car Q)) (t2-equal? (cdr P) (cdr Q)) #f))))
(define (t2-allzero? P) (cond ((null? P) #t) ((tr-equal? (car P) (tr-zero)) (t2-allzero? (cdr P)) ) (else #f)))

; coefficient-wise height-one derivative
(define (t2-dcoeffs P mono1) (if (null? P) '() (cons (tr-deriv (car P) mono1) (t2-dcoeffs (cdr P) mono1))))
; chain-rule part: sum_{k>=1} k b_k (D theta2) theta2^{k-1}
(define (t2-chain-go P Dth2 k) (if (null? P) '() (cons (t2-trmul (t2-triscale k (car P)) Dth2) (t2-chain-go (cdr P) Dth2 (+ k 1)))))
; the two-level derivation
(define (t2-deriv P Dth2 mono1) (t2-add (t2-dcoeffs P mono1) (if (null? P) '() (t2-chain-go (cdr P) Dth2 1))))

; ----- certified exact-power integration: INT theta2^k (D theta2) dx = theta2^{k+1}/(k+1) -----
(define (t2-int-exact-power k) (t2-monomial (t2-trrat (/ 1 (+ k 1))) (+ k 1)))
(define (t2-int-exact-power-verify k Dth2 mono1)
  (t2-equal? (t2-deriv (t2-int-exact-power k) Dth2 mono1) (t2-monomial Dth2 k)))
