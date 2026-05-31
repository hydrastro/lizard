; -*- lisp -*-
; examples/140-hygienic-macros.lisp
;
; Phase 4 — hygienic macros with ellipsis.  syntax-rules now supports the
; ellipsis (...) for variadic patterns and templates — basic, recursive,
; parallel, and nested (depth-2) — and the expander renames template-introduced
; binders so a macro cannot capture the caller's identifiers (hygiene).  Macros
; defined in a module and imported expand the same way.
;
; Self-checking: each `must` raises if its claim is false, so a clean exit means
; every expansion produced the expected value.

(import "macros.lisp")   ; my-or, my-and, unless, swap! — hygienic, ellipsis

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'macro-regression)))

(newline)
(display "Ellipsis in syntax-rules:") (newline)

; basic ellipsis: zero-or-more sub-forms
(define-syntax my-list
  (syntax-rules () ((_ x ...) (list x ...))))
(must "basic    (my-list 1 2 3 4) = (1 2 3 4)"
      (equal? (my-list 1 2 3 4) (list 1 2 3 4)))

; parallel ellipsis: two sequences (name ... and val ...) in lockstep
(define-syntax my-let
  (syntax-rules ()
    ((_ ((name val) ...) body ...)
     ((lambda (name ...) body ...) val ...))))
(must "parallel (my-let ((a 3)(b 4)) (+ a b)) = 7"
      (equal? (my-let ((a 3) (b 4)) (+ a b)) 7))

; nested (depth-2) ellipsis: (x ...) ...  with template (+ x ...) ...
(define-syntax sums
  (syntax-rules () ((_ (x ...) ...) (list (+ x ...) ...))))
(must "nested   per-row sums = (6 30 100)"
      (equal? (sums (1 2 3) (10 20) (100)) (list 6 30 100)))

(newline)
(display "Imported macros (defined in lib/macros.lisp):") (newline)

(must "recursive ellipsis (my-or #f #f 7 #f) = 7"
      (equal? (my-or #f #f 7 #f) 7))
(must "short-circuit (my-and 1 #f 3) = #f"
      (equal? (my-and 1 #f 3) #f))
(must "body form (unless #f 42) = 42"
      (equal? (unless #f 42) 42))

(newline)
(display "Hygiene — a macro's temporaries do not capture caller names:") (newline)

; my-or introduces `t`; the caller's own `t` must survive unshadowed.
(define t 99)
(must "(my-or #f t) = 99, not the macro's internal t"
      (equal? (my-or #f t) 99))

; swap! introduces `tmp`; swapping a caller variable literally named `tmp` works.
(define tmp 1)
(define other 2)
(swap! tmp other)
(must "swap! with a caller var named tmp: tmp=2, other=1"
      (and (equal? tmp 2) (equal? other 1)))

(newline)
(display "ellipsis + hygiene working, including across import") (newline)
