; -*- lisp -*-
; lib/cas/intlog.lisp -- the COMPLETE integrator for a rational function of a single logarithm:
; INT A/D dx with A, D polynomials in theta = log x over Q(x).  This is the capstone of the
; logarithmic direction: it integrates an arbitrary such A/D, with rational residues, fully certified.
;
; Polynomial division in theta over Q(x) writes A = Q D + Rem with deg_theta(Rem) < deg_theta(D), so
;   INT A/D = INT Q  +  INT Rem/D.
; The polynomial part INT Q is the primitive-case polynomial integration of primint.lisp (rational
; coefficients, logarithm absorption), and the proper part INT Rem/D is the Hermite reduction plus
; tower Rothstein-Trager of towerrt.lisp.  Correctness needs no cross-module reasoning: the split is
; an exact polynomial identity, and differentiation is linear, so if A = Q D + Rem holds and each of
; INT Q and INT Rem/D is independently certified by its own module, then the derivative of their sum
; is Q + Rem/D = A/D.  The whole integral is elementary exactly when both parts are; a non-elementary
; polynomial part (an unabsorbable logarithm) or proper part (x-dependent or algebraic residues) is
; reported as such.  This integrates, e.g., (log x)^3 plus a two-residue proper part in one call.
; Builds on primint.lisp and towerrt.lisp.

(import "cas/primint.lisp")
(import "cas/towerrt.lisp")

(define (ilog-mono) (list 'log))
(define (ilog-tr->rde r) (rde-rmake (car r) (car (cdr r))))           ; tower-rat (num den) -> rderat (num . den)
(define (ilog-terms Q) (map ilog-tr->rde Q))                          ; rfpoly in theta -> primint coefficient list

; INT A/D, A/D rational in theta = log x  ->  (list 'ok Q Rem D)
;   Q   = polynomial part (rfpoly in theta), integrated by primint.lisp
;   Rem = proper numerator, Rem/D integrated by towerrt.lisp
(define (int-log-rational A D)
  (let ((qr (rfpoly-divmod A D))) (list 'ok (car qr) (car (cdr qr)) D)))

; the split A = Q D + Rem is an exact rfpoly identity
(define (ilog-split-ok? A D Q Rem) (rfpoly-zero? (rfpoly-sub A (rfpoly-add (rfpoly-mul Q D) Rem))))

; both parts elementary?  (poly part via primint, proper part via towerrt)
(define (ilog-poly-ok? Q) (int-prim-poly-elementary? (ilog-terms Q)))
(define (ilog-proper-ok? Rem D) (if (rfpoly-zero? Rem) #t (int-prim-rational-elementary? Rem D (ilog-mono))))
(define (int-log-rational-elementary? A D)
  (let ((r (int-log-rational A D)))
    (if (ilog-poly-ok? (car (cdr r))) (ilog-proper-ok? (car (cdr (cdr r))) (car (cdr (cdr (cdr r))))) #f)))

; certificate: split exact, AND poly part certified, AND proper part certified (=> derivative of the
; combined answer equals A/D, by linearity)
(define (int-log-rational-verify A D)
  (let ((r (int-log-rational A D)))
    (let ((Q (car (cdr r))) (Rem (car (cdr (cdr r)))) (Dn (car (cdr (cdr (cdr r))))))
      (if (ilog-split-ok? A D Q Rem)
          (if (int-prim-poly-verify (ilog-terms Q))
              (if (rfpoly-zero? Rem) #t (int-prim-rational-verify Rem Dn (ilog-mono)))
              #f)
          #f))))
