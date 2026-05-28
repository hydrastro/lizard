; -*- lisp -*-
; ============================================================
;  EXAMPLE 110 — Track R: syntax-rules with ellipsis
; ============================================================
;
; A real pattern-based macro engine with `...` (ellipsis), the
; headline Track R feature. We define macros as data and expand
; uses of them into core forms.

(import "match.lisp")
(import "syntax-rules.lisp")

(display "=== SIMPLE MACRO: swap ===") (newline)
; (swap a b) => (let ((tmp a)) (set! a b) (set! b tmp))
(define swap-macro
  (make-macro '()
    (list
      (list '(swap a b)
            '(let ((tmp a)) (set! a b) (set! b tmp))))))
(display "  (swap x y) expands to:") (newline)
(display "    ") (display (macro-apply swap-macro '(swap x y))) (newline)
(newline)

(display "=== CONDITIONAL: my-when ===") (newline)
; (my-when c body) => (if c body #f)
(define when-macro
  (make-macro '()
    (list
      (list '(my-when c body) '(if c body #f)))))
(display "  (my-when (> x 0) 'pos) expands to:") (newline)
(display "    ") (display (macro-apply when-macro '(my-when (> x 0) (quote pos)))) (newline)
(newline)

(display "=== ELLIPSIS: my-list ===") (newline)
; (my-list e ...) => (list e ...)
(define list-macro
  (make-macro '()
    (list
      (list '(my-list e ...) '(list e ...)))))
(display "  (my-list 1 2 3 4) expands to:") (newline)
(display "    ") (display (macro-apply list-macro '(my-list 1 2 3 4))) (newline)
(display "  (my-list a) expands to:") (newline)
(display "    ") (display (macro-apply list-macro '(my-list a))) (newline)
(newline)

(display "=== ELLIPSIS: my-begin ===") (newline)
; (my-begin e ...) => (begin e ...)
(define begin-macro
  (make-macro '()
    (list
      (list '(my-begin e ...) '(begin e ...)))))
(display "  (my-begin (a) (b) (c)) expands to:") (newline)
(display "    ") (display (macro-apply begin-macro '(my-begin (a) (b) (c)))) (newline)
(newline)

(display "=== ELLIPSIS WITH TRANSFORM: double-each ===") (newline)
; (double-each x ...) => (list (* 2 x) ...)
(define double-macro
  (make-macro '()
    (list
      (list '(double-each x ...) '(list (* 2 x) ...)))))
(display "  (double-each 1 2 3) expands to:") (newline)
(display "    ") (display (macro-apply double-macro '(double-each 1 2 3))) (newline)
(newline)

(display "=== LET VIA ELLIPSIS ===") (newline)
; (my-let ((v e) ...) body) => ((lambda (v ...) body) e ...)
(define let-macro
  (make-macro '()
    (list
      (list '(my-let ((v e) ...) body)
            '((lambda (v ...) body) e ...)))))
(display "  (my-let ((x 1) (y 2)) (+ x y)) expands to:") (newline)
(display "    ")
(display (macro-apply let-macro '(my-let ((x 1) (y 2)) (+ x y)))) (newline)
(newline)

(display "=== MULTIPLE RULES (recursive shape) ===") (newline)
; (my-and) => #t
; (my-and e) => e
; (my-and e rest ...) => (if e (my-and rest ...) #f)
(define and-macro
  (make-macro '()
    (list
      (list '(my-and) '#t)
      (list '(my-and e) 'e)
      (list '(my-and e rest ...) '(if e (my-and rest ...) #f)))))
(display "  (my-and): ") (display (macro-apply and-macro '(my-and))) (newline)
(display "  (my-and a): ") (display (macro-apply and-macro '(my-and a))) (newline)
(display "  (my-and a b c) expands to:") (newline)
(display "    ") (display (macro-apply and-macro '(my-and a b c))) (newline)
(newline)

(display "=== LITERALS (keywords that must match) ===") (newline)
; (for x in lst do body) — 'in and 'do are literals
(define for-macro
  (make-macro '(in do)
    (list
      (list '(for x in lst do body)
            '(for-each (lambda (x) body) lst)))))
(display "  (for i in nums do (print i)) expands to:") (newline)
(display "    ")
(display (macro-apply for-macro '(for i in nums do (print i)))) (newline)
(newline)

(display "=== Track R milestone: syntax-rules + ellipsis ✓ ===") (newline)
