; -*- lisp -*-
; lib/cas/elliptic3general.lisp -- the GENERAL third-kind logarithmic part on y^2 = q(x): work with g = A + B*y
; where BOTH A and B are rational functions (B nonconstant allowed), the case beyond elliptic3complete's
; g = u + sqrt(q) (B = 1) (docs/CAS.md -- summit S1: fully general third kind, g = A + B sqrt q).
;
; For g = A + B y in K = Q(x)[y]/(y^2 - q), the conjugate is gbar = A - B y and the NORM is
;     N = g * gbar = A^2 - B^2 q   (a rational function of x).
; The logarithmic derivative omega = g'/g satisfies a structural identity that pins down N:
;     omega + omegabar = g'/g + gbar'/gbar = (g gbar)'/(g gbar) = N'/N,
; and since omega + omegabar = 2 * (rational part of omega) = 2a (writing omega = a + b y), we get
;     a = (1/2) N'/N.
; So the rational part of any third-kind omega must be HALF the logarithmic derivative of the norm N; this is an
; exact, checkable necessary condition, and N is recovered from a (when 2a is the logarithmic derivative of a
; rational function).  This module computes the norm of a supplied g, certifies the general recognizer
; omega = g'/g (which is already exact in K for nonconstant B), and checks the norm identity a = (1/2) N'/N --
; the sound, exact core of the general third-kind decision.  The remaining construction (solving the coupled
; system A^2 - B^2 q = N together with the y-component to BUILD g from omega in general) is the Jacobian/torsion
; part of Trager's algorithm and is genuinely research-grade; here we provide certification and the norm relation,
; never a guessed g.
;
; Public (q a rat-from-poly'd radicand; g an af- element A + B y; omega an af- element a + b y):
;   e3g-norm q g               -> the norm N = A^2 - B^2 q as a rational function
;   e3g-logderiv q g           -> omega = g'/g in K (general A + B y)
;   e3g-recognize q g omega    -> #t iff omega = g'/g (the general recognizer, exact in K)
;   e3g-norm-relation? q g     -> #t iff the rational part of g'/g equals (1/2) (norm)'/(norm) (the identity)
;   e3g-integrate-known q g     -> (list 'elementary-log g) once e3g-recognize confirms g, else 'unknown
;
; Verified: for g = (x^2+1) + x*y on y^2 = x^2+1 the norm is x^2+1, the recognizer certifies omega = g'/g, and the
; norm relation a = (1/2)N'/N holds; for g = x + x*y on the same curve the recognizer and norm relation also hold
; (nonconstant B); a wrong omega is rejected by the recognizer.
;
; Builds on algfunc.lisp, elliptic3.lisp, ratfun.lisp, tower.lisp, poly.lisp.

(import "cas/algfunc.lisp")
(import "cas/elliptic3.lisp")
(import "cas/ratfun.lisp")
(import "cas/tower.lisp")
(import "cas/poly.lisp")

; ----- the norm N = A^2 - B^2 q (A = af-u g, B = af-v g), as a rational function -----
(define (e3g-norm q g) (rat-sub (rat-mul (af-u g) (af-u g)) (rat-mul (rat-mul (af-v g) (af-v g)) q)))

; ----- the general logarithmic derivative (delegates to the exact K-arithmetic; valid for nonconstant B) -----
(define (e3g-logderiv q g) (e3-logderiv q g))

; ----- the general recognizer: certify a supplied g (exact in K, B nonconstant allowed) -----
(define (e3g-recognize q g omega) (e3-certify q g omega))

; ----- the norm identity: rational part of g'/g equals (1/2) N'/N -----
(define (e3g-norm-relation? q g) (e3g-check-rel (af-u (e3-logderiv q g)) (e3g-norm q g)))
(define (e3g-check-rel a N) (rat-equal? a (rat-scale (/ 1 2) (e3g-logder-rat N))))
(define (e3g-logder-rat N) (rat-div (rat-deriv N) N))           ; N'/N

; ----- integrate, given a g the recognizer confirms -----
(define (e3g-integrate-known q g) (e3g-result q g (e3-logderiv q g)))
(define (e3g-result q g omega) (if (e3-certify q g omega) (list (quote elementary-log) g) (list (quote unknown) (quote not-recognized))))
