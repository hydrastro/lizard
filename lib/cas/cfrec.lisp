; -*- lisp -*-
; lib/cas/cfrec.lisp -- rational generating functions for C-finite linear recurrences.
;
; linrec gives closed forms only when the characteristic polynomial splits over Q (rational roots),
; so it cannot touch Fibonacci, Pell, or any recurrence with irrational/complex roots.  The GENERATING
; FUNCTION sidesteps that: a recurrence  a_n = c_1 a_{n-1} + ... + c_d a_{n-d}  with initial values
; a_0..a_{d-1} always has a RATIONAL generating function
;     G(x) = sum_n a_n x^n = N(x) / D(x),   D(x) = 1 - c_1 x - c_2 x^2 - ... - c_d x^d,
; with N a polynomial of degree < d fixed by the initial values:  N_k = a_k - sum_{j=1}^{k} c_j a_{k-j}.
; This holds with no condition on the roots, so it captures every C-finite sequence.  The result is
; CERTIFIED against the recurrence itself: the Taylor series of N/D (via the certified rational-function
; series of series.lisp) is checked to reproduce the terms a_0..a_{M-1} generated directly by the
; recurrence, to high order.  Builds on series.lisp and poly.lisp.

(import "cas/series.lisp")

(define (cfrec-nth lst i) (if (= i 0) (car lst) (cfrec-nth (cdr lst) (- i 1))))
(define (cfrec-take lst n) (if (= n 0) (quote ()) (cons (car lst) (cfrec-take (cdr lst) (- n 1)))))

; D(x) = 1 - c_1 x - ... - c_d x^d   (low->high)
(define (cfrec-D coeffs) (cons 1 (map (lambda (c) (- 0 c)) coeffs)))
; N_k = a_k - sum_{j=1}^k c_j a_{k-j}
(define (cfrec-conv coeffs inits k j acc)
  (if (> j k) acc (cfrec-conv coeffs inits k (+ j 1) (+ acc (* (cfrec-nth coeffs (- j 1)) (cfrec-nth inits (- k j)))))))
(define (cfrec-N-go coeffs inits k d)
  (if (>= k d) (quote ()) (cons (- (cfrec-nth inits k) (cfrec-conv coeffs inits k 1 0)) (cfrec-N-go coeffs inits (+ k 1) d))))
(define (cfrec-N coeffs inits d) (cfrec-N-go coeffs inits 0 d))
; the generating function as (list numerator-poly denominator-poly), both low->high
(define (cfrec-gf coeffs inits) (list (cfrec-N coeffs inits (length inits)) (cfrec-D coeffs)))

; direct term generation from the recurrence: a_n = sum_{j=1}^d c_j a_{n-j}
(define (cfrec-next coeffs acc n d j s)
  (if (> j d) s (cfrec-next coeffs acc n d (+ j 1) (+ s (* (cfrec-nth coeffs (- j 1)) (cfrec-nth acc (- n j)))))))
(define (cfrec-extend coeffs acc d n M)
  (if (>= n M) acc (cfrec-extend coeffs (append acc (list (cfrec-next coeffs acc n d 1 0))) d (+ n 1) M)))
(define (cfrec-terms coeffs inits M)
  (let ((d (length inits)))
    (if (<= M d) (cfrec-take inits M) (cfrec-extend coeffs (cfrec-take inits d) d d M))))

; CERTIFICATE: series of N/D reproduces the recurrence's own terms to order M
(define (cfrec-gf-verify coeffs inits M)
  (let ((gf (cfrec-gf coeffs inits)))
    (equal? (ser-trunc (ratfun->series (car gf) (car (cdr gf)) M) M) (cfrec-terms coeffs inits M))))
