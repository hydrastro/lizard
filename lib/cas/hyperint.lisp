; -*- lisp -*-
; lib/cas/hyperint.lisp -- the UNIFIED HYPERELLIPTIC INTEGRATION DRIVER: one entry point that integrates over the
; genus-g hyperelliptic field y^2 = f, dispatching the second-kind (polynomial-over-radical) and third-kind
; (logarithmic) cases to the certified machinery and returning a single verdict (docs/TRAGER_ROADMAP.md -- the
; general algebraic Risch for the hyperelliptic family, one decision procedure across genera).
;
; The hyperelliptic algebraic-integration pieces were a toolbox dispatched by hand:
;   - hyperell (he-integrate): INT P(x)/sqrt(f) for a polynomial numerator -- the SECOND-kind part by Hermite
;     reduction, returning the elementary Q sqrt(f) when the first-kind remainder vanishes, else a PROOF of
;     non-elementarity (the first-kind obstruction);
;   - hyperthird (ht-recognize / ht-cert): the THIRD-kind logarithm INT (d log(a+y)) = log(a+y) over the field,
;     recovering a from the differential's denominator and certifying by differentiation.
; This module is the single entry point the algebraic Risch decision is meant to be for the hyperelliptic family:
; given an integrand classified by kind, it routes to the right certified routine and returns EITHER an elementary
; antiderivative (with its certificate) OR an honest verdict (non-elementary first-kind, or a bounded third-kind
; "not of the recognized shape").  Each positive answer carries the same differentiation certificate the underlying
; module supplies, so soundness is inherited; the driver only classifies and dispatches, it never guesses.
;
; Kinds:
;   'second  : INT P(x)/sqrt(f) dx, P a polynomial -> he-integrate (elementary Q sqrt f | non-elementary first-kind)
;   'third   : a third-kind differential over a polynomial denominator D -> ht-recognize (log(a+y) | not-recognized)
;
; Public (f the squarefree curve polynomial; P a polynomial numerator; D a third-kind denominator):
;   hi-genus f                 -> the genus floor((deg f - 1)/2) of y^2 = f
;   hi-second f P              -> (list 'elementary 'algebraic Q) [INT = Q sqrt f] | (list 'non-elementary 'first-kind g S)
;   hi-second-verify f P       -> #t iff the second-kind elementary answer differentiates back (he-integrate-certify)
;   hi-third f D               -> (list 'elementary 'logarithm (log (a+y))) | (list 'non-elementary 'not-recognized)
;   hi-integrate f kind arg    -> the unified dispatch: kind 'second with arg = P, or 'third with arg = D
;   hi-decides? f kind arg     -> #t iff the driver returns a definite, certified verdict
;
; Verified: on y^2 = x^5 + 1 (genus 2), INT (5/2) x^4 / sqrt(f) = sqrt(f) is second-kind elementary and certified;
; INT 1/sqrt(f) is non-elementary first-kind (genus 2, holomorphic); the third-kind differential over N(x+y)
; integrates to log(x + y); a non-recognized third-kind denominator is reported; genus is reported correctly.
;
; Builds on hyperell.lisp (second kind) and hyperthird.lisp (third kind).

(import "cas/hyperell.lisp")
(import "cas/hyperthird.lisp")

; ----- genus -----
(define (hi-genus f) (he-genus (poly-deg f)))

; ----- second kind: INT P(x)/sqrt(f) -----
(define (hi-second f P) (hi-second-wrap (he-integrate P f)))
(define (hi-second-wrap r) (if (equal? (car r) (quote elementary))
                               (list (quote elementary) (quote algebraic) (car (cdr r)))
                               (list (quote non-elementary) (quote first-kind) (car (cdr r)) (car (cdr (cdr r))))))
(define (hi-second-verify f P) (he-integrate-certify P f))

; ----- third kind: a differential over denominator D -----
(define (hi-third f D) (hi-third-wrap (ht-recognize f D)))
(define (hi-third-wrap r) (if (equal? r (quote not-third-kind-a+y))
                              (list (quote non-elementary) (quote not-recognized))
                              (list (quote elementary) (quote logarithm) r)))

; ----- unified dispatch -----
(define (hi-integrate f kind arg) (if (equal? kind (quote second)) (hi-second f arg) (hi-third f arg)))
(define (hi-decides? f kind arg) (hi-dec (hi-integrate f kind arg) f kind arg))
(define (hi-dec r f kind arg)
  (cond ((equal? (car r) (quote non-elementary)) #t)
        ((equal? kind (quote second)) (hi-second-verify f arg))
        (else (ht-cert f (ht-recover-a f arg)))))   ; third-kind elementary: certify the recovered log
