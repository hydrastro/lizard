; -*- lisp -*-
; lib/hm-infer.lisp — Hindley-Milner type inference (Algorithm W).
;
; A complete type inferencer with unification, let-polymorphism,
; generalization and instantiation — the algorithm behind ML,
; Haskell, and OCaml type checkers. Complements lizard's dependent
; kernel with the classic decidable inference engine.
;
; TYPES:
;   (tvar n)        type variable
;   int / bool      base types
;   (arrow a b)     function type  a -> b
;   (pair a b)      product type
; SCHEMES:
;   (mono t)              a monomorphic type
;   (forall (n ...) t)    a polymorphic type scheme
; EXPRESSIONS:
;   number / #t #f        literals
;   symbol                variable
;   (lam x body)          abstraction
;   (app f a)             application
;   (let x e body)        polymorphic let
;   (pair-of a b)         pair construction

; ============================================================
;  TYPE CONSTRUCTORS
; ============================================================

(define (tvar n) (list 'tvar n))
(define (tvar? t) (and (pair? t) (equal? (car t) 'tvar)))
(define (tvar-n t) (car (cdr t)))

(define (arrow a b) (list 'arrow a b))
(define (arrow? t) (and (pair? t) (equal? (car t) 'arrow)))
(define (arrow-dom t) (car (cdr t)))
(define (arrow-cod t) (car (cdr (cdr t))))

(define (tpair a b) (list 'tpair a b))
(define (tpair? t) (and (pair? t) (equal? (car t) 'tpair)))

; ============================================================
;  FRESH TYPE VARIABLES
; ============================================================

(define hm-counter (atom 0))
(define (fresh-tvar)
  (swap! hm-counter (lambda (n) (+ n 1)))
  (tvar (deref hm-counter)))

; ============================================================
;  SUBSTITUTIONS  (alist: tvar-n -> type)
; ============================================================

(define (subst-empty) '())

(define (subst-lookup s n)
  (let ((b (assoc n s))) (if b (cdr b) #f)))

; Apply a substitution to a type (recursively).
(define (apply-subst s t)
  (if (tvar? t)
      (let ((v (subst-lookup s (tvar-n t))))
        (if v (apply-subst s v) t))
    (if (arrow? t)
        (arrow (apply-subst s (arrow-dom t)) (apply-subst s (arrow-cod t)))
      (if (tpair? t)
          (tpair (apply-subst s (car (cdr t))) (apply-subst s (car (cdr (cdr t)))))
          t))))

; Compose: apply s2 after s1.
(define (subst-compose s2 s1)
  (append
    (map (lambda (b) (cons (car b) (apply-subst s2 (cdr b)))) s1)
    s2))

(define (subst-extend n t s) (cons (cons n t) s))

; ============================================================
;  UNIFICATION
; ============================================================
; Returns a substitution, or 'fail.

(define (occurs? n t)
  (if (tvar? t) (equal? (tvar-n t) n)
    (if (arrow? t) (or (occurs? n (arrow-dom t)) (occurs? n (arrow-cod t)))
      (if (tpair? t) (or (occurs? n (car (cdr t))) (occurs? n (car (cdr (cdr t)))))
        #f))))

(define (unify t1 t2)
  (if (and (tvar? t1) (tvar? t2) (equal? (tvar-n t1) (tvar-n t2)))
      (subst-empty)
    (if (tvar? t1)
        (if (occurs? (tvar-n t1) t2)
            (begin (display "  TYPE ERROR: occurs check") (newline) 'fail)
            (subst-extend (tvar-n t1) t2 (subst-empty)))
      (if (tvar? t2)
          (unify t2 t1)
        (if (and (arrow? t1) (arrow? t2))
            (let ((s1 (unify (arrow-dom t1) (arrow-dom t2))))
              (if (equal? s1 'fail) 'fail
                (let ((s2 (unify (apply-subst s1 (arrow-cod t1))
                                 (apply-subst s1 (arrow-cod t2)))))
                  (if (equal? s2 'fail) 'fail (subst-compose s2 s1)))))
          (if (and (tpair? t1) (tpair? t2))
              (let ((s1 (unify (car (cdr t1)) (car (cdr t2)))))
                (if (equal? s1 'fail) 'fail
                  (let ((s2 (unify (apply-subst s1 (car (cdr (cdr t1))))
                                   (apply-subst s1 (car (cdr (cdr t2)))))))
                    (if (equal? s2 'fail) 'fail (subst-compose s2 s1)))))
            (if (equal? t1 t2) (subst-empty)
              (begin (display "  TYPE ERROR: cannot unify ")
                     (display t1) (display " with ") (display t2) (newline)
                     'fail))))))))

; ============================================================
;  TYPE SCHEMES  (let-polymorphism)
; ============================================================

(define (mono t) (list 'mono t))
(define (forall vars t) (list 'forall vars t))
(define (scheme-mono? s) (equal? (car s) 'mono))

; Instantiate a scheme: replace quantified vars with fresh tvars.
(define (instantiate s)
  (if (scheme-mono? s)
      (car (cdr s))
      (let ((vars (car (cdr s)))
            (t (car (cdr (cdr s)))))
        (let ((sub (map (lambda (v) (cons v (fresh-tvar))) vars)))
          (apply-subst sub t)))))

; Free type variables of a type.
(define (ftv t)
  (if (tvar? t) (list (tvar-n t))
    (if (arrow? t) (union-set (ftv (arrow-dom t)) (ftv (arrow-cod t)))
      (if (tpair? t) (union-set (ftv (car (cdr t))) (ftv (car (cdr (cdr t)))))
        '()))))

(define (union-set a b)
  (if (null? a) b
    (if (member (car a) b) (union-set (cdr a) b)
      (cons (car a) (union-set (cdr a) b)))))

(define (diff-set a b)
  (filter (lambda (x) (not (member x b))) a))

; Free type variables of an environment.
(define (env-ftv env)
  (fold-left (lambda (acc binding)
               (let ((s (cdr binding)))
                 (union-set acc
                   (if (scheme-mono? s) (ftv (car (cdr s)))
                     (diff-set (ftv (car (cdr (cdr s)))) (car (cdr s)))))))
             '()
             env))

; Generalize: quantify type vars free in t but not in the env.
(define (generalize env t)
  (let ((vars (diff-set (ftv t) (env-ftv env))))
    (if (null? vars) (mono t) (forall vars t))))

; ============================================================
;  TYPE ENVIRONMENT
; ============================================================

(define (env-empty) '())
(define (env-add env x scheme) (cons (cons x scheme) env))
(define (env-lookup env x)
  (let ((b (assoc x env))) (if b (cdr b) #f)))
(define (env-apply-subst s env)
  (map (lambda (b)
         (cons (car b)
               (let ((sch (cdr b)))
                 (if (scheme-mono? sch)
                     (mono (apply-subst s (car (cdr sch))))
                     (forall (car (cdr sch))
                             (apply-subst s (car (cdr (cdr sch)))))))))
       env))

; ============================================================
;  ALGORITHM W  —  infer : env x expr -> (type . subst)
; ============================================================

(define (infer env expr)
  (if (number? expr) (cons 'int (subst-empty))
    (if (equal? expr #t) (cons 'bool (subst-empty))
      (if (equal? expr #f) (cons 'bool (subst-empty))
        (if (symbol? expr) (infer-var env expr)
          (if (pair? expr) (infer-form env expr)
            (cons 'unknown (subst-empty))))))))

(define (infer-var env x)
  (let ((sch (env-lookup env x)))
    (if sch
        (cons (instantiate sch) (subst-empty))
        (begin (display "  TYPE ERROR: unbound variable ") (display x) (newline)
               (cons 'error (subst-empty))))))

(define (infer-form env expr)
  (let ((head (car expr)))
    (if (equal? head 'lam) (infer-lam env expr)
      (if (equal? head 'app) (infer-app env expr)
        (if (equal? head 'let) (infer-let env expr)
          (if (equal? head 'pair-of) (infer-pair env expr)
            (cons 'unknown (subst-empty))))))))

; (lam x body): fresh tvar for x, infer body, build arrow.
(define (infer-lam env expr)
  (let ((x (car (cdr expr)))
        (body (car (cdr (cdr expr)))))
    (let ((tv (fresh-tvar)))
      (let ((r (infer (env-add env x (mono tv)) body)))
        (let ((t-body (car r)) (s (cdr r)))
          (cons (arrow (apply-subst s tv) t-body) s))))))

; (app f a): infer f and a, unify f's type with (a-type -> fresh).
(define (infer-app env expr)
  (let ((f (car (cdr expr)))
        (a (car (cdr (cdr expr)))))
    (let ((rf (infer env f)))
      (let ((tf (car rf)) (sf (cdr rf)))
        (let ((ra (infer (env-apply-subst sf env) a)))
          (let ((ta (car ra)) (sa (cdr ra)))
            (let ((tv (fresh-tvar)))
              (let ((s3 (unify (apply-subst sa tf) (arrow ta tv))))
                (if (equal? s3 'fail)
                    (cons 'error (subst-empty))
                    (cons (apply-subst s3 tv)
                          (subst-compose s3 (subst-compose sa sf))))))))))))

; (let x e body): infer e, generalize, infer body in extended env.
(define (infer-let env expr)
  (let ((x (car (cdr expr)))
        (e (car (cdr (cdr expr))))
        (body (car (cdr (cdr (cdr expr))))))
    (let ((re (infer env e)))
      (let ((te (car re)) (se (cdr re)))
        (let ((env2 (env-apply-subst se env)))
          (let ((scheme (generalize env2 te)))
            (let ((rb (infer (env-add env2 x scheme) body)))
              (cons (car rb) (subst-compose (cdr rb) se)))))))))

(define (infer-pair env expr)
  (let ((ra (infer env (car (cdr expr)))))
    (let ((rb (infer (env-apply-subst (cdr ra) env) (car (cdr (cdr expr))))))
      (cons (tpair (apply-subst (cdr rb) (car ra)) (car rb))
            (subst-compose (cdr rb) (cdr ra))))))

; ============================================================
;  TOP-LEVEL: infer the (principal) type of a closed expression
; ============================================================

(define (type-of expr)
  (let ((r (infer (env-empty) expr)))
    (apply-subst (cdr r) (car r))))

; Pretty-print a type.
(define (type->string t)
  (if (tvar? t) (string-append "t" (number->string (tvar-n t)))
    (if (arrow? t)
        (string-append "(" (type->string (arrow-dom t))
                       " -> " (type->string (arrow-cod t)) ")")
      (if (tpair? t)
          (string-append "(" (type->string (car (cdr t)))
                         " * " (type->string (car (cdr (cdr t)))) ")")
        (symbol->string t)))))
