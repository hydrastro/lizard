; -*- lisp -*-
; lib/macro-expand.lisp — Recursive + hygienic macro expansion (Track R).
;
; Builds on lib/syntax-rules.lisp:
;   * a macro environment (name -> macro)
;   * expand-all: recursively expand macros throughout a form, to a
;     fixpoint (handles macros that expand into other macros)
;   * gensym + hygiene: template-introduced identifiers are renamed
;     to fresh names so they cannot capture the user's variables

(import "match.lisp")
(import "syntax-rules.lisp")

; ============================================================
;  GENSYM
; ============================================================

(define gensym-counter (atom 0))

(define (gensym base)
  (swap! gensym-counter (lambda (n) (+ n 1)))
  (string->symbol
    (string-append (symbol->string base) "." (number->string (deref gensym-counter)))))

; ============================================================
;  MACRO ENVIRONMENT
; ============================================================

(define (make-macro-env) '())

(define (macro-env-add menv name macro)
  (cons (cons name macro) menv))

(define (macro-lookup name menv)
  (let ((b (assoc name menv)))
    (if b (cdr b) #f)))

; ============================================================
;  RECURSIVE EXPANSION
; ============================================================
; Expand a form fully: if its head is a macro, expand and re-expand
; the result; otherwise recurse into all sub-forms.

(define (expand-all form menv)
  (if (pair? form)
      (if (and (symbol? (car form)) (macro-lookup (car form) menv))
          (expand-all (macro-apply (macro-lookup (car form) menv) form) menv)
          (expand-each form menv))
      form))

(define (expand-each forms menv)
  (if (pair? forms)
      (cons (expand-all (car forms) menv) (expand-each (cdr forms) menv))
      forms))

; Expand exactly one level (for inspecting intermediate steps).
(define (expand-once form menv)
  (if (and (pair? form) (symbol? (car form)) (macro-lookup (car form) menv))
      (macro-apply (macro-lookup (car form) menv) form)
      form))

; ============================================================
;  HYGIENE
; ============================================================
; A hygienic macro names the identifiers its template INTRODUCES
; (binders not coming from the pattern). On each use, those are
; renamed to fresh gensyms, so they cannot capture user variables.

(define (make-hygienic-macro literals fresh-ids rules)
  (list 'hmacro literals fresh-ids rules))

(define (hmacro? x) (and (pair? x) (equal? (car x) 'hmacro)))
(define (hmacro-literals m) (car (cdr m)))
(define (hmacro-fresh m) (car (cdr (cdr m))))
(define (hmacro-rules m) (car (cdr (cdr (cdr m)))))

; Rename a set of identifiers throughout a template.
(define (rename-ids tmpl rmap)
  (if (symbol? tmpl)
      (let ((b (assoc tmpl rmap)))
        (if b (cdr b) tmpl))
    (if (pair? tmpl)
        (cons (rename-ids (car tmpl) rmap) (rename-ids (cdr tmpl) rmap))
        tmpl)))

; Apply a hygienic macro: freshen its introduced ids, then expand.
(define (hmacro-apply m form)
  (let ((rmap (map (lambda (id) (cons id (gensym id))) (hmacro-fresh m))))
    (let ((fresh-rules
            (map (lambda (r)
                   (list (car r) (rename-ids (car (cdr r)) rmap)))
                 (hmacro-rules m))))
      (try-rules fresh-rules (hmacro-literals m) form))))

; Unified expander that handles both plain and hygienic macros.
(define (expand-all-h form menv)
  (if (pair? form)
      (let ((m (and (symbol? (car form)) (macro-lookup (car form) menv))))
        (if m
            (if (hmacro? m)
                (expand-all-h (hmacro-apply m form) menv)
                (expand-all-h (macro-apply m form) menv))
            (expand-each-h form menv)))
      form))

(define (expand-each-h forms menv)
  (if (pair? forms)
      (cons (expand-all-h (car forms) menv) (expand-each-h (cdr forms) menv))
      forms))
