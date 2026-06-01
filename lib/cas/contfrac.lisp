; -*- lisp -*-
; lib/cas/contfrac.lisp -- continued fractions over the integers.
;
; The continued fraction of a rational p/q is finite, produced by the Euclidean
; algorithm: [a0; a1, a2, ...] with a_k = floor of the current value and the next value
; the reciprocal of the fractional part.  Its convergents h_k/k_k come from the standard
; recurrence h_k = a_k h_{k-1} + h_{k-2}, k_k = a_k k_{k-1} + k_{k-2}, and the last
; convergent reconstructs p/q exactly -- the certificate for the rational case.
;
; For a non-square d the continued fraction of sqrt(d) is eventually periodic (Lagrange),
; computed exactly with the integer (m, Q, a) recurrence; the period ends at the term
; equal to 2*a0.  This is certified through Pell's equation: if the period length is r,
; the convergent built from a0..a_{r-1} gives the FUNDAMENTAL solution of
; x^2 - d y^2 = (-1)^r, which we check exactly.  So the periodicity and the Pell identity
; together witness the expansion.
;
; Self-contained over the bignums (uses floor on exact rationals and an exact integer
; square root).  Top-level helpers only.

(define (lastelt l) (if (null? (cdr l)) (car l) (lastelt (cdr l))))

; ---------- exact integer square root: floor(sqrt n), n >= 0 ----------
(define (isqrt n) (if (< n 2) n (isqrt-bs n 1 n)))
(define (isqrt-bs n lo hi)                       ; largest x with x*x <= n, lo<=x<=hi
  (if (>= lo hi) (if (<= (* hi hi) n) hi (- hi 1))
    (let ((mid (quotient (+ lo hi 1) 2)))
      (if (<= (* mid mid) n) (isqrt-bs n mid hi) (isqrt-bs n lo (- mid 1))))))
(define (square? n) (let ((r (isqrt n))) (= (* r r) n)))

; ---------- continued fraction of a rational ----------
(define (cf-rational x) (cfr (numerator x) (denominator x)))
(define (cfr p q) (if (= q 0) '() (let ((a (floor (/ p q)))) (cons a (cfr q (- p (* a q)))))))

; ---------- convergents ----------
(define (cf-convergents cf) (conv cf 1 0 0 1))
(define (conv cf hp hpp kp kpp)
  (if (null? cf) '()
    (let ((h (+ (* (car cf) hp) hpp)) (k (+ (* (car cf) kp) kpp)))
      (cons (cons h k) (conv (cdr cf) h hp k kp)))))
(define (cf->rational cf) (let ((c (lastelt (cf-convergents cf)))) (/ (car c) (cdr c))))

; ---------- continued fraction of sqrt(d) ----------
; returns (list a0 period), period a list a1..ar with ar = 2 a0 (empty if d is a square)
(define (cf-sqrt d) (let ((a0 (isqrt d))) (if (square? d) (list a0 '()) (list a0 (sqrt-period d a0 0 1 a0)))))
(define (sqrt-period d a0 m dq a)
  (let ((m2 (- (* dq a) m)))
    (let ((dq2 (quotient (- d (* m2 m2)) dq)))
      (let ((a2 (floor (/ (+ a0 m2) dq2))))
        (cons a2 (if (= a2 (* 2 a0)) '() (sqrt-period d a0 m2 dq2 a2)))))))
(define (cf-sqrt-period-length d) (length (car (cdr (cf-sqrt d)))))

; ---------- certificates ----------
(define (cf-rational-ok? x) (= (cf->rational (cf-rational x)) x))
; fundamental Pell solution from one period of sqrt(d); returns (x . y) with x^2 - d y^2 = +-1
(define (pell-solution d)
  (let ((sd (cf-sqrt d)))
    (let ((cf (cons (car sd) (car (cdr sd)))))
      (let ((cs (cf-convergents cf))) (nth-elt cs (- (length cf) 2))))))
(define (nth-elt l i) (if (= i 0) (car l) (nth-elt (cdr l) (- i 1))))
(define (pell-value d) (let ((s (pell-solution d))) (- (* (car s) (car s)) (* d (cdr s) (cdr s)))))
(define (cf-sqrt-ok? d) (let ((v (pell-value d))) (or (= v 1) (= v -1))))

; ---------- display ----------
(define (cf->string cf) (if (null? cf) "[]" (string-append "[" (number->string (car cf)) (cf-tail (cdr cf)) "]")))
(define (cf-tail rest) (if (null? rest) "" (string-append "; " (commas rest))))
(define (commas l) (if (null? (cdr l)) (number->string (car l)) (string-append (number->string (car l)) ", " (commas (cdr l)))))
(define (cf-sqrt->string d)
  (let ((sd (cf-sqrt d)))
    (string-append "[" (number->string (car sd)) "; bar{" (commas-or (car (cdr sd))) "}]")))
(define (commas-or l) (if (null? l) "" (commas l)))
