; -*- lisp -*-
; examples/142-contracts-blame.lisp
;
; Phase 4 — contracts with blame.  A contract guards a value at the boundary
; between two parties; when the value misbehaves, the contract system names the
; party at fault: the CALLER for a bad argument, the FUNCTION for a bad result.
; Function contracts compose (curried), so the blame is assigned correctly even
; for the second argument of a curried function.
;
; Self-checking: each `must` raises if its claim is false.

(import "contract.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'contract-regression)))

(define (nonneg? x) (>= x 0))
(define (nonzero? x) (not (equal? x 0)))

; integer square root, contracted to map nonneg -> nonneg
(define (isqrt x)
  (cond ((equal? x 0) 0) ((equal? x 1) 1) ((equal? x 4) 2)
        ((equal? x 9) 3) (else 1)))
(define checked-sqrt
  (protect (-> (flat nonneg? "nonneg") (flat nonneg? "nonneg"))
           isqrt "sqrt" "client"))

(newline)
(display "A well-behaved contracted function:") (newline)
(must "(checked-sqrt 9) = 3" (equal? (checked-sqrt 9) 3))
(must "(checked-sqrt 4) = 2" (equal? (checked-sqrt 4) 2))

(newline)
(display "A bad ARGUMENT is the caller's fault:") (newline)
(must "(checked-sqrt -1) is a contract violation" (violation? (checked-sqrt -1)))
(must "...and the blamed party is the client"
      (equal? (blamed-party (checked-sqrt -1)) "client"))
(must "...on the \"nonneg\" contract"
      (equal? (violated-contract (checked-sqrt -1)) "nonneg"))

(newline)
(display "A bad RESULT is the function's fault:") (newline)
; this implementation lies: it promises nonneg but returns -5
(define buggy
  (protect (-> (flat nonneg? "nonneg") (flat nonneg? "nonneg"))
           (lambda (x) -5) "buggy-impl" "client"))
(must "(buggy 3) is a contract violation" (violation? (buggy 3)))
(must "...and the blamed party is the implementation"
      (equal? (blamed-party (buggy 3)) "buggy-impl"))

(newline)
(display "Curried / higher-order contract — blame on the 2nd argument:") (newline)
; safe division: divisor must be nonzero; the contract prevents the / by zero
(define safe-div
  (protect (-> (flat number? "number")
               (-> (flat nonzero? "nonzero") (flat number? "number")))
           (lambda (a) (lambda (b) (quotient a b)))
           "div" "client"))
(must "((safe-div 10) 2) = 5" (equal? ((safe-div 10) 2) 5))
(must "((safe-div 10) 0) is a contract violation" (violation? ((safe-div 10) 0)))
(must "...and the blamed party is the client (bad divisor)"
      (equal? (blamed-party ((safe-div 10) 0)) "client"))

(newline)
(display "contract violations correctly name the blaming party") (newline)
