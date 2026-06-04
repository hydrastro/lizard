; -*- lisp -*-
; lib/cas/algtower2log.lisp -- the LOGARITHMIC INTEGRAL in the double algebraic tower Q(x)[y][z]/(z^2-y, y^2-x):
; the field inverse, the logarithmic derivative e'/e of a tower element, and the construction INT (e'/e) dx = log(e),
; certified (docs/TRAGER_ROADMAP.md -- RUNG 5, the third-kind logarithm over a stacked algebraic tower).
;
; algtower2.lisp gives the ring Q(x)[y][z] of x^(1/4) with its certified derivation; integration of a third-kind
; differential needs the field INVERSE (to form e'/e) and the logarithm construction.  The inverse uses the outer
; conjugate: for e = a + b z the conjugate over the inner field is ebar = a - b z, and e*ebar = a^2 - b^2 z^2 =
; a^2 - b^2 y =: M, an element of the inner field Q(x)[y].  Then
;     e^(-1) = ebar / M = (a*Minv) + (-b*Minv) z,   Minv = the inner inverse af-inv(M),
; so one inner inverse suffices (verified e*e^(-1) = 1).  The logarithmic derivative is then e'/e = D(e) * e^(-1),
; a tower element, and INT (e'/e) dx = log(e) by definition of the derivation; this is CONSTRUCTIVE and certified
; two independent ways: the multiplicative cleared identity e * (e'/e) = D(e) in the tower (no inverse trusted),
; and differentiation -- D(e)/e recomputed is exactly the integrand.  An integrand presented as a tower element F
; is recognized as d(log e) iff e * F = D(e) for the given e, else 'not-log-deriv: sound, never a guessed logarithm.
;
; Public (e, F tower elements as in algtower2; e nonzero):
;   t2l-conj e                 -> the outer conjugate a - b z
;   t2l-mnorm e                -> M = a^2 - b^2 y, the inner-field element e*conj(e) (the relative norm to Q(x)[y])
;   t2l-inv e                  -> e^(-1) = conj(e) * inner-inverse(M), a tower element
;   t2l-logderiv e             -> e'/e = D(e) * e^(-1), the logarithmic-derivative differential of e
;   t2l-log-of e               -> (list 'log e) meaning INT (t2l-logderiv e) dx = log(e)
;   t2l-cleared-cert e         -> #t iff e * (t2l-logderiv e) = D(e) in the tower (the inverse-free certificate)
;   t2l-is-logderiv? e F       -> #t iff e * F = D(e) (F is d(log e)); sound recognizer
;   t2l-verify-log e           -> #t iff differentiating log(e) [= t2l-logderiv e] reproduces e'/e (round trip)
;
; Verified: for e = 1 + z, e*e^(-1) = 1; INT (e'/e) dx = log(1 + x^(1/4)) with e*(e'/e) = D(e) certified; for
; e = z, e'/e = (1/(4x)) (since z' = z/(4x)) and INT = log(z) = log(x^(1/4)); a non-logarithmic F is rejected.
;
; Builds on algtower2.lisp (the tower) and algfunc.lisp (the inner inverse af-inv).

(import "cas/algtower2.lisp")

; ----- outer conjugate a - b z -----
(define (t2l-conj e) (t2-make (t2-a e) (af-neg (t2-b e))))

; ----- relative norm M = a^2 - b^2 y (inner-field element) -----
(define (t2l-mnorm e) (af-sub (af-mul (t2-zP) (t2-a e) (t2-a e))
                              (af-mul (t2-zP) (af-mul (t2-zP) (t2-b e) (t2-b e)) (af-y))))

; ----- inverse: conj(e) * inner-inverse(M) -----
(define (t2l-inv e) (t2l-inv-go e (af-inv (t2-zP) (t2l-mnorm e))))
(define (t2l-inv-go e Minv) (t2-make (af-mul (t2-zP) (t2-a e) Minv) (af-mul (t2-zP) (af-neg (t2-b e)) Minv)))

; ----- logarithmic derivative e'/e = D(e) * e^(-1) -----
(define (t2l-logderiv e) (t2-mul (t2-deriv e) (t2l-inv e)))

; ----- the logarithm construction -----
(define (t2l-log-of e) (list (quote log) e))

; ----- inverse-free cleared certificate: e * (e'/e) = D(e) -----
(define (t2l-cleared-cert e) (t2-equal? (t2-mul e (t2l-logderiv e)) (t2-deriv e)))

; ----- sound recognizer: F is d(log e) iff e * F = D(e) -----
(define (t2l-is-logderiv? e F) (t2-equal? (t2-mul e F) (t2-deriv e)))

; ----- round-trip verification: d(log e) recomputed equals e'/e -----
(define (t2l-verify-log e) (t2l-is-logderiv? e (t2l-logderiv e)))
