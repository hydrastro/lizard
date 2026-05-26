; -*- lisp -*-
; Pattern matching as a runtime dispatcher.
;
; In a hygienic Scheme this would be a `syntax-rules` macro; lizard
; doesn't have varargs in lambdas, so we build it as a regular
; higher-order function that takes a list of (tag thunk) clauses.

(define (map f xs) (if (null? xs) '() (cons (f (car xs)) (map f (cdr xs)))))

; Dispatch on the tag (car) of `e`. `clauses` is a list of
; (tag thunk) — when the tag matches, the thunk is invoked.
; A clause whose tag is the symbol `else` always matches.
(define (match-tag e clauses)
  (cond ((null? clauses) 'no-match)
        ((or (= (car (car clauses)) 'else)
             (= (car e) (car (car clauses))))
         ((car (cdr (car clauses)))))
        (else (match-tag e (cdr clauses)))))

; --- Example: a tiny tagged-data interpreter ------------------------------

(define (lit n)    (list 'lit n))
(define (plus a b) (list 'plus a b))
(define (times a b) (list 'times a b))
(define (neg a)    (list 'neg a))

(define (eval-expr e)
  (match-tag e
    (list
     (list 'lit   (lambda () (car (cdr e))))
     (list 'plus  (lambda () (+ (eval-expr (car (cdr e)))
                                (eval-expr (car (cdr (cdr e)))))))
     (list 'times (lambda () (* (eval-expr (car (cdr e)))
                                (eval-expr (car (cdr (cdr e)))))))
     (list 'neg   (lambda () (- (eval-expr (car (cdr e))))))
     (list 'else  (lambda () 'unknown)))))

(define expr1 (plus (lit 3) (times (lit 4) (lit 5))))
(define expr2 (neg (plus (lit 1) (lit 9))))
(define expr3 (times (plus (lit 2) (lit 3)) (neg (lit 4))))

(display "expr1 = ") (display expr1) (newline)
(display "  evaluates to ") (display (eval-expr expr1)) (newline)
(newline)
(display "expr2 = ") (display expr2) (newline)
(display "  evaluates to ") (display (eval-expr expr2)) (newline)
(newline)
(display "expr3 = ") (display expr3) (newline)
(display "  evaluates to ") (display (eval-expr expr3)) (newline)
(newline)

; --- Symbolic differentiation using the same dispatcher -------------------

(define (var s) (list 'var s))

(define (deriv e v)
  (match-tag e
    (list
     (list 'lit  (lambda () (lit 0)))
     (list 'var  (lambda () (if (= (car (cdr e)) v) (lit 1) (lit 0))))
     (list 'plus (lambda () (plus (deriv (car (cdr e)) v)
                                  (deriv (car (cdr (cdr e))) v))))
     (list 'times
       (lambda ()
         (plus (times (deriv (car (cdr e)) v) (car (cdr (cdr e))))
               (times (car (cdr e))           (deriv (car (cdr (cdr e))) v)))))
     (list 'neg  (lambda () (neg (deriv (car (cdr e)) v))))
     (list 'else (lambda () 'unknown)))))

; d/dx (3 + 4*x) symbolically.
(define expr-a (plus (lit 3) (times (lit 4) (var 'x))))
(display "d/dx (3 + 4*x) = ") (display (deriv expr-a 'x)) (newline)

; d/dx (x*x) — expect 2x in expanded form.
(define expr-b (times (var 'x) (var 'x)))
(display "d/dx (x*x)     = ") (display (deriv expr-b 'x)) (newline)

; d/dx -(x + 7) — expect -1.
(define expr-c (neg (plus (var 'x) (lit 7))))
(display "d/dx -(x + 7)  = ") (display (deriv expr-c 'x)) (newline)
(display "  evaluated    = ") (display (eval-expr (deriv expr-c 'x))) (newline)
