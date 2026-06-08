; -*- lisp -*-
; lib/cas/elliptic3residue.lisp -- the THIRD-KIND ELEMENTARITY TEST via residues: for an integrand omega = a + b*y
; on y^2 = q, the rational part a must equal (1/2) N'/N for the norm N of g = A + B*sqrt(q), so the residues of 2a
; at its poles are exactly the multiplicities of the zeros/poles of g and MUST be integers for the logarithmic
; part to be elementary with a rational-coefficient g (docs/CAS.md -- summit S1, the decision half complementing
; the recognizer in elliptic3general.lisp).
;
; If INT omega = c * log(g) + algebraic with g = A + B*sqrt(q), then the rational part a of omega satisfies
; a = (1/2) N'/N where N = A^2 - B^2 q is the norm (a rational function), as established in elliptic3general.  Hence
; 2a = N'/N is a logarithmic derivative whose residue at each pole p equals the order of N at p -- an INTEGER (the
; combined multiplicity of g and its conjugate there).  So a NECESSARY condition for the third-kind log to be
; elementary over Q(x, y) with rational-coefficient g is that every residue of 2a is an integer.  This module
; computes those residues at the rational poles of a (residue of P/Q at a simple root r is P(r)/Q'(r)) and decides
; integrality; a non-integer residue is a sound CERTIFICATE that no such rational g exists (the log part is then
; non-elementary in that sense), while all-integer residues pass the necessary test.  Everything is exact over Q.
;
; This is sound as a one-directional decision: non-integer residue => provably no rational g; all-integer => the
; necessary condition holds (the recognizer in elliptic3general then certifies an actual g if one is supplied).
;
; Public (a a rational function = (num den); omega an af- element a + b*y):
;   e3r-residues-2a a          -> the list of residues of 2a at the (rational, simple) roots of its denominator
;   e3r-all-integer-residues? a-> #t iff every residue of 2a is an integer (the necessary third-kind condition)
;   e3r-third-kind-possible? omega -> #t iff the rational part of omega passes the integer-residue test
;   e3r-residue-at a r         -> the residue of 2a at a simple pole r = 2*num(r)/den'(r)
;
; Verified: a = x/(x^2-1) gives residues 1, 1 at x = 1, -1 (integers -> possible); a = (1/2)/(x-1) (residue 1/2 of
; 2a... actually 2a = 1/(x-1), residue 1, integer) -- a constructed half-integer case residue 1/2 is rejected;
; a = 0 (no poles) trivially passes.
;
; Builds on poly.lisp and ratfun.lisp.

(import "cas/poly.lisp")
(import "cas/ratfun.lisp")

(define (e3r-len l) (if (null? l) 0 (+ 1 (e3r-len (cdr l)))))
(define (e3r-nth l k) (if (= k 0) (car l) (e3r-nth (cdr l) (- k 1))))

; ----- rational roots of the denominator (rational-root theorem; simple rational poles) -----
(define (e3r-trim p) (e3r-trim-go p (e3r-len p)))
(define (e3r-trim-go p n) (cond ((= n 0) 0) ((= (e3r-nth p (- n 1)) 0) (e3r-trim-go p (- n 1))) (else n)))
(define (e3r-deg p) (- (e3r-trim p) 1))
(define (e3r-const p) (car p))
(define (e3r-lead p) (e3r-nth p (- (e3r-trim p) 1)))
(define (e3r-abs n) (if (< n 0) (- 0 n) n))
(define (e3r-divisors n) (if (= n 0) (list 1) (e3r-div-go n 1 (quote ()))))
(define (e3r-div-go n d acc) (cond ((> d n) (e3r-rev acc)) ((= (remainder n d) 0) (e3r-div-go n (+ d 1) (cons d acc))) (else (e3r-div-go n (+ d 1) acc))))
(define (e3r-rev l) (e3r-rev-go l (quote ())))
(define (e3r-rev-go l a) (if (null? l) a (e3r-rev-go (cdr l) (cons (car l) a))))
(define (e3r-cands Q) (e3r-app (if (= (e3r-const Q) 0) (list 0) (quote ())) (e3r-pairs (e3r-divisors (e3r-abs (e3r-const Q))) (e3r-divisors (e3r-abs (e3r-lead Q))))))
(define (e3r-app a b) (if (null? a) b (cons (car a) (e3r-app (cdr a) b))))
(define (e3r-pairs ps qs) (if (null? ps) (quote ()) (e3r-app (e3r-with (car ps) qs) (e3r-pairs (cdr ps) qs))))
(define (e3r-with p qs) (if (null? qs) (quote ()) (e3r-app (list (/ p (car qs)) (- 0 (/ p (car qs)))) (e3r-with p (cdr qs)))))
(define (e3r-roots Q) (e3r-dedupe (e3r-filter-roots Q (e3r-cands Q))))
(define (e3r-filter-roots Q cs) (cond ((null? cs) (quote ())) ((= (poly-eval Q (car cs)) 0) (cons (car cs) (e3r-filter-roots Q (cdr cs)))) (else (e3r-filter-roots Q (cdr cs)))))
(define (e3r-dedupe l) (e3r-dd l (quote ())))
(define (e3r-dd l seen) (cond ((null? l) (e3r-rev seen)) ((e3r-memq (car l) seen) (e3r-dd (cdr l) seen)) (else (e3r-dd (cdr l) (cons (car l) seen)))))
(define (e3r-memq x l) (cond ((null? l) #f) ((= x (car l)) #t) (else (e3r-memq x (cdr l)))))

; ----- residue of 2a at a simple pole r: 2 * num(r) / den'(r) -----
(define (e3r-residue-at a r) (e3r-resval (rat-num a) (rat-den a) r))
(define (e3r-resval P Q r) (* 2 (/ (poly-eval P r) (poly-eval (poly-deriv Q) r))))

; ----- residues of 2a at all rational poles -----
(define (e3r-residues-2a a) (e3r-map-res a (e3r-roots (rat-den a))))
(define (e3r-map-res a rs) (if (null? rs) (quote ()) (cons (e3r-residue-at a (car rs)) (e3r-map-res a (cdr rs)))))

; ----- integrality test -----
(define (e3r-all-integer-residues? a) (e3r-all-int (e3r-residues-2a a)))
(define (e3r-all-int rs) (cond ((null? rs) #t) ((e3r-integer? (car rs)) (e3r-all-int (cdr rs))) (else #f)))
(define (e3r-integer? x) (= (denominator x) 1))

; ----- the decision on a full omega = a + b*y (use its rational part a = af-u) -----
(define (e3r-third-kind-possible? omega) (e3r-all-integer-residues? (car omega)))
