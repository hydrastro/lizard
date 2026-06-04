; -*- lisp -*-
; lib/cas/radmember.lisp -- RADICAL ideal membership (the Nullstellensatz / variety-consequence test) by the
; Rabinowitsch trick, on top of the Groebner solver (docs/CAS.md -- frontier 4, multivariate: deciding whether a
; polynomial vanishes on the whole VARIETY of a system, strictly stronger than ideal membership).
;
; Ideal membership (in-ideal?) decides whether p is a polynomial combination of the generators; radical
; membership decides the geometrically meaningful question, whether p VANISHES AT EVERY COMMON ZERO of the
; system -- i.e. p in sqrt(I) <=> V(I) subset V(p).  By Hilbert's Nullstellensatz (over an algebraically closed
; field) these differ: x is in sqrt(<x^2>) because x = 0 wherever x^2 = 0, yet x is not in <x^2> as an ideal.
;
; The Rabinowitsch reduction makes radical membership decidable with the machinery already in hand:
;     p in sqrt(I)   <=>   1 in < I , 1 - t*p >   in Q[x_1,...,x_v, t]   (t a fresh variable),
; because adjoining 1 - t p forces p != 0 on any would-be common zero, so the augmented system is INCONSISTENT
; exactly when p cannot avoid vanishing on V(I).  We lift every generator (and p) into the (v+1)-variable ring by
; appending a zero t-exponent, form 1 - t p, and test consistency of the augmented system via the existing
; Groebner consistency check (psys-consistent?).  The verdict is therefore backed by the same Groebner
; certificate: radical membership holds iff the augmented Groebner basis is {1}.
;
; Public (polynomials in groebner.lisp's representation; v = number of original variables):
;   rad-lift p v               -> p with a zero t-exponent appended to every monomial (into the v+1 ring)
;   rad-rabinowitsch p v       -> the generator 1 - t*p in the v+1 ring (t is variable index v)
;   rad-member? p F v          -> #t iff p is in the radical of <F> (p vanishes on the entire variety of F)
;
; Verified: x in sqrt(<x^2>) is #t while x in <x^2> (ideal) is #f; for the line-circle system <x^2+y^2-1, x-y>
; the polynomial 2y^2 - 1 (which vanishes at both solutions) is in the radical; a polynomial that does not vanish
; on the variety (e.g. x - 5 on that system) is correctly reported not in the radical.
;
; Builds on polysolve.lisp / groebner.lisp.

(import "cas/polysolve.lisp")
(import "cas/groebner.lisp")

(define (rm-app a b) (if (null? a) b (cons (car a) (rm-app (cdr a) b))))

; ----- lift a polynomial into the (v+1)-variable ring by appending a 0 t-exponent to every monomial -----
(define (rad-lift p v) (rm-map-lift p))
(define (rm-map-lift p) (if (null? p) (quote ()) (cons (cons (car (car p)) (rm-app (cdr (car p)) (list 0))) (rm-map-lift (cdr p)))))

; ----- build 1 - t*p in the v+1 ring; t is the new last variable (index v), exponent vector length v+1 -----
; t*p : multiply each term of (lifted) p by the monomial t = (0...0 1).  1 = constant term in the v+1 ring.
(define (rad-rabinowitsch p v) (mpoly-sub (rm-one v) (rm-tmul (rad-lift p v) v)))
(define (rm-one v) (list (cons 1 (rm-zeros (+ v 1)))))
(define (rm-zeros n) (if (= n 0) (quote ()) (cons 0 (rm-zeros (- n 1)))))
; multiply a lifted polynomial by t = the monomial with exponent 1 in slot v, 0 elsewhere
(define (rm-tmul lp v) (if (null? lp) (quote ()) (cons (cons (car (car lp)) (rm-bump (cdr (car lp)) v)) (rm-tmul (cdr lp) v))))
; add 1 to the t-slot (index v) of a monomial of length v+1
(define (rm-bump m v) (rm-bump-go m v 0))
(define (rm-bump-go m v j) (if (null? m) (quote ()) (cons (if (= j v) (+ (car m) 1) (car m)) (rm-bump-go (cdr m) v (+ j 1)))))

; ----- radical membership: augment F (lifted) with 1 - t*p and test INCONSISTENCY (basis = {1}) -----
(define (rad-member? p F v) (if (rad-augmented-consistent? p F v) #f #t))
(define (rad-augmented-consistent? p F v) (psys-consistent? (rm-app (rm-lift-all F v) (list (rad-rabinowitsch p v)))))
(define (rm-lift-all F v) (if (null? F) (quote ()) (cons (rad-lift (car F) v) (rm-lift-all (cdr F) v))))
