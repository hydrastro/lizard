; -*- lisp -*-
; lib/cas/numbertheory.lisp — exact integer number theory over the bignums.
;
; Self-contained arithmetic on arbitrary-precision integers: extended Euclid (Bezout),
; modular exponentiation and inverse, deterministic Miller-Rabin primality (the witness
; set {2,3,...,37} is a proof for every n below 3.3e24, and catches Carmichael numbers
; that fool the Fermat test), integer factorization by trial division (which always
; finds a prime factor <= sqrt n of a composite, so it terminates and is exact), the
; Euler totient and divisor functions read off the factorization, the Chinese remainder
; construction, and multiplicative order.  Results are checkable: a factorization
; multiplies back to n with every part prime; a modular inverse times its argument is 1;
; a CRT solution satisfies each congruence.
;
; No modulo operator exists, so imod builds a nonnegative remainder by hand.  Top-level
; helpers only.

(define (iabs n) (if (< n 0) (- 0 n) n))
(define (imod a m) (let ((r (remainder a m))) (if (< r 0) (+ r m) r)))     ; nonneg residue, m>0

; ---------- gcd / extended gcd ----------
(define (igcd a b) (if (= b 0) (iabs a) (igcd b (remainder a b))))
(define (ilcm a b) (if (or (= a 0) (= b 0)) 0 (iabs (* (quotient a (igcd a b)) b))))
; iegcd a b -> (g s t) with s*a + t*b = g
(define (iegcd a b)
  (if (= b 0) (list a 1 0)
    (let ((res (iegcd b (remainder a b))))
      (list (car res) (car (cdr (cdr res)))
            (- (car (cdr res)) (* (quotient a b) (car (cdr (cdr res)))))))))

; ---------- modular exponentiation / inverse ----------
(define (mod-exp b e m) (me (imod b m) e m 1))
(define (me b e m acc) (if (= e 0) acc (me (imod (* b b) m) (quotient e 2) m (if (= (remainder e 2) 1) (imod (* acc b) m) acc))))
(define (mod-inverse a m) (let ((res (iegcd (imod a m) m))) (if (= (iabs (car res)) 1) (imod (* (car (cdr res)) (car res)) m) 'none)))

; ---------- deterministic Miller-Rabin ----------
(define (odd-part k) (if (= (remainder k 2) 1) k (odd-part (quotient k 2))))
(define (twos-count k) (if (= (remainder k 2) 1) 0 (+ 1 (twos-count (quotient k 2)))))
(define (mr-pass? a n d s)                                    ; #t if a does not witness compositeness
  (let ((x (mod-exp a d n))) (if (or (= x 1) (= x (- n 1))) #t (mr-sq x n (- s 1)))))
(define (mr-sq x n r) (cond ((= r 0) #f) (else (let ((y (imod (* x x) n))) (cond ((= y (- n 1)) #t) ((= y 1) #f) (else (mr-sq y n (- r 1))))))))
(define (mr-all ws n d s) (cond ((null? ws) #t) ((>= (car ws) n) (mr-all (cdr ws) n d s)) ((mr-pass? (car ws) n d s) (mr-all (cdr ws) n d s)) (else #f)))
(define (prime? n)
  (cond ((< n 2) #f) ((= n 2) #t) ((= (remainder n 2) 0) #f)
        (else (mr-all (list 2 3 5 7 11 13 17 19 23 29 31 37) n (odd-part (- n 1)) (twos-count (- n 1))))))

; ---------- factorization (trial division; exact and terminating) ----------
(define (trial-factor n k) (cond ((> (* k k) n) n) ((= (remainder n k) 0) k) (else (trial-factor n (if (= k 2) 3 (+ k 2))))))
(define (factor-flat n) (cond ((= n 1) '()) ((prime? n) (list n)) (else (let ((d (trial-factor n 2))) (cons d (factor-flat (quotient n d)))))))
(define (insert x s) (cond ((null? s) (list x)) ((<= x (car s)) (cons x s)) (else (cons (car s) (insert x (cdr s))))))
(define (isort lst) (if (null? lst) '() (insert (car lst) (isort (cdr lst)))))
(define (rle-go cur cnt rest) (cond ((null? rest) (list (cons cur cnt))) ((= (car rest) cur) (rle-go cur (+ cnt 1) (cdr rest))) (else (cons (cons cur cnt) (rle-go (car rest) 1 (cdr rest))))))
(define (factor-int n) (if (<= n 1) '() (let ((l (isort (factor-flat n)))) (rle-go (car l) 1 (cdr l)))))

; ---------- multiplicative functions from the factorization ----------
(define (totient n) (if (<= n 1) (if (= n 1) 1 0) (tot (factor-int n))))
(define (tot fs) (if (null? fs) 1 (* (* (expt (car (car fs)) (- (cdr (car fs)) 1)) (- (car (car fs)) 1)) (tot (cdr fs)))))
(define (num-divisors n) (if (<= n 1) (if (= n 1) 1 0) (nd (factor-int n))))
(define (nd fs) (if (null? fs) 1 (* (+ (cdr (car fs)) 1) (nd (cdr fs)))))
(define (sigma1 n) (if (<= n 1) (if (= n 1) 1 0) (sg (factor-int n))))
(define (sg fs) (if (null? fs) 1 (* (quotient (- (expt (car (car fs)) (+ (cdr (car fs)) 1)) 1) (- (car (car fs)) 1)) (sg (cdr fs)))))

; ---------- Chinese remainder (coprime moduli) ----------
(define (crt2 a1 m1 a2 m2)
  (let ((inv (mod-inverse m1 m2)))
    (if (equal? inv 'none) 'none (imod (+ a1 (* m1 (imod (* (- a2 a1) inv) m2))) (* m1 m2)))))

; ---------- multiplicative order of a mod m ----------
(define (order-mod a m) (if (and (> m 1) (= (igcd a m) 1)) (let ((b (imod a m))) (ord-inc b m b 1)) 'none))
(define (ord-inc b m cur k) (if (= cur 1) k (ord-inc b m (imod (* cur b) m) (+ k 1))))

; ---------- certificates ----------
(define (reconstruct-int fs) (if (null? fs) 1 (* (expt (car (car fs)) (cdr (car fs))) (reconstruct-int (cdr fs)))))
(define (all-prime? fs) (cond ((null? fs) #t) ((prime? (car (car fs))) (all-prime? (cdr fs))) (else #f)))
(define (factor-int-ok? n fs) (and (= (reconstruct-int fs) n) (all-prime? fs)))

; ---------- display ----------
(define (factor-int->string fs) (if (null? fs) "1" (fi-go fs "")))
(define (fi-go fs acc)
  (if (null? fs) acc
    (let ((piece (if (= (cdr (car fs)) 1) (number->string (car (car fs))) (string-append (number->string (car (car fs))) "^" (number->string (cdr (car fs)))))))
      (fi-go (cdr fs) (if (equal? acc "") piece (string-append acc " * " piece))))))
