(display "hello") (display " ") (display "world") (newline)
(display (* 6 7)) (newline)
(define (fact n) (if (= n 0) 1 (* n (fact (- n 1)))))
(display "20! = ") (display (fact 20)) (newline)
