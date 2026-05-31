; -*- lisp -*-
; lib/macros.lisp — control-flow macros built on syntax-rules with ellipsis.
;
; Unlike lib/control.lisp (which takes thunks, since lizard evaluates eagerly),
; these are real macros: they expand at macro-expansion time and short-circuit
; naturally.  Each is hygienic — identifiers the macro introduces (the `t` in
; my-or/my-and, the `tmp` in swap!) are renamed so they cannot capture names
; the caller passes in.  Imported with  (import "macros.lisp").

; (my-or e ...) — short-circuiting disjunction.  Introduces a temporary `t`.
(define-syntax my-or
  (syntax-rules ()
    ((_)            #f)
    ((_ e)          e)
    ((_ e1 e2 ...)  (let ((t e1)) (if t t (my-or e2 ...))))))

; (my-and e ...) — short-circuiting conjunction.
(define-syntax my-and
  (syntax-rules ()
    ((_)            #t)
    ((_ e)          e)
    ((_ e1 e2 ...)  (if e1 (my-and e2 ...) #f))))

; (unless c body ...) — evaluate the body unless the condition holds.
(define-syntax unless
  (syntax-rules ()
    ((_ c body ...) (if c #f (begin body ...)))))

; (swap! a b) — exchange two places.  Introduces a temporary `tmp`.
(define-syntax swap!
  (syntax-rules ()
    ((_ a b) (let ((tmp a)) (set! a b) (set! b tmp)))))
