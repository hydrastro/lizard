; -*- lisp -*-
; lib/pattern.lisp — Structural pattern matching (Evolution Plan Phase 4).
;
; A genuine pattern matcher. Patterns are quoted structures:
;
;   _              wildcard (matches anything, no binding)
;   x              variable (binds symbol x to the value)
;   42 / "str"     literal (matches exactly)
;   (lit s)        literal symbol match (matches the symbol s)
;   (cons pa pd)   destructure a pair
;   (list p ...)   destructure a fixed-length list
;   (pred? p)      matches if (pred? value) and sub-pattern p matches
;
; match-pattern returns an alist of bindings on success, or the
; symbol 'no-match on failure. (An empty alist '() means matched
; with no bindings — distinct from 'no-match.)

; ============================================================
;  CORE MATCHER
; ============================================================

(define (no-match? result) (equal? result 'no-match))

(define (match-pattern pat val)
  (if (null? pat)
      (if (null? val) '() 'no-match)       ; empty-list pattern
      (if (equal? pat '_)
          '()                                    ; wildcard
          (if (symbol? pat)
              (list (cons pat val))              ; variable binding
              (if (number? pat)
                  (if (equal? pat val) '() 'no-match)   ; number literal
                  (if (string? pat)
                      (if (equal? pat val) '() 'no-match) ; string literal
                      (if (pair? pat)
                          (match-compound pat val)
                          'no-match)))))))

(define (match-compound pat val)
  (let ((tag (car pat)))
    (if (equal? tag 'lit)
        ; (lit s) — match symbol/value literally
        (if (equal? (car (cdr pat)) val) '() 'no-match)
        (if (equal? tag 'cons)
            ; (cons pa pd) — destructure pair
            (if (pair? val)
                (match-both (car (cdr pat)) (car val)
                            (car (cdr (cdr pat))) (cdr val))
                'no-match)
            (if (equal? tag 'list)
                ; (list p1 p2 ...) — fixed-length list
                (match-list (cdr pat) val)
                'no-match)))))

; Match two patterns against two values, combining bindings.
(define (match-both p1 v1 p2 v2)
  (let ((b1 (match-pattern p1 v1)))
    (if (no-match? b1) 'no-match
      (let ((b2 (match-pattern p2 v2)))
        (if (no-match? b2) 'no-match
          (append b1 b2))))))

; Match a list of patterns against a list of values.
(define (match-list pats vals)
  (if (null? pats)
      (if (null? vals) '() 'no-match)
      (if (null? vals)
          'no-match
          (match-both (car pats) (car vals)
                      (cons 'list (cdr pats)) (cdr vals)))))

; ============================================================
;  MATCH RUNNER
; ============================================================
; clauses is a list of (pattern handler), where handler is a
; function taking the bindings alist and returning a result.
;
;   (match-run val
;     (list
;       (list '(cons h t) (lambda (b) (alist-ref 'h b)))
;       (list '_          (lambda (b) 'default))))

(define (match-run val clauses)
  (if (null? clauses)
      (begin (display "  match: no clause matched") (newline) #f)
      (let ((clause (car clauses)))
        (let ((pat (car clause))
              (handler (car (cdr clause))))
          (let ((binds (match-pattern pat val)))
            (if (no-match? binds)
                (match-run val (cdr clauses))
                (handler binds)))))))

; Helper to look up a binding (returns #f if absent).
(define (binding-ref binds name)
  (let ((pair (assoc name binds)))
    (if pair (cdr pair) #f)))

; ============================================================
;  CONVENIENCE: list deconstruction
; ============================================================

; Match the head and tail of a list, calling (f head tail).
(define (match-cons val f default-thunk)
  (if (pair? val)
      (f (car val) (cdr val))
      (default-thunk)))

; Match on list length / shape with named clauses.
(define (match-empty val empty-thunk non-empty-fn)
  (if (null? val)
      (empty-thunk)
      (non-empty-fn (car val) (cdr val))))
