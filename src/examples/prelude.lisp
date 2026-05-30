; -*- lisp -*-
; lizard prelude — common list, math, and predicate helpers.
;
; Load at the top of a script with:
;   (load "examples/prelude.lisp")
;
; Or concatenate at startup:
;   cat prelude.lisp your-script.lisp | ./lizard

; ---- predicates -----------------------------------------------------

(define (atom? x) (not (pair? x)))
(define (zero? n) (= n 0))
(define (positive? n) (> n 0))
(define (negative? n) (< n 0))
(define (odd?  n) (= (% n 2) 1))
(define (even? n) (= (% n 2) 0))

; ---- list combinators ----------------------------------------------

(define (length xs)
  (if (null? xs) 0 (+ 1 (length (cdr xs)))))

(define (reverse xs)
  (define (loop xs acc)
    (if (null? xs) acc (loop (cdr xs) (cons (car xs) acc))))
  (loop xs '()))

(define (append xs ys)
  (if (null? xs) ys (cons (car xs) (append (cdr xs) ys))))

(define (map f xs)
  (if (null? xs)
      '()
      (cons (f (car xs)) (map f (cdr xs)))))

(define (filter p xs)
  (cond ((null? xs) '())
        ((p (car xs)) (cons (car xs) (filter p (cdr xs))))
        (else         (filter p (cdr xs)))))

(define (fold-left f acc xs)
  (if (null? xs) acc (fold-left f (f acc (car xs)) (cdr xs))))

(define (fold-right f acc xs)
  (if (null? xs)
      acc
      (f (car xs) (fold-right f acc (cdr xs)))))

(define (for-each f xs)
  (if (null? xs)
      'done
      (begin (f (car xs)) (for-each f (cdr xs)))))

(define (nth n xs)
  (if (= n 0) (car xs) (nth (- n 1) (cdr xs))))

(define (last xs)
  (if (null? (cdr xs)) (car xs) (last (cdr xs))))

(define (member? x xs)
  (cond ((null? xs) #f)
        ((= x (car xs)) #t)
        (else (member? x (cdr xs)))))

; ---- ranges --------------------------------------------------------

(define (range lo hi)
  (if (>= lo hi) '() (cons lo (range (+ lo 1) hi))))

; ---- numeric helpers -----------------------------------------------

(define (abs n) (if (negative? n) (- n) n))

(define (min a b) (if (< a b) a b))
(define (max a b) (if (> a b) a b))

(define (sum xs)     (fold-left + 0 xs))
(define (product xs) (fold-left * 1 xs))

(define (fact n)
  (if (= n 0) 1 (* n (fact (- n 1)))))
