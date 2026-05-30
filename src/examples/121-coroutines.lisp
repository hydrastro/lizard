; -*- lisp -*-
; ============================================================
;  EXAMPLE 121 — Track C: CSP coroutines with blocking
; ============================================================
;
; Real blocking communication: a consumer that receives from an
; empty channel PARKS until a producer sends. The scheduler wakes
; it when the value arrives.

(import "match.lisp")
(import "coroutine.lisp")

(display "=== PRODUCER / CONSUMER (blocking) ===") (newline)
(define ch (co-channel))
(define r (make-corun))
(co-spawn! r (co-producer ch 5))
(co-spawn! r (co-consumer ch 5))
(define steps (co-run! r 1000))
(display "  ran ") (display steps) (display " coroutine steps") (newline)
(display "  consumer received: ") (display (deref (corun-results r))) (newline)
(display "  (producer sent 0..4; consumer blocked & resumed each time)") (newline)
(newline)

(display "=== PIPELINE: producer → transformer → consumer ===") (newline)
(define c1 (co-channel))
(define c2 (co-channel))
(define r2 (make-corun))
(co-spawn! r2 (co-producer c1 4))                       ; sends 0 1 2 3
(co-spawn! r2 (co-transform c1 c2 4 (lambda (x) (* x x)))) ; squares them
(co-spawn! r2 (co-consumer c2 4))                       ; collects
(co-run! r2 1000)
(display "  pipeline result (squares of 0..3): ")
(display (deref (corun-results r2))) (newline)
(newline)

(display "=== YIELD (generator-style) ===") (newline)
; A coroutine that yields a sequence of values.
(define (counter-coro n)
  (define (go i)
    (if (>= i n)
        (co-done 'finished)
        (co-yield (* i 10) (lambda () (go (+ i 1))))))
  (lambda () (go 0)))
(define r3 (make-corun))
(co-spawn! r3 (counter-coro 5))
(co-run! r3 1000)
(display "  yielded values: ") (display (deref (corun-yields r3))) (newline)
(newline)

(display "=== MULTIPLE PRODUCERS, ONE CONSUMER ===") (newline)
(define shared (co-channel))
(define r4 (make-corun))
(co-spawn! r4 (co-producer shared 3))   ; sends 0 1 2
(co-spawn! r4 (co-producer shared 3))   ; sends 0 1 2 (again)
(co-spawn! r4 (co-consumer shared 6))   ; receives all 6
(co-run! r4 1000)
(display "  consumer got 6 values from 2 producers: ")
(display (deref (corun-results r4))) (newline)
(newline)

(display "=== HOW BLOCKING WORKS ===") (newline)
(display "  When a consumer receives from an empty channel, it is") (newline)
(display "  PARKED on the channel's waiter list (not requeued). A") (newline)
(display "  later send pops a waiter and hands it the value directly,") (newline)
(display "  re-scheduling the consumer. This is genuine CSP blocking,") (newline)
(display "  built without host-level continuations.") (newline)
(newline)

(display "=== Track C milestone: blocking CSP ✓ ===") (newline)
