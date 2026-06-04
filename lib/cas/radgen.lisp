; -*- lisp -*-
; lib/cas/radgen.lisp -- the explicit RADICAL GENERATOR of a univariate principal ideal, with the squarefree
; (primary-like) decomposition and a constructive radical-membership test: the CONSTRUCTIVE complement to
; radideal, which decided f in sqrt(I) but produced no generator (docs/CAS.md -- summit S4, radical generators).
;
; For a univariate principal ideal <f> over Q[x], the radical is again principal:
;     sqrt(<f>) = <rad(f)>,   rad(f) = f / gcd(f, f'),
; the squarefree part of f -- the product of the distinct irreducible factors, each to the first power.  Yun's
; squarefree decomposition f = prod_k g_k^k exposes the primary-like structure (the factor g_k collects the roots
; of multiplicity exactly k), and the radical is prod_k g_k.  Radical membership for a principal ideal is then a
; DIVISIBILITY test, exact and constructive: h in sqrt(<f>) iff rad(f) divides h.  Everything reuses Yun's
; decomposition and polynomial division from poly.lisp; the radical generator is returned explicitly (monic), the
; decomposition is returned as (multiplicity . factor) pairs, and the reconstruction prod g_k^k = f is the
; certificate that the decomposition is correct.
;
; Public (univariate coefficient lists low->high, f monic non-constant):
;   rg-radical f               -> the radical generator rad(f) = product of the distinct factors (monic)
;   rg-decomposition f         -> Yun's squarefree decomposition as a list of (multiplicity . factor)
;   rg-reconstruct decomp      -> prod factor^multiplicity from a decomposition (for certification)
;   rg-decomposition-ok? f     -> #t iff reconstructing the decomposition returns f (up to monic scaling)
;   rg-in-radical? f h         -> #t iff h is in sqrt(<f>), i.e. rad(f) divides h (constructive membership)
;   rg-is-radical-ideal? f     -> #t iff <f> is already a radical ideal (f equals its own radical, i.e. squarefree)
;
; Verified: rad((x-1)^2 (x-2)) = (x-1)(x-2); the decomposition is ((1,(x-2)),(2,(x-1))) and reconstructs the input;
; (x-1)(x-2) is squarefree so equals its own radical; (x-1) is in sqrt(<(x-1)^2(x-2)>) while (x-3) is not.
;
; Builds on poly.lisp.

(import "cas/poly.lisp")

(define (rg-len l) (if (null? l) 0 (+ 1 (rg-len (cdr l)))))

; ----- Yun's squarefree decomposition as (multiplicity . factor) pairs -----
(define (rg-decomposition f) (rg-to-pairs (yun-square-free (rg-monic f))))
(define (rg-to-pairs ys) (if (null? ys) (quote ()) (cons (cons (car (car ys)) (car (cdr (car ys)))) (rg-to-pairs (cdr ys)))))
; make monic (Yun expects monic): divide by leading coefficient
(define (rg-monic f) (poly-scale (/ 1 (rg-lead f)) f))
(define (rg-lead f) (rg-nth f (- (rg-trim f) 1)))
(define (rg-nth l k) (if (= k 0) (car l) (rg-nth (cdr l) (- k 1))))
(define (rg-trim f) (rg-trim-n f (rg-len f)))
(define (rg-trim-n f n) (cond ((= n 0) 0) ((= (rg-nth f (- n 1)) 0) (rg-trim-n f (- n 1))) (else n)))

; ----- the radical generator: product of the distinct factors (each multiplicity once) -----
(define (rg-radical f) (rg-prod-factors (rg-decomposition f)))
(define (rg-prod-factors decomp) (if (null? decomp) (list 1) (poly-mul (cdr (car decomp)) (rg-prod-factors (cdr decomp)))))

; ----- reconstruct prod factor^multiplicity (certification) -----
(define (rg-reconstruct decomp) (if (null? decomp) (list 1) (poly-mul (rg-pow (cdr (car decomp)) (car (car decomp))) (rg-reconstruct (cdr decomp)))))
(define (rg-pow p m) (if (<= m 0) (list 1) (poly-mul p (rg-pow p (- m 1)))))
(define (rg-decomposition-ok? f) (rg-poly-eq? (rg-reconstruct (rg-decomposition f)) (rg-monic f)))
(define (rg-poly-eq? a b) (equal? (rg-trimlist a) (rg-trimlist b)))
(define (rg-trimlist p) (rg-take p (rg-trim p)))
(define (rg-take p n) (if (= n 0) (quote ()) (rg-app (rg-take p (- n 1)) (list (rg-nth p (- n 1))))))
(define (rg-app a b) (if (null? a) b (cons (car a) (rg-app (cdr a) b))))

; ----- constructive radical membership: rad(f) | h -----
(define (rg-in-radical? f h) (rg-zero? (car (cdr (poly-divmod h (rg-radical f))))))
(define (rg-zero? p) (cond ((null? p) #t) ((= (car p) 0) (rg-zero? (cdr p))) (else #f)))

; ----- is <f> already radical (f squarefree, i.e. equals its radical up to scaling)? -----
(define (rg-is-radical-ideal? f) (rg-poly-eq? (rg-radical f) (rg-monic f)))
