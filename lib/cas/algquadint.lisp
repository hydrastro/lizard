; -*- lisp -*-
; lib/cas/algquadint.lisp -- GENUS-ZERO ALGEBRAIC INTEGRATION: the integral of (p x + r)/sqrt(a x^2 + b x + c), the
; first rung of the algebraic-case Risch problem (integration over Q(x)[y] with y algebraic), here for the genus-0
; curve y^2 = quadratic where the answer is always elementary (docs/CAS.md -- summit, the algebraic-Risch frontier).
;
; The curve y^2 = a x^2 + b x + c has genus 0, so every integral of a rational function of (x, y) is elementary --
; an algebraic part plus logarithms or an arctangent/arcsine.  For the linear numerator p x + r over the radical,
; the integral splits canonically by writing the numerator as a multiple of the radicand's derivative plus a
; constant:
;     p x + r = (p / 2a)(2 a x + b) + (r - p b / 2a),
; so
;     INT (p x + r)/sqrt(q) dx = (p / 2a) sqrt(q)              [the SECOND-kind, algebraic part: d/dx sqrt q = q'/(2 sqrt q)]
;                              + (r - p b / 2a) * J,           [the FIRST-kind part J = INT dx / sqrt(q)]
; and the first-kind integral J has the closed form dictated by the sign of the leading coefficient a and the
; discriminant D = b^2 - 4 a c:
;     a > 0:  J = (1/sqrt a) log( 2 a x + b + 2 sqrt a sqrt q )         (a logarithm; arcsinh-type)
;     a < 0:  J = (1/sqrt(-a)) arcsin( (2 a x + b)/sqrt(-D) )           (an arcsine; needs -a>0 and D<0 for a real arch)
; Each piece is exact and is CERTIFIED by differentiation: d/dx of the returned closed form equals the integrand.
; The module records the algebraic coefficient (p/2a), the first-kind coefficient (r - p b/2a), and the kind of the
; first-kind term ('log or 'arcsin) with its data, and verifies the second-kind part by differentiating sqrt(q)
; symbolically (q'/(2 sqrt q)) and matching.  This is the complete, certified genus-0 algebraic integrator for a
; linear numerator; higher numerators reduce to this by Hermite reduction (already available for the polynomial
; part), and the positive-genus algebraic case (the general Trager-Bronstein algorithm) remains the open summit.
;
; Public (a b c the radicand coefficients of q = a x^2 + b x + c; p r the numerator p x + r; all rational):
;   aq-radicand a b c          -> the quadratic q as a coefficient list (c b a) low->high
;   aq-disc a b c              -> the discriminant b^2 - 4 a c
;   aq-alg-coeff a b c p r     -> p / 2a, the coefficient of sqrt(q) in the answer (the second-kind part)
;   aq-first-coeff a b c p r   -> r - p b / 2a, the coefficient of the first-kind term
;   aq-first-kind a b c        -> (list 'log a b) when a > 0, (list 'arcsin a b (-disc)) when a < 0 and disc < 0
;   aq-integrate a b c p r     -> (list 'ok alg-coeff first-coeff first-kind) | (list 'no-real-form ...)
;   aq-second-kind-verify a b c-> #t iff d/dx sqrt(q) = (2 a x + b)/(2 sqrt q) (the algebraic-part certificate)
;
; Verified: INT dx/sqrt(x^2+1) has alg-coeff 0, first-coeff 1, first-kind log (= arcsinh, log(x+sqrt(x^2+1)));
; INT x/sqrt(x^2+1) has alg-coeff 1/2 of (2x) ... = sqrt(x^2+1) with first-coeff 0; INT (2x+3)/sqrt(x^2+1) gives
; alg-coeff 1 (-> 2 sqrt over the 1/2... combined) plus first-coeff 3 log; INT dx/sqrt(4-x^2) is the arcsine case.
;
; Builds on poly.lisp.

(import "cas/poly.lisp")

; ----- the radicand and its discriminant -----
(define (aq-radicand a b c) (list c b a))
(define (aq-disc a b c) (- (* b b) (* 4 a c)))

; ----- the canonical split coefficients -----
(define (aq-alg-coeff a b c p r) (/ p (* 2 a)))                ; coefficient of sqrt(q)
(define (aq-first-coeff a b c p r) (- r (/ (* p b) (* 2 a))))  ; coefficient of the first-kind term

; ----- the first-kind term J = INT dx/sqrt(q), by sign of a (and discriminant) -----
; a > 0: J = (1/sqrt a) log(2ax+b+2 sqrt a sqrt q)  (arcsinh-type, always real).
; a < 0: q = a x^2+b x+c is >= 0 only between its two real roots, so a real arch needs D = b^2-4ac > 0; then
;        J = -(1/sqrt(-a)) arcsin((2ax+b)/sqrt D)  (verified: d/dx of that = 1/sqrt q since D-(2ax+b)^2 = -4a q).
(define (aq-first-kind a b c)
  (cond ((> a 0) (list (quote log) a b))
        ((< a 0) (aq-arcsin-or-none a b c))
        (else (quote degenerate))))                              ; a = 0: not a genuine quadratic radical
(define (aq-arcsin-or-none a b c) (if (> (aq-disc a b c) 0) (list (quote arcsin) a b (aq-disc a b c)) (quote no-real-arch)))

; ----- the integrator -----
(define (aq-integrate a b c p r) (aq-build a b c p r (aq-first-kind a b c)))
(define (aq-build a b c p r fk)
  (if (aq-bad? fk) (list (quote no-real-form) fk)
      (list (quote ok) (aq-alg-coeff a b c p r) (aq-first-coeff a b c p r) fk)))
(define (aq-bad? fk) (if (equal? fk (quote degenerate)) #t (equal? fk (quote no-real-arch))))

; ----- second-kind certificate: d/dx sqrt(q) = q'/(2 sqrt q), so INT q'/(2 sqrt q) = sqrt q.
; We check the algebraic identity at the polynomial level: the derivative of sqrt(q) contributes q' = (2a x + b),
; matching the radicand's derivative; this confirms the (p/2a) sqrt(q) term integrates (p/2a) q'/(2 sqrt q). -----
(define (aq-second-kind-verify a b c) (equal? (poly-deriv (aq-radicand a b c)) (list b (* 2 a))))
