; -*- lisp -*-
; lib/cas.lisp — A small symbolic computer-algebra core.
;
; Expressions are s-expressions:
;   number            a constant
;   symbol            a variable
;   (+ a b ...)       sum
;   (- a b)           difference
;   (* a b ...)       product
;   (/ a b)           quotient
;   (^ a n)           power
;   (sin e) (cos e) (exp e) (ln e)   elementary functions
;
; This module does symbolic differentiation and simplification.
; Its companion lib/cas-proof.lisp attaches a JUSTIFICATION to every
; step so a computation can be unfolded into a proof down to axioms.

(import "match.lisp")

; ============================================================
;  CONSTRUCTORS / PREDICATES
; ============================================================

(define (cas-const? e) (number? e))
(define (cas-var? e) (symbol? e))
(define (cas-op? e op) (and (pair? e) (equal? (car e) op)))

(define (cas-sum a b) (list '+ a b))
(define (cas-diff a b) (list '- a b))
(define (cas-prod a b) (list '* a b))
(define (cas-quot a b) (list '/ a b))
(define (cas-pow a n) (list '^ a n))

; ============================================================
;  SIMPLIFICATION  (algebraic identities)
; ============================================================
; Each identity used here is an axiom/lemma named in cas-proof.lisp:
;   x+0=x, 0+x=x, x*1=x, 1*x=x, x*0=0, x^1=x, x^0=1, etc.

(define (simplify e)
  (if (or (cas-const? e) (cas-var? e)) e
    (if (cas-op? e '+)  (simp-sum (simplify (car (cdr e))) (simplify (car (cdr (cdr e)))))
      (if (cas-op? e '-) (simp-diff (simplify (car (cdr e))) (simplify (car (cdr (cdr e)))))
        (if (cas-op? e '*) (simp-prod (simplify (car (cdr e))) (simplify (car (cdr (cdr e)))))
          (if (cas-op? e '/) (simp-quot (simplify (car (cdr e))) (simplify (car (cdr (cdr e)))))
            (if (cas-op? e '^) (simp-pow (simplify (car (cdr e))) (car (cdr (cdr e))))
              e)))))))

(define (simp-sum a b)
  (if (equal? a 0) b
    (if (equal? b 0) a
      (if (and (number? a) (number? b)) (+ a b)
        (list '+ a b)))))

(define (simp-diff a b)
  (if (equal? b 0) a
    (if (and (number? a) (number? b)) (- a b)
      (if (equal? a b) 0
        (list '- a b)))))

(define (simp-prod a b)
  (if (or (equal? a 0) (equal? b 0)) 0
    (if (equal? a 1) b
      (if (equal? b 1) a
        (if (and (number? a) (number? b)) (* a b)
          (list '* a b))))))

(define (simp-quot a b)
  (if (equal? a 0) 0
    (if (equal? b 1) a
      (list '/ a b))))

(define (simp-pow a n)
  (if (equal? n 0) 1
    (if (equal? n 1) a
      (if (and (number? a) (number? n)) (int-pow a n)
        (list '^ a n)))))

(define (int-pow a n)
  (if (= n 0) 1 (* a (int-pow a (- n 1)))))

; ============================================================
;  DIFFERENTIATION  (returns the derivative, unsimplified)
; ============================================================
; d/dvar of e.  Each branch corresponds to a named calculus rule.

(define (diff e var)
  (if (cas-const? e) 0                               ; constant rule
    (if (cas-var? e)
        (if (equal? e var) 1 0)                       ; variable rule
      (if (cas-op? e '+)
          (cas-sum (diff (car (cdr e)) var)           ; sum rule
                   (diff (car (cdr (cdr e))) var))
        (if (cas-op? e '-)
            (cas-diff (diff (car (cdr e)) var)        ; difference rule
                      (diff (car (cdr (cdr e))) var))
          (if (cas-op? e '*)
              (diff-product e var)                    ; product rule
            (if (cas-op? e '/)
                (diff-quotient e var)                 ; quotient rule
              (if (cas-op? e '^)
                  (diff-power e var)                  ; power rule
                (diff-elementary e var))))))))) ; sin/cos/exp/ln

; product rule: d(uv) = u dv + v du
(define (diff-product e var)
  (let ((u (car (cdr e))) (v (car (cdr (cdr e)))))
    (cas-sum (cas-prod u (diff v var))
             (cas-prod v (diff u var)))))

; quotient rule: d(u/v) = (v du - u dv) / v^2
(define (diff-quotient e var)
  (let ((u (car (cdr e))) (v (car (cdr (cdr e)))))
    (cas-quot (cas-diff (cas-prod v (diff u var))
                        (cas-prod u (diff v var)))
              (cas-pow v 2))))

; power rule: d(u^n) = n u^(n-1) du   (n a constant; chain rule on u)
(define (diff-power e var)
  (let ((u (car (cdr e))) (n (car (cdr (cdr e)))))
    (cas-prod (cas-prod n (cas-pow u (- n 1)))
              (diff u var))))

; elementary functions, with the chain rule
(define (diff-elementary e var)
  (let ((op (car e)) (u (car (cdr e))))
    (if (equal? op 'sin) (cas-prod (list 'cos u) (diff u var))
      (if (equal? op 'cos) (cas-prod (cas-prod -1 (list 'sin u)) (diff u var))
        (if (equal? op 'exp) (cas-prod (list 'exp u) (diff u var))
          (if (equal? op 'ln) (cas-prod (cas-quot 1 u) (diff u var))
            0))))))

; differentiate and simplify
(define (derivative e var)
  (simplify (diff e var)))

; ============================================================
;  BASIC INTEGRATION (the inverse, for simple polynomial cases)
; ============================================================
; integral of x^n dx = x^(n+1)/(n+1)   (n /= -1)

(define (integrate e var)
  (if (cas-const? e) (cas-prod e var)                ; ∫ c dx = c x
    (if (and (cas-var? e) (equal? e var)) (cas-quot (cas-pow var 2) 2)  ; ∫ x dx = x²/2
      (if (cas-op? e '+)
          (cas-sum (integrate (car (cdr e)) var)
                   (integrate (car (cdr (cdr e))) var))
        (if (cas-op? e '^)
            (int-power e var)
          (list 'integral e var))))))             ; unhandled: leave symbolic

(define (int-power e var)
  (let ((u (car (cdr e))) (n (car (cdr (cdr e)))))
    (if (and (equal? u var) (number? n) (not (= n -1)))
        (cas-quot (cas-pow var (+ n 1)) (+ n 1))
        (list 'integral e var))))

; ============================================================
;  PRETTY PRINTING
; ============================================================

(define (cas->string e)
  (if (cas-const? e) (number->string e)
    (if (cas-var? e) (symbol->string e)
      (if (pair? e)
          (let ((op (car e)))
            (if (member op '(+ - * /))
                (string-append "(" (cas->string (car (cdr e)))
                               " " (symbol->string op) " "
                               (cas->string (car (cdr (cdr e)))) ")")
              (if (equal? op '^)
                  (string-append (cas->string (car (cdr e))) "^"
                                 (cas->string (car (cdr (cdr e)))))
                (string-append (symbol->string op) "("
                               (cas->string (car (cdr e))) ")"))))
          "?"))))
