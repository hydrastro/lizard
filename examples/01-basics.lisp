; -*- lisp -*-
; Basics: numbers, definition, lambda, application.

; Arithmetic is variadic.
(+ 1 2 3 4 5)            ; => 15
(* 2 3 4)                ; => 24
(- 100 1 1 1)            ; => 97
(/ 100 5 2)              ; => 10
(- 7)                    ; => -7        (unary negation)
(% 17 5)                 ; => 2         (modulo)
(^ 2 10)                 ; => 1024      (power)

; Comparisons return #t or #f.
(= 1 1 1)                ; => #t
(< 1 2 3)                ; => #t
(< 3 2 1)                ; => #f
(>= 5 5 4)               ; => #t

; define names a value.
(define pi 314159)
(define answer 42)

; Lambdas are first-class.
(define square (lambda (x) (* x x)))
(square 12)              ; => 144

; (define (name args...) body) is sugar for (define name (lambda (args...) body))
(define (cube x) (* x x x))
(cube 4)                 ; => 64

; set! mutates an existing binding (it does NOT create a new one).
(define n 1)
(set! n (+ n 1))
n                         ; => 2

; let introduces local bindings.
(let ((a 3) (b 4))
  (+ (* a a) (* b b)))   ; => 25
