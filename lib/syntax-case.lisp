; -*- lisp -*-
; lib/syntax-case.lisp — Procedural macros via syntax-case (Track R).
;
; syntax-case is strictly more powerful than syntax-rules: each
; clause is (pattern fender transformer) where
;   * pattern    — a syntax-rules pattern (vars, literals, _, ...)
;   * fender     — a predicate on the bindings (a guard); #t = always
;   * transformer— a PROCEDURE (bindings -> output form)
; so the right-hand side can run arbitrary code, inspect the matched
; sub-forms, compute, and assemble output programmatically.

(import "match.lisp")
(import "syntax-rules.lisp")

; ============================================================
;  BINDING ACCESS (for transformers)
; ============================================================

; Look up a pattern variable's value in the bindings.
(define (sc-ref binds var)
  (let ((b (assoc var binds)))
    (if b
        (if (and (pair? (cdr b)) (equal? (car (cdr b)) 'ellipsis))
            (cdr (cdr b))            ; ellipsis var -> list of values
            (cdr b))                 ; normal var
        #f)))

; Is a variable an ellipsis (sequence) binding?
(define (sc-seq? binds var)
  (let ((b (assoc var binds)))
    (and b (pair? (cdr b)) (equal? (car (cdr b)) 'ellipsis))))

; ============================================================
;  SYNTAX-CASE CLAUSES
; ============================================================
; A clause: (list pattern fender transformer)

(define (sc-clause pattern fender transformer)
  (list pattern fender transformer))

(define (clause-pattern c) (car c))
(define (clause-fender c) (car (cdr c)))
(define (clause-transformer c) (car (cdr (cdr c))))

; ============================================================
;  THE MATCHER / DISPATCHER
; ============================================================
; Try each clause: match the pattern, run the fender on the
; bindings, and if it passes, call the transformer.

(define (syntax-case form lits clauses)
  (if (null? clauses)
      (begin (display "  syntax-case: no clause matched ") (display form) (newline)
             form)
    (let ((c (car clauses)))
      (let ((binds (sr-match (clause-pattern c) form lits)))
        (if (equal? binds 'fail)
            (syntax-case form lits (cdr clauses))
            (if (run-fender (clause-fender c) binds)
                ((clause-transformer c) binds)
                (syntax-case form lits (cdr clauses))))))))

; A fender is either #t (always pass) or a procedure binds -> bool.
(define (run-fender fender binds)
  (if (equal? fender #t)
      #t
      (fender binds)))

; ============================================================
;  A PROCEDURAL MACRO
; ============================================================

(define (make-sc-macro literals clauses)
  (list 'sc-macro literals clauses))

(define (sc-macro? x) (and (pair? x) (equal? (car x) 'sc-macro)))
(define (sc-macro-lits m) (car (cdr m)))
(define (sc-macro-clauses m) (car (cdr (cdr m))))

(define (sc-apply m form)
  (syntax-case form (sc-macro-lits m) (sc-macro-clauses m)))

; ============================================================
;  HELPERS FOR BUILDING OUTPUT FORMS
; ============================================================

; Wrap a body (a list of forms) in a begin if there's more than one.
(define (sc-body forms)
  (if (null? forms) '()
    (if (null? (cdr forms))
        (car forms)
        (cons 'begin forms))))

; Map a transformer over an ellipsis variable's values.
(define (sc-map f seq) (map f seq))
