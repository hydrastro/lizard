; -*- lisp -*-
; Functional quicksort with a custom comparator.

(define (filter p xs)
  (cond ((null? xs) '())
        ((p (car xs)) (cons (car xs) (filter p (cdr xs))))
        (else         (filter p (cdr xs)))))

(define (append xs ys)
  (if (null? xs) ys (cons (car xs) (append (cdr xs) ys))))

; Generic quicksort parameterised by a `less?` predicate.
(define (sort less? xs)
  (if (null? xs)
      '()
      (let ((pivot (car xs)) (rest (cdr xs)))
        (append
          (sort less? (filter (lambda (x) (less? x pivot)) rest))
          (cons pivot
                (sort less?
                      (filter (lambda (x) (not (less? x pivot))) rest)))))))

(define data '(50 28 17 99 4 3 71 35 8 21 64 12 73 41 5 88 19 96 6 100))

(display "input:      ") (display data) (newline)
(display "ascending:  ") (display (sort < data)) (newline)
(display "descending: ") (display (sort > data)) (newline)
(display "by mod-10:  ") (display (sort (lambda (a b) (< (% a 10) (% b 10))) data)) (newline)

; Sort a list of cons-pairs by their car.
(define pairs '((3 c) (1 a) (4 d) (1 b) (5 e) (9 f) (2 g)))
(display "by first:   ")
(display (sort (lambda (p q) (< (car p) (car q))) pairs))
(newline)
