; TRIGONOMETRIC INTEGRATION -- closed-form antiderivatives of sin^m(x) cos^n(x), every result CERTIFIED by
; differentiating it back to the integrand.  A genuinely new integration capability alongside the rational and
; algebraic integrators: those handle R(x) and R(x,y) with y algebraic, this handles trigonometric integrands.
;
; The method is the classical reduction formula:
;   INT sin^m cos^n dx = sin^{m+1} cos^{n-1}/(m+n) + (n-1)/(m+n) INT sin^m cos^{n-2} dx   (lower the cos power),
;   INT sin^m cos^n dx = -sin^{m-1} cos^{n+1}/(m+n) + (m-1)/(m+n) INT sin^{m-2} cos^n dx   (lower the sin power),
; applied until a base case INT 1 = x, INT cos = sin, or INT sin = -cos.  The antiderivative is A(s,c) + B*x
; with A a polynomial in s = sin x, c = cos x and B a rational constant -- a shape closed under d/dx (d s = c,
; d c = -s), so the differentiate-back certificate is exact (using the relation s^2 = 1 - c^2 to canonicalize).
;
; A trig polynomial prints as a list of terms (coeff s-power c-power); a trig integral as (trigpoly . B).
(import "cas/trigint.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Trigonometric integration of sin^m(x) cos^n(x), every antiderivative differentiate-back certified.") (newline) (newline)

(display "an odd power gives a clean polynomial in sin/cos (no x term):") (newline)
(display "  INT sin^3 x dx = -cos x + cos^3 x / 3  ->  ") (display (ti-show (ti-integrate 3 0))) (newline)
(chk "INT sin^3 dx differentiates back to sin^3" (ti-certify 3 0))
(display "  INT sin^2 x cos x dx = sin^3 x / 3  ->  ") (display (ti-show (ti-integrate 2 1))) (newline)
(chk "INT sin^2 cos dx differentiates back to sin^2 cos" (ti-certify 2 1))

(display "an even-even integrand contributes a linear x term:") (newline)
(display "  INT cos^2 x dx = x/2 + (sin x cos x)/2  ->  ") (display (ti-show (ti-integrate 0 2))) (newline)
(chk "INT cos^2 dx differentiates back to cos^2" (ti-certify 0 2))
(display "  INT sin^2 x cos^2 x dx = x/8 + (sin^3 x cos x - sin x cos^3 x)/8  ->  ") (display (ti-show (ti-integrate 2 2))) (newline)
(chk "INT sin^2 cos^2 dx differentiates back to sin^2 cos^2" (ti-certify 2 2))

(display "a battery of cases, all certified by differentiation:") (newline)
(chk "INT sin^4 dx" (ti-certify 4 0))
(chk "INT cos^5 dx" (ti-certify 0 5))
(chk "INT sin^3 cos^3 dx" (ti-certify 3 3))
(chk "INT sin^5 cos^2 dx" (ti-certify 5 2))
(chk "INT sin^4 cos^4 dx" (ti-certify 4 4))
(chk "INT sin^6 dx" (ti-certify 6 0))
(chk "INT sin x cos x dx" (ti-certify 1 1))

(newline)
(display "Trigonometric integration: sin^m cos^n reduced to closed form by the reduction formulas, the answer a") (newline)
(display "polynomial in sin and cos plus a linear term, every case verified by differentiating back to the integrand.") (newline)
