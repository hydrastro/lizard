; -*- lisp -*-
; lib/lambda-calc.lisp — Untyped lambda calculus in lizard.
;
; Terms:
;   x              variable (any symbol other than 'lam / 'app)
;   (lam x body)   abstraction  λx. body
;   (app f a)      application  (f a)
;
; Implements free variables, capture-avoiding substitution,
; normal-order (leftmost-outermost) beta reduction, and
; normalization to normal form (with a step bound).

; ============================================================
;  TERM PREDICATES / ACCESSORS
; ============================================================

(define (lc-var? t) (symbol? t))
(define (lc-lam? t) (and (pair? t) (equal? (car t) 'lam)))
(define (lc-app? t) (and (pair? t) (equal? (car t) 'app)))

(define (lam-var t)  (car (cdr t)))
(define (lam-body t) (car (cdr (cdr t))))
(define (app-fn t)   (car (cdr t)))
(define (app-arg t)  (car (cdr (cdr t))))

; ============================================================
;  FREE VARIABLES
; ============================================================

(define (lc-remove x lst)
  (filter (lambda (y) (not (equal? x y))) lst))

(define (lc-union a b)
  (if (null? a) b
    (if (member (car a) b)
        (lc-union (cdr a) b)
        (cons (car a) (lc-union (cdr a) b)))))

(define (free-vars t)
  (if (lc-var? t) (list t)
    (if (lc-lam? t) (lc-remove (lam-var t) (free-vars (lam-body t)))
      (if (lc-app? t)
          (lc-union (free-vars (app-fn t)) (free-vars (app-arg t)))
          '()))))

; ============================================================
;  FRESH VARIABLES
; ============================================================

(define lc-counter (atom 0))
(define (lc-fresh base)
  (swap! lc-counter (lambda (n) (+ n 1)))
  (string->symbol
    (string-append (symbol->string base) "%" (number->string (deref lc-counter)))))

; ============================================================
;  CAPTURE-AVOIDING SUBSTITUTION  [x := s] t
; ============================================================

(define (lc-subst t x s)
  (if (lc-var? t)
      (if (equal? t x) s t)
    (if (lc-app? t)
        (list 'app (lc-subst (app-fn t) x s) (lc-subst (app-arg t) x s))
      (if (lc-lam? t)
          (let ((y (lam-var t)))
            (if (equal? y x)
                t                                  ; x is bound here
              (if (member y (free-vars s))
                  ; would capture: alpha-rename y to a fresh name
                  (let ((fresh (lc-fresh y)))
                    (list 'lam fresh
                          (lc-subst (lc-subst (lam-body t) y fresh) x s)))
                  (list 'lam y (lc-subst (lam-body t) x s)))))
          t))))

; ============================================================
;  BETA REDUCTION (normal order: leftmost-outermost)
; ============================================================
; Returns (reduced t') if a step was taken, or (normal t) otherwise.

(define (reduced? r) (equal? (car r) 'reduced))
(define (step-term r) (car (cdr r)))

(define (beta-step t)
  (if (lc-app? t)
      (if (lc-lam? (app-fn t))
          ; redex: (app (lam x body) arg) → [x := arg] body
          (list 'reduced
                (lc-subst (lam-body (app-fn t)) (lam-var (app-fn t)) (app-arg t)))
          ; reduce the function part first
          (let ((f (beta-step (app-fn t))))
            (if (reduced? f)
                (list 'reduced (list 'app (step-term f) (app-arg t)))
                ; then the argument
                (let ((a (beta-step (app-arg t))))
                  (if (reduced? a)
                      (list 'reduced (list 'app (app-fn t) (step-term a)))
                      (list 'normal t))))))
    (if (lc-lam? t)
        (let ((b (beta-step (lam-body t))))
          (if (reduced? b)
              (list 'reduced (list 'lam (lam-var t) (step-term b)))
              (list 'normal t)))
        (list 'normal t))))

; Normalize: reduce until normal form or the step bound is hit.
(define (lc-normalize t max-steps)
  (if (= max-steps 0) t
    (let ((r (beta-step t)))
      (if (reduced? r)
          (lc-normalize (step-term r) (- max-steps 1))
          t))))

; Count reduction steps to normal form (bounded).
(define (lc-steps t max-steps)
  (define (go t n)
    (if (= n max-steps) n
      (let ((r (beta-step t)))
        (if (reduced? r) (go (step-term r) (+ n 1)) n))))
  (go t 0))

; ============================================================
;  CHURCH ENCODING HELPERS
; ============================================================

; Build the Church numeral term for n: λf.λx. f (f ... (f x))
(define (church-term n)
  (list 'lam 'f (list 'lam 'x (church-body n))))

(define (church-body n)
  (if (= n 0) 'x
    (list 'app 'f (church-body (- n 1)))))

; Decode a normal-form Church numeral back to an integer.
; Counts the applications of f in λf.λx. <body>.
(define (church-decode t)
  (if (lc-lam? t)
      (church-decode (lam-body t))
    (count-apps t)))

(define (count-apps t)
  (if (lc-app? t)
      (+ 1 (count-apps (app-arg t)))
      0))

; Church arithmetic terms.
(define church-succ
  '(lam n (lam f (lam x (app f (app (app n f) x))))))

(define church-plus
  '(lam m (lam n (lam f (lam x (app (app m f) (app (app n f) x)))))))

(define church-mult
  '(lam m (lam n (lam f (app m (app n f))))))
