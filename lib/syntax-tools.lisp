; -*- lisp -*-
; lib/syntax-tools.lisp — Syntax object helpers (Track R).
;
; Builds on the syntax-object primitives:
;   datum->syntax, syntax->datum, syntax-e, syntax-source, syntax?
;
; Syntax objects wrap a datum together with lexical and source
; context. These helpers make it easier to inspect and transform
; them — the foundation for a hygienic macro system.

; ============================================================
;  CONSTRUCTION + INSPECTION
; ============================================================

; Wrap a datum as a syntax object (no source context).
(define (syntax-quote datum)
  (datum->syntax #f datum))

; Strip syntax wrapping back to a plain datum.
(define (syntax-unwrap stx)
  (if (syntax? stx)
      (syntax->datum stx)
      stx))

; Is this a syntax object wrapping a list?
(define (syntax-list? stx)
  (and (syntax? stx)
       (pair? (syntax-e stx))))

; Is this a syntax object wrapping a symbol?
(define (syntax-symbol? stx)
  (and (syntax? stx)
       (symbol? (syntax-e stx))))

; ============================================================
;  TEMPLATE EXPANSION
; ============================================================
; A simple template system: substitute bindings into a template.
; A template is an s-expression; symbols starting with the
; binding keys get replaced.

; Look up a symbol in an alist of (symbol . value) bindings.
(define (template-lookup sym bindings)
  (let ((pair (assoc sym bindings)))
    (if pair (cdr pair) sym)))

; Expand a template by substituting bindings throughout.
(define (template-expand template bindings)
  (if (pair? template)
      (cons (template-expand (car template) bindings)
            (template-expand (cdr template) bindings))
      (if (symbol? template)
          (template-lookup template bindings)
          template)))

; ============================================================
;  COMMON MACRO PATTERNS (as functions)
; ============================================================
; These show how macros would expand. Since lizard evaluates
; eagerly, they operate on quoted forms.

; (swap-args '(f a b)) => '(f b a)
(define (swap-args form)
  (if (and (pair? form) (pair? (cdr form)) (pair? (cdr (cdr form))))
      (list (car form)
            (car (cdr (cdr form)))
            (car (cdr form)))
      form))

; Build a let-expression form from bindings and body.
; (make-let '((x 1) (y 2)) 'body) => '(let ((x 1) (y 2)) body)
(define (make-let bindings body)
  (list 'let bindings body))

; Build a lambda form.
(define (make-lambda params body)
  (list 'lambda params body))

; Build an if form.
(define (make-if test then else)
  (list 'if test then else))

; ============================================================
;  AST WALKING
; ============================================================
; Walk an s-expression form, applying a transform to each node.

(define (walk-form f form)
  (let ((transformed (f form)))
    (if (pair? transformed)
        (cons (walk-form f (car transformed))
              (walk-form f (cdr transformed)))
        transformed)))

; Count the nodes in a form.
(define (count-nodes form)
  (if (pair? form)
      (+ 1 (count-nodes (car form)) (count-nodes (cdr form)))
      1))

; Collect all symbols appearing in a form.
(define (collect-symbols form)
  (if (pair? form)
      (append (collect-symbols (car form))
              (collect-symbols (cdr form)))
      (if (symbol? form) (list form) '())))
