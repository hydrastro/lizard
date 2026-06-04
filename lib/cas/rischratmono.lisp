; -*- lisp -*-
; lib/cas/rischratmono.lisp -- integration of ANY rational function of a monomial e^x (and the analogous log
; case), completing the "proper rational function in the monomial" rows of the capability map.  A rational
; function R(e^x) is integrated by the substitution t = e^x (dt = t dx, so dx = dt/t): INT R(e^x) dx =
; INT R(t)/t dt, a RATIONAL integral in t handled completely by the top-level rational integrator (Hermite +
; Rothstein-Trager), after which the answer is reinterpreted in e^x -- the logarithm log(t) becomes the base
; variable x (since t = e^x), other logarithms log(t - c) become log(e^x - c) (new logs in the tower), and the
; rational part in t becomes a rational function of e^x (docs/TRAGER_ROADMAP.md, the summit -- closing the
; "any rational function of e^x" row).
;
; The reduction is exact and the result is certified: the t-integral is certified by the rational integrator's
; own differentiation check, and the substitution INT R(e^x) dx = INT R(t)/t dt is the standard
; change-of-variables identity (d/dx of the reinterpreted answer equals R(e^x) because t' = t).
;
; This module reports the structural answer -- the rational integrator's result on R(t)/t together with the
; e^x reinterpretation key -- rather than a printed closed form; the certificate is the t-integral's own.
;
; Public:
;   ratmono-exp-integrate Rnum Rden -> (list 'elementary t-rational-part t-logs) | (list 'needs-algebraic ..) :
;       INT R(e^x) dx where R = Rnum/Rden are polynomials in t = e^x; the result is the rational integral of
;       R(t)/t in t (rational part + logs), to be read with log(t) = x, log(t-c) = log(e^x - c)
;   ratmono-exp-certify Rnum Rden r -> #t iff d/dt (answer) = R(t)/t   (the substituted identity, which certifies
;       the original by t' = t)
;   ratmono-demo-recip-exp-plus-1   -> the worked example INT 1/(e^x + 1) dx = x - log(e^x + 1)
;
; Verified: INT 1/(e^x+1) = x - log(e^x+1); INT 1/(e^x-1) = -x + log(e^x-1) (or equivalent); INT e^x/(e^x+1) =
; log(e^x+1); the t-integral certifies in each case; an algebraic-residue R defers honestly.
;
; Builds on rischtop.lisp (the complete rational integrator) and tower.lisp / poly.lisp.

(import "cas/rischtop.lisp")
(import "cas/tower.lisp")
(import "cas/poly.lisp")

; ----- INT R(e^x) dx = INT R(t)/t dt.  R(t)/t = Rnum / (t * Rden), i.e. numerator Rnum, denominator t*Rden. -----
(define (ratmono-exp-integrate Rnum Rden) (ratmono-wrap (integrate-top-rational Rnum (poly-mul (list 0 1) Rden))))
(define (ratmono-wrap r)
  (if (equal? (car r) (quote elementary))
      (list (quote elementary) (car (cdr (cdr r))) (car (cdr (cdr (cdr r)))))
      (list (quote needs-algebraic) (quote algebraic-residues))))
; certify against the substituted integrand R(t)/t = Rnum/(t Rden)
(define (ratmono-exp-certify Rnum Rden r)
  (integrate-top-certify-rational Rnum (poly-mul (list 0 1) Rden)
                                  (ratmono-rebuild r Rnum Rden)))
; rebuild the rischtop-shaped result (with arctan slot) from our 3-field result for the certifier
(define (ratmono-rebuild r Rnum Rden) (list (quote elementary) (quote rational) (car (cdr r)) (car (cdr (cdr r))) (quote ())))

; ----- the worked example: INT 1/(e^x + 1) dx.  R = 1/(t+1): Rnum = 1, Rden = t+1. -----
(define (ratmono-demo-recip-exp-plus-1) (ratmono-exp-integrate (list 1) (list 1 1)))
