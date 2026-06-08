; -*- lisp -*-
; lib/cas/twosquares.lisp -- representing integers as a sum of two squares.
;
; Fermat's theorem: a prime p is a sum of two squares iff p = 2 or p = 1 (mod 4), and a
; positive integer n is a sum of two squares iff every prime p = 3 (mod 4) divides n to an
; even power.  For a prime p = 1 (mod 4) the representation is found by Cornacchia's
; method: take x with x^2 = -1 (mod p) (a square root of -1, which exists because p = 1
; mod 4) and run the Euclidean algorithm on (p, x) until the remainder drops to or below
; sqrt(p); the last pair (a, b) then satisfies a^2 + b^2 = p.
;
; A general n is handled multiplicatively through the Brahmagupta-Fibonacci identity
;     (a^2 + b^2)(c^2 + d^2) = (ac - bd)^2 + (ad + bc)^2,
; combining the representations of its prime-power factors (with 2 = 1^2 + 1^2 and a prime
; q = 3 mod 4 of even power 2k contributing (q^k, 0)).  Every representation is gated by
; the identity a^2 + b^2 = n, so a wrong answer is never returned.
;
; Builds on modsqrt.lisp (square roots mod p) and, through it, numbertheory.lisp.

(import "cas/modsqrt.lisp")

(define (isqrt n) (if (< n 2) n (isqrt-bs n 1 n)))
(define (isqrt-bs n lo hi)
  (if (>= lo hi) (if (<= (* hi hi) n) hi (- hi 1))
    (let ((mid (quotient (+ lo hi 1) 2)))
      (if (<= (* mid mid) n) (isqrt-bs n mid hi) (isqrt-bs n lo (- mid 1))))))

; ---------- a prime p = 1 mod 4 as a^2 + b^2 (Cornacchia) ----------
(define (ts-reduce L a b) (if (> a L) (ts-reduce L b (imod a b)) (cons a b)))
(define (two-squares-prime p) (ts-reduce (isqrt p) p (sqrt-mod (- p 1) p)))

; ---------- Brahmagupta-Fibonacci product of Gaussian norms ----------
(define (gmul u v) (cons (- (* (car u) (car v)) (* (cdr u) (cdr v))) (+ (* (car u) (cdr v)) (* (cdr u) (car v)))))
(define (gpow u e acc) (if (= e 0) acc (gpow u (- e 1) (gmul acc u))))

; ---------- representation of a prime power p^e ----------
(define (prime-power-rep p e)
  (cond ((= p 2) (gpow (cons 1 1) e (cons 1 0)))
        ((= (imod p 4) 1) (gpow (two-squares-prime p) e (cons 1 0)))
        (else (cons (expt p (quotient e 2)) 0))))         ; p = 3 mod 4, e even (checked elsewhere)

; ---------- existence and representation for general n ----------
(define (bad-factor? f) (and (= (imod (car f) 4) 3) (= (remainder (cdr f) 2) 1)))
(define (any-bad? fs) (cond ((null? fs) #f) ((bad-factor? (car fs)) #t) (else (any-bad? (cdr fs)))))
(define (sum-of-two-squares? n) (cond ((< n 0) #f) ((= n 0) #t) (else (not (any-bad? (factor-int n))))))
(define (combine-reps fs) (if (null? fs) (cons 1 0) (gmul (prime-power-rep (car (car fs)) (cdr (car fs))) (combine-reps (cdr fs)))))
(define (two-squares n)
  (cond ((= n 0) (cons 0 0))
        ((not (sum-of-two-squares? n)) 'none)
        (else (norm-pair (combine-reps (factor-int n))))))
(define (norm-pair u) (cons (iabs (car u)) (iabs (cdr u))))

; ---------- certificate ----------
(define (two-squares-ok? n)
  (let ((r (two-squares n)))
    (if (equal? r 'none) (not (sum-of-two-squares? n))
        (= (+ (* (car r) (car r)) (* (cdr r) (cdr r))) n))))

; ---------- display ----------
(define (two-squares->string n)
  (let ((r (two-squares n)))
    (if (equal? r 'none) "no representation"
        (string-append (number->string (car r)) "^2 + " (number->string (cdr r)) "^2"))))
