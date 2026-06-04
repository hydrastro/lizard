; -*- lisp -*-
; lib/cas/elliptic3.lisp -- the elliptic THIRD-KIND recognizer: deciding when an integrand on the curve
; y^2 = q(x) is a logarithmic derivative and hence integrates to a logarithm log(A + B sqrt(q)) -- the elementary
; third-kind case that the pure first/second-kind reduction (elliptic.lisp) deliberately left as inconclusive
; (docs/TRAGER_ROADMAP.md, beyond the genus-1 frontier).
;
; By Liouville's theorem, beyond the algebraic part d/dx[A sqrt q] an elliptic integral can be elementary through
; logarithms: INT omega is elementary with a logarithmic term exactly when omega = sum c_i d/dx log(g_i) for
; g_i in the function field K = Q(x)[y]/(y^2 - q) and constants c_i.  The fundamental constructive fact, which is
; sound and certifiable, is the converse direction made into a recognizer:
;   for any g = A + B y in K (A, B rational functions over Q), d/dx log(g) = g'/g is a specific element of K,
;   computed exactly as g'/g = g' * conj(g) / Norm(g) with conj(g) = A - B y and Norm(g) = A^2 - B^2 q;
; so GIVEN a candidate g, the integrand g'/g is integrated by log(g), and the result is CERTIFIED by recomputing
; the K-derivative.  This recognizes the third-kind logarithmic integrals constructively: if an integrand is
; presented as (or matched to) the logarithmic derivative of a K-element, its integral is the corresponding
; logarithm -- elementary, certified -- complementing the first/second-kind non-elementarity proofs.
;
; A concrete decidable sub-case wired here: integrands omega = g'/g for an explicitly supplied g.  This is exactly
; the third-kind family, and it lets the system return log(A + B sqrt q) for those integrals (e.g. inverse
; hyperbolic / logarithmic forms over the radical) with a differentiation certificate, where the pure reduction
; would only say "non-elementary first/second kind".
;
; Public:
;   e3-logderiv q g            -> g'/g as a K-element (the integrand whose integral is log g), g = (A B)
;   e3-integrate-logderiv q g  -> (list 'elementary-log g) : INT (g'/g) dx = log(g), with g returned
;   e3-certify q g omega       -> #t iff d/dx log(g) = omega in K (the third-kind certificate)
;   e3-recognize q omega g     -> (list 'elementary-log g) | (list 'not-this-log ..) : decide whether the given
;                                 omega equals g'/g for the supplied candidate g, and if so return log(g)
;
; Verified: d/dx log(x + sqrt(x^2+1)) = 1/sqrt(x^2+1) recognized and certified; d/dx log of a genuine genus-1
; K-element over y^2 = x^3+1 recognized; a non-matching candidate rejected.
;
; Builds on algfunc.lisp (K = Q(x)[y]/(y^2-q), af-deriv, af-div, af-equal?) and tower.lisp / poly.lisp.

(import "cas/algfunc.lisp")
(import "cas/tower.lisp")
(import "cas/poly.lisp")

; g = (A B) meaning A + B y ; reuse the af- element representation (list u v)
; ----- the logarithmic derivative g'/g in K -----
(define (e3-logderiv q g) (af-div q (af-deriv q g) g))

; ----- integrate a presented logarithmic derivative: INT (g'/g) dx = log(g) -----
(define (e3-integrate-logderiv q g) (list (quote elementary-log) g))

; ----- the third-kind certificate: d/dx log(g) = omega in K ? -----
(define (e3-certify q g omega) (af-equal? (e3-logderiv q g) omega))

; ----- recognizer: does the given omega equal g'/g for the supplied candidate g? -----
(define (e3-recognize q omega g) (if (e3-certify q g omega) (list (quote elementary-log) g) (list (quote not-this-log) (quote candidate-mismatch))))
