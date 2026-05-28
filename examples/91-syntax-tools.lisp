; -*- lisp -*-
; ============================================================
;  EXAMPLE 91 — Track R: syntax objects and transformation
; ============================================================

(import "syntax-tools.lisp")

(display "=== SYNTAX OBJECTS ===") (newline)
(define stx (syntax-quote '(lambda (x) (+ x 1))))
(display "  syntax? ") (display (syntax? stx)) (newline)
(display "  unwrapped: ") (display (syntax-unwrap stx)) (newline)
(display "  syntax-list? ") (display (syntax-list? stx)) (newline)
(newline)

(define sym-stx (syntax-quote 'hello))
(display "  symbol syntax-symbol? ") (display (syntax-symbol? sym-stx)) (newline)
(newline)

(display "=== TEMPLATE EXPANSION ===") (newline)
(define template '(if cond then-branch else-branch))
(define bindings '((cond . (> x 0))
                   (then-branch . "positive")
                   (else-branch . "non-positive")))
(display "  template: ") (display template) (newline)
(display "  expanded: ") (display (template-expand template bindings)) (newline)
(newline)

(display "=== FORM BUILDERS ===") (newline)
(display "  make-let: ")
(display (make-let '((x 1) (y 2)) '(+ x y))) (newline)
(display "  make-lambda: ")
(display (make-lambda '(a b) '(* a b))) (newline)
(display "  make-if: ")
(display (make-if '(> x 0) 'pos 'neg)) (newline)
(display "  swap-args '(sub a b): ")
(display (swap-args '(sub a b))) (newline)
(newline)

(display "=== AST WALKING ===") (newline)
(define expr '(+ (* 2 3) (- 10 (/ 8 4))))
(display "  expr: ") (display expr) (newline)
(display "  node count: ") (display (count-nodes expr)) (newline)
(display "  symbols: ") (display (collect-symbols expr)) (newline)
(newline)

(display "=== FORM TRANSFORMATION ===") (newline)
; Transform: replace all 'x with 'y
(define (rename-x form)
  (if (equal? form 'x) 'y form))
(define original '(+ x (* x x)))
(display "  original: ") (display original) (newline)
(display "  x→y: ") (display (walk-form rename-x original)) (newline)
(newline)

(display "=== Macro-style expansion demo ===") (newline)
; A "when" macro expands (when c body) => (if c body '())
(define (expand-when form)
  (make-if (car (cdr form))
           (car (cdr (cdr form)))
           (quote ())))
(display "  (when (> x 0) 'yes) expands to:") (newline)
(display "    ") (display (expand-when '(when (> x 0) (quote yes)))) (newline)
(newline)

(display "=== End of syntax tools ===") (newline)
