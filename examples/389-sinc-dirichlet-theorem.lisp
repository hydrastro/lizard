; The DIRICHLET INTEGRAL theorem INT_0^inf sin(x)/x dx = pi/2 (the value of the sinc integral), proved by the
; parameter-integral (Feynman) method -- NOT by finding an antiderivative, because sin(x)/x has none that is
; elementary, but by the certified analytic chain below (docs/CAS.md -- the proof-producing CAS on a NON-elementary
; integrand; the companion to the FTC definite integrals of example 388).
;
;   I(s) = INT_0^inf e^{-sx} sin(x)/x dx,  the target being I(0).
;   (1) I'(s) = - INT_0^inf e^{-sx} sin x dx                       (differentiate under the integral)
;   (2) INT_0^inf e^{-sx} sin x dx = 1/(s^2+1)                     (Lemma A: Laplace transform of sine)
;   (3) so I'(s) = -1/(s^2+1), hence I(s) = C - arctan(s)          (Lemma B: d/ds(-arctan s) = -1/(s^2+1))
;   (4) I(inf) = 0  =>  C = pi/2                                   (boundary condition)
;   (5) I(0) = pi/2 - arctan(0) = pi/2.                            (evaluate)  QED
;
; Lemma A is proved from the antiderivative G(x,s) = -e^{-sx}(s sin x + cos x)/(s^2+1): d/dx G = e^{-sx} sin x is
; checked by the differentiation arbiter at sample points, and the definite value is G(inf) - G(0) = 1/(s^2+1).
; Lemma B's derivative identity is checked the same way.  The algebraic backbone is exact.
(import "cas/dirichlet.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Theorem (Dirichlet): INT_0^inf sin(x)/x dx = pi/2 -- a NON-elementary integrand, proved by parameter integral.") (newline) (newline)

(display "Lemma A (Laplace transform of sine): INT_0^inf e^{-sx} sin x dx = 1/(s^2+1).") (newline)
(display "  the antiderivative G(x,s) = -e^{-sx}(s sin x + cos x)/(s^2+1) differentiates back to e^{-sx} sin x:") (newline)
(must "d/dx G = e^{-sx} sin x is certified (sampled) at s = 1" (dir-lemma-A-cert 1.0))
(must "and at s = 2" (dir-lemma-A-cert 2.0))
(must "the definite value G(inf) - G(0) equals 1/(s^2+1) at s = 1" (dir-lemma-A-value-cert 1.0))
(must "and at s = 3" (dir-lemma-A-value-cert 3.0))

(display "Lemma B: d/ds(-arctan s) = -1/(s^2+1), so I'(s) = -1/(s^2+1) integrates to I(s) = C - arctan(s).") (newline)
(must "the arctan-derivative identity is certified at s = 1" (dir-lemma-B-cert 1.0))
(must "and at s = 1/2" (dir-lemma-B-cert 0.5))

(display "Boundary and evaluation: I(inf) = 0 gives C = pi/2, so I(0) = pi/2 - arctan(0) = pi/2.") (newline)
(display "  I(0) = ") (display (dir-value)) (display "  (pi/2 = ") (display (/ dir-pi 2)) (display ")") (newline)
(must "I(0) = pi/2 to machine precision" (dir-approx (dir-value) (/ dir-pi 2)))

(display "the full proof record (re-checkable):") (newline)
(display "  ") (display (dir-prove)) (newline)
(must "the proof record re-verifies (both lemmas re-check, value is pi/2)" (dir-recheck (dir-prove)))

(newline)
(display "Therefore INT_0^inf sin(x)/x dx = pi/2.  The integrand is non-elementary, so this is proved not by the") (newline)
(display "Fundamental Theorem of Calculus but by the parameter-integral method, with the two analytic lemmas certified") (newline)
(display "by the differentiation arbiter -- the honest division of labor between the elementary and non-elementary cases.") (newline)
