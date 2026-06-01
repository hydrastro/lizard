; -*- lisp -*-
; lib/cas/cyclotomic.lisp -- cyclotomic polynomials over Q.
;
; The n-th cyclotomic polynomial is defined by the product identity
;     prod_{d | n} Phi_d(x) = x^n - 1,
; which gives the exact recurrence
;     Phi_n(x) = (x^n - 1) / prod_{d | n, d < n} Phi_d(x),
; an exact polynomial division (exact quotient, zero remainder).  Phi_n is monic of
; degree phi(n), the Euler totient, and is irreducible over Q.
;
; To compute each Phi_d only once, the divisors of n are processed in increasing order
; while a table of (d . Phi_d) is threaded through; every proper divisor of a divisor d
; of n is itself a divisor of n, so it is already in the table when Phi_d is built.
;
; Each result is checked two ways: the product of Phi_d over all divisors of n rebuilds
; x^n - 1 exactly, and deg Phi_n equals phi(n) counted independently.  So Phi_1 = x - 1,
; Phi_4 = x^2 + 1, Phi_6 = x^2 - x + 1, Phi_12 = x^4 - x^2 + 1, and -- famously -- Phi_105
; is the smallest cyclotomic polynomial with a coefficient outside {-1, 0, 1}: it has
; coefficients equal to -2.
;
; Self-contained over poly.lisp.

(import "cas/poly.lisp")

(define (range a b) (if (> a b) '() (cons a (range (+ a 1) b))))
(define (zeros k) (if (= k 0) '() (cons 0 (zeros (- k 1)))))
(define (igcd a b) (if (= b 0) a (igcd b (remainder a b))))
(define (divisors n) (filter (lambda (d) (= (remainder n d) 0)) (range 1 n)))
(define (proper-divisors n) (filter (lambda (d) (and (= (remainder n d) 0) (< d n))) (range 1 n)))
(define (totient-count n) (length (filter (lambda (k) (= (igcd k n) 1)) (range 1 n))))

(define (xn-1 n) (cons -1 (append (zeros (- n 1)) (list 1))))     ; x^n - 1

; ---------- memoized table of (d . Phi_d) for all divisors of n ----------
(define (lookup d memo) (cond ((null? memo) (list 1)) ((= (car (car memo)) d) (cdr (car memo))) (else (lookup d (cdr memo)))))
(define (prod-memo ds memo) (if (null? ds) (list 1) (poly-mul (lookup (car ds) memo) (prod-memo (cdr ds) memo))))
(define (phi-from d memo) (if (= d 1) (list -1 1) (car (poly-divmod (xn-1 d) (prod-memo (proper-divisors d) memo)))))
(define (ct-build ds memo) (if (null? ds) memo (ct-build (cdr ds) (append memo (list (cons (car ds) (phi-from (car ds) memo)))))))
(define (cyclo-table n) (ct-build (divisors n) '()))

; ---------- public: single query ----------
(define (cyclotomic-tab n tab) (lookup n tab))
(define (cyclotomic n) (lookup n (cyclo-table n)))

; ---------- certificates ----------
(define (cyclotomic-ok-tab? n tab) (and (equal? (poly-norm (prod-memo (divisors n) tab)) (poly-norm (xn-1 n))) (= (poly-deg (lookup n tab)) (totient-count n))))
(define (cyclotomic-ok? n) (cyclotomic-ok-tab? n (cyclo-table n)))

; ---------- coefficient queries ----------
(define (mn l) (if (null? (cdr l)) (car l) (min (car l) (mn (cdr l)))))
(define (mx l) (if (null? (cdr l)) (car l) (max (car l) (mx (cdr l)))))
(define (min-coeff-tab n tab) (mn (poly-norm (lookup n tab))))
(define (max-coeff-tab n tab) (mx (poly-norm (lookup n tab))))

; ---------- display ----------
(define (cyclotomic->string n) (poly->string (cyclotomic n) "x"))
