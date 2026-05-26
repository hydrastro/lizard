; -*- lisp -*-
; N-queens — find every safe placement of N queens on an N×N board.
;
; A "solution" is a list of column positions, one per row. Two
; queens attack iff they share a column or a diagonal.

(define (filter p xs)
  (cond ((null? xs) '())
        ((p (car xs)) (cons (car xs) (filter p (cdr xs))))
        (else         (filter p (cdr xs)))))

(define (length xs) (if (null? xs) 0 (+ 1 (length (cdr xs)))))

(define (range lo hi)
  (if (>= lo hi) '() (cons lo (range (+ lo 1) hi))))

(define (map f xs)
  (if (null? xs) '() (cons (f (car xs)) (map f (cdr xs)))))

(define (concat xss)
  (if (null? xss)
      '()
      (let ((head (car xss)) (rest (cdr xss)))
        (if (null? head)
            (concat rest)
            (cons (car head) (concat (cons (cdr head) rest)))))))

(define (flatmap f xs) (concat (map f xs)))

(define (abs n) (if (< n 0) (- n) n))

; Does placing a queen at column `col` in the next row conflict with
; any queen in `placed`? `placed` is a column list newest-first.
(define (safe? col placed)
  (define (loop ps dist)
    (cond ((null? ps) #t)
          ((= (car ps) col) #f)
          ((= (abs (- (car ps) col)) dist) #f)
          (else (loop (cdr ps) (+ dist 1)))))
  (loop placed 1))

; Build every safe solution row by row.
(define (queens n)
  (define (place rows-left)
    (if (= rows-left 0)
        (list '())
        (flatmap
          (lambda (partial)
            (map (lambda (col) (cons col partial))
                 (filter (lambda (c) (safe? c partial))
                         (range 1 (+ n 1)))))
          (place (- rows-left 1)))))
  (place n))

(define s4 (queens 4))
(display "4-queens: ") (display (length s4)) (display " solutions") (newline)
(display s4) (newline) (newline)

(define s6 (queens 6))
(display "6-queens: ") (display (length s6)) (display " solutions") (newline)
(newline)

(define s8 (queens 8))
(display "8-queens: ") (display (length s8)) (display " solutions") (newline)
(display "first solution: ") (display (car s8)) (newline)
(display "last  solution: ")
; Helper to fetch the last element of a list.
(define (last xs) (if (null? (cdr xs)) (car xs) (last (cdr xs))))
(display (last s8)) (newline)
