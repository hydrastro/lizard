; -*- lisp -*-
; lib/cas/tower2primrat.lisp -- height-two primitive integrals with rational coefficients, via the
; substitution that turns them into rational-function integration.
;
; For a primitive second monomial theta2 (D2 theta2 = Dtheta2 in K1 = Q(x)(theta1)), an integrand of the
; form Dtheta2 * Abar(theta2) / Dbar(theta2) with Abar, Dbar in Q[theta2] (rational-NUMBER coefficients,
; constant in x and theta1) is, by the chain rule d/dx F(theta2) = F'(theta2) Dtheta2, exactly the
; pullback of the rational-function integral INT Abar/Dbar d(theta2):
;     INT Dtheta2 Abar(theta2)/Dbar(theta2) dx = [ INT Abar(t)/Dbar(t) dt ] with t = theta2.
; So the entire Rothstein-Trager logarithmic part is computed by the trusted rational-function integrator
; rat-integrate (rischrat.lisp) over Q -- no Sylvester resultant over K1, no K1 fraction arithmetic, and
; therefore none of the memory blow-up that the K1-coefficient resultant path suffers.  This unlocks
; height-two primitive integrals of arbitrary denominator degree whose logarithm arguments and residues
; are rational, e.g.
;     INT (Dtheta2)(6 theta2^2 - 10 theta2 + 2)/(theta2^3 - 3 theta2^2 + 2 theta2) dx
;        = log(theta2) + 2 log(theta2 - 1) + 3 log(theta2 - 2),
; a cubic denominator (three rational residues) that the determinant-based resultant could not evaluate.
; The reduction is gated by reconstructing each coefficient: Abar_i is accepted only if A_i = Dtheta2 *
; Abar_i exactly in K1 (and Dbar_i = D_i), so the substitution is certified to apply; the logarithmic part
; then carries rat-integrate's own differentiation certificate over Q.  Builds on tower2rt.lisp and
; rischrat.lisp (imported in that order so the three-argument tower hermite stays live for the K1 ops
; while rat-integrate uses the two-argument rational hermite).

(import "cas/tower2rt.lisp")
(import "cas/rischrat.lisp")

(define (t2pr-coeff c fac)                       ; c/fac as a rational NUMBER, or 'notrat if not a pure rational
  (let ((q (k1-div c fac)))
    (if (rat-zero? (k1rt-rf-c0 (car (cdr q))))   ; denominator theta1-constant-term zero => theta1-dependent, not rational
        (quote notrat)
        (let ((r (k1-to-rational q)))
          (if (tr-equal? (tr-reduce q) (tr-reduce (k1-from-rat r))) r (quote notrat))))))
(define (t2pr-list p fac acc)                    ; extract Q-poly (low->high) from h2poly p, dividing each coeff by fac
  (if (null? p) (reverse acc)
      (let ((r (t2pr-coeff (car p) fac)))
        (if (equal? r (quote notrat)) (quote notrat) (t2pr-list (cdr p) fac (cons r acc))))))
(define (int-h2-prim-rat A D Dth2 mono1)         ; -> (ok Abar Dbar (ratnum ratden logterms complete?)) | (notreducible)
  (let ((Abar (t2pr-list A Dth2 (quote ()))))
    (if (equal? Abar (quote notrat)) (list (quote notreducible))
        (let ((Dbar (t2pr-list D (k1-one) (quote ()))))
          (if (equal? Dbar (quote notrat)) (list (quote notreducible))
              (list (quote ok) Abar Dbar (rat-integrate Abar Dbar)))))))
(define (int-h2-prim-rat-elementary? A D Dth2 mono1)
  (let ((r (int-h2-prim-rat A D Dth2 mono1)))
    (if (equal? (car r) (quote notreducible)) #f (rat-integrate-complete? (car (cdr r)) (car (cdr (cdr r)))))))
(define (int-h2-prim-rat-verify A D Dth2 mono1)  ; certified: reduction applies (A = Dth2*Abar, D = Dbar) AND d/dtheta2 = Abar/Dbar
  (let ((r (int-h2-prim-rat A D Dth2 mono1)))
    (if (equal? (car r) (quote notreducible)) #f
        (rat-integrate-verify (car (cdr r)) (car (cdr (cdr r)))))))
