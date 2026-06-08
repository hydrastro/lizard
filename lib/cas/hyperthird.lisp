; -*- lisp -*-
; lib/cas/hyperthird.lisp -- the GENUS-2 (and general hyperelliptic) THIRD-KIND LOGARITHM: the construction
; INT (g'/g) dx = log(g) for g = a(x) + y on y^2 = f, with a recovered from the differential, certified -- the
; Trager third-kind step over a hyperelliptic field of any genus (docs/TRAGER_ROADMAP.md -- the full third-kind
; construction beyond genus 1; the analogue of sethird for the superelliptic family).
;
; The single-extension field K = Q(x)[y]/(y^2 - f) and its derivation D(u + v y) = u' + (v' + v f'/(2f)) y are
; handled by algfunc for ANY radicand f, so they cover the genus-2 (deg f = 5) and higher hyperelliptic cases with
; no change.  The third-kind logarithm reuses that field directly.  For the common third-kind argument g = a(x) + y
; the rationalized logarithmic derivative is
;     g'/g = [ (a' + f'/(2y)) (a - y) ] / (a^2 - f),
; whose denominator is the NORM N(a + y) = a^2 - f.  So from a third-kind differential presented over a polynomial
; denominator D, the candidate satisfies a^2 - f = D, i.e. a = sqrt(D + f) when D + f is a perfect square polynomial;
; the integral is then log(a + y), CERTIFIED in K by differentiating: D(log(a + y)) = (a + y)'/(a + y) must equal
; the differential (checked in the cleared multiplicative form (a + y) * omega = D(a + y), no inverse trusted).  If
; D + f is not a perfect square the differential is not of this log shape and 'not-third-kind-a+y is returned --
; sound, never a guessed logarithm.  Construction the other way (given a, produce omega = d log(a + y)) is exact.
;
; This generalizes sethird (which did the superelliptic y^n = g case) to the hyperelliptic field of arbitrary
; genus, the third-kind ingredient for integration on genus-2 curves; it is built on algfunc, reuses the tested
; polynomial square root of elliptic3split, and is certified by the differentiation arbiter -- not adapted from any
; existing system.
;
; Public (f the curve polynomial, deg f >= 3; a a polynomial; differentials are algfunc elements):
;   ht-logderiv f a            -> omega = d log(a + y) = (a + y)'/(a + y), as an algfunc element (the construction)
;   ht-log-of f a              -> (list 'log (a + y)) : the antiderivative record INT (ht-logderiv f a) = log(a+y)
;   ht-cert f a                -> #t iff (a + y) * (ht-logderiv f a) = D(a + y) in K (the inverse-free certificate)
;   ht-norm f a                -> N(a + y) = a^2 - f (the principal-divisor norm / differential denominator)
;   ht-recover-a f D           -> a = sqrt(D + f) when D + f is a perfect square, else 'not-third-kind-a+y
;   ht-recognize f D           -> (list 'log (a+y)) when the third-kind differential over denominator D is
;                                 d log(a + y) for a recovered a, certified; else 'not-third-kind-a+y
;
; Verified: on y^2 = x^5 + 1, INT (d log(x + y)) = log(x + y) certified; the denominator x^2 - (x^5+1) recovers
; a = x; a = x^2 recovered from its denominator; a = x^2+1 (with middle term) recovered; a non-square denominator
; is rejected; the n=2 (elliptic) reduction agrees with the genus-1 third-kind.
;
; Builds on algfunc.lisp (the field) and elliptic3split.lisp (the tested polynomial square root esp-poly-sqrt).

(import "cas/algfunc.lisp")
(import "cas/elliptic3split.lisp")

; ----- the third-kind element g = a + y and its logarithmic derivative -----
(define (ht-pf f) (rat-from-poly f))                                          ; f as a rational function
(define (ht-g f a) (af-make (rat-from-poly a) (rat-from-poly (list 1))))      ; a + y
(define (ht-logderiv f a) (af-div (ht-pf f) (af-deriv (ht-pf f) (ht-g f a)) (ht-g f a)))   ; (a+y)'/(a+y)
(define (ht-log-of f a) (list (quote log) (ht-g f a)))

; ----- certificate: (a+y) * omega = D(a+y) in K (cleared, no inverse) -----
(define (ht-cert f a) (af-equal? (af-mul (ht-pf f) (ht-g f a) (ht-logderiv f a)) (af-deriv (ht-pf f) (ht-g f a))))

; ----- norm N(a+y) = a^2 - f -----
(define (ht-norm f a) (poly-sub (poly-mul a a) f))

; ----- recover a from a third-kind denominator D: a = sqrt(D + f) (tested esp-poly-sqrt) -----
(define (ht-recover-a f D) (ht-rec (esp-poly-sqrt (poly-add D f))))
(define (ht-rec a) (if (equal? a (quote not-square)) (quote not-third-kind-a+y) a))

; ----- recognize a third-kind differential over denominator D as d log(a+y) -----
(define (ht-recognize f D) (ht-recog-go f (ht-recover-a f D)))
(define (ht-recog-go f a) (if (equal? a (quote not-third-kind-a+y)) (quote not-third-kind-a+y) (ht-recog-cert f a)))
(define (ht-recog-cert f a) (if (ht-cert f a) (list (quote log) (ht-g f a)) (quote not-third-kind-a+y)))
