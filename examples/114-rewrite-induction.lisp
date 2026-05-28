; -*- lisp -*-
; ============================================================
;  EXAMPLE 114 — Track K: rewriting & induction
; ============================================================

(import "match.lisp")
(import "rewrite.lisp")

(display "=== TERM REWRITING (the simpl/rewrite engine) ===") (newline)
; Rules for simplifying arithmetic expressions:
(define arith-rules
  (list
    (rule '(+ ?x 0) '?x)            ; x + 0 = x
    (rule '(+ 0 ?x) '?x)            ; 0 + x = x
    (rule '(* ?x 1) '?x)            ; x * 1 = x
    (rule '(* 1 ?x) '?x)            ; 1 * x = x
    (rule '(* ?x 0) '0)            ; x * 0 = 0
    (rule '(* 0 ?x) '0)))          ; 0 * x = 0

(display "  (+ a 0) → ")
(display (rewrite-normalize arith-rules '(+ a 0) 50)) (newline)
(display "  (* (+ b 0) 1) → ")
(display (rewrite-normalize arith-rules '(* (+ b 0) 1) 50)) (newline)
(display "  (* (+ x 0) 0) → ")
(display (rewrite-normalize arith-rules '(* (+ x 0) 0) 50)) (newline)
(display "  (+ (* 1 a) (* b 0)) → ")
(display (rewrite-normalize arith-rules '(+ (* 1 a) (* b 0)) 50)) (newline)
(newline)

(display "=== REWRITE TRACE (step by step) ===") (newline)
(display "  steps for (* (+ a 0) 1):") (newline)
(for-each (lambda (s) (display "    → ") (display s) (newline))
          (rewrite-trace arith-rules '(* (+ a 0) 1) 50))
(newline)

(display "=== BOOLEAN SIMPLIFICATION ===") (newline)
(define bool-rules
  (list
    (rule '(and ?x true) '?x)
    (rule '(and true ?x) '?x)
    (rule '(and ?x false) 'false)
    (rule '(or ?x false) '?x)
    (rule '(not (not ?x)) '?x)))
(display "  (and (not (not p)) true) → ")
(display (rewrite-normalize bool-rules '(and (not (not p)) true) 50)) (newline)
(newline)

(display "=== INDUCTION PRINCIPLE (Nat) ===") (newline)
; sum 0..n computed by the Nat recursor (= induction)
(display "  sum 0..5 (via nat-induction) = ") (display (sum-to 5)) (newline)
(display "  sum 0..10 = ") (display (sum-to 10)) (newline)
(newline)

(display "=== PROOF BY INDUCTION (verified computationally) ===") (newline)
; Theorem: sum 0..n = n(n+1)/2.  Proven by induction; we VERIFY the
; computational content holds for 0..20.
(display "  Theorem: sum(0..n) = n(n+1)/2") (newline)
(display "  Verified for n in 0..20: ")
(display (verify-nat-property
          (lambda (n) (= (sum-to n) (gauss-formula n)))
          20)) (newline)
(newline)

(display "=== INDUCTION PRINCIPLE (List) ===") (newline)
; length and sum via list-induction (the list recursor)
(display "  length of (a b c d) = ")
(display (list-induction 0 (lambda (x acc) (+ 1 acc)) '(a b c d))) (newline)
(display "  sum of (1 2 3 4 5) = ")
(display (list-induction 0 (lambda (x acc) (+ x acc)) '(1 2 3 4 5))) (newline)
(newline)

(display "=== Track K milestone: rewrite + induction ✓ ===") (newline)
