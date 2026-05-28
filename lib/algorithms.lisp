; -*- lisp -*-
; lib/algorithms.lisp — Classic algorithms in lizard.

; ============================================================
;  SORTING
; ============================================================

; Quicksort
(define (quicksort lst)
  (if (null? lst) '()
    (let ((pivot (car lst))
          (rest (cdr lst)))
      (append
        (quicksort (filter (lambda (x) (< x pivot)) rest))
        (cons pivot
              (quicksort (filter (lambda (x) (>= x pivot)) rest)))))))

; Merge sort
(define (merge a b)
  (if (null? a) b
    (if (null? b) a
      (if (< (car a) (car b))
          (cons (car a) (merge (cdr a) b))
          (cons (car b) (merge a (cdr b)))))))

(define (split-half lst)
  (let ((n (length lst)))
    (let ((mid (quotient n 2)))
      (list (take mid lst) (drop mid lst)))))

(define (mergesort lst)
  (if (null? lst) '()
    (if (null? (cdr lst)) lst
      (let ((halves (split-half lst)))
        (merge (mergesort (car halves))
               (mergesort (car (cdr halves))))))))

; ============================================================
;  SEARCHING
; ============================================================

; Linear search — returns index or -1
(define (linear-search lst target)
  (define (go lst i)
    (if (null? lst) -1
      (if (equal? (car lst) target) i
        (go (cdr lst) (+ i 1)))))
  (go lst 0))

; Binary search on sorted vector — returns index or -1
(define (binary-search vec target)
  (define (go lo hi)
    (if (> lo hi) -1
      (let ((mid (quotient (+ lo hi) 2)))
        (let ((v (pvec-ref vec mid)))
          (if (= v target) mid
            (if (< v target) (go (+ mid 1) hi)
              (go lo (- mid 1))))))))
  (go 0 (- (pvec-count vec) 1)))

; ============================================================
;  NUMBER THEORY
; ============================================================

; Factorial
(define (factorial n)
  (if (= n 0) 1 (* n (factorial (- n 1)))))

; Fibonacci (iterative, fast)
(define (fib n)
  (define (go a b k)
    (if (= k 0) a
      (go b (+ a b) (- k 1))))
  (go 0 1 n))

; GCD (Euclid)
(define (my-gcd a b)
  (if (= b 0) a (my-gcd b (remainder a b))))

; Is prime?
(define (prime? n)
  (define (check d)
    (if (> (* d d) n) #t
      (if (= 0 (remainder n d)) #f
        (check (+ d 1)))))
  (if (< n 2) #f (check 2)))

; Primes up to n (trial division)
(define (primes-up-to n)
  (filter prime? (range 2 (+ n 1))))

; Collatz sequence length
(define (collatz-length n)
  (define (go n steps)
    (if (= n 1) steps
      (if (even? n)
          (go (quotient n 2) (+ steps 1))
          (go (+ (* 3 n) 1) (+ steps 1)))))
  (go n 0))

; ============================================================
;  STRING ALGORITHMS
; ============================================================

; Is palindrome?
(define (palindrome? s)
  (equal? s (string-reverse s)))

; Count occurrences of char in string
(define (char-count s ch)
  (length (filter (lambda (c) (equal? c ch)) (string->list s))))
