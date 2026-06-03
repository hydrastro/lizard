; -*- lisp -*-
; lib/cas/algfunc.lisp -- a first step into ALGEBRAIC FUNCTIONS: the differential field of a square root.
;
; Everything above this point integrates in transcendental towers.  This module opens the algebraic case
; with the simplest nontrivial algebraic function field K = Q(x)[y]/(y^2 - p), where p is a polynomial in x
; and y = sqrt(p).  Elements are u + v y with u, v in Q(x); K is a field (the conjugate u - v y inverts any
; nonzero element), and it carries the unique derivation extending d/dx with y' = p'/(2y), i.e.
;     D(u + v y) = u' + ( v' + v p'/(2p) ) y.
; On top of this we integrate the standard quadratic-radical families in closed form and CERTIFY each by
; differentiation inside K -- the differentiation certificate is the same arbiter used everywhere else,
; now over an algebraic (not transcendental) extension:
;     INT (a x + b)/sqrt(p) dx = a sqrt(p) + (b - a*b1/2) log( x + b1/2 + sqrt(p) ),
;     INT sqrt(p) dx          = ((x + b1/2)/2) sqrt(p) + ((c0 - b1^2/4)/2) log( x + b1/2 + sqrt(p) ),
; for p = x^2 + b1 x + c0 (monic).  The logarithm argument g = x + b1/2 + sqrt(p) lies in K, and the
; certificate checks D(alg-part + clog * log(g)) = integrand exactly in K.  This is the rationalizable /
; standard-radical slice; the general algebraic-function integration of Trager (arbitrary curves, the
; elliptic and higher-genus non-elementary cases) is the genuine summit beyond it.  Builds on tower.lisp
; only for the Q(x) coefficient algebra with its d/dx derivation (rat-deriv).

(import "cas/tower.lisp")

; ----- K = Q(x)[y]/(y^2 - p): element (list u v) = u + v y, with u, v rats over Q(x) -----
(define (af-u e) (car e))
(define (af-v e) (car (cdr e)))
(define (af-make u v) (list u v))
(define (af-zero) (list (rat-zero) (rat-zero)))
(define (af-one) (list (rat-one) (rat-zero)))
(define (af-from-rat r) (list r (rat-zero)))
(define (af-y) (list (rat-zero) (rat-one)))
(define (af-add a b) (list (rat-add (af-u a) (af-u b)) (rat-add (af-v a) (af-v b))))
(define (af-neg a) (list (rat-neg (af-u a)) (rat-neg (af-v a))))
(define (af-sub a b) (af-add a (af-neg b)))
(define (af-two) (rat-from-poly (list 2)))
(define (af-mul p a b)
  (list (rat-add (rat-mul (af-u a) (af-u b)) (rat-mul (rat-mul (af-v a) (af-v b)) p))
        (rat-add (rat-mul (af-u a) (af-v b)) (rat-mul (af-u b) (af-v a)))))
(define (af-inv p a)
  (let ((den (rat-sub (rat-mul (af-u a) (af-u a)) (rat-mul (rat-mul (af-v a) (af-v a)) p))))
    (list (rat-div (af-u a) den) (rat-div (rat-neg (af-v a)) den))))
(define (af-div p a b) (af-mul p a (af-inv p b)))
(define (af-equal? a b) (if (rat-equal? (af-u a) (af-u b)) (rat-equal? (af-v a) (af-v b)) #f))
; the half-logarithmic-derivative of the radicand: p'/(2p), a rat
(define (af-hp p) (rat-div (rat-deriv p) (rat-mul (af-two) p)))
; D(u + v y) = u' + (v' + v p'/(2p)) y
(define (af-deriv p a)
  (list (rat-deriv (af-u a)) (rat-add (rat-deriv (af-v a)) (rat-mul (af-v a) (af-hp p)))))

; ----- the differentiation certificate over K: D(alg + clog log(g)) = integrand ? -----
;   clog a rat (Q(x)) constant-or-function coefficient, g and alg and integrand all in K
(define (af-logderiv p clog g) (af-mul p (af-from-rat clog) (af-div p (af-deriv p g) g)))   ; clog * g'/g
(define (af-certify p alg clog g integrand) (af-equal? (af-add (af-deriv p alg) (af-logderiv p clog g)) integrand))

; ----- closed-form quadratic-radical integrals, p = x^2 + b1 x + c0 (monic) -----
(define (af-half-b p) (/ (poly-coeff (rat-num p) 1) 2))                       ; b1/2   (p monic, den 1)
(define (af-c0 p) (poly-coeff (rat-num p) 0))                                 ; c0
(define (af-logarg p) (af-make (rat-from-poly (list (af-half-b p) 1)) (rat-one)))   ; x + b1/2 + y
; INT (a x + b)/sqrt(p) dx  ->  (list alg clog g)
(define (int-sqrt-linear a b p)
  (list (af-make (rat-zero) (rat-from-poly (list a)))                          ; a y
        (rat-from-poly (list (- b (* a (af-half-b p)))))                       ; b - a b1/2
        (af-logarg p)))
(define (int-sqrt-linear-integrand a b p) (af-mul p (af-from-rat (rat-from-poly (list b a))) (af-inv p (af-y))))  ; (a x + b)/y
; INT sqrt(p) dx  ->  (list alg clog g)
(define (int-sqrt-quad p)
  (list (af-mul p (af-from-rat (rat-make (list (af-half-b p) 1) (list 2))) (af-y))   ; ((x+b1/2)/2) y
        (rat-from-poly (list (/ (- (af-c0 p) (* (af-half-b p) (af-half-b p))) 2)))   ; (c0 - (b1/2)^2)/2
        (af-logarg p)))
(define (int-sqrt-quad-integrand p) (af-y))                                          ; sqrt(p)
