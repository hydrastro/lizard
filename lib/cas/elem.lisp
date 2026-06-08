; -*- lisp -*-
; lib/cas/elem.lisp — elementary integration over a single transcendental
; monomial by reduction to the COMPLETE, certified rational-function integrator.
;
; Two exact substitutions finish large classes the in-tower machinery would
; otherwise need a full Rothstein-Trager for:
;
;   primitive (theta = log x):   INT (1/x) R(log x) dx  =  [ INT R(t) dt ]_{t = log x}
;                                 (since (1/x) dx = dt)
;   exponential (theta = e^x):   INT R(e^x) dx          =  [ INT R(u)/u du ]_{u = e^x}
;                                 (since dx = du/u)
;
; In both cases the right-hand integral is an ORDINARY rational-function integral
; in the monomial, which lib/cas/integrate.lisp solves and certifies by
; differentiating back over Q (handling the polynomial part, multiple
; logarithms, and arctangents).  Correctness is then: that certificate in the
; monomial variable, plus the substitution theorem (the chain rule).  This gives,
; e.g., INT 1/(x((log x)^2-1)) = (1/2)log((log x -1)/(log x +1)), INT 1/(x((log
; x)^2+1)) = arctan(log x), and INT 1/(e^(2x)+1) dx = log(e^x) - (1/2)log(e^(2x)+1).
;
; Top-level helpers only; builds on lib/cas/integrate.lisp.

(import "cas/integrate.lisp")

; INT (1/x) * R(log x) dx, with R = num/den (polynomials in the monomial)
;   -> (list 'ok answer-string certified?) | (list 'cannot reason)
(define (integrate-primitive-log num den)
  (let ((res (integrate-rational num den)))
    (if (equal? (car res) 'ok)
        (list 'ok (integral->string res "log(x)") (integrate-verify num den res))
        (list 'cannot (car (cdr res))))))

; INT R(e^x) dx, with R = num/den  ->  INT R(u)/u du, then u = e^x
(define (integrate-exp-rational num den)
  (let ((den2 (poly-mul den (list 0 1))))          ; denominator R(u)/u has denom den*u
    (let ((res (integrate-rational num den2)))
      (if (equal? (car res) 'ok)
          (list 'ok (integral->string res "(e^x)") (integrate-verify num den2 res))
          (list 'cannot (car (cdr res)))))))

; INT (1/x) * (poly in log x) dx is the special case num arbitrary, den = 1
; (kept as a named entry for clarity)
(define (integrate-log-poly p) (integrate-primitive-log p (list 1)))

(define (elem-result->string r)
  (if (equal? (car r) 'ok) (car (cdr r)) "not resolved (reduces to an integral needing algebraic residues)"))
(define (elem-certified? r) (and (equal? (car r) 'ok) (car (cdr (cdr r)))))
