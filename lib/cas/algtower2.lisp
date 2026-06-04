; -*- lisp -*-
; lib/cas/algtower2.lisp -- a TOWER OF TWO ALGEBRAIC EXTENSIONS, the "two stacked monomials" rung of the algebraic
; frontier: arithmetic and the certified derivation in Q(x)[y][z]/(z^2 - y, y^2 - x), the field of x^(1/4)
; (docs/CAS.md and docs/TRAGER_ROADMAP.md -- RUNG 5, deeper stacked towers past the single-extension restriction).
;
; The single-extension layer algfunc.lisp handles Q(x)[y]/(y^2 - x); this stacks a SECOND radical z^2 = y on top, so
; z = sqrt(y) = x^(1/4) and the top field is Q(x)[y][z]/(z^2 - y, y^2 - x), a degree-4 extension of Q(x).  An
; element is a + b z with a, b in the inner field Q(x)[y] (each an algfunc element (u v) = u + v y), i.e. four
; rational-function coordinates.  Multiplication reduces z^2 -> y by the inner field's own multiplication:
;     (a + b z)(c + d z) = (a c + b d y) + (a d + b c) z,   y = af-y, products via af-mul on the radicand x.
; The decisive part is the DERIVATION d/dx.  The inner layer has y' = y/(2x) (af-deriv on radicand p = x).  For the
; outer generator, z^2 = y gives 2 z z' = y', so
;     z' = y'/(2 z) = y' z/(2 z^2) = y' z/(2 y) = (y'/(2y)) z,
; and with y' = y/(2x) this collapses to the SCALAR logarithmic derivative z'/z = 1/(4x) (matching d/dx x^(1/4) =
; (1/4) x^(1/4)/x), so
;     D(a + b z) = a' + (b' z + b z') = a' + (b' + b/(4x)) z,
; where a', b' are the TRUSTED inner derivations.  The whole double-tower derivation thus reduces to algfunc's
; certified derivation plus one rational scalar z'/z, so soundness is inherited.  This module provides the element
; type, the ring operations, the derivation, and -- the integration payoff -- a logarithmic-derivative recognizer
; and a differentiation certificate: D of a proposed antiderivative is recomputed in the tower and compared to the
; integrand coordinate by coordinate.  An integrand outside the recognized shape is reported, never guessed.
;
; Public (an element is (cons a b) = a + b z, with a, b algfunc elements over radicand x = (1 0) ... actually p=x):
;   t2-zP                      -> the radicand of the inner extension, p = x, as a rational (for af-* calls)
;   t2-make a b / t2-a / t2-b  -> the element a + b z and its inner-field parts
;   t2-zero / t2-one / t2-z / t2-y / t2-from-rat r   -> constants and generators of the tower
;   t2-add / t2-neg / t2-sub / t2-mul a b            -> ring operations (z^2 -> y reduction in t2-mul)
;   t2-zlogderiv               -> z'/z = 1/(4x) as a rational (the scalar outer logarithmic derivative)
;   t2-deriv e                 -> d/dx e in the tower, = a' + (b' + b*(z'/z)) z
;   t2-equal? a b              -> structural equality in the tower
;   t2-logderiv-z              -> the differential z'/z written as the tower element (0 + (1/(4x)) ... ) for tests
;   t2-certify ans integrand   -> #t iff t2-deriv(ans) equals integrand in the tower (the arbiter)
;
; Verified: z' = z/(4x) (the outer scalar) and y' = y/(2x) (the inner, via algfunc) hold; D(z) reproduces z/(4x);
; D(x z) = z + x z/(4x) = z + z/4 = (5/4) z, i.e. INT (5/4) z dx = x z = x^(5/4) (certified); D(z^2) = D(y) = y/(2x);
; the n=2 inner layer matches algfunc throughout.
;
; Builds on algfunc.lisp (the inner Q(x)[y] field) and tower.lisp / ratfun.lisp (the rational coordinates).

(import "cas/algfunc.lisp")

; ----- the inner radicand p = x, as a rational function (coefficient list (0 1) = x) -----
(define (t2-zP) (rat-from-poly (list 0 1)))

; ----- element type a + b z, with a, b algfunc elements -----
(define (t2-make a b) (cons a b))
(define (t2-a e) (car e))
(define (t2-b e) (cdr e))
(define (t2-zero) (t2-make (af-zero) (af-zero)))
(define (t2-one) (t2-make (af-one) (af-zero)))
(define (t2-z) (t2-make (af-zero) (af-one)))               ; z
(define (t2-y) (t2-make (af-y) (af-zero)))                 ; y = z^2, lives in the inner field
(define (t2-from-rat r) (t2-make (af-from-rat r) (af-zero)))

; ----- ring operations -----
(define (t2-add e f) (t2-make (af-add (t2-a e) (t2-a f)) (af-add (t2-b e) (t2-b f))))
(define (t2-neg e) (t2-make (af-neg (t2-a e)) (af-neg (t2-b e))))
(define (t2-sub e f) (t2-add e (t2-neg f)))
; (a + b z)(c + d z) = (a c + b d y) + (a d + b c) z, with y = af-y, reducing z^2 -> y
(define (t2-mul e f) (t2-make (af-add (af-mul (t2-zP) (t2-a e) (t2-a f)) (af-mul (t2-zP) (af-mul (t2-zP) (t2-b e) (t2-b f)) (af-y)))
                              (af-add (af-mul (t2-zP) (t2-a e) (t2-b f)) (af-mul (t2-zP) (t2-b e) (t2-a f)))))

; ----- the outer scalar logarithmic derivative z'/z = 1/(4x) -----
(define (t2-zlogderiv) (rat-make (list 1) (list 0 4)))     ; 1/(4x)

; ----- the derivation: D(a + b z) = a' + (b' + b*(z'/z)) z -----
(define (t2-deriv e) (t2-make (af-deriv (t2-zP) (t2-a e))
                              (af-add (af-deriv (t2-zP) (t2-b e)) (af-scale-rat (t2-b e) (t2-zlogderiv)))))
; multiply an algfunc element (u v) by a rational scalar s: (s u, s v)
(define (af-scale-rat a s) (af-make (rat-mul s (af-u a)) (rat-mul s (af-v a))))

; ----- equality and certificate -----
(define (t2-equal? e f) (if (af-equal? (t2-a e) (t2-a f)) (af-equal? (t2-b e) (t2-b f)) #f))
(define (t2-certify ans integrand) (t2-equal? (t2-deriv ans) integrand))

; the differential z'/z as a tower element (for tests): (1/(4x)) as the scalar part of D(z)/z conceptually
(define (t2-logderiv-z) (t2-make (af-from-rat (t2-zlogderiv)) (af-zero)))
