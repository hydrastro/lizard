; -*- lisp -*-
; lib/cas/partition.lisp -- the integer partition function p(n) via the pentagonal number theorem.
;
; p(n) counts the ways to write n as a sum of positive integers, order disregarded.  Euler's
; pentagonal number theorem gives the fast recurrence
;   p(n) = sum_{k>=1} (-1)^{k-1} [ p(n - k(3k-1)/2) + p(n - k(3k+1)/2) ],
; with p(0) = 1 and p(m) = 0 for m < 0; the offsets are the generalized pentagonal numbers, so
; only O(sqrt n) terms contribute at each step and the whole table p(0..n) is built in roughly
; O(n sqrt n) exact-integer operations.  The result is cross-checked two independent ways: a
; direct partitions-into-parts-<=k counting recurrence (no pentagonal numbers involved) must
; agree for small n, and the classical value p(100) = 190569292 must come out exactly.
; Self-contained; no imports.

; ---------- list-as-table helpers ----------
(define (pt-nth pv i) (if (= i 0) (car pv) (pt-nth (cdr pv) (- i 1))))
(define (pt-len pv) (if (null? pv) 0 (+ 1 (pt-len (cdr pv)))))
(define (pt-get pv i) (if (or (< i 0) (>= i (pt-len pv))) 0 (pt-nth pv i)))

; ---------- pentagonal-number-theorem recurrence ----------
(define (pent-sign k) (if (= (remainder k 2) 1) 1 -1))          ; (-1)^{k-1}
(define (part-of pv m) (pent-sum pv m 1))
(define (pent-sum pv m k)
  (let ((g1 (/ (* k (- (* 3 k) 1)) 2)) (g2 (/ (* k (+ (* 3 k) 1)) 2)))
    (if (> g1 m) 0
        (+ (* (pent-sign k) (+ (pt-get pv (- m g1)) (if (> g2 m) 0 (pt-get pv (- m g2)))))
           (pent-sum pv m (+ k 1))))))
(define (part-table n) (pt-build (list 1) 1 n))                 ; p(0..n) as a list
(define (pt-build pv m n) (if (> m n) pv (pt-build (append pv (list (part-of pv m))) (+ m 1) n)))
(define (partition n) (pt-get (part-table n) n))

; ---------- independent check: partitions of n into parts <= k ----------
(define (part-count-le n k)
  (cond ((= n 0) 1) ((<= k 0) 0) ((< n 0) 0)
        (else (+ (part-count-le n (- k 1)) (part-count-le (- n k) k)))))
(define (partition-bf n) (part-count-le n n))

; ---------- certificates ----------
(define (part-agrees? n) (= (partition n) (partition-bf n)))    ; pentagonal vs direct count
(define (part-range-ok? n) (pr-go 0 n))
(define (pr-go i n) (if (> i n) #t (and (part-agrees? i) (pr-go (+ i 1) n))))
(define (part-known-ok?)                                        ; classical values
  (and (= (partition 5) 7) (= (partition 10) 42) (= (partition 100) 190569292)))
(define (part-info n) (string-append "p(" (number->string n) ") = " (number->string (partition n))))
