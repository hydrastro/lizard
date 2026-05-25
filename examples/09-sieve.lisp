; -*- lisp -*-
; Sieve of Eratosthenes — functional implementation.

(define (range lo hi)
  (if (>= lo hi) '() (cons lo (range (+ lo 1) hi))))

(define (filter p xs)
  (cond ((null? xs) '())
        ((p (car xs)) (cons (car xs) (filter p (cdr xs))))
        (else         (filter p (cdr xs)))))

(define (length xs)
  (if (null? xs) 0 (+ 1 (length (cdr xs)))))

(define (sieve xs)
  (if (null? xs)
      '()
      (let ((p (car xs)))
        (cons p
              (sieve (filter (lambda (x) (not (= (% x p) 0)))
                             (cdr xs)))))))

(define (primes-below n) (sieve (range 2 n)))

(define primes-100 (primes-below 100))
(display "primes < 100 (")  (display (length primes-100))  (display " of them):") (newline)
(display primes-100) (newline) (newline)

(define primes-500 (primes-below 500))
(display "count of primes < 500: ") (display (length primes-500)) (newline)

(define primes-1000 (primes-below 1000))
(display "count of primes < 1000: ") (display (length primes-1000)) (newline)
