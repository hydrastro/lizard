; -*- lisp -*-
; ============================================================
;  EXAMPLE 119 — Track R: syntax-case (procedural macros)
; ============================================================
;
; Unlike syntax-rules, each clause's right-hand side is a PROCEDURE
; that runs arbitrary code over the matched bindings — so macros can
; compute, inspect sub-forms, and assemble output programmatically.

(import "match.lisp")
(import "syntax-rules.lisp")
(import "syntax-case.lisp")

(display "=== BASIC: transformer is a procedure ===") (newline)
; (square x) => (* x x), but built by code, not a template
(define square-macro
  (make-sc-macro '()
    (list
      (sc-clause '(square x) #t
        (lambda (b)
          (let ((x (sc-ref b 'x)))
            (list '* x x)))))))
(display "  (square (+ a 1)) => ")
(display (sc-apply square-macro '(square (+ a 1)))) (newline)
(newline)

(display "=== FENDERS (guards): match only if a predicate holds ===") (newline)
; (classify n): one clause fires only when n is a literal number.
(define classify-macro
  (make-sc-macro '()
    (list
      (sc-clause '(classify n)
        (lambda (b) (number? (sc-ref b 'n)))     ; fender: n must be a number
        (lambda (b) (list 'the-number (sc-ref b 'n))))
      (sc-clause '(classify n) #t                 ; fallback
        (lambda (b) (list 'some-expr (sc-ref b 'n)))))))
(display "  (classify 42) => ")
(display (sc-apply classify-macro '(classify 42))) (newline)
(display "  (classify (foo)) => ")
(display (sc-apply classify-macro '(classify (foo)))) (newline)
(display "  (the fender routes literals vs expressions)") (newline)
(newline)

(display "=== COMPUTING WITH ELLIPSIS BINDINGS ===") (newline)
; (sum-list e ...) => a right-folded (+ e1 (+ e2 ... 0)) built by code
(define sum-macro
  (make-sc-macro '()
    (list
      (sc-clause '(sum-list e ...) #t
        (lambda (b)
          (let ((es (sc-ref b 'e)))    ; es is the LIST of matched forms
            (fold-right-build es)))))))
(define (fold-right-build es)
  (if (null? es) 0
    (list '+ (car es) (fold-right-build (cdr es)))))
(display "  (sum-list a b c) => ")
(display (sc-apply sum-macro '(sum-list a b c))) (newline)
(display "  (sum-list x) => ")
(display (sc-apply sum-macro '(sum-list x))) (newline)
(newline)

(display "=== GENERATING CODE: swap-when ===") (newline)
; (reverse-args (f a b c)) => (f c b a) — reverse the argument order
(define rev-macro
  (make-sc-macro '()
    (list
      (sc-clause '(reverse-args (f a ...)) #t
        (lambda (b)
          (cons (sc-ref b 'f) (reverse (sc-ref b 'a))))))))
(display "  (reverse-args (sub 1 2 3)) => ")
(display (sc-apply rev-macro '(reverse-args (sub 1 2 3)))) (newline)
(newline)

(display "=== BUILDING A let FROM PAIRS ===") (newline)
; (my-let2 ((v e) ...) body) => ((lambda (v ...) body) e ...)
(define let2-macro
  (make-sc-macro '()
    (list
      (sc-clause '(my-let2 ((v e) ...) body) #t
        (lambda (b)
          (cons (list 'lambda (sc-ref b 'v) (sc-ref b 'body))
                (sc-ref b 'e)))))))
(display "  (my-let2 ((x 1) (y 2)) (+ x y)) => ")
(display (sc-apply let2-macro '(my-let2 ((x 1) (y 2)) (+ x y)))) (newline)
(newline)

(display "=== Track R milestone: syntax-case ✓ ===") (newline)
