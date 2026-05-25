; -*- lisp -*-
; Bignum showcase — arbitrarily large integers via GMP.

(define (fact n)
  (if (= n 0) 1 (* n (fact (- n 1)))))

(define (fib-iter a b k)
  (if (= k 0) a (fib-iter b (+ a b) (- k 1))))
(define (fib n) (fib-iter 0 1 n))

; Fast exponentiation, O(log e).
(define (pow b e)
  (cond ((= e 0) 1)
        ((= (% e 2) 0) (pow (* b b) (/ e 2)))
        (else (* b (pow (* b b) (/ (- e 1) 2))))))

; Binomial coefficient n choose k.
(define (choose n k) (/ (fact n) (* (fact k) (fact (- n k)))))

(display "100!          = ") (display (fact 100))  (newline) (newline)
(display "fib(500)      = ") (display (fib 500))   (newline) (newline)
(display "2^1024        = ") (display (pow 2 1024)) (newline) (newline)
(display "3^200         = ") (display (pow 3 200)) (newline) (newline)
(display "C(100,50)     = ") (display (choose 100 50)) (newline) (newline)
(display "Mersenne #31  = ") (display (- (pow 2 31) 1)) (newline)
(display "Mersenne #61  = ") (display (- (pow 2 61) 1)) (newline)
(display "Mersenne #127 = ") (display (- (pow 2 127) 1)) (newline)
