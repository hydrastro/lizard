; -*- lisp -*-
; lib/cas/ode1.lisp — closed-form solutions of separable first-order ODEs, with
; certificates inherited from the certified rational-function integrator.
;
; A separable equation  y' = f(x) g(y)  with f and g rational is solved by separating
; variables and integrating both sides:
;
;     dy / g(y) = f(x) dx     =>     INT (1/g(y)) dy  =  INT f(x) dx  +  C.
;
; Writing g = gnum/gden, the left integrand is gden/gnum (rational in y) and the right
; is fnum/fden (rational in x).  Each side is handed to integrate-rational
; (lib/cas/integrate.lisp), which returns an antiderivative and -- crucially --
; verifies it by differentiating back over Q.  So the implicit solution
; G(y) = F(x) + C is certified exactly when both antiderivatives are: differentiating
; it implicitly gives G'(y) y' = F'(x), i.e. (1/g(y)) y' = f(x), i.e. y' = f(x) g(y),
; which is the original equation.  No separate differentiation engine is needed -- the
; integrator's own FTC certificate is the proof.
;
; Examples (all certified): y'=y gives log y = x + C;  y'=y^2 gives -1/y = x + C;
; y'=1+y^2 gives arctan(y) = x + C;  y'=x/y gives y^2/2 = x^2/2 + C.
;
; Builds on integrate.lisp (via rt-tower.lisp so algebraic-residue integrals are also
; available).  Top-level helpers only.

(import "cas/rt-tower.lisp")

(define (strip-C s)
  (let ((n (string-length s)))
    (if (and (>= n 4) (equal? (substring s (- n 4) n) " + C")) (substring s 0 (- n 4)) s)))

; separable  y' = (fnum/fden)(x) * (gnum/gden)(y)
;   -> (list 'ok "G(y) = F(x) + C" certified?) | (list 'cannot reason)
(define (solve-separable fnum fden gnum gden)
  (let ((G (integrate-rational gden gnum)) (F (integrate-rational fnum fden)))
    (if (and (equal? (car G) 'ok) (equal? (car F) 'ok))
        (list 'ok
              (string-append (strip-C (integral->string G "y")) " = " (strip-C (integral->string F "x")) " + C")
              (and (integrate-verify gden gnum G) (integrate-verify fnum fden F)))
        (list 'cannot 'integral-not-rational-elementary))))

; autonomous  y' = g(y)   is the special case f = 1
(define (solve-autonomous gnum gden) (solve-separable (list 1) (list 1) gnum gden))

(define (ode1-result->string r) (if (equal? (car r) 'ok) (car (cdr r)) "not resolved (an integral is not rational-elementary)"))
(define (ode1-certified? r) (and (equal? (car r) 'ok) (car (cdr (cdr r)))))
