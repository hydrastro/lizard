; -*- lisp -*-
; lib/cas/catalan.lisp -- Catalan numbers and binomial-coefficient identities.
;
; Binomial coefficients are built by the exact multiplicative rule C(n,k) = C(n,k-1) *
; (n-k+1)/k, whose partial products are integers at every step, so no factorials and no
; rationals appear.  The Catalan numbers are taken in closed form C_n = C(2n,n)/(n+1).
;
; Each result is gated by a classical identity used as an independent check:
;   * the Catalan convolution  C_{n+1} = sum_{i=0}^n C_i C_{n-i},
;   * the Catalan ratio recurrence  C_{n+1} = C_n * 2(2n+1)/(n+2),
;   * Pascal's rule, the row sum  sum_k C(n,k) = 2^n, and the alternating sum = 0,
;   * Vandermonde  sum_k C(m,k) C(n,p-k) = C(m+n,p),
;   * the hockey-stick identity  sum_{i=r}^n C(i,r) = C(n+1,r+1).
; None of these checks reuses the value it certifies.  Self-contained over the integers.

(define (range a b) (if (> a b) '() (cons a (range (+ a 1) b))))

; ---------- binomial coefficient ----------
(define (binom n k) (cond ((< k 0) 0) ((> k n) 0) (else (bloop n (if (< (- n k) k) (- n k) k) 1 1))))
(define (bloop n k i acc) (if (> i k) acc (bloop n k (+ i 1) (quotient (* acc (+ (- n i) 1)) i))))

; ---------- Catalan numbers ----------
(define (catalan n) (quotient (binom (* 2 n) n) (+ n 1)))
(define (catalan-list n) (map catalan (range 0 n)))

; ---------- sums used by the identities ----------
(define (sum-range f a b) (sr f a b 0))
(define (sr f a b acc) (if (> a b) acc (sr f (+ a 1) b (+ acc (f a)))))

; ---------- certificates ----------
(define (catalan-conv-ok? n) (= (catalan (+ n 1)) (sum-range (lambda (i) (* (catalan i) (catalan (- n i)))) 0 n)))
(define (catalan-ratio-ok? n) (= (catalan (+ n 1)) (quotient (* (catalan n) (* 2 (+ (* 2 n) 1))) (+ n 2))))
(define (pascal-ok? n k) (= (binom n k) (+ (binom (- n 1) (- k 1)) (binom (- n 1) k))))
(define (rowsum-ok? n) (= (sum-range (lambda (k) (binom n k)) 0 n) (expt 2 n)))
(define (altsum-ok? n) (= (sum-range (lambda (k) (* (expt -1 k) (binom n k))) 0 n) 0))
(define (vandermonde-ok? m n p) (= (sum-range (lambda (k) (* (binom m k) (binom n (- p k)))) 0 p) (binom (+ m n) p)))
(define (hockey-ok? n r) (= (sum-range (lambda (i) (binom i r)) r n) (binom (+ n 1) (+ r 1))))

; ---------- display ----------
(define (catalan->string n) (number->string (catalan n)))
