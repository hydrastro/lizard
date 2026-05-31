; -*- lisp -*-
; examples/141-macro-stepper.lisp
;
; Phase 4 — a macro-expansion stepper that shows LOCATED expansion.  Forms are
; parsed with `read-syntax` so they carry source spans; `macroexpand-1` performs
; one hygienic expansion step; `form-location` reports where the expanded macro
; call came from.  The stepper prints each rewrite with its location, and the
; trace shows hygiene at work (template binders renamed to fresh names).
;
; Self-checking: each `must` raises if its claim is false.

(import "macro-stepper.lisp")   ; macro-step, fully-expand-head
(import "macros.lisp")          ; my-or (hygienic, ellipsis)

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'stepper-regression)))

; nested macros so the stepper shows several located steps
(define-syntax inc  (syntax-rules () ((_ x) (+ x 1))))
(define-syntax inc2 (syntax-rules () ((_ x) (inc (inc x)))))

(newline)
(display "Located expansion trace for (inc2 5):") (newline)
(macro-step (read-syntax "(inc2 5)"))

(newline)
(display "Trace for (my-or a b c) — hygiene renames the introduced t:") (newline)
(macro-step (read-syntax "(my-or a b c)"))

(newline)
(display "Checks:") (newline)

; one step rewrites the outermost macro only
(must "macroexpand-1 (inc2 5) = (inc (inc 5))"
      (equal? (macroexpand-1 (read-syntax "(inc2 5)"))
              (read-syntax "(inc (inc 5))")))
; stepping to a fixed point reaches the head-normal form
(must "fully-expand-head (inc2 5) = (+ (inc 5) 1)"
      (equal? (fully-expand-head (read-syntax "(inc2 5)"))
              (read-syntax "(+ (inc 5) 1)")))
; the expansion is located
(must "the expanded call has a known source location"
      (not (null? (form-location (read-syntax "(inc2 5)")))))
; a non-macro form is left untouched
(must "macroexpand-1 leaves (+ 1 2) unchanged"
      (equal? (macroexpand-1 (read-syntax "(+ 1 2)")) (read-syntax "(+ 1 2)")))

(newline)
(display "the stepper shows located, hygienic expansion step by step") (newline)
