; -*- lisp -*-
; ============================================================
;  EXAMPLE 106 — Tracing & debugging (Phase 8)
; ============================================================

(import "match.lisp")
(import "trace.lisp")

(display "=== TRACED RECURSION (factorial) ===") (newline)
; Define factorial, then trace it. The traced version logs each
; call and return with indentation, revealing the call tree.
(define fact-impl
  (lambda (n)
    (if (= n 0) 1 (* n ((trace-fn1 "fact" fact-impl) (- n 1))))))
(define traced-fact (trace-fn1 "fact" fact-impl))
(display "  (fact 4):") (newline)
(traced-fact 4)
(newline)

(display "=== TRACED FIBONACCI (see the tree) ===") (newline)
(define fib-impl
  (lambda (n)
    (if (< n 2) n
      (+ ((trace-fn1 "fib" fib-impl) (- n 1))
         ((trace-fn1 "fib" fib-impl) (- n 2))))))
(define traced-fib (trace-fn1 "fib" fib-impl))
(display "  (fib 4):") (newline)
(traced-fib 4)
(newline)

(display "=== CALL COUNTING ===") (newline)
(define counter (make-call-counter))
(define (slow-fib n)
  (if (< n 2) n
    (+ (slow-fib (- n 1)) (slow-fib (- n 2)))))
(define counted-fib (counted "fib" counter slow-fib))
(counted-fib 5)
(display "  naive fib(5) outer calls counted: ") (display (deref counter)) (newline)
(newline)

(display "=== STEP MEASUREMENT (deterministic cost) ===") (newline)
(define (count-down n)
  (if (= n 0) 'done
    (begin (tick-step!) (count-down (- n 1)))))
(measure-steps "count-down 100" (lambda () (count-down 100)))
(measure-steps "count-down 50" (lambda () (count-down 50)))
(newline)

(display "=== INLINE DEBUG ===") (newline)
; debug returns its value, so it can be spliced into expressions
(define result
  (* (debug "factor-a" 6)
     (debug "factor-b" 7)))
(display "  product: ") (display result) (newline)
(newline)

(display "=== ASSERTIONS ===") (newline)
(assert! (= (+ 2 2) 4) "two plus two is four")
(assert-equal! (* 3 3) 9 "three squared")
(assert-equal! (+ 1 1) 3 "intentional failure (demo)")
(newline)

(display "=== Phase 8 milestone: tracing & debug ✓ ===") (newline)
