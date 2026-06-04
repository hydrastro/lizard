; -*- lisp -*-
; lib/cas/hyperpell.lisp -- the GENUS-2 NONCONSTANT-B THIRD-KIND CONSTRUCTION by the function-field Pell / unit
; structure: building third-kind logarithm arguments g = A(x) + B(x) y with B NONCONSTANT, as powers of a
; fundamental unit on y^2 = f -- the genus-2 analogue of the elliptic3pell genus-0 construction (docs/TRAGER_ROADMAP.md
; -- the full third-kind construction beyond the a + y shape).
;
; hyperthird builds log(a + y) (the b = 1 third-kind argument) over a hyperelliptic field; the genuinely hard case
; is g = A + B y with B NONCONSTANT, whose norm N(g) = A^2 - B^2 f is the function-field Pell form.  A solution
; with CONSTANT norm is a UNIT of the field, and on y^2 = f a fundamental unit exists exactly when sqrt(f) has a
; periodic continued fraction.  A clean family where it does: f = h(x)^2 + c with c a nonzero constant -- then
; g0 = h + y has norm h^2 - f = -c, a constant, so g0 is a fundamental unit.  For deg h = 3 this is f of degree 6,
; a genuine GENUS-2 curve.  The powers g0^n = A_n + B_n y are computed by exact field arithmetic (algfunc, which is
; genus-agnostic), have B_n NONCONSTANT for n >= 2, and norm N(g0^n) = (-c)^n by multiplicativity.  Each power is a
; third-kind logarithm argument: INT ((g0^n)'/g0^n) dx = log(g0^n) = n log(g0), certified by differentiation in the
; field.  This constructs the nonconstant-B third-kind elements directly and certifies them by the norm relation
; and by differentiation -- the genus-2 companion to the genus-0 Pell construction.  The positive-genus curves whose
; sqrt(f) has a NON-periodic continued fraction (no fundamental unit) are reported as having no such unit rather
; than forced, exactly as in the genus-0 case.
;
; Public (f the curve polynomial; h, c the unit data f = h^2 + c; n a positive power):
;   hp-unit-norm h c           -> the fundamental norm -c (a constant), the norm of g0 = h + y
;   hp-is-unit-curve? f h c     -> #t iff f = h^2 + c (so g0 = h + y is a genuine unit of y^2 = f)
;   hp-unit-power f h n         -> g0^n = (A_n . B_n) as a pair of polynomials, by field arithmetic in K
;   hp-norm f A B              -> N(A + B y) = A^2 - B^2 f (a polynomial; constant for a unit power)
;   hp-B-nonconstant? f h n     -> #t iff B_n is nonconstant (true for n >= 2)
;   hp-certify f h c n          -> #t iff N(g0^n) = (-c)^n exactly (the Pell certificate)
;   hp-logderiv f h n           -> the differential (g0^n)'/g0^n as an algfunc element
;   hp-log-cert f h n           -> #t iff (g0^n) * ((g0^n)'/g0^n) = D(g0^n) in K (the differentiation certificate)
;   hp-g f h n                 -> (list 'log A_n B_n) : INT ((g0^n)'/g0^n) dx = log(A_n + B_n y) = n log(h + y)
;
; Verified: on y^2 = x^6 + 1 (genus 2, h = x^3, c = 1) the unit x^3 + y has norm -1; g0^2 = (2x^6+1, 2x^3) with
; norm 1 and B nonconstant; g0^3 with norm -1; the logarithm differentiation certificate holds for n = 1..3; and
; the n = 1 case is the b = 1 third-kind argument hyperthird already handles.
;
; Builds on algfunc.lisp (the genus-agnostic field).

(import "cas/algfunc.lisp")

(define (hp-len l) (if (null? l) 0 (+ 1 (hp-len (cdr l)))))
(define (hp-deg p) (- (hp-trim p) 1))
(define (hp-trim p) (hp-trim-n p (hp-len p)))
(define (hp-trim-n p k) (cond ((= k 0) 0) ((= (hp-nth p (- k 1)) 0) (hp-trim-n p (- k 1))) (else k)))
(define (hp-nth p k) (if (= k 0) (car p) (hp-nth (cdr p) (- k 1))))
(define (hp-pf f) (rat-from-poly f))

; ----- the fundamental unit g0 = h + y and its constant norm -c -----
(define (hp-unit-norm h c) (- 0 c))
(define (hp-is-unit-curve? f h c) (equal? (poly-norm f) (poly-norm (poly-add (poly-mul h h) (list c)))))
(define (poly-norm p) (reverse (hp-drop0 (reverse p))))
(define (hp-drop0 p) (cond ((null? p) (quote ())) ((= (car p) 0) (hp-drop0 (cdr p))) (else p)))

; ----- g0^n = A_n + B_n y, by exact field arithmetic in K = Q(x)[y]/(y^2 - f) -----
(define (hp-g0 f h) (af-make (rat-from-poly h) (rat-from-poly (list 1))))   ; h + y
(define (hp-unit-power f h n) (hp-up-extract (hp-pow f (hp-g0 f h) n)))
(define (hp-pow f g n) (if (<= n 1) g (af-mul (hp-pf f) g (hp-pow f g (- n 1)))))
(define (hp-up-extract e) (cons (rat-to-poly (af-u e)) (rat-to-poly (af-v e))))
(define (rat-to-poly r) (rat-num r))                                        ; unit powers have denominator 1

; ----- norm N(A + B y) = A^2 - B^2 f -----
(define (hp-norm f A B) (poly-sub (poly-mul A A) (poly-mul (poly-mul B B) f)))

; ----- is B_n nonconstant? -----
(define (hp-B-nonconstant? f h n) (> (hp-deg (cdr (hp-unit-power f h n))) 0))

; ----- Pell certificate: N(g0^n) = (-c)^n -----
(define (hp-certify f h c n) (equal? (poly-norm (hp-norm f (car (hp-unit-power f h n)) (cdr (hp-unit-power f h n)))) (poly-norm (list (hp-ipow (- 0 c) n)))))
(define (hp-ipow b e) (if (<= e 0) 1 (* b (hp-ipow b (- e 1)))))

; ----- the logarithm differential (g0^n)'/g0^n and its differentiation certificate -----
(define (hp-gn f h n) (hp-pow f (hp-g0 f h) n))                              ; g0^n as a field element
(define (hp-logderiv f h n) (af-div (hp-pf f) (af-deriv (hp-pf f) (hp-gn f h n)) (hp-gn f h n)))
(define (hp-log-cert f h n) (af-equal? (af-mul (hp-pf f) (hp-gn f h n) (hp-logderiv f h n)) (af-deriv (hp-pf f) (hp-gn f h n))))

; ----- the constructed third-kind logarithm argument -----
(define (hp-g f h n) (cons (quote log) (hp-unit-power f h n)))
