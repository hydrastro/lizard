; -*- lisp -*-
; lib/cas/tower2sub.lisp -- height-two integrals of substitution type, for ANY second monomial.
;
; The chain rule d/dx F(theta2) = F'(theta2) D2(theta2) holds whether theta2 is primitive OR exponential,
; so for any rational g in Q(theta2),
;     INT D2(theta2) g(theta2) dx = [ INT g(t) dt ]_{t = theta2}.
; Equivalently, an integrand A(theta2)/D(theta2) over K1 = Q(x)(theta1) is of this type exactly when the
; rational function g = A / (D * D2theta2), reduced over K1[theta2], lies in Q(theta2) (rational-NUMBER
; coefficients).  When it does, the whole integral is the trusted rational-function integrator
; integrate.lisp over Q -- logarithms for linear factors, arctangents for irreducible quadratics -- with
; NO Sylvester resultant over K1 and none of its memory blow-up.  This subsumes tower2primrat.lisp /
; tower2primfull.lisp (where D2theta2 is a scalar in K1) and, crucially, ALSO covers the EXPONENTIAL
; second monomial, where D2theta2 = u' theta2 is degree one in theta2 and the reduction needs a genuine
; polynomial gcd over K1[theta2].  For theta2 = exp(e^x) (so D2theta2 = e^x theta2):
;     INT e^x exp(e^x) / (exp(2 e^x) + 1) dx = arctan(exp(e^x)),
;     INT e^x exp(e^x) / (exp(e^x) - 1)   dx = log(exp(e^x) - 1).
; The reduction g = A/(D D2theta2) is computed by an exact polynomial gcd in K1[theta2] (h2-gcd/h2-div),
; then normalized by the leading coefficient of its denominator and read off as Q-polynomials; the gcd is
; cheap (no resultant).  The certificate is two-part and both halves are light: (i) the reduction is exact
; in K1[theta2], checked by cross-multiplication A * Dbar = D * D2theta2 * Abar, which certifies that the
; integrand really is D2theta2 * Abar/Dbar; and (ii) integrate-verify certifies d/dtheta2 of the answer
; equals Abar/Dbar over Q.  By the chain rule these give the height-two certificate D2(answer) = A/D with
; no heavy K1 derivation.  Only integrands whose g is constant in x (coefficients in Q) are of this type;
; x-dependent arguments and residues algebraic over K1 still need the resultant.  Builds on tower2rt.lisp
; and integrate.lisp (imported in that order; the rational type rat is shared with ratfun.lisp).

(import "cas/tower2rt.lisp")
(import "cas/integrate.lisp")

(define (t2sub-coeff c fac)                      ; c/fac as a rational NUMBER, or 'notrat if not a pure rational
  (let ((q (k1-div c fac)))
    (if (rat-zero? (k1rt-rf-c0 (car (cdr q))))
        (quote notrat)
        (let ((r (k1-to-rational q)))
          (if (tr-equal? (tr-reduce q) (tr-reduce (k1-from-rat r))) r (quote notrat))))))
(define (t2sub-list p fac acc)                   ; Q-poly (low->high) from h2poly p, dividing each coeff by fac
  (if (null? p) (reverse acc)
      (let ((r (t2sub-coeff (car p) fac)))
        (if (equal? r (quote notrat)) (quote notrat) (t2sub-list (cdr p) fac (cons r acc))))))
(define (t2sub-toh2 q) (if (null? q) (quote ()) (cons (k1-from-rat (car q)) (t2sub-toh2 (cdr q)))))   ; Q-poly -> h2poly

(define (int-h2-sub A D Dth2 mono1)              ; -> (ok Abar Dbar (ok ratpart logs arctans)) | (notreducible)
  (let ((DD (h2-mul D Dth2)))
    (let ((G (h2-gcd A DD)))
      (let ((gn (h2-div A G)) (gd (h2-div DD G)))
        (let ((ld (h2-lead gd)))
          (let ((Abar (t2sub-list gn ld (quote ()))))
            (if (equal? Abar (quote notrat)) (list (quote notreducible))
                (let ((Dbar (t2sub-list gd ld (quote ()))))
                  (if (equal? Dbar (quote notrat)) (list (quote notreducible))
                      (list (quote ok) Abar Dbar (integrate-rational Abar Dbar)))))))))))
(define (int-h2-sub-elementary? A D Dth2 mono1)
  (let ((r (int-h2-sub A D Dth2 mono1)))
    (if (equal? (car r) (quote ok)) (equal? (car (car (cdr (cdr (cdr r))))) (quote ok)) #f)))
(define (int-h2-sub-verify A D Dth2 mono1)       ; (i) reduction exact in K1[theta2]  AND  (ii) integrate-verify over Q
  (let ((r (int-h2-sub A D Dth2 mono1)))
    (if (equal? (car r) (quote ok))
        (if (h2-equal? (h2-norm (h2-mul A (t2sub-toh2 (car (cdr (cdr r))))))
                       (h2-norm (h2-mul (h2-mul D Dth2) (t2sub-toh2 (car (cdr r))))))
            (integrate-verify (car (cdr r)) (car (cdr (cdr r))) (car (cdr (cdr (cdr r)))))
            #f)
        #f)))
