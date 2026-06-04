; -*- lisp -*-
; lib/cas/radideal.lisp -- RADICAL MEMBERSHIP f in sqrt(I): does f vanish on the variety V(I)?  Decided by the
; Rabinowitsch trick and Hilbert's Nullstellensatz, atop the Groebner machinery (docs/CAS.md -- summit S4, a
; strictly stronger test than ordinary ideal membership).
;
; By the Nullstellensatz, f vanishes on V(I) exactly when f lies in the radical sqrt(I), and that holds iff the
; ideal <I, 1 - t*f> in one extra variable t is the whole ring -- equivalently iff 1 reduces to 0 modulo a
; Groebner basis of {generators of I} union {1 - t*f}.  This module lifts I's generators into one more variable,
; adjoins 1 - t*f, computes a Groebner basis, and tests whether the constant 1 reduces to zero there.  It is a
; sound DECISION: the answer is exact and two-sided (1 -> 0 means f is in the radical; otherwise it is not), and it
; is strictly stronger than ordinary membership -- f can vanish on V(I) (in the radical) without lying in I itself
; (the example x in sqrt(<x^2>) but x not in <x^2>).
;
; Everything reuses groebner.lisp's Buchberger engine and normal form nf; the only new ingredient is the variable
; lift and the 1 - t*f construction.  The fresh variable t is added as a NEW LAST coordinate, so I's generators
; (in variables x_0 .. x_{n-1}) are extended with a zero exponent for t and f likewise.
;
; Public (mpolys in groebner.lisp's representation; nv = number of original variables; t becomes index nv):
;   ri-lift p nv               -> p with a zero t-exponent appended (embed into nv+1 variables)
;   ri-rabinowitsch F f nv     -> the generating set {lift(g) : g in F} union {1 - t*f} in nv+1 variables
;   ri-in-radical? F f nv      -> #t iff f is in sqrt(<F>), i.e. f vanishes on V(F) (the Nullstellensatz test)
;   ri-in-ideal? F f nv        -> #t iff f is in <F> directly (normal form zero) -- for contrast with the radical
;
; Verified: x is in sqrt(<x^2>) though not in <x^2>; on <x^2 + y^2 - 1, x - y> the generator x - y is in the
; radical while x + y is not; 1 is trivially in every radical; a polynomial nonzero on the variety is rejected.
;
; Builds on groebner.lisp.

(import "cas/groebner.lisp")

(define (ri-app a b) (if (null? a) b (cons (car a) (ri-app (cdr a) b))))
(define (ri-map f l) (if (null? l) (quote ()) (cons (f (car l)) (ri-map f (cdr l)))))

; ----- lift a polynomial into one extra (last) variable by appending a zero exponent to each monomial -----
(define (ri-lift p nv) (ri-map (lambda (term) (cons (car term) (ri-app (cdr term) (list 0)))) p))

; ----- the monomial t (index nv) and the polynomial t*f, then 1 - t*f -----
(define (ri-t-times-f f nv) (ri-map (lambda (term) (cons (car term) (ri-bump (cdr term)))) (ri-lift f nv)))
; bump the LAST exponent (t's slot) by 1
(define (ri-bump m) (ri-set-last m (+ (ri-last m) 1)))
(define (ri-last m) (if (null? (cdr m)) (car m) (ri-last (cdr m))))
(define (ri-set-last m v) (if (null? (cdr m)) (list v) (cons (car m) (ri-set-last (cdr m) v))))
(define (ri-one nv) (list (cons 1 (ri-zeros (+ nv 1)))))
(define (ri-zeros k) (if (<= k 0) (quote ()) (cons 0 (ri-zeros (- k 1)))))
(define (ri-one-minus-tf f nv) (mpoly-sub (ri-one nv) (ri-t-times-f f nv)))

; ----- the Rabinowitsch generating set -----
(define (ri-rabinowitsch F f nv) (ri-app (ri-map (lambda (g) (ri-lift g nv)) F) (list (ri-one-minus-tf f nv))))

; ----- radical membership: 1 reduces to 0 modulo GB of the Rabinowitsch set -----
(define (ri-in-radical? F f nv) (ri-reduces-to-zero (ri-one nv) (groebner (ri-rabinowitsch F f nv))))
(define (ri-reduces-to-zero one G) (null? (nf one G)))

; ----- ordinary ideal membership for contrast -----
(define (ri-in-ideal? F f nv) (null? (nf f (groebner F))))
