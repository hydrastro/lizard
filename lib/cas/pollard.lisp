; -*- lisp -*-
; lib/cas/pollard.lisp -- Pollard's rho integer factorization.
;
; Trial division (in numbertheory) only reaches small factors; Pollard's rho finds factors
; far beyond it.  Iterating the pseudo-random map f(x) = x^2 + c (mod n) and watching the
; sequence with Floyd's two-pointer cycle detection, gcd(|x - y|, n) eventually exposes a
; nontrivial divisor of a composite n; if a run collapses (gcd = n) the constant c is
; bumped and it restarts.  Full factorization strips small primes, then recurses with rho,
; testing primality by the deterministic Miller-Rabin from numbertheory so each piece is
; split until prime.
;
; The result is checked the only way that matters: the returned factors must multiply back
; to n, and every one of them must be prime.  So 8051 = 83 * 97, the Project-Euler number
; 600851475143 = 71 * 839 * 1471 * 6857, and Fermat's 4294967297 = 641 * 6700417 all come
; out fully factored and certified.  Builds on numbertheory.lisp.

(import "cas/numbertheory.lisp")

(define (iabs n) (if (< n 0) (- 0 n) n))
(define (rstep x c n) (remainder (+ (* x x) c) n))

; ---------- one nontrivial factor by Pollard's rho (Floyd) ----------
(define (pollard-rho n) (if (= (remainder n 2) 0) 2 (try-c n 1)))
(define (try-c n c) (let ((d (rho-go n c 2 2 0))) (if (equal? d 'fail) (try-c n (+ c 1)) d)))
(define (rho-go n c x y steps)
  (if (> steps 200000) 'fail
    (let ((x2 (rstep x c n)) (y2 (rstep (rstep y c n) c n)))
      (let ((d (gcd (iabs (- x2 y2)) n)))
        (cond ((= d n) 'fail) ((> d 1) d) (else (rho-go n c x2 y2 (+ steps 1))))))))

; ---------- full factorization (sorted, with multiplicity) ----------
(define (pollard-factorize n) (isort (factz n)))
(define (factz n)
  (cond ((<= n 1) '())
        ((= (remainder n 2) 0) (cons 2 (factz (quotient n 2))))
        ((prime? n) (list n))
        (else (let ((d (pollard-rho n))) (append (factz d) (factz (quotient n d)))))))
(define (isort l) (if (null? l) '() (insrt (car l) (isort (cdr l)))))
(define (insrt x l) (cond ((null? l) (list x)) ((<= x (car l)) (cons x l)) (else (cons (car l) (insrt x (cdr l))))))

; ---------- run-length form ((p . e) ...) ----------
(define (rle-go cur cnt rest) (cond ((null? rest) (list (cons cur cnt))) ((= (car rest) cur) (rle-go cur (+ cnt 1) (cdr rest))) (else (cons (cons cur cnt) (rle-go (car rest) 1 (cdr rest))))))
(define (pollard-factor-rle n) (let ((l (pollard-factorize n))) (if (null? l) '() (rle-go (car l) 1 (cdr l)))))

; ---------- certificate ----------
(define (prod l) (if (null? l) 1 (* (car l) (prod (cdr l)))))
(define (all-prime? l) (cond ((null? l) #t) ((prime? (car l)) (all-prime? (cdr l))) (else #f)))
(define (pollard-ok? n) (let ((fs (pollard-factorize n))) (and (= (prod fs) n) (all-prime? fs))))

; ---------- display ----------
(define (rle->string fs) (if (null? fs) "1" (rs fs)))
(define (rs fs) (cond ((null? fs) "") ((null? (cdr fs)) (term (car fs))) (else (string-append (term (car fs)) " * " (rs (cdr fs))))))
(define (term f) (if (= (cdr f) 1) (number->string (car f)) (string-append (number->string (car f)) "^" (number->string (cdr f)))))
(define (pollard->string n) (rle->string (pollard-factor-rle n)))
