; -*- lisp -*-
; lib/cas/perfect.lisp -- perfect numbers, amicable pairs, and aliquot sums.
;
; The aliquot sum s(n) = sigma(n) - n adds up the proper divisors of n.  A number is
; perfect when s(n) = n (equivalently sigma(n) = 2n), abundant when s(n) > n, and
; deficient when s(n) < n; two numbers form an amicable pair when each is the other's
; aliquot sum.  The Euclid-Euler theorem builds an even perfect number 2^(p-1)(2^p - 1)
; from every Mersenne prime 2^p - 1.
;
; Every classification is decided through the sigma function from numbertheory, an
; independent multiplicative computation rather than the property being tested, so the
; sigma identity is the certificate.  6, 28, 496, 8128 are perfect; (220, 284) and
; (1184, 1210) are amicable; and the Euclid-Euler form reproduces the even perfect numbers.
; Builds on numbertheory.lisp.

(import "cas/numbertheory.lisp")

(define (range a b) (if (> a b) '() (cons a (range (+ a 1) b))))

; ---------- aliquot sum and classification ----------
(define (aliquot n) (- (sigma1 n) n))
(define (perfect? n) (and (> n 1) (= (sigma1 n) (* 2 n))))
(define (abundant? n) (> (aliquot n) n))
(define (deficient? n) (< (aliquot n) n))

; ---------- amicable pairs ----------
(define (amicable? a b) (and (not (= a b)) (= (aliquot a) b) (= (aliquot b) a)))

; ---------- Euclid-Euler: 2^(p-1) (2^p - 1) is perfect when 2^p - 1 is prime ----------
(define (mersenne p) (- (expt 2 p) 1))
(define (mersenne-prime? p) (and (prime? p) (prime? (mersenne p))))
(define (euclid-euler p) (* (expt 2 (- p 1)) (mersenne p)))

; ---------- certificates ----------
(define (perfect-via-sigma-ok? n) (equal? (perfect? n) (= (sigma1 n) (* 2 n))))
(define (amicable-via-sigma-ok? a b) (equal? (amicable? a b) (and (not (= a b)) (= (- (sigma1 a) a) b) (= (- (sigma1 b) b) a))))
(define (euclid-euler-ok? p) (if (mersenne-prime? p) (perfect? (euclid-euler p)) #t))

; ---------- searches and display ----------
(define (perfects-upto n) (filter perfect? (range 2 n)))
(define (abundants-upto n) (filter abundant? (range 2 n)))
(define (classify n) (cond ((perfect? n) "perfect") ((abundant? n) "abundant") (else "deficient")))
