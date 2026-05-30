; -*- lisp -*-
; mini-lisp — a tiny Scheme evaluator written in lizard itself.
;
; lizard's parser eagerly turns `(lambda …)` and `(if …)` into
; AST_LAMBDA / AST_IF nodes — even inside a quote. To keep the meta
; language safely in DATA form, we use different keywords:
;
;     (FN (x) <body>)       — lambda
;     (BR test then else)   — branch / if
;
; The meta-evaluator below handles: numbers, symbols, FN, BR, quote,
; +, -, *, =, <, and ordinary application via association-list envs.

(define (lookup sym env)
  (cond ((null? env) (display "  unbound: ") (display sym) (newline) 'unbound)
        ((= (car (car env)) sym) (car (cdr (car env))))
        (else (lookup sym (cdr env)))))

(define (extend env name val) (cons (list name val) env))

(define (extend-many env ks vs)
  (if (null? ks)
      env
      (extend-many (extend env (car ks) (car vs)) (cdr ks) (cdr vs))))

(define (map f xs)
  (if (null? xs) '() (cons (f (car xs)) (map f (cdr xs)))))

(define (apply-closure proc args)
  ; proc = (closure params body env)
  (let ((params (car (cdr proc)))
        (body   (car (cdr (cdr proc))))
        (cenv   (car (cdr (cdr (cdr proc))))))
    (mini-eval body (extend-many cenv params args))))

(define (mini-eval expr env)
  (cond
    ((number? expr) expr)
    ((symbol? expr) (lookup expr env))
    ((pair?   expr)
     (let ((h (car expr)))
       (cond
         ((= h 'quote) (car (cdr expr)))
         ((= h 'BR)
          (if (mini-eval (car (cdr expr)) env)
              (mini-eval (car (cdr (cdr expr))) env)
              (mini-eval (car (cdr (cdr (cdr expr)))) env)))
         ((= h 'FN)
          (list 'closure (car (cdr expr)) (car (cdr (cdr expr))) env))
         ((= h '+) (+ (mini-eval (car (cdr expr)) env)
                      (mini-eval (car (cdr (cdr expr))) env)))
         ((= h '-) (- (mini-eval (car (cdr expr)) env)
                      (mini-eval (car (cdr (cdr expr))) env)))
         ((= h '*) (* (mini-eval (car (cdr expr)) env)
                      (mini-eval (car (cdr (cdr expr))) env)))
         ((= h '=) (= (mini-eval (car (cdr expr)) env)
                      (mini-eval (car (cdr (cdr expr))) env)))
         ((= h '<) (< (mini-eval (car (cdr expr)) env)
                      (mini-eval (car (cdr (cdr expr))) env)))
         (else
           (apply-closure (mini-eval h env)
                          (map (lambda (a) (mini-eval a env)) (cdr expr)))))))
    (else expr)))

; --- demos --------------------------------------------------------------

(display "(+ (* 2 3) 4)            in mini-lisp = ")
(display (mini-eval '(+ (* 2 3) 4) '())) (newline)

(display "((FN (x) (* x x)) 9)     in mini-lisp = ")
(display (mini-eval '((FN (x) (* x x)) 9) '())) (newline)

(display "((FN (x y) (+ x y)) 3 4) in mini-lisp = ")
(display (mini-eval '((FN (x y) (+ x y)) 3 4) '())) (newline)

(display "branch: (BR (= 0 0) 'yes 'no) in mini-lisp = ")
(display (mini-eval '(BR (= 0 0) (quote yes) (quote no)) '())) (newline)

; Factorial in the inner lisp via Y combinator (no `define` needed).
(define Y-fact
  '((FN (f)
      ((FN (x) (f (FN (v) ((x x) v))))
       (FN (x) (f (FN (v) ((x x) v))))))
    (FN (self)
      (FN (n)
        (BR (= n 0) 1 (* n (self (- n 1))))))))

(display "Y-fact(0)  in mini-lisp = ")
(display (mini-eval (list Y-fact 0) '())) (newline)
(display "Y-fact(5)  in mini-lisp = ")
(display (mini-eval (list Y-fact 5) '())) (newline)
(display "Y-fact(10) in mini-lisp = ")
(display (mini-eval (list Y-fact 10) '())) (newline)
