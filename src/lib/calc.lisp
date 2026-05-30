; -*- lisp -*-
; lib/calc.lisp — A small expression evaluator written in lizard.
;
; Demonstrates that lizard is self-sufficient: this is a complete
; little interpreter for arithmetic expressions with variables,
; written entirely in lizard itself.
;
; Expressions are s-expressions:
;   (+ a b)  (- a b)  (* a b)  (/ a b)
;   (let x e1 e2)  — bind x to e1 in e2
;   x              — variable reference
;   42             — literal
;
; An environment is an association list of (name . value).

; ============================================================
;  ENVIRONMENT
; ============================================================

(define (calc-env-empty) '())

(define (calc-env-lookup env name)
  (let ((pair (assoc name env)))
    (if pair (cdr pair)
      (begin (display "  ERROR: unbound variable ")
             (display name) (newline)
             0))))

(define (calc-env-extend env name val)
  (cons (cons name val) env))

; ============================================================
;  EVALUATOR
; ============================================================

(define (calc-eval expr env)
  (if (number? expr)
      expr                                ; literal
      (if (symbol? expr)
          (calc-env-lookup env expr)      ; variable
          (calc-eval-form expr env))))    ; compound form

(define (calc-eval-form expr env)
  (let ((op (car expr)))
    (if (equal? op 'let)
        ; (let x e1 e2)
        (let ((name (car (cdr expr)))
              (val (calc-eval (car (cdr (cdr expr))) env))
              (body (car (cdr (cdr (cdr expr))))))
          (calc-eval body (calc-env-extend env name val)))
        ; binary arithmetic
        (let ((a (calc-eval (car (cdr expr)) env))
              (b (calc-eval (car (cdr (cdr expr))) env)))
          (calc-apply op a b)))))

(define (calc-apply op a b)
  (if (equal? op '+) (+ a b)
    (if (equal? op '-) (- a b)
      (if (equal? op '*) (* a b)
        (if (equal? op '/) (/ a b)
          (if (equal? op 'mod) (% a b)
            (begin (display "  ERROR: unknown op ")
                   (display op) (newline)
                   0)))))))

; ============================================================
;  RUNNER
; ============================================================

; Evaluate an expression in the empty environment.
(define (calc expr)
  (calc-eval expr (calc-env-empty)))

; Evaluate and print.
(define (calc-run label expr)
  (display "  ")
  (display label)
  (display " = ")
  (display (calc expr))
  (newline))
