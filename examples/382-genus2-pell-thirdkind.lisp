; The GENUS-2 NONCONSTANT-B THIRD-KIND CONSTRUCTION by the function-field Pell / unit structure: building
; third-kind logarithm arguments g = A(x) + B(x) y with B NONCONSTANT, as powers of a fundamental unit on
; y^2 = f -- the genus-2 analogue of the genus-0 Pell construction (docs/TRAGER_ROADMAP.md -- the full third-kind
; construction beyond the a + y shape).
;
; hyperthird handles g = a + y (b = 1); the hard case is g = A + B y with B nonconstant, whose norm A^2 - B^2 f is
; the function-field Pell form.  A unit (constant norm) generates these: on f = h(x)^2 + c the element g0 = h + y
; has norm -c, a constant, so its powers g0^n = A_n + B_n y have B_n nonconstant for n >= 2 and norm (-c)^n.  For
; deg h = 3 this is a genuine genus-2 curve.  Each power is a third-kind logarithm argument, certified by the norm
; relation and by differentiation in the field.
(import "cas/hyperpell.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define f (list 1 0 0 0 0 0 1))   ; y^2 = x^6 + 1, a genus-2 curve
(define h (list 0 0 0 1))          ; h = x^3, so f = h^2 + 1

(display "On y^2 = x^6 + 1 (genus 2): nonconstant-B third-kind arguments as powers of the unit x^3 + y.") (newline) (newline)

(display "the fundamental unit g0 = x^3 + y has constant norm -1:") (newline)
(must "f = h^2 + c with h = x^3, c = 1" (hp-is-unit-curve? f h 1))
(must "the fundamental norm is -1" (= (hp-unit-norm h 1) -1))
(must "g0^1 = (x^3, 1), the fundamental unit with constant B" (if (hp-B-nonconstant? f h 1) #f #t))

(display "g0^2 = (2x^6 + 1) + (2x^3) y: a genuine NONCONSTANT B, with norm (-1)^2 = 1:") (newline)
(display "  ") (display (hp-unit-power f h 2)) (newline)
(must "B = 2x^3 is nonconstant" (hp-B-nonconstant? f h 2))
(must "the Pell certificate N(g0^2) = (-1)^2 holds" (hp-certify f h 1 2))
(must "INT ((g0^2)'/g0^2) dx = log(g0^2), differentiation-certified" (hp-log-cert f h 2))

(display "g0^3 has nonconstant B and norm (-1)^3 = -1:") (newline)
(display "  ") (display (hp-unit-power f h 3)) (newline)
(must "B is nonconstant" (hp-B-nonconstant? f h 3))
(must "the Pell certificate N(g0^3) = (-1)^3 holds" (hp-certify f h 1 3))
(must "the logarithm differentiation certificate holds" (hp-log-cert f h 3))

(display "soundness: a curve that is not of the form h^2 + c has no such unit and is reported:") (newline)
(must "y^2 = x^5 + 1 is not a unit curve for h = x^3" (if (hp-is-unit-curve? (list 1 0 0 0 0 1) h 1) #f #t))

(display "a second unit curve, y^2 = (x^3 + x)^2 + 2:") (newline)
(define h2 (list 0 1 0 1))
(define f2 (poly-add (poly-mul h2 h2) (list 2)))
(must "(x^3 + x)^2 + 2 is a unit curve" (hp-is-unit-curve? f2 h2 2))
(must "its g0^2 certifies against (-2)^2 = 4" (hp-certify f2 h2 2 2))
(must "its logarithm certificate holds" (hp-log-cert f2 h2 2))

(newline)
(display "The third-kind construction now reaches nonconstant-B arguments at genus 2: powers of a function-field") (newline)
(display "Pell unit g0 = h + y on y^2 = h^2 + c give g0^n = A_n + B_n y with B_n nonconstant, each a certified") (newline)
(display "third-kind logarithm.  This is the genus-2 companion to the genus-0 Pell construction.  Curves whose") (newline)
(display "sqrt(f) has a non-periodic continued fraction (no fundamental unit) remain out of scope, reported not forced.") (newline)
