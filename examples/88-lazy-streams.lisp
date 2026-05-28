; -*- lisp -*-
; ============================================================
;  EXAMPLE 88 — Lazy infinite streams
; ============================================================
;
; Streams represent infinite sequences. Only the elements you
; actually use get computed. The tail is a thunk (delayed).

(import "streams.lisp")

(display "=== Natural numbers ===") (newline)
(display "  First 10 naturals: ")
(display (stream-take 10 nats)) (newline)
(display "  Naturals from 100, first 5: ")
(display (stream-take 5 (naturals-from 100))) (newline)
(newline)

(display "=== Stream map ===") (newline)
(display "  First 10 squares: ")
(display (stream-take 10 (stream-map (lambda (x) (* x x)) nats))) (newline)
(newline)

(display "=== Stream filter ===") (newline)
(display "  First 10 even naturals: ")
(display (stream-take 10 (stream-filter (lambda (x) (= 0 (remainder x 2))) nats))) (newline)
(newline)

(display "=== Stream iterate ===") (newline)
(display "  Powers of 2 (first 10): ")
(display (stream-take 10 (stream-iterate (lambda (x) (* x 2)) 1))) (newline)
(newline)

(display "=== Fibonacci stream ===") (newline)
(display "  First 15 Fibonacci numbers: ")
(display (stream-take 15 (fib-stream))) (newline)
(newline)

(display "=== Sieve of Eratosthenes (lazy primes) ===") (newline)
(display "  First 15 primes: ")
(display (stream-take 15 (primes-stream))) (newline)
(newline)

(display "=== Stream aggregation ===") (newline)
(display "  Sum of first 100 naturals: ")
(display (stream-sum 100 nats)) (newline)
(display "  Sum of first 10 squares: ")
(display (stream-sum 10 (stream-map (lambda (x) (* x x)) (naturals-from 1)))) (newline)
(newline)

(display "╔════════════════════════════════════════════╗") (newline)
(display "║  Infinite streams — computed on demand.     ║") (newline)
(display "║  We defined infinite sequences but only      ║") (newline)
(display "║  evaluated the finitely many elements used.  ║") (newline)
(display "╚════════════════════════════════════════════╝") (newline)
