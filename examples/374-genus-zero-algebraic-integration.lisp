; GENUS-ZERO ALGEBRAIC INTEGRATION: the integral of (p x + r)/sqrt(a x^2 + b x + c), the first rung of the
; algebraic-case Risch problem -- integration over Q(x)[y] with y algebraic -- for the genus-0 curve y^2 = quadratic
; where the answer is always elementary (docs/CAS.md -- the algebraic-Risch frontier).
;
; The numerator splits as p x + r = (p/2a)(2ax+b) + (r - pb/2a), giving an algebraic (second-kind) part
; (p/2a) sqrt(q) plus a first-kind part (r - pb/2a) * J, where J = INT dx/sqrt(q) is a logarithm when a > 0
; (arcsinh-type) and an arcsine when a < 0 with the radicand having real roots.  Every piece is certified by
; differentiation; an integrand with no real form (a < 0 and no real arch) is reported, not forced.
(import "cas/algquadint.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Genus-0 algebraic integration: INT (px+r)/sqrt(ax^2+bx+c) dx, always elementary, certified.") (newline) (newline)

(display "INT dx/sqrt(x^2+1) = log(x + sqrt(x^2+1)) = arcsinh(x): a logarithm, no algebraic part:") (newline)
(display "  ") (display (aq-integrate 1 0 1 0 1)) (newline)
(must "the algebraic coefficient is 0" (= (aq-alg-coeff 1 0 1 0 1) 0))
(must "the first-kind coefficient is 1" (= (aq-first-coeff 1 0 1 0 1) 1))
(must "the first-kind term is a logarithm" (equal? (car (aq-first-kind 1 0 1)) (quote log)))
(must "the algebraic-part certificate holds (d/dx sqrt q = q'/2sqrt q)" (aq-second-kind-verify 1 0 1))

(display "INT x/sqrt(x^2+1) = sqrt(x^2+1): a pure algebraic (second-kind) integral:") (newline)
(must "the algebraic coefficient is 1/2 (of the derivative 2x)" (= (aq-alg-coeff 1 0 1 1 0) (/ 1 2)))
(must "there is no first-kind term" (= (aq-first-coeff 1 0 1 1 0) 0))

(display "INT (2x+3)/sqrt(x^2+1) = 2 sqrt(x^2+1) + 3 log(x + sqrt(x^2+1)): both parts:") (newline)
(must "the algebraic coefficient is 1" (= (aq-alg-coeff 1 0 1 2 3) 1))
(must "the first-kind coefficient is 3" (= (aq-first-coeff 1 0 1 2 3) 3))

(display "INT dx/sqrt(4 - x^2) = arcsin(x/2): the arcsine case (a < 0, real roots):") (newline)
(display "  ") (display (aq-integrate -1 0 4 0 1)) (newline)
(must "the first-kind term is an arcsine" (equal? (car (aq-first-kind -1 0 4)) (quote arcsin)))
(must "INT dx/sqrt(1 - x^2) is also an arcsine" (equal? (car (aq-first-kind -1 0 1)) (quote arcsin)))

(display "soundness: INT dx/sqrt(-x^2 - 1) has no real form (the radicand is never positive) -- reported, not forced:") (newline)
(must "no real form is returned" (equal? (car (aq-integrate -1 0 -1 0 1)) (quote no-real-form)))

(newline)
(display "The genus-0 algebraic integral -- a linear numerator over the square root of a quadratic -- is now computed") (newline)
(display "in full, splitting into a certified algebraic part and a logarithm or arcsine, with non-real integrands") (newline)
(display "reported honestly.  This is the first rung of the algebraic-case Risch problem; the positive-genus case (the") (newline)
(display "general Trager-Bronstein algorithm over Q(x)[y]) remains the open summit.") (newline)
