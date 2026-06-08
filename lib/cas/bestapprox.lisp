; -*- lisp -*-
; lib/cas/bestapprox.lisp -- best rational approximation under a denominator bound.
;
; Given a rational x and a bound N, find the fraction p/q with 1 <= q <= N minimising
; |x - p/q|.  By the classical theory the optimum is always either a convergent of the
; continued fraction of x or a SEMICONVERGENT (an intermediate fraction).  We walk the
; convergents h_i/k_i, threading the last two; when the next convergent's denominator
; would exceed N we form the best semiconvergent (h_{i-2} + t h_{i-1}) / (k_{i-2} + t
; k_{i-1}) with t = floor((N - k_{i-2}) / k_{i-1}) the largest t keeping the denominator
; within bound, and return whichever of {last convergent, semiconvergent} sits closer to
; x.  The answer is certified against an exhaustive O(N) search that, for every q <= N,
; tests the nearest numerator round(x q); the certificate checks that the two errors are
; equal (ties between equally-good fractions are fine).  Builds on contfrac.lisp.

(import "cas/contfrac.lisp")

(define (ferr x h k) (abs (- x (/ h k))))                  ; |x - h/k|, exact rational
(define (closer x a b)                                     ; nearer of (h.k) pairs a, b
  (if (<= (ferr x (car a) (cdr a)) (ferr x (car b) (cdr b))) a b))

; ---------- convergent / semiconvergent search ----------
(define (best-approx x N) (ba-go x N (cf-rational x) 1 0 0 1))
(define (ba-go x N as hp kp hp2 kp2)                       ; hp/kp = h_{i-1}/k_{i-1}, hp2/kp2 = h_{i-2}/k_{i-2}
  (if (null? as)
      (cons hp kp)                                         ; CF exhausted: x = hp/kp exactly, and it fits
      (let ((a (car as)))
        (let ((hi (+ (* a hp) hp2)) (ki (+ (* a kp) kp2)))
          (if (<= ki N)
              (ba-go x N (cdr as) hi ki hp kp)
              (ba-semi x N hp kp hp2 kp2))))))
(define (ba-semi x N hp kp hp2 kp2)
  (let ((t (quotient (- N kp2) kp)))                       ; largest t with kp2 + t*kp <= N
    (closer x (cons hp kp) (cons (+ hp2 (* t hp)) (+ kp2 (* t kp))))))

; ---------- exhaustive certificate ----------
(define (rnd r) (floor (+ r (/ 1 2))))                     ; nearest integer to a rational
(define (brute x N) (brute-go x N 1 (cons (rnd x) 1)))
(define (brute-go x N q best)
  (if (> q N) best (brute-go x N (+ q 1) (closer x best (cons (rnd (* x q)) q)))))
(define (best-approx-ok? x N)
  (let ((a (best-approx x N)) (b (brute x N)))
    (= (ferr x (car a) (cdr a)) (ferr x (car b) (cdr b)))))

; ---------- display ----------
(define (approx->string a) (string-append (number->string (car a)) "/" (number->string (cdr a))))
(define (best-approx->string x N) (approx->string (best-approx x N)))
