; A first step into ALGEBRAIC FUNCTIONS.  Every earlier integrator works in a transcendental tower; this
; example integrates genuine algebraic functions -- square roots of quadratics -- in the algebraic function
; field K = Q(x)[y]/(y^2 - p), y = sqrt(p).  K is a field (the conjugate u - v y inverts u + v y) carrying
; the derivation that extends d/dx with y' = p'/(2y).  The standard quadratic-radical antiderivatives are
; produced in closed form and CERTIFIED by differentiation INSIDE K -- the same differentiation arbiter used
; throughout the project, now over an algebraic extension rather than a transcendental one:
;     INT dx/sqrt(x^2+1)        = log(x + sqrt(x^2+1))            ( = arcsinh x ),
;     INT x/sqrt(x^2+1) dx      = sqrt(x^2+1),
;     INT (2x+3)/sqrt(x^2+1) dx = 2 sqrt(x^2+1) + 3 log(x + sqrt(x^2+1)),
;     INT dx/sqrt(x^2+2x+5)     = log(x + 1 + sqrt(x^2+2x+5)),
;     INT sqrt(x^2+1) dx        = (x/2) sqrt(x^2+1) + (1/2) log(x + sqrt(x^2+1)).
; Each certificate checks D(alg-part + clog*log(g)) = integrand exactly in K, with g = x + b1/2 + sqrt(p)
; the logarithm argument.  This is the rationalizable / standard-radical slice; the general algebraic-
; function integration of Trager (arbitrary curves; the elliptic and higher-genus non-elementary integrals)
; is the genuine summit beyond it.
(import "cas/algfunc.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define (cert-lin a b p) (let ((r (int-sqrt-linear a b p)))
  (af-certify p (car r) (car (cdr r)) (car (cdr (cdr r))) (int-sqrt-linear-integrand a b p))))
(define (cert-quad p) (let ((r (int-sqrt-quad p)))
  (af-certify p (car r) (car (cdr r)) (car (cdr (cdr r))) (int-sqrt-quad-integrand p))))
(define P1 (rat-from-poly (list 1 0 1)))      ; x^2 + 1
(define P2 (rat-from-poly (list 5 2 1)))      ; x^2 + 2x + 5
(display "Algebraic functions: square roots of quadratics, certified by differentiation in Q(x)[sqrt p]") (newline) (newline)
(must "INT dx/sqrt(x^2+1) = arcsinh x = log(x+sqrt(x^2+1))"      (cert-lin 0 1 P1))
(must "INT x/sqrt(x^2+1) dx = sqrt(x^2+1)"                        (cert-lin 1 0 P1))
(must "INT (2x+3)/sqrt(x^2+1) dx (mixed: algebraic + log)"        (cert-lin 2 3 P1))
(must "INT dx/sqrt(x^2+2x+5) = log(x+1+sqrt(x^2+2x+5))"          (cert-lin 0 1 P2))
(must "INT (5x-4)/sqrt(x^2+2x+5) dx"                              (cert-lin 5 -4 P2))
(must "INT sqrt(x^2+1) dx = (x/2)sqrt(x^2+1) + (1/2)log(...)"     (cert-quad P1))
(must "INT sqrt(x^2+2x+5) dx"                                     (cert-quad P2))
(newline) (display "quadratic-radical algebraic-function integrals certified.") (newline)
