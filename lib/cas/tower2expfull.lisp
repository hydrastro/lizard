; -*- lisp -*-
; lib/cas/tower2expfull.lisp -- the EXPONENTIAL single-logarithm recognizer that completes the proper-fraction
; branch of the height-two exponential integrator (docs/CAS.md -- promoting the exponential second-monomial row
; from partial to a genuine integrator on the single-logarithm case).
;
; The unified driver tower2risch.lisp integrates the proper part of an exponential height-two integrand A/D
; (theta2 = exp(u), D2 theta2 = u' theta2) by Hermite reduction followed by a power-sum step, but its power-sum
; wrapper only accepts a squarefree remainder whose denominator is a BARE POWER c*theta2^j -- any other squarefree
; denominator (for instance theta2 - e^x) was reported as an obstruction even when the integral is an honest single
; logarithm.  This module supplies the missing recognizer, the exponential mirror of the primitive h2-newlog in
; tower2int.lisp:
;
;   after Hermite leaves a squarefree remainder As/Ds, the integral is a single logarithm c log(Ds) exactly when
;   As = c * D2(Ds) for a CONSTANT c of the tower; we divide As by D2(Ds) = t2e-deriv(Ds) over K1[theta2] and, if
;   the division is exact with a quotient that is a tower constant (its two-level derivative vanishes, so it lies
;   in Q), that quotient is the residue c.
;
; This is CHEAP -- one polynomial division over K1[theta2], no Sylvester resultant -- so it closes the common
; single-residue case the driver previously rejected, without the memory cost of the general fraction-free
; RootSum (which remains available, computed once, in tower2expff.lisp for the multi-residue x-dependent case).
; The recovered c log(Ds) is CERTIFIED by differentiating with the exponential derivation: D2(c log Ds) =
; c D2(Ds)/Ds must equal As/Ds exactly in K1(theta2).  Sound both ways -- a recognized logarithm is verified, and
; a remainder of any other shape is left for the resultant path rather than misreported.
;
; Public (As, Ds height-two polynomials in theta2 over K1; Ds squarefree in theta2; uprime = u', mono1 = theta1):
;   t2ef-newlog As Ds uprime mono1   -> (list 'log c Ds) when As/Ds = c D2(Ds)/Ds for constant c, else 'none
;   t2ef-newlog-verify As Ds uprime mono1 -> #t iff the recognized c log(Ds) differentiates back to As/Ds
;   t2ef-proper As Ds uprime mono1   -> the single-log integral record, or 'notrecognized (defer to resultant)
;
; Verified: As/Ds = D2(theta2 - e^x)/(theta2 - e^x) is recognized as log(theta2 - e^x) with residue 1 (a
; squarefree denominator that is NOT a bare power, which the driver's power-sum wrapper rejects); 2 D2(Ds)/Ds is
; recognized with residue 2; a remainder that is not a constant multiple of D2(Ds)/Ds returns 'none.
;
; Builds on tower2exphermite.lisp (the exponential derivation t2e-deriv and the h2/k1 layer) and tower2int.lisp
; (the tower-constant test k1-constant?).

(import "cas/tower2exphermite.lisp")
(import "cas/tower2int.lisp")

; ----- the exponential single-logarithm recognizer -----
; As/Ds = c D2(Ds)/Ds  <=>  As = c D2(Ds), c a constant of the tower.
(define (t2ef-newlog As Ds uprime mono1) (t2ef-nl As Ds (t2e-deriv Ds uprime mono1) mono1))
(define (t2ef-nl As Ds dDs mono1)
  (if (h2-zero? dDs) (quote none)
      (t2ef-check As Ds (h2-divmod As dDs) mono1)))
(define (t2ef-check As Ds dm mono1)
  (if (h2-zero? (car (cdr dm)))                         ; division exact?
      (t2ef-quot Ds (car dm) mono1)
      (quote none)))
(define (t2ef-quot Ds q mono1)
  (if (<= (h2-deg q) 0)                                 ; quotient is a scalar in K1?
      (t2ef-const Ds (t2ef-lead q) mono1)
      (quote none)))
(define (t2ef-lead q) (if (null? (h2-norm q)) (k1-zero) (car (h2-norm q))))
(define (t2ef-const Ds c mono1)
  (if (k1-constant? c mono1) (list (quote log) c Ds) (quote none)))   ; constant residue => genuine log

; ----- certificate: D2(c log Ds) = c D2(Ds)/Ds equals As/Ds -----
(define (t2ef-newlog-verify As Ds uprime mono1) (t2ef-vcheck As Ds (t2ef-newlog As Ds uprime mono1) uprime mono1))
(define (t2ef-vcheck As Ds res uprime mono1)
  (if (equal? res (quote none)) #f
      (h2tr-equal? (list (h2-cscale (car (cdr res)) (t2e-deriv Ds uprime mono1)) Ds) (list As Ds))))

; ----- the proper-part driver: single logarithm, else defer -----
(define (t2ef-proper As Ds uprime mono1) (t2ef-wrap (t2ef-newlog As Ds uprime mono1)))
(define (t2ef-wrap res) (if (equal? res (quote none)) (quote notrecognized) (list (quote ok-log) res)))
