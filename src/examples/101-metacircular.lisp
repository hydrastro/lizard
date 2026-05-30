; -*- lisp -*-
; ============================================================
;  EXAMPLE 101 — A Lisp interpreter written in lizard
; ============================================================
;
; meta-eval is a complete Lisp evaluator written in lizard.
; Below, we run programs in THAT interpreter — lizard hosting
; its own semantics. This is the metacircular evaluator, the
; classic demonstration of computational self-reference.

(import "metacircular.lisp")

(display "╔════════════════════════════════════════════════╗") (newline)
(display "║  Lisp running on Lisp running on lizard.        ║") (newline)
(display "╚════════════════════════════════════════════════╝") (newline)
(newline)

(display "=== Literals ===") (newline)
(meta-show "42" 42)
(meta-show "(quote hello)" '(quote hello))
(newline)

(display "=== Arithmetic (in the meta-interpreter) ===") (newline)
(meta-show "(+ 1 2 3)" '(+ 1 2 3))
(meta-show "(* 6 7)" '(* 6 7))
(meta-show "(- 100 (* 7 8))" '(- 100 (* 7 8)))
(newline)

(display "=== Conditionals ===") (newline)
(meta-show "(if (< 1 2) 'yes 'no)" '(if (< 1 2) (quote yes) (quote no)))
(meta-show "(if (> 1 2) 'yes 'no)" '(if (> 1 2) (quote yes) (quote no)))
(newline)

(display "=== let bindings ===") (newline)
(meta-show "(let ((x 10)) (* x x))" '(let ((x 10)) (* x x)))
(meta-show "(let ((x 3) (y 4)) (+ (* x x) (* y y)))"
           '(let ((x 3) (y 4)) (+ (* x x) (* y y))))
(newline)

(display "=== lambda + application ===") (newline)
(meta-show "((lambda (x) (* x x)) 7)"
           '((lambda (x) (* x x)) 7))
(meta-show "((lambda (x y) (+ x y)) 20 22)"
           '((lambda (x y) (+ x y)) 20 22))
(newline)

(display "=== Closures capture environment ===") (newline)
(meta-show "let make-adder, apply"
           '(let ((make-adder (lambda (n) (lambda (x) (+ x n)))))
              (let ((add5 (make-adder 5)))
                (add5 100))))
(newline)

(display "=== Lists in the meta-interpreter ===") (newline)
(meta-show "(cons 1 (cons 2 (cons 3 (quote ()))))"
           '(cons 1 (cons 2 (cons 3 (quote ())))))
(meta-show "(car (cons 1 2))" '(car (cons 1 2)))
(meta-show "(cdr (list 1 2 3))" '(cdr (list 1 2 3)))
(newline)

(display "=== Higher-order in the meta-interpreter ===") (newline)
(meta-show "apply twice"
           '(let ((twice (lambda (f x) (f (f x)))))
              (let ((inc (lambda (n) (+ n 1))))
                (twice inc 10))))
(newline)

(display "╔════════════════════════════════════════════════╗") (newline)
(display "║  A complete interpreter, ~150 lines of lizard.  ║") (newline)
(display "║  Self-reference all the way down.               ║") (newline)
(display "╚════════════════════════════════════════════════╝") (newline)
