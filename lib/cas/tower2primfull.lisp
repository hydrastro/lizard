; -*- lisp -*-
; lib/cas/tower2primfull.lisp -- height-two primitive integrals with ALGEBRAIC residues (arctangents),
; by the same substitution that powers tower2primrat.lisp, but routed through the complete rational-
; function integrator integrate.lisp so that an irreducible-quadratic denominator factor produces an
; honest arctangent rather than being reported non-elementary.
;
; For a primitive second monomial theta2 (D2 theta2 = Dtheta2 in K1), the chain rule d/dx F(theta2) =
; F'(theta2) Dtheta2 gives, for Abar, Dbar in Q[theta2],
;     INT Dtheta2 Abar(theta2)/Dbar(theta2) dx = [ INT Abar/Dbar d(theta2) ]_{t = theta2},
; the pullback of an ordinary rational-function integral in theta2 over Q.  integrate-rational handles
; that completely up to irreducible quadratics: a linear denominator factor gives a logarithm (rational
; residue) and an irreducible quadratic with negative discriminant gives an arctangent, all certified by
; differentiation over Q.  So the whole height-two integral is obtained with NO Sylvester resultant over
; K1 and none of its memory blow-up, e.g.
;     INT (Dtheta2)/(theta2^2 + 1) dx = arctan(theta2) = arctan(log(e^x + 1)),
; and the mixed INT (Dtheta2)(theta2^2 + theta2 + 1)/(theta2^3 + theta2) dx = log(theta2) + arctan(theta2).
; The reduction is gated by reconstructing each coefficient (A_i = Dtheta2 * Abar_i exactly in K1, D_i =
; Dbar_i), so the substitution is certified to apply; integrate-verify then supplies the differentiation
; certificate over Q, and together they give the height-two D2 certificate.  This complements
; tower2primrat.lisp (rational residues via rat-integrate) by closing the arctangent case; the general
; K1-coefficient case (x-dependent arguments, higher algebraic residues over K1) still routes through the
; resultant and remains memory-bound.  Builds on tower2rt.lisp and integrate.lisp (imported in that order
; so the three-argument tower hermite stays available for the K1 ops while integrate-rational integrates
; over Q via partial fractions; the rational type rat is shared between the tower and ratfun.lisp).

(import "cas/tower2rt.lisp")
(import "cas/integrate.lisp")

(define (t2pf-coeff c fac)                       ; c/fac as a rational NUMBER, or 'notrat if not a pure rational
  (let ((q (k1-div c fac)))
    (if (rat-zero? (k1rt-rf-c0 (car (cdr q))))   ; denominator theta1-constant-term zero => theta1-dependent, not rational
        (quote notrat)
        (let ((r (k1-to-rational q)))
          (if (tr-equal? (tr-reduce q) (tr-reduce (k1-from-rat r))) r (quote notrat))))))
(define (t2pf-list p fac acc)                    ; extract Q-poly (low->high) from h2poly p, dividing each coeff by fac
  (if (null? p) (reverse acc)
      (let ((r (t2pf-coeff (car p) fac)))
        (if (equal? r (quote notrat)) (quote notrat) (t2pf-list (cdr p) fac (cons r acc))))))
(define (int-h2-prim-full A D Dth2 mono1)        ; -> (ok Abar Dbar (ok ratpart logs arctans)) | (notreducible)
  (let ((Abar (t2pf-list A Dth2 (quote ()))))
    (if (equal? Abar (quote notrat)) (list (quote notreducible))
        (let ((Dbar (t2pf-list D (k1-one) (quote ()))))
          (if (equal? Dbar (quote notrat)) (list (quote notreducible))
              (list (quote ok) Abar Dbar (integrate-rational Abar Dbar)))))))
(define (int-h2-prim-full-elementary? A D Dth2 mono1)
  (let ((r (int-h2-prim-full A D Dth2 mono1)))
    (if (equal? (car r) (quote ok)) (equal? (car (car (cdr (cdr (cdr r))))) (quote ok)) #f)))
(define (int-h2-prim-full-verify A D Dth2 mono1) ; certified: reduction applies (A = Dth2*Abar, D = Dbar) AND d/dtheta2 = Abar/Dbar
  (let ((r (int-h2-prim-full A D Dth2 mono1)))
    (if (equal? (car r) (quote ok))
        (integrate-verify (car (cdr r)) (car (cdr (cdr r))) (car (cdr (cdr (cdr r)))))
        #f)))
