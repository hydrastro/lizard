; -*- lisp -*-
; lib/combinatorics.lisp — Permutations, combinations, and more.

(import "match.lisp")   ; for range, etc.

; ---- factorial / binomial (counts) ----
(define (comb-factorial n)
  (if (= n 0) 1 (* n (comb-factorial (- n 1)))))

; n choose k
(define (choose n k)
  (if (or (< k 0) (> k n)) 0
    (quotient (comb-factorial n)
              (* (comb-factorial k) (comb-factorial (- n k))))))

; number of permutations P(n,k)
(define (perm-count n k)
  (if (or (< k 0) (> k n)) 0
    (quotient (comb-factorial n) (comb-factorial (- n k)))))

; ---- all permutations of a list ----
(define (permutations lst)
  (if (null? lst) (list '())
    (apply-append
      (map (lambda (x)
             (map (lambda (p) (cons x p))
                  (permutations (remove-first x lst))))
           lst))))

(define (remove-first x lst)
  (if (null? lst) '()
    (if (equal? (car lst) x)
        (cdr lst)
        (cons (car lst) (remove-first x (cdr lst))))))

; flatten one level (concat a list of lists)
(define (apply-append lists)
  (if (null? lists) '()
    (append (car lists) (apply-append (cdr lists)))))

; ---- all combinations (subsets of size k) ----
(define (combinations lst k)
  (if (= k 0) (list '())
    (if (null? lst) '()
      (append
        ; combinations including the first element
        (map (lambda (c) (cons (car lst) c))
             (combinations (cdr lst) (- k 1)))
        ; combinations excluding the first element
        (combinations (cdr lst) k)))))

; ---- power set (all subsets) ----
(define (power-set lst)
  (if (null? lst) (list '())
    (let ((rest (power-set (cdr lst))))
      (append rest
              (map (lambda (s) (cons (car lst) s)) rest)))))

; ---- cartesian product of two lists ----
(define (cartesian a b)
  (apply-append
    (map (lambda (x)
           (map (lambda (y) (list x y)) b))
         a)))

; ---- integer partitions of n ----
; All ways to write n as a sum of positive integers (non-increasing).
(define (partitions n)
  (partitions-max n n))

(define (partitions-max n maxpart)
  (let ((k (if (> maxpart n) n maxpart)))
    (if (= n 0) (list '())
      (if (or (<= k 0) (< n 0)) '()
        (append
          ; use k once, then partition the rest with parts <= k
          (map (lambda (p) (cons k p))
               (partitions-max (- n k) k))
          ; don't use k; use parts <= k-1
          (partitions-max n (- k 1)))))))

; ---- count derangements (no element in its place) ----
(define (derangements n)
  (if (= n 0) 1
    (if (= n 1) 0
      (* (- n 1) (+ (derangements (- n 1)) (derangements (- n 2)))))))
