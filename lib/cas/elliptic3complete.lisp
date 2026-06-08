; -*- lisp -*-
; lib/cas/elliptic3complete.lisp -- the COMPLETE third-kind solver: given an integrand omega on the curve
; y^2 = q(x), DIRECTLY solve for g = u(x) + sqrt(q) with omega = g'/g (u a polynomial of ANY degree), with no
; brute-force search and no degree bound -- closing frontier (a), unbounded third-kind g (docs/CAS.md).
;
; The matching equations are LINEAR and solvable in closed form.  Writing omega = a + b*y as an element of
; K = Q(x)[y]/(y^2 - q) (a, b rational functions over Q), and g = u + y with u a polynomial, the condition
; omega = g'/g (equivalently omega*g = g', since g' = u' + (q'/(2q)) y) splits into
;     a + b u = q'/(2q)      (the y-component)   ... (Y)
;     a u + b q = u'         (the rational part) ... (R)
; Equation (Y) gives u DIRECTLY: u = (q'/(2q) - a) / b  (when b != 0).  So rather than search, we COMPUTE the
; unique candidate u from omega, then accept iff (i) u is a genuine polynomial (its rational form has denominator
; 1) and (ii) the second equation (R) holds and the K-certificate e3-certify confirms d/dx log(u + y) = omega.
; This is exact over Q and sound: a hit is certified by differentiation, and a non-polynomial u or a failed
; certificate yields an honest no-solution -- now with NO degree limit, the complete decision for this g-shape.
;
; Public:
;   e3c-candidate-u q omega    -> the rational function u = (q'/(2q) - a)/b (a, b the components of omega), or
;                                 'no-candidate when b = 0
;   e3c-solve q omega          -> (list 'found g) | (list 'no-solution ..) : the unique g = u + sqrt(q) with
;                                 g'/g = omega when u is a polynomial and the certificate holds; g is certified
;   e3c-integrate q omega      -> (list 'elementary-log g) | (list 'unknown ..) : INT omega = log(g) if solved
;
; Verified: omega = d/dx log(x + sqrt(x^3+1)) is solved DIRECTLY with g = x + sqrt(x^3+1) (no search); a
; higher-degree u, e.g. g = (x^2 - 1) + sqrt(x^5+1), is recovered directly; omega = 1/sqrt(x^3+1) (first kind,
; b makes u non-polynomial) returns no-solution; agreement with the bounded searcher on the cases it can reach.
;
; Builds on elliptic3.lisp (e3-certify, e3-logderiv) and algfunc.lisp / tower.lisp / poly.lisp.

(import "cas/elliptic3.lisp")
(import "cas/algfunc.lisp")
(import "cas/tower.lisp")
(import "cas/poly.lisp")

; ----- the direct candidate: u = (q'/(2q) - a)/b, where omega = a + b y -----
(define (e3c-candidate-u q omega) (e3c-cand (af-u omega) (af-v omega) q))
(define (e3c-cand a b q) (if (rat-zero? b) (quote no-candidate) (rat-div (rat-sub (e3c-half-logder q) a) b)))
(define (e3c-half-logder q) (rat-div (rat-deriv q) (rat-mul (rat-from-poly (list 2)) q)))   ; q'/(2q)

; ----- solve: compute u, require it be a polynomial (denominator 1), build g = u + y, certify -----
(define (e3c-solve q omega) (e3c-dispatch q omega (e3c-candidate-u q omega)))
(define (e3c-dispatch q omega u)
  (cond ((equal? u (quote no-candidate)) (list (quote no-solution) (quote y-component-zero)))
        ((e3c-poly? u) (e3c-check q omega u))
        (else (list (quote no-solution) (quote u-not-polynomial)))))
; a rational function is a polynomial iff its denominator is a nonzero constant (degree 0)
(define (e3c-poly? u) (= (poly-deg (rat-den u)) 0))
(define (e3c-check q omega u) (e3c-finish q omega (af-make u (rat-one))))
(define (e3c-finish q omega g) (if (e3-certify q g omega) (list (quote found) g) (list (quote no-solution) (quote certificate-failed))))

; ----- integrate -----
(define (e3c-integrate q omega) (e3c-result (e3c-solve q omega)))
(define (e3c-result s) (if (equal? (car s) (quote found)) (list (quote elementary-log) (car (cdr s))) (list (quote unknown) (car (cdr s)))))
