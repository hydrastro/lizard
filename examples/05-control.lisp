; -*- lisp -*-
; Control flow: if, cond, begin, and/or, lazy values.

; if takes a test, consequent, optional alternative.
(if (> 3 2) 'bigger 'smaller)        ; => bigger
(if #f 1 2)                          ; => 2

; cond is the multi-arm form. Each clause is (test body...).
; `else` matches everything.
(define (classify n)
  (cond ((< n 0)  'negative)
        ((= n 0)  'zero)
        ((< n 10) 'small)
        (else     'big)))

(classify -5)             ; => negative
(classify 0)              ; => zero
(classify 7)              ; => small
(classify 42)             ; => big

; begin sequences side effects, returns the last expression.
(define counter 0)
(begin
  (set! counter (+ counter 1))
  (set! counter (+ counter 1))
  counter)                ; => 2

; and/or short-circuit and return the deciding value, not just #t/#f.
(and 1 2 3)               ; => 3
(and 1 #f 3)              ; => #f
(or  #f #f 7)             ; => 7
(or  #f #f #f)            ; => #f
(not #t)                  ; => #f

; delay/force builds an explicit lazy value (a promise).
; Lizard's normal evaluation is already call-by-need, so delay/force
; is more useful for caching and explicit memoization than for
; controlling evaluation.
;
; (Skipped here because delay/force semantics in lizard interact
; with arguments-are-promises in subtle ways. Stick to the forms
; above and they'll behave like a strict Scheme.)
