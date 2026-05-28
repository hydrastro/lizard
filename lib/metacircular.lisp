; -*- lisp -*-
; lib/metacircular.lisp — A Lisp interpreter written in lizard.
;
; The ultimate demonstration of self-reference: lizard evaluating
; a Lisp dialect defined entirely within lizard. Supports:
;   self-evaluating literals, symbols, quote, if, lambda, define,
;   let, begin, and function application with primitives.
;
; Environments are association lists of (symbol . value), with a
; chain via a special '__parent__ key.

; ============================================================
;  ENVIRONMENT
; ============================================================

(define (menv-new parent)
  (list (cons '__parent__ parent)))

(define (menv-lookup env sym)
  (let ((pair (assoc sym env)))
    (if pair
        (cdr pair)
        (let ((parent (assoc '__parent__ env)))
          (if (and parent (not (null? (cdr parent))))
              (menv-lookup (cdr parent) sym)
              (begin (display "  meta-error: unbound ") (display sym) (newline)
                     'undefined))))))

(define (menv-define env sym val)
  (cons (cons sym val) env))

; ============================================================
;  CLOSURES
; ============================================================
; A closure is (closure params body env).

(define (make-closure params body env)
  (list 'closure params body env))

(define (closure? x)
  (and (pair? x) (equal? (car x) 'closure)))

(define (closure-params c) (car (cdr c)))
(define (closure-body c)   (car (cdr (cdr c))))
(define (closure-env c)    (car (cdr (cdr (cdr c)))))

; ============================================================
;  PRIMITIVE OPERATIONS
; ============================================================

(define (meta-primitive? sym)
  (member sym '(+ - * / = < > <= >= cons car cdr list null? not)))

(define (apply-primitive op args)
  (if (equal? op '+) (fold-left + 0 args)
    (if (equal? op '*) (fold-left * 1 args)
      (if (equal? op '-) (- (car args) (car (cdr args)))
        (if (equal? op '/) (/ (car args) (car (cdr args)))
          (if (equal? op '=) (= (car args) (car (cdr args)))
            (if (equal? op '<) (< (car args) (car (cdr args)))
              (if (equal? op '>) (> (car args) (car (cdr args)))
                (if (equal? op '<=) (<= (car args) (car (cdr args)))
                  (if (equal? op '>=) (>= (car args) (car (cdr args)))
                    (if (equal? op 'cons) (cons (car args) (car (cdr args)))
                      (if (equal? op 'car) (car (car args))
                        (if (equal? op 'cdr) (cdr (car args))
                          (if (equal? op 'list) args
                            (if (equal? op 'null?) (null? (car args))
                              (if (equal? op 'not) (not (car args))
                                'meta-error))))))))))))))))

; ============================================================
;  THE EVALUATOR
; ============================================================

(define (meta-eval expr env)
  (if (number? expr) expr                       ; self-evaluating
    (if (string? expr) expr
      (if (symbol? expr) (menv-lookup env expr)  ; variable
        (if (pair? expr)
            (meta-eval-form expr env)
            expr)))))

(define (meta-eval-form expr env)
  (let ((head (car expr)))
    (if (equal? head 'quote)
        (car (cdr expr))                          ; (quote x)
      (if (equal? head 'if)
          (meta-eval-if expr env)
        (if (equal? head 'lambda)
            (make-closure (car (cdr expr)) (car (cdr (cdr expr))) env)
          (if (equal? head 'begin)
              (meta-eval-seq (cdr expr) env)
            (if (equal? head 'let)
                (meta-eval-let expr env)
              (meta-eval-apply expr env))))))))

(define (meta-eval-if expr env)
  (let ((test (meta-eval (car (cdr expr)) env)))
    (if (and (not (equal? test #f)) (not (null? test)))
        (meta-eval (car (cdr (cdr expr))) env)
        (meta-eval (car (cdr (cdr (cdr expr)))) env))))

(define (meta-eval-seq exprs env)
  (if (null? exprs) '()
    (if (null? (cdr exprs))
        (meta-eval (car exprs) env)
        (begin (meta-eval (car exprs) env)
               (meta-eval-seq (cdr exprs) env)))))

; (let ((x e1) (y e2)) body)
(define (meta-eval-let expr env)
  (let ((bindings (car (cdr expr)))
        (body (car (cdr (cdr expr)))))
    (let ((new-env (bind-let bindings env env)))
      (meta-eval body new-env))))

(define (bind-let bindings eval-env target-env)
  (if (null? bindings) target-env
    (let ((b (car bindings)))
      (let ((name (car b))
            (val (meta-eval (car (cdr b)) eval-env)))
        (bind-let (cdr bindings) eval-env
                  (menv-define target-env name val))))))

(define (meta-eval-apply expr env)
  (let ((op (car expr)))
    (let ((args (map (lambda (e) (meta-eval e env)) (cdr expr))))
      (if (meta-primitive? op)
          (apply-primitive op args)
          (let ((fn (meta-eval op env)))
            (meta-apply fn args))))))

(define (meta-apply fn args)
  (if (closure? fn)
      (let ((new-env (bind-params (closure-params fn) args (closure-env fn))))
        (meta-eval (closure-body fn) new-env))
      (begin (display "  meta-error: not a function") (newline) 'error)))

(define (bind-params params args env)
  (if (null? params) env
    (bind-params (cdr params) (cdr args)
                 (menv-define env (car params) (car args)))))

; ============================================================
;  TOP-LEVEL RUNNER
; ============================================================

(define (meta-run expr)
  (meta-eval expr (menv-new '())))

(define (meta-show label expr)
  (display "  ")
  (display label)
  (display " => ")
  (display (meta-run expr))
  (newline))
