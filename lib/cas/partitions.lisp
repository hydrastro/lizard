; -*- lisp -*-
; lib/cas/partitions.lisp -- the integer partition function p(n).
;
; p(n) counts the ways to write n as an unordered sum of positive integers.  Euler's
; pentagonal number theorem gives the recurrence
;     p(n) = sum_{k>=1} (-1)^{k-1} ( p(n - k(3k-1)/2) + p(n - k(3k+1)/2) ),
; with p(0) = 1 and p(m) = 0 for m < 0, summing while the arguments stay non-negative.
; The generalized pentagonal numbers k(3k-1)/2 and k(3k+1)/2 make this an O(n^{1.5})
; recurrence, computed here with a memo table.
;
; The values are certified independently against the generating function
;     sum_n p(n) x^n = prod_{m>=1} 1 / (1 - x^m),
; whose coefficients up to x^N are obtained by multiplying the truncated series for each
; 1/(1-x^m).  The two computations must agree on p(0)..p(N), so a wrong value is caught.
; Self-contained over the bignums.

(define (pnth l i) (if (= i 0) (car l) (pnth (cdr l) (- i 1))))
(define (prange a b) (if (> a b) '() (cons a (prange (+ a 1) b))))
(define (zeros k) (if (= k 0) '() (cons 0 (zeros (- k 1)))))
(define (lst-get l i) (if (or (< i 0) (>= i (length l))) 0 (pnth l i)))

; ---------- pentagonal-number-theorem recurrence ----------
(define (gpent k) (/ (* k (- (* 3 k) 1)) 2))
(define (gpent2 k) (/ (* k (+ (* 3 k) 1)) 2))
(define (p-sum n prev k acc)
  (let ((g1 (gpent k)))
    (if (> g1 n) acc
      (let ((s (if (= (remainder k 2) 1) 1 -1)))
        (p-sum n prev (+ k 1) (+ acc (* s (+ (lst-get prev (- n g1)) (lst-get prev (- n (gpent2 k)))))))))))
(define (p-of n prev) (if (= n 0) 1 (p-sum n prev 1 0)))
(define (part-build n N acc) (if (> n N) acc (part-build (+ n 1) N (append acc (list (p-of n acc))))))
(define (part-table N) (part-build 0 N '()))
(define (partition n) (lst-get (part-table n) n))

; ---------- generating-function cross-check ----------
(define (one-series L) (cons 1 (zeros (- L 1))))
(define (miv s m L i acc) (if (>= i L) acc (miv s m L (+ i 1) (append acc (list (+ (lst-get s i) (if (>= (- i m) 0) (lst-get acc (- i m)) 0)))))))
(define (mul-inv-1mx s m L) (miv s m L 0 '()))
(define (gf-mult m N s) (if (> m N) s (gf-mult (+ m 1) N (mul-inv-1mx s m (+ N 1)))))
(define (gf-partitions N) (gf-mult 1 N (one-series (+ N 1))))

; ---------- certificate ----------
(define (partitions-ok? N) (equal? (part-table N) (gf-partitions N)))

; ---------- display ----------
(define (partition-list N) (part-table N))
