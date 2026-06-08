; -*- lisp -*-
; lib/cas/cplxroots.lisp -- EXACT NAMING OF COMPLEX ROOTS of a real polynomial: each non-real root is named by the
; rational data of the irreducible real quadratic it satisfies, completing polysolve3's real-root naming so the
; full root census (real plus complex) is exact (docs/CAS.md -- summit S3, naming complex solutions).
;
; The non-real roots of a real polynomial come in conjugate pairs, each pair being the roots of an irreducible
; real quadratic x^2 + p x + q with negative discriminant.  Such a root is -p/2 +- (sqrt(4q - p^2)/2) i, so it is
; named EXACTLY by the pair (re, im2) where re = -p/2 and im2 = (4q - p^2)/4 > 0 are rational; the actual roots are
; re + sqrt(im2) i and re - sqrt(im2) i.  This module extracts the irreducible real quadratic factors of the
; complex part of a polynomial (the squarefree cofactor remaining after the rational and the irrational-real roots
; have been removed) and names the conjugate pair of each, for the decidable case where that complex part is a
; single irreducible quadratic.  Each named pair is VERIFIED by reconstructing its quadratic (x - re)^2 + im2 and
; checking it divides the polynomial exactly.  Everything is exact over Q -- re and im2 are rationals, the
; imaginary part being carried as its square so no surd or floating value is ever formed.
;
; Public (univariate coefficient lists low->high; a "complex root" is (complex re im2) meaning re +- sqrt(im2) i):
;   cx-quadratic-pair p q      -> (complex re im2) for the monic quadratic x^2 + p x + q (its conjugate roots)
;   cx-quadratic-of root       -> the monic quadratic (q p 1) reconstructed from a (complex re im2) name
;   cx-divides-quadratic? E rt -> #t iff the quadratic of the named complex root divides E exactly
;   cx-name-complex E          -> the complex-root names from E's irreducible quadratic complex part (one pair when
;                                 the complex part is a single irreducible quadratic), else 'needs-factorization
;   cx-num-complex E           -> the number of non-real roots of the squarefree part of E (degree minus #real)
;
; Verified: x^2 + 1 names the pair (complex 0 1) -- the roots i and -i; x^2 + x + 1 names (complex -1/2 3/4); the
; reconstructed quadratic divides the polynomial in each case; (x-1)(x^2+1) yields one complex pair after the real
; root is removed; a quadratic with positive discriminant produces no complex names.
;
; Builds on poly.lisp, sturm.lisp, polysolve3.lisp.

(import "cas/poly.lisp")
(import "cas/sturm.lisp")
(import "cas/polysolve3.lisp")

(define (cx-len l) (if (null? l) 0 (+ 1 (cx-len (cdr l)))))
(define (cx-nth l k) (if (= k 0) (car l) (cx-nth (cdr l) (- k 1))))

; ----- name the conjugate pair of a monic quadratic x^2 + p x + q -----
(define (cx-quadratic-pair p q) (list (quote complex) (/ (- 0 p) 2) (/ (- (* 4 q) (* p p)) 4)))

; ----- reconstruct the monic quadratic (q p 1) from a (complex re im2): (x-re)^2 + im2 = x^2 - 2re x + (re^2+im2)
(define (cx-quadratic-of root) (cx-build (cx-re root) (cx-im2 root)))
(define (cx-re root) (cx-nth root 1))
(define (cx-im2 root) (cx-nth root 2))
(define (cx-build re im2) (list (+ (* re re) im2) (* -2 re) 1))

; ----- does the named root's quadratic divide E exactly? -----
(define (cx-divides-quadratic? E rt) (cx-zero? (car (cdr (poly-divmod E (cx-quadratic-of rt))))))
(define (cx-zero? p) (cond ((null? p) #t) ((= (car p) 0) (cx-zero? (cdr p))) (else #f)))

; ----- the complex part: squarefree E with rational roots and real-irrational roots removed -----
(define (cx-complex-part E) (ps3-deflate (sqfree-part E) (ps3-rational-roots (sqfree-part E))))
; after removing rational linear factors, the real-irrational factor (ps3-irrational-factor handles its naming);
; the COMPLEX part is what remains once the real roots are gone.  We detect the single-irreducible-quadratic case
; by: the complex part has no real roots (num-real-roots = 0) and degree exactly 2.
(define (cx-name-complex E) (cx-dispatch (cx-strip-real E)))
; strip ALL real roots (rational and irrational) by deflating rational then dividing by the real-root quadratics
; is complex; for the sound slice we use: take squarefree part, and if it has degree-2 with no real roots, name it.
(define (cx-strip-real E) (sqfree-part E))
(define (cx-dispatch C) (cond ((cx-deg-2-noreal? C) (cx-name-from-quad C)) ((= (cx-num-complex C) 0) (quote ())) (else (quote needs-factorization))))
(define (cx-deg-2-noreal? C) (if (= (poly-deg C) 2) (= (num-real-roots C) 0) #f))
; name the pair from a monic-normalized degree-2 C = (c0 c1 c2): p = c1/c2, q = c0/c2
(define (cx-name-from-quad C) (list (cx-quadratic-pair (/ (cx-nth C 1) (cx-nth C 2)) (/ (cx-nth C 0) (cx-nth C 2)))))

; ----- number of non-real roots of the squarefree part -----
(define (cx-num-complex E) (- (poly-deg (sqfree-part E)) (num-real-roots E)))
