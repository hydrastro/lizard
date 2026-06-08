; -*- lisp -*-
; lib/cas/rischrde2.lisp -- the GENERAL exponential-integral decider built on the rational-coefficient RDE
; solver (rischrde.lisp): decides INT R(x) e^{g(x)} dx for R an ARBITRARY rational function and g a polynomial,
; by the exact reduction to a Risch differential equation.  This SUBSUMES the polynomial-R decider
; (liouville.lisp) and the pole-at-origin rational decider (liouvillerat.lisp) into a single rational-coefficient
; procedure -- the rational-function-coefficient solver that the recursive Risch procedure needs at each level
; (docs/TRAGER_ROADMAP.md, the summit).
;
; The reduction.  INT R e^g dx is elementary iff INT R e^g = y e^g for some rational y; differentiating,
; (y e^g)' = (y' + g' y) e^g, so y must solve the Risch differential equation
;     y' + g' y = R ,
; with g' the (polynomial) derivative of g and R the rational right-hand side.  rischrde.rde-solve decides this
; in the rationals: a solution y proves elementarity (INT R e^g = y e^g, certified by re-differentiating), and
; its absence proves non-elementarity.  For g = x (g' = 1) this is y' + y = R; for g = k x it is y' + k y = R;
; for higher-degree g the coefficient g' is a genuine polynomial.
;
; Public:
;   re2-decide R g          -> (list 'elementary y) with INT R e^g = y e^g, | (list 'non-elementary 'no-rational-y)
;       (R a rational (num . den); g a polynomial, the exponent so that the integrand is R e^g)
;   re2-certify R g y        -> #t iff (y e^g)' = R e^g, i.e. y' + g' y = R  (the exact rational identity)
;   re2-antiderivative R g   -> y (the rational multiplier, so INT R e^g = y e^g) | 'none
;
; Verified: INT x e^x = (x-1) e^x; INT e^x/x (Ei) non-elementary; INT (1/x - 1/x^2) e^x = (1/x) e^x; INT x e^{x^2}
; = (1/2) e^{x^2} (g = x^2, g' = 2x); INT e^{x^2} (erf) non-elementary; INT (2/x^2 ...) designed cases.  All
; verdicts agree with liouville / liouvillerat where those apply, now extended to arbitrary rational R.
;
; Builds on rischrde.lisp (the rational RDE solver) and tower.lisp / poly.lisp.

(import "cas/rischrde.lisp")
(import "cas/tower.lisp")
(import "cas/poly.lisp")

; ----- the RDE reduction: y' + g' y = R.  g' is the polynomial derivative of g (lifted to a rational). -----
(define (re2-gprime-rat g) (rat-from-poly (poly-deriv g)))
(define (re2-antiderivative R g) (rde-solve (re2-gprime-rat g) R))

; ----- the decision -----
(define (re2-decide R g) (re2-decide-go (re2-antiderivative R g)))
(define (re2-decide-go y) (if (equal? y (quote no-rational-solution)) (list (quote non-elementary) (quote no-rational-y)) (list (quote elementary) y)))

; ----- certificate: y' + g' y = R (so (y e^g)' = R e^g) -----
(define (re2-certify R g y) (rat-equal? (rat-add (rat-deriv y) (rat-mul (re2-gprime-rat g) y)) R))
