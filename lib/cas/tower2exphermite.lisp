; -*- lisp -*-
; lib/cas/tower2exphermite.lisp -- Hermite reduction for an exponential second monomial at height two.
;
; This is the exponential mirror of the primitive Hermite reduction in tower2herm.lisp.  For
; theta2 = exp(u) over K1 = Q(x)(theta1) with derivation D2 theta2 = u' theta2, an integrand A/D whose
; denominator D is coprime to theta2 (the normal part) and not squarefree in theta2 is reduced to a
; rational part g plus a remainder A*/D* with D* squarefree in theta2, exactly
;   INT A/D = g + INT A*/D*.
; The algorithm is identical to the primitive case -- squarefree-factor D in theta2 (h2-yun), and for the
; factor V of highest multiplicity m peel one order using the Bezout relation against V and the
; derivative term, lowering the power of V by one -- with the single change that the derivation is the
; exponential t2e-deriv instead of the primitive t2-deriv.  Correctness rests on the normality of
; exponential monomials: a squarefree V coprime to theta2 satisfies gcd(V, D2 V) = 1, so the Bezout step
; is valid.  The reduction is purely rational (gcd, division, Bezout over K1[theta2]); it does not use the
; Rothstein-Trager resultant, so it is unaffected by the resultant memory wall.  This integrates, e.g.,
;   INT -theta1 theta2/(theta2 - 1)^2 dx = theta2/(theta2 - 1),
; whose antiderivative is purely rational, with a zero squarefree remainder.  The result is certified by
; differentiating the rational part with D2 and adding the remainder, matching A/D over K1[theta2].
; Builds on tower2exp.lisp (and through it tower2herm.lisp for the h2poly machinery).

(import "cas/tower2exp.lisp")

(define (t2eh-deriv N D uprime mono1)            ; quotient rule for an h2tr fraction under D2
  (list (h2-sub (h2-mul (t2e-deriv N uprime mono1) D) (h2-mul N (t2e-deriv D uprime mono1))) (h2-mul D D)))
(define (t2e-hermite-step a d m v uprime mono1)  ; peel one order of the highest-multiplicity factor v
  (let ((w (h2-div d (h2-pow v m))) (Dv (t2e-deriv v uprime mono1)))
    (let ((b (h2-rem (h2-mul (h2-neg a) (h2-invmod (h2-iscale (- m 1) (h2-mul w Dv)) v)) v)))
      (let ((Db (t2e-deriv b uprime mono1)))
        (let ((num (h2-add a (h2-add (h2-neg (h2-mul w (h2-mul Db v))) (h2-iscale (- m 1) (h2-mul w (h2-mul b Dv)))))))
          (list (list b (h2-pow v (- m 1)))
                (h2-div num v)
                (h2-mul (h2-pow v (- m 1)) w)))))))
(define (t2e-hermite-loop a d g uprime mono1)
  (let ((hi (h2-max-mult (h2-yun d) (cons 0 (quote ())))))
    (if (<= (car hi) 1) (list g a d)
      (let ((step (t2e-hermite-step a d (car hi) (cdr hi) uprime mono1)))
        (t2e-hermite-loop (car (cdr step)) (car (cdr (cdr step))) (h2tr-add g (car step)) uprime mono1)))))
(define (t2e-hermite a d uprime mono1) (t2e-hermite-loop a d (h2tr-zero) uprime mono1))  ; -> (g-h2tr a* d*), d* squarefree in theta2
(define (t2e-hermite-check H A D uprime mono1)   ; certify an ALREADY-COMPUTED reduction (no recomputation)
  (h2tr-equal? (h2tr-add (t2eh-deriv (car (car H)) (car (cdr (car H))) uprime mono1) (list (car (cdr H)) (car (cdr (cdr H))))) (list A D)))
(define (t2e-hermite-verify A D uprime mono1)
  (let ((H (t2e-hermite A D uprime mono1)))
    (h2tr-equal? (h2tr-add (t2eh-deriv (car (car H)) (car (cdr (car H))) uprime mono1) (list (car (cdr H)) (car (cdr (cdr H))))) (list A D))))
