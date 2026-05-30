; -*- lisp -*-
; Recursion: factorials, Fibonacci, Ackermann, GCD.
;
; Numbers in lizard are arbitrary-precision (GMP under the hood),
; so factorials and powers can grow as large as you want.

(define (fact n)
  (if (= n 0)
      1
      (* n (fact (- n 1)))))

(fact 5)                  ; => 120
(fact 25)                 ; => 15511210043330985984000000
(fact 100)                ; => a 158-digit bignum

(define (fib n)
  (if (< n 2)
      n
      (+ (fib (- n 1)) (fib (- n 2)))))

(fib 10)                  ; => 55
(fib 25)                  ; => 75025

; GCD via Euclid.
(define (gcd a b)
  (if (= b 0) a (gcd b (% a b))))

(gcd 1071 462)            ; => 21

; Ackermann — small inputs only; it grows like nothing else.
(define (ack m n)
  (cond ((= m 0) (+ n 1))
        ((= n 0) (ack (- m 1) 1))
        (else    (ack (- m 1) (ack m (- n 1))))))

(ack 2 3)                 ; => 9
(ack 3 3)                 ; => 61

; Mutual recursion works too.
(define (even? n) (if (= n 0) #t (odd?  (- n 1))))
(define (odd?  n) (if (= n 0) #f (even? (- n 1))))

(even? 10)                ; => #t
(odd?  7)                 ; => #t
