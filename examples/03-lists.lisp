; -*- lisp -*-
; Lists are built from cons-pairs terminated by ().

; The four basic list operations.
(cons 1 2)                ; => (1 . 2)   improper pair
(cons 1 (cons 2 (cons 3 '())))   ; => (1 2 3)
(list 1 2 3)              ; => (1 2 3)   shorthand
(car '(a b c))            ; => a
(cdr '(a b c))            ; => (b c)

; Predicates.
(null? '())               ; => #t
(null? '(1))              ; => #f
(pair? '(1 2))            ; => #t
(pair? '())               ; => #f

; Quote gives you the data without evaluating it.
'(+ 1 2)                  ; => (+ 1 2)    -- a 3-element list
(+ 1 2)                   ; => 3

; Mapping a function over a list — written as a user-defined helper
; because lizard's standard library is small. Building map yourself
; is a Scheme rite of passage anyway.
(define (my-map f xs)
  (if (null? xs)
      '()
      (cons (f (car xs)) (my-map f (cdr xs)))))

(my-map (lambda (x) (* x x)) '(1 2 3 4 5))
                          ; => (1 4 9 16 25)

(define (my-length xs)
  (if (null? xs) 0 (+ 1 (my-length (cdr xs)))))

(my-length '(a b c d e))  ; => 5

(define (my-reverse xs)
  (if (null? xs)
      '()
      (let ((r (my-reverse (cdr xs))))
        ; append-one-at-end via cons + reverse — slow but clear
        (my-append r (list (car xs))))))

(define (my-append xs ys)
  (if (null? xs) ys (cons (car xs) (my-append (cdr xs) ys))))

(my-reverse '(1 2 3 4))   ; => (4 3 2 1)
(my-append '(a b) '(c d)) ; => (a b c d)
