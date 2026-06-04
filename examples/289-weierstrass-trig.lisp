; WEIERSTRASS SUBSTITUTION -- integrals of rational functions of sin and cos.  The substitution t = tan(x/2)
; gives sin x = 2t/(1+t^2), cos x = (1-t^2)/(1+t^2), dx = 2 dt/(1+t^2), turning ANY integral of a rational
; trigonometric function into the integral of a rational function of t, which the certified rational integrator
; handles completely.  This extends trigonometric integration from the sin^m cos^n monomials (trigint.lisp) to
; arbitrary rational trigonometric integrands, with the answer exact in t = tan(x/2) and carried by the rational
; integrator's own differentiate-back certificate.
;
; The integrand N(s,c)/D(s,c) is given as trig polynomials (lists of terms (coeff s-power c-power)).  Each
; monomial s^i c^j maps to (2t)^i (1-t^2)^j / (1+t^2)^{i+j}; clearing the common (1+t^2) powers and folding in
; dx = 2/(1+t^2) yields a rational function P(t)/Q(t), which is integrated and verified.  Results are reported in
; the rational integrator's (ok ratpart logs arctans) form, in the variable t = tan(x/2).
(import "cas/weier.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Weierstrass substitution t = tan(x/2): rational trig integrals reduced to the rational integrator.") (newline) (newline)

(display "the classic INT dx/(1 + cos x):") (newline)
(display "  substitutes to the rational integrand P/Q = ") (display (we-substitute (we-const 1) (we-a+bcos 1 1))) (display "  (= 2/2 = 1 in t)") (newline)
(define r1 (we-integrate (we-const 1) (we-a+bcos 1 1)))
(display "  INT dx/(1+cos x) = t = tan(x/2)  ->  ") (display r1) (newline)
(chk "INT dx/(1+cos x) verified by the rational integrator" (we-verify (we-const 1) (we-a+bcos 1 1) r1))

(display "INT dx/(2 + cos x) (an arctangent answer):") (newline)
(define r2 (we-integrate (we-const 1) (we-a+bcos 2 1)))
(display "  rational integrand 2/(3 + t^2); INT = (2/sqrt 3) arctan(t/sqrt 3)  ->  ") (display r2) (newline)
(chk "INT dx/(2+cos x) verified" (we-verify (we-const 1) (we-a+bcos 2 1) r2))

(display "INT dx/(2 + sin x):") (newline)
(define r3 (we-integrate (we-const 1) (we-a+bsin 2 1)))
(display "  -> ") (display r3) (newline)
(chk "INT dx/(2+sin x) verified" (we-verify (we-const 1) (we-a+bsin 2 1) r3))

(display "a non-constant numerator, INT cos x/(1 + cos x) dx:") (newline)
(define r4 (we-integrate (we-a+bcos 0 1) (we-a+bcos 1 1)))
(display "  -> ") (display r4) (newline)
(chk "INT cos x/(1+cos x) verified" (we-verify (we-a+bcos 0 1) (we-a+bcos 1 1) r4))

(display "INT dx/(5 + 4 cos x) (a rational answer, no transcendental part needed beyond arctan):") (newline)
(define r5 (we-integrate (we-const 1) (we-a+bcos 5 4)))
(display "  -> ") (display r5) (newline)
(chk "INT dx/(5+4 cos x) verified" (we-verify (we-const 1) (we-a+bcos 5 4) r5))

(newline)
(display "Weierstrass substitution: every rational function of sin and cos integrates by reduction to a rational") (newline)
(display "function of t = tan(x/2), and the result inherits the rational integrator's exact differentiate-back proof.") (newline)
