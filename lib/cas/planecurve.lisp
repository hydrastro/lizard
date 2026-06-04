; -*- lisp -*-
; lib/cas/planecurve.lisp -- the GENUS of a plane algebraic curve by the genus-degree (Plucker) formula, with
; corrections for ordinary singular points: a complementary view of genus to the superelliptic cyclic-cover
; formula, agreeing with it where both apply (docs/CAS.md -- summit S2, genus of general curves).
;
; A SMOOTH projective plane curve of degree d has genus
;     g = (d - 1)(d - 2) / 2.
; Each ordinary singular point of multiplicity m contributes a delta-invariant m(m-1)/2 that lowers the geometric
; genus, so for a curve with ordinary singularities of multiplicities m_1, ..., m_s,
;     g = (d - 1)(d - 2)/2 - sum_i m_i (m_i - 1)/2
; (a node is m = 2 with delta 1; an ordinary cusp/triple point m = 3 with delta 3).  This is exact integer
; arithmetic on the degree and the singularity multiplicities.  For the smooth case it gives the classical values
; (conic 0, cubic 1, quartic 3, quintic 6, sextic 10), and where a curve is BOTH a smooth plane curve and a
; superelliptic curve -- the smooth plane cubic and y^2 = cubic, both genus 1 -- the two independent genus
; computations agree, which is the cross-check.
;
; Public:
;   pc-smooth-genus d              -> (d-1)(d-2)/2, the genus of a smooth plane curve of degree d
;   pc-delta m                     -> the delta-invariant m(m-1)/2 of an ordinary m-fold point
;   pc-genus d mults               -> the geometric genus after subtracting the delta-invariants in the list mults
;   pc-is-rational? d mults        -> #t iff the corrected genus is 0 (the curve is rational / unirational)
;   pc-agrees-superelliptic-cubic? -> #t iff the smooth plane cubic genus equals the y^2=cubic superelliptic genus
;
; Verified: smooth conic/cubic/quartic/quintic/sextic genera are 0,1,3,6,10; a nodal cubic (one node) has genus 0;
; a 3-nodal quartic has genus 0; the smooth plane cubic agrees with the superelliptic y^2=cubic genus (both 1).
;
; Builds on poly.lisp and superelliptic.lisp (for the cross-check).

(import "cas/poly.lisp")
(import "cas/superelliptic.lisp")

(define (pc-len l) (if (null? l) 0 (+ 1 (pc-len (cdr l)))))

; ----- the smooth genus-degree formula -----
(define (pc-smooth-genus d) (quotient (* (- d 1) (- d 2)) 2))

; ----- delta-invariant of an ordinary m-fold point -----
(define (pc-delta m) (quotient (* m (- m 1)) 2))

; ----- corrected geometric genus -----
(define (pc-genus d mults) (- (pc-smooth-genus d) (pc-sum-delta mults)))
(define (pc-sum-delta mults) (if (null? mults) 0 (+ (pc-delta (car mults)) (pc-sum-delta (cdr mults)))))

; ----- rationality -----
(define (pc-is-rational? d mults) (= (pc-genus d mults) 0))

; ----- cross-check: the smooth plane cubic (d=3) vs the superelliptic y^2 = cubic -----
(define (pc-agrees-superelliptic-cubic?) (= (pc-smooth-genus 3) (sup-genus 2 (list 1 0 0 1))))
