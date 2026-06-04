; -*- lisp -*-
; lib/cas/weier.lisp -- WEIERSTRASS SUBSTITUTION for integrals of rational functions of sin and cos.
; The substitution t = tan(x/2) gives sin x = 2t/(1+t^2), cos x = (1-t^2)/(1+t^2), dx = 2 dt/(1+t^2), turning
; ANY integral INT R(sin x, cos x) dx into the integral of a rational function of t -- which the certified
; rational integrator (integrate.lisp) handles completely.  This extends trigonometric integration from the
; sin^m cos^n monomials (trigint.lisp) to arbitrary rational trigonometric integrands, and the answer is exact
; in t = tan(x/2), carried by the rational integrator's own differentiate-back certificate.
;
; The integrand is given as a quotient N(s,c)/D(s,c) with N and D each a "trig polynomial": a list of terms
; (coeff s-power c-power).  Each monomial s^i c^j maps to (2t)^i (1-t^2)^j / (1+t^2)^{i+j}; clearing the common
; (1+t^2) powers and folding in dx = 2/(1+t^2) yields a rational function P(t)/Q(t):
;     INT N/D dx  =  INT [ Nbar(t) * 2 ] / [ Dbar(t) * (1+t^2) ] dt,
; where Nbar, Dbar lift N, D to a common (1+t^2)-power.  This module forms P, Q and calls integrate-rational.
;
; Public: we-substitute Nterms Dterms -> (cons P Q), the rational function of t to integrate; we-integrate
; Nterms Dterms -> the rational integrator's result in t (with t = tan(x/2)); we-verify checks it.  Verified:
; INT dx/(1+cos x) = tan(x/2); INT dx/(2+cos x) = (2/sqrt 3) arctan(tan(x/2)/sqrt 3); INT dx/(2+sin x).
;
; Builds on poly.lisp and integrate.lisp.

(import "cas/poly.lisp")
(import "cas/integrate.lisp")

(define (we-co t) (car t))
(define (we-sp t) (car (cdr t)))
(define (we-cp t) (car (cdr (cdr t))))

(define (we-ppow p n) (if (= n 0) (list 1) (poly-mul p (we-ppow p (- n 1)))))
; max total degree (i+j) over the terms -- the common (1+t^2) power we lift everything to
(define (we-maxdeg terms) (we-md-go terms 0))
(define (we-md-go terms m) (if (null? terms) m (let ((d (+ (we-sp (car terms)) (we-cp (car terms))))) (we-md-go (cdr terms) (if (> d m) d m)))))

; convert one trig polynomial (list of (co sp cp)) to a polynomial in t, lifted to denominator (1+t^2)^D.
; a term co s^i c^j -> co (2t)^i (1-t^2)^j (1+t^2)^{D-i-j}.
(define (we-tp->poly terms D) (we-tp-go terms D (list 0)))
(define (we-tp-go terms D acc)
  (if (null? terms) acc
      (let ((t (car terms)))
        (let ((i (we-sp t)) (j (we-cp t)))
          (let ((term (poly-scale (we-co t)
                        (poly-mul (we-ppow (list 0 2) i)
                          (poly-mul (we-ppow (list 1 0 -1) j)
                                    (we-ppow (list 1 0 1) (- D (+ i j))))))))
            (we-tp-go (cdr terms) D (poly-add acc term)))))))

; the rational function of t for INT N/D dx.  Lift N and D to a common (1+t^2)-power Dmax = max(deg N, deg D),
; then INT [Nbar/( (1+t^2)^Dmax )] / [Dbar/( (1+t^2)^Dmax )] dx = INT (Nbar/Dbar) dx; with dx = 2/(1+t^2):
;   P/Q = (Nbar * 2) / (Dbar * (1+t^2)).
(define (we-substitute Nterms Dterms)
  (let ((Dmax (let ((a (we-maxdeg Nterms)) (b (we-maxdeg Dterms))) (if (> a b) a b))))
    (let ((Nbar (we-tp->poly Nterms Dmax)) (Dbar (we-tp->poly Dterms Dmax)))
      (cons (poly-scale 2 Nbar) (poly-mul Dbar (list 1 0 1))))))

; integrate INT N/D dx via the rational integrator (result is in t = tan(x/2))
(define (we-integrate Nterms Dterms)
  (let ((PQ (we-substitute Nterms Dterms)))
    (integrate-rational (we-strip (car PQ)) (we-strip (cdr PQ)))))
(define (we-strip p) (we-strip-go (reverse p)))                 ; drop trailing (high-degree) zero coeffs
(define (we-strip-go r) (cond ((null? r) (list 0)) ((= (car r) 0) (we-strip-go (cdr r))) (else (reverse r))))

; verify the rational-integrator result against the substituted rational function P/Q
(define (we-verify Nterms Dterms result)
  (let ((PQ (we-substitute Nterms Dterms)))
    (integrate-verify (we-strip (car PQ)) (we-strip (cdr PQ)) result)))

; convenience constructors for the common one-line integrands
(define (we-const a) (list (list a 0 0)))                        ; the trig polynomial "a"
(define (we-a+bcos a b) (list (list a 0 0) (list b 0 1)))        ; a + b cos x
(define (we-a+bsin a b) (list (list a 0 0) (list b 1 0)))        ; a + b sin x
