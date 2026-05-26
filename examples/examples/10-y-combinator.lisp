; -*- lisp -*-
; Y combinator — anonymous recursion. The fact/fib functions never name
; themselves; the combinator wires up the recursion at call time.
;
; The applicative-order (lazy) Y is also called the Z combinator:
;     Y f = (λx. f (λv. (x x) v)) (λx. f (λv. (x x) v))
;
; Each "almost-X" takes its future self as the first parameter.

(define Y
  (lambda (f)
    ((lambda (x) (f (lambda (v) ((x x) v))))
     (lambda (x) (f (lambda (v) ((x x) v)))))))

(define almost-fact
  (lambda (self)
    (lambda (n)
      (if (= n 0) 1 (* n (self (- n 1)))))))

(define fact (Y almost-fact))

(display "fact(10) via Y combinator = ") (display (fact 10)) (newline)
(display "fact(20) via Y combinator = ") (display (fact 20)) (newline)
(display "fact(50) via Y combinator = ") (display (fact 50)) (newline)
(newline)

; Fibonacci via Y combinator too.
(define almost-fib
  (lambda (self)
    (lambda (n)
      (if (< n 2) n
          (+ (self (- n 1)) (self (- n 2)))))))

(define fib (Y almost-fib))

(display "fib(15) via Y combinator = ") (display (fib 15)) (newline)
(display "fib(20) via Y combinator = ") (display (fib 20)) (newline)
(newline)

; Composing Y with an inline lambda — completely anonymous factorial.
(display "fact(7) anonymous = ")
(display
  ((Y (lambda (self)
        (lambda (n)
          (if (= n 0) 1 (* n (self (- n 1)))))))
   7))
(newline)
