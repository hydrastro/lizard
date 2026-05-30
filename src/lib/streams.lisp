; -*- lisp -*-
; lib/streams.lisp — Lazy infinite streams via delay/force.
;
; A stream is (cons head promise-of-tail). Using delay/force,
; we can represent infinite sequences and only compute what we need.

; Construct a stream cell. The tail must be wrapped in (delay ...).
; Since lizard evaluates arguments eagerly, we use a thunk (lambda).
(define (stream-cons-fn head tail-thunk)
  (cons head tail-thunk))

(define (stream-head s) (car s))
(define (stream-tail s) ((cdr s)))   ; force the thunk

(define stream-empty '())
(define (stream-empty? s) (null? s))

; Take the first n elements as a regular list.
(define (stream-take n s)
  (if (or (= n 0) (stream-empty? s)) '()
    (cons (stream-head s)
          (stream-take (- n 1) (stream-tail s)))))

; Map a function over a stream (lazily).
(define (stream-map f s)
  (if (stream-empty? s) stream-empty
    (stream-cons-fn (f (stream-head s))
                    (lambda () (stream-map f (stream-tail s))))))

; Filter a stream (lazily).
(define (stream-filter pred s)
  (if (stream-empty? s) stream-empty
    (if (pred (stream-head s))
        (stream-cons-fn (stream-head s)
                        (lambda () (stream-filter pred (stream-tail s))))
        (stream-filter pred (stream-tail s)))))

; The infinite stream of natural numbers starting at n.
(define (naturals-from n)
  (stream-cons-fn n (lambda () (naturals-from (+ n 1)))))

; The infinite stream of all natural numbers.
(define nats (naturals-from 0))

; Infinite stream where each element is (f applied k times to seed).
(define (stream-iterate f seed)
  (stream-cons-fn seed (lambda () (stream-iterate f (f seed)))))

; Sum the first n elements of a stream.
(define (stream-sum n s)
  (fold-left + 0 (stream-take n s)))

; The Fibonacci stream.
(define (fib-stream)
  (define (gen a b)
    (stream-cons-fn a (lambda () (gen b (+ a b)))))
  (gen 0 1))

; Sieve of Eratosthenes as an infinite prime stream.
(define (sieve s)
  (let ((p (stream-head s)))
    (stream-cons-fn p
      (lambda ()
        (sieve (stream-filter
                 (lambda (x) (not (= 0 (remainder x p))))
                 (stream-tail s)))))))

(define (primes-stream)
  (sieve (naturals-from 2)))
