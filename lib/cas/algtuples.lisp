; -*- lisp -*-
; lib/cas/algtuples.lisp -- ASSEMBLY OF COMPLETE ALGEBRAIC SOLUTION TUPLES for a triangular system whose leading
; coordinate is irrational: given the minimal polynomial of x_0 = alpha and the later coordinates expressed as
; polynomials in x_0, build the full solution point (alpha, h_1(alpha), ...) with every coordinate an exact
; element of Q(alpha), and certify it by evaluating every generator to zero in Q(alpha) (docs/CAS.md -- summit S3,
; the tuple assembly that polysolve3's per-coordinate naming left open).
;
; polysolve3 names each coordinate's value separately (rational, or an algebraic number alpha = root of a minimal
; polynomial).  This module pairs them into a complete solution POINT.  For the triangular system
;     m(x_0) = 0,   x_1 = h_1(x_0),   x_2 = h_2(x_0),  ...
; (the shape a lex Groebner basis takes once the eliminant m is the minimal polynomial of the leading coordinate),
; each root alpha of m gives the point (alpha, h_1(alpha), ..., h_k(alpha)), and every later coordinate h_j(alpha)
; is computed EXACTLY in the number field Q(alpha) by alg-eval (Horner in Q(alpha)).  The point is certified by
; substituting it into every original generator and checking the result is zero IN Q(alpha) -- exact algebraic
; arithmetic, no floating point and no numerical tolerance.  Coordinates are carried as algebraic-number objects,
; so the tuple is a genuine point of the variety over Q(alpha), not a decimal approximation.
;
; Public (minp = minimal polynomial of alpha, low->high; hs = list of polynomials h_j(x_0); a "tuple" is a list of
; algebraic-number coordinates over Q(alpha)):
;   at-root minp               -> alpha = the field generator (a root of minp) as an algebraic number
;   at-coord minp h            -> h(alpha) as an element of Q(alpha)
;   at-tuple minp hs           -> the solution point (alpha, h_1(alpha), ..., h_k(alpha)) over Q(alpha)
;   at-eval-gen minp gen tuple -> the 2-variable generator evaluated at the tuple, as an element of Q(alpha)
;   at-certify minp gens tuple -> #t iff every generator in gens vanishes (is zero in Q(alpha)) at the tuple
;
; Verified: for <x^2 - 2, y - x> the tuple (alpha, alpha) with alpha = sqrt(2) is assembled and certified to make
; both x^2 - 2 and y - x vanish in Q(sqrt 2); for <x^2 - 2, y - x^2> the second coordinate evaluates to the
; rational 2 inside Q(sqrt 2); a wrong tuple (alpha, alpha + 1) fails the certificate.
;
; Builds on algnum.lisp and poly.lisp.

(import "cas/algnum.lisp")
(import "cas/poly.lisp")

(define (at-len l) (if (null? l) 0 (+ 1 (at-len (cdr l)))))
(define (at-nth l k) (if (= k 0) (car l) (at-nth (cdr l) (- k 1))))
(define (at-map f l) (if (null? l) (quote ()) (cons (f (car l)) (at-map f (cdr l)))))

; ----- the algebraic generator alpha and a coordinate h(alpha) in Q(alpha) -----
(define (at-root minp) (alg-gen minp))
(define (at-coord minp h) (alg-eval h (at-root minp)))

; ----- the full tuple (alpha, h_1(alpha), ..., h_k(alpha)) -----
(define (at-tuple minp hs) (cons (at-root minp) (at-map (lambda (h) (at-coord minp h)) hs)))

; ----- evaluate a 2-variable mpoly generator at a tuple (each coordinate an algebraic number in Q(alpha)) -----
; generator term = (coeff . (e0 e1)); value = sum coeff * x0^e0 * x1^e1 in Q(alpha).
(define (at-eval-gen minp gen tuple) (at-sum-terms minp gen tuple))
(define (at-sum-terms minp gen tuple) (if (null? gen) (alg-zero minp) (alg-add (at-term minp (car gen) tuple) (at-sum-terms minp (cdr gen) tuple))))
(define (at-term minp term tuple) (alg-scale-q (car term) (at-mono minp (cdr term) tuple)))
; monomial x0^e0 * x1^e1 * ... : multiply the tuple coordinates raised to the exponents, in Q(alpha)
(define (at-mono minp exps tuple) (at-mono-go minp exps tuple 0 (alg-one minp)))
(define (at-mono-go minp exps tuple j acc) (if (null? exps) acc (at-mono-go minp (cdr exps) tuple (+ j 1) (alg-mul acc (at-pow minp (at-nth tuple j) (car exps))))))
(define (at-pow minp a e) (if (<= e 0) (alg-one minp) (alg-mul a (at-pow minp a (- e 1)))))
; scale an algebraic number by a rational constant: represent the constant in Q(alpha) and multiply
(define (alg-scale-q c a) (alg-mul (alg-from-q (alg-min a) c) a))

; ----- certify: every generator vanishes in Q(alpha) at the tuple -----
(define (at-certify minp gens tuple) (at-all-zero minp gens tuple))
(define (at-all-zero minp gens tuple) (cond ((null? gens) #t) ((alg-zero? (at-eval-gen minp (car gens) tuple)) (at-all-zero minp (cdr gens) tuple)) (else #f)))
