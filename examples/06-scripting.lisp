; -*- lisp -*-
; Scripting with display & newline.
; Run with:  ./lizard < examples/06-scripting.lisp

(define (greet name)
  (display "Hello, ")
  (display name)
  (display "!")
  (newline))

(greet "world")
(greet "lizard")

(define (print-line)
  (display "----------------------------------------")
  (newline))

(define (fact n)
  (if (= n 0) 1 (* n (fact (- n 1)))))

(print-line)
(display "Factorials:")
(newline)
(print-line)

(define (loop i n)
  (if (> i n)
      'done
      (begin
        (display i) (display "! = ") (display (fact i)) (newline)
        (loop (+ i 1) n))))

(loop 1 10)

(print-line)
(display "20! is exactly: ")
(display (fact 20))
(newline)
(print-line)
