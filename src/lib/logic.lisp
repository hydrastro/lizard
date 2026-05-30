; -*- lisp -*-
; lib/logic.lisp — A Prolog-style logic engine in lizard.
;
; Features: unification, a clause database (facts + rules),
; SLD resolution with backtracking, and multiple solutions.
;
; TERMS:
;   variables  : symbols beginning with "?"  (e.g. ?X, ?who)
;   atoms      : other symbols and numbers
;   compounds  : lists (functor arg ...)     (e.g. (parent tom bob))
;
; Internally a variable is tagged (lvar name). The 'lvar functor
; is reserved.

; ============================================================
;  VARIABLE REPRESENTATION
; ============================================================

(define (lvar name) (list 'lvar name))
(define (lvar? x) (and (pair? x) (equal? (car x) 'lvar)))
(define (lvar-name v) (car (cdr v)))

; Does a symbol denote a variable (starts with "?")?
(define (var-symbol? sym)
  (if (symbol? sym)
      (let ((s (symbol->string sym)))
        (and (> (string-length s) 0)
             (equal? (string-ref s 0) "?")))
      #f))

; Convert a quoted term, turning ?X symbols into (lvar ?X).
(define (parse-term t)
  (if (var-symbol? t) (lvar t)
    (if (pair? t)
        (cons (parse-term (car t)) (parse-term (cdr t)))
        t)))

; ============================================================
;  SUBSTITUTIONS
; ============================================================
; A substitution is an alist of (var-name . term).
; 'fail is a sentinel meaning "no substitution / failure".

(define (subst-empty) '())

(define (subst-lookup name subst)
  (if (null? subst) #f
    (if (equal? (car (car subst)) name)
        (car subst)
        (subst-lookup name (cdr subst)))))

(define (subst-extend name val subst)
  (cons (cons name val) subst))

; walk: resolve a term as far as the substitution allows.
(define (walk t subst)
  (if (lvar? t)
      (let ((b (subst-lookup (lvar-name t) subst)))
        (if b (walk (cdr b) subst) t))
      t))

; ============================================================
;  UNIFICATION
; ============================================================

(define (unify t1 t2 subst)
  (if (equal? subst 'fail) 'fail
    (let ((a (walk t1 subst))
          (b (walk t2 subst)))
      (if (and (lvar? a) (lvar? b) (equal? (lvar-name a) (lvar-name b)))
          subst
        (if (lvar? a)
            (subst-extend (lvar-name a) b subst)
          (if (lvar? b)
              (subst-extend (lvar-name b) a subst)
            (if (and (pair? a) (pair? b))
                (unify-pairs a b subst)
              (if (equal? a b) subst 'fail))))))))

(define (unify-pairs a b subst)
  (let ((s (unify (car a) (car b) subst)))
    (if (equal? s 'fail) 'fail
      (unify (cdr a) (cdr b) s))))

; ============================================================
;  CLAUSE DATABASE
; ============================================================
; A clause is (clause head body), body a list of goals.
; A fact has an empty body.

(define (db-fact head) (list 'clause (parse-term head) '()))

(define (db-rule head body)
  (list 'clause (parse-term head) (map parse-term body)))

(define (clause-head c) (car (cdr c)))
(define (clause-body c) (car (cdr (cdr c))))

; ============================================================
;  FRESH RENAMING (rename clause variables apart per use)
; ============================================================

(define gen-counter (atom 0))
(define (next-gen)
  (swap! gen-counter (lambda (n) (+ n 1)))
  (deref gen-counter))

; Rename every variable in a term by appending a generation tag.
(define (rename-term t gen)
  (if (lvar? t)
      (lvar (string-append (symbol->string (lvar-name t))
                           "#"
                           (number->string gen)))
    (if (pair? t)
        (cons (rename-term (car t) gen) (rename-term (cdr t) gen))
        t)))

; ============================================================
;  RESOLUTION (returns a list of solution substitutions)
; ============================================================

; Solve a list of goals; returns all substitutions that satisfy them.
(define (solve-goals goals db subst)
  (if (equal? subst 'fail) '()
    (if (null? goals) (list subst)
      (apply-append-l
        (map (lambda (s) (solve-goals (cdr goals) db s))
             (solve-goal (car goals) db subst))))))

; Solve a single goal against every clause in the database.
(define (solve-goal goal db subst)
  (apply-append-l
    (map (lambda (clause)
           (let ((gen (next-gen)))
             (let ((h (rename-term (clause-head clause) gen))
                   (b (rename-term-list (clause-body clause) gen)))
               (let ((s (unify goal h subst)))
                 (if (equal? s 'fail) '()
                   (solve-goals b db s))))))
         db)))

(define (rename-term-list lst gen)
  (map (lambda (g) (rename-term g gen)) lst))

; concat a list of lists
(define (apply-append-l lists)
  (if (null? lists) '()
    (append (car lists) (apply-append-l (cdr lists)))))

; ============================================================
;  QUERY INTERFACE
; ============================================================

; Run a query (a goal). Returns all solution substitutions.
(define (query db goal)
  (solve-goals (list (parse-term goal)) db (subst-empty)))

; Resolve a variable's value fully under a substitution.
(define (resolve var-sym subst)
  (deep-walk (lvar var-sym) subst))

(define (deep-walk t subst)
  (let ((w (walk t subst)))
    (if (pair? w)
        (cons (deep-walk (car w) subst) (deep-walk (cdr w) subst))
        w)))

; Get the value of one variable across all solutions.
(define (query-var db goal var-sym)
  (map (lambda (s) (resolve var-sym s))
       (query db goal)))

; Does the query have at least one solution?
(define (provable? db goal)
  (not (null? (query db goal))))

(define (solution-count db goal)
  (length (query db goal)))
