; SPECIAL FUNCTIONS -- the Gamma function, the error function erf, and the Bessel functions J_n -- closing the
; one capability Maxima has and lizard had none of.  Each is computed and CERTIFIED exactly by its power series
; (rational coefficients) together with its defining functional / differential identities, checked with the
; series engine (series.lisp).
;
; GAMMA: integer values Gamma(n) = (n-1)!; half-integer values are rational multiples of sqrt(pi) carried via
; the functional equation Gamma(x+1) = x Gamma(x).  ERF: the series sum (-1)^n x^{2n+1}/(n!(2n+1)) (the 2/sqrt
; pi factor omitted) differentiates exactly to the series of e^{-x^2}, i.e. erf'(x) = (2/sqrt pi) e^{-x^2}.
; BESSEL: J_n(x) = sum_m (-1)^m/(m!(m+n)!) (x/2)^{2m+n}, satisfying J_0' = -J_1 and the Bessel equation.
(import "cas/special.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Special functions: Gamma, erf, and Bessel J_n -- power series with certified identities.") (newline) (newline)

(display "the Gamma function:") (newline)
(display "  Gamma(5) = 4! = ") (display (sp-gamma-int 5)) (newline)
(display "  the half-integer values as coefficients of sqrt(pi):  Gamma(1/2)/sqrt(pi) = ") (display (sp-gamma-half 0))
(display ", Gamma(3/2)/sqrt(pi) = ") (display (sp-gamma-half 1)) (display ", Gamma(7/2)/sqrt(pi) = ") (display (sp-gamma-half 3)) (newline)
(chk "Gamma(5) = 24 and Gamma(7/2) = (15/8) sqrt(pi)" (if (= (sp-gamma-int 5) 24) (= (sp-gamma-half 3) (/ 15 8)) #f))
(chk "the functional equation Gamma(x+1) = x Gamma(x) holds at the half-integers" (sp-gamma-func-check 2))

(display "the error function erf:") (newline)
(display "  the reduced series (without 2/sqrt pi) = ") (display (sp-erf-coeffs 8)) (newline)
(display "  its derivative is the series of e^{-x^2} = ") (display (sp-emx2 8)) (newline)
(chk "erf'(x) = (2/sqrt pi) e^{-x^2} (the reduced series differentiates to e^{-x^2})" (sp-erf-deriv-check 8))

(display "the Bessel functions J_n:") (newline)
(display "  J_0(x) = ") (display (sp-besselj 0 7)) (newline)
(display "  J_1(x) = ") (display (sp-besselj 1 6)) (newline)
(display "  J_2(x) = ") (display (sp-besselj 2 7)) (newline)
(chk "the contiguous relation J_0'(x) = -J_1(x)" (sp-bessel-deriv-check 6))
(chk "J_0 satisfies the Bessel equation x^2 y'' + x y' + x^2 y = 0" (sp-bessel-ode-check 7))

(newline)
(display "Special functions closed: Gamma, erf, and Bessel J_n, each as a rational power series whose defining") (newline)
(display "identities (functional equation, erf' = e^{-x^2}, J_0' = -J_1, the Bessel ODE) are certified exactly.") (newline)
