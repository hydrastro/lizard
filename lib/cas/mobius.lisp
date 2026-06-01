; -*- lisp -*-
; lib/cas/mobius.lisp -- the Mobius function and Dirichlet convolution.
;
; The Mobius function is mu(1) = 1, mu(n) = (-1)^k if n is a product of k distinct primes,
; and 0 if n has a squared factor.  Dirichlet convolution (f * g)(n) = sum_{d | n} f(d)
; g(n/d) turns arithmetic functions into a commutative ring whose identity is
; epsilon(n) = [n = 1], and Mobius inversion is the statement that the constant function 1
; and mu are convolution inverses: g = f * 1 iff f = g * mu.
;
; The module verifies the structural identities directly: mu * 1 = epsilon (so the divisor
; sum of mu is 1 at n = 1 and 0 otherwise), phi * 1 = N (the divisor sum of Euler's totient
; is n), and a full Mobius-inversion round trip recovering an arbitrary function from its
; summatory function.  These convolution identities are the certificates -- each is an
; independent check that does not reuse the value being tested.  Builds on numbertheory.lisp.

(import "cas/numbertheory.lisp")

(define (range a b) (if (> a b) '() (cons a (range (+ a 1) b))))
(define (divisors n) (filter (lambda (d) (= (remainder n d) 0)) (range 1 n)))

; ---------- Mobius function ----------
(define (any-square? fs) (cond ((null? fs) #f) ((> (cdr (car fs)) 1) #t) (else (any-square? (cdr fs)))))
(define (mobius n) (if (= n 1) 1 (let ((fs (factor-int n))) (if (any-square? fs) 0 (if (= (remainder (length fs) 2) 0) 1 -1)))))

; ---------- Mertens summatory function M(n) = sum_{k=1}^n mu(k) ----------
(define (mertens n) (msum 1 n 0))
(define (msum k n acc) (if (> k n) acc (msum (+ k 1) n (+ acc (mobius k)))))

; ---------- Dirichlet convolution (f * g)(n) ----------
(define (dirichlet f g n) (dsum f g n (divisors n)))
(define (dsum f g n ds) (if (null? ds) 0 (+ (* (f (car ds)) (g (quotient n (car ds)))) (dsum f g n (cdr ds)))))

; standard arithmetic functions as first-class values
(define (one n) 1)
(define (id n) n)
(define (epsilon n) (if (= n 1) 1 0))

; ---------- Mobius inversion: from a summatory function back to its summand ----------
; given f, its summatory function is (f * 1); inversion recovers f as ((f*1) * mu)
(define (summatory f) (lambda (n) (dirichlet f one n)))
(define (invert g) (lambda (n) (dirichlet g mobius n)))

; ---------- certificates (the convolution identities) ----------
(define (agree-on f g n) (cond ((> 1 n) #t) (else (ag f g 1 n))))
(define (ag f g k n) (cond ((> k n) #t) ((= (f k) (g k)) (ag f g (+ k 1) n)) (else #f)))
(define (mu-1-is-epsilon? n) (agree-on (lambda (k) (dirichlet mobius one k)) epsilon n))
(define (phi-1-is-N? n) (agree-on (lambda (k) (dirichlet totient one k)) id n))
(define (inversion-ok? f n) (agree-on (invert (summatory f)) f n))

; ---------- display ----------
(define (mobius-list n) (map mobius (range 1 n)))
(define (mertens-list n) (map mertens (range 1 n)))
