; -*- lisp -*-
; lib/macro-stepper.lisp — a macro-expansion stepper (Phase 4 tooling).
;
; It repeatedly expands the OUTERMOST macro one step with `macroexpand-1`,
; printing each rewrite together with the source location (`form-location`) of
; the macro call it expanded.  Parse the source with `read-syntax` so the forms
; carry real spans:
;
;   (macro-step (read-syntax "(my-or a b c)"))
;
; Each step is hygienic — template-introduced binders are renamed — so the trace
; shows the actual, capture-free expansion.

; Print one located rewrite.
(define (macro-step-show before after)
  (display "  => ") (write after)
  (display "      [from the form at ") (display (form-location before)) (display "]")
  (newline))

; Step the outermost macro to a fixed point, printing each rewrite.  Returns the
; fully head-expanded form.
(define (macro-step-loop form)
  (let ((next (macroexpand-1 form)))
    (if (equal? next form)
        form
        (begin (macro-step-show form next)
               (macro-step-loop next)))))

; Public entry: show the starting form, then the located steps.
(define (macro-step form)
  (display "    ") (write form) (newline)
  (macro-step-loop form))

; Non-printing variant: just the fully head-expanded form (for tests).
(define (fully-expand-head form)
  (let ((next (macroexpand-1 form)))
    (if (equal? next form) form (fully-expand-head next))))
