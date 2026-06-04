; -*- lisp -*-
; lib/cas/cplxtuples.lisp -- COMPLEX SOLUTION TUPLES over the Gaussian rationals Q(i): assemble complete solution
; POINTS of a triangular polynomial system whose coordinates are complex, exactly, when the imaginary parts are
; rational (docs/CAS.md -- summit S3, complex coordinates and full varieties, beyond the real algebraic tuples of
; algtuples and the complex-root NAMING of cplxroots).
;
; cplxroots names a complex root of a real polynomial as (complex re im2): the conjugate pair re +- sqrt(im2) i.
; When im2 is a perfect square its square root is rational, so the root is a GAUSSIAN RATIONAL a + b i with a, b
; in Q -- a genuine element of Q(i) on which arithmetic is exact and finite.  This module carries such numbers as
; pairs (gr a b), implements Q(i) arithmetic (add, multiply, power), turns a perfect-square (complex re im2) into
; its two Gaussian-rational roots, assembles a triangular system m(x_0)=0, x_j = h_j(x_0) into the complete complex
; points (alpha, h_1(alpha), ...) by evaluating each h_j at the Gaussian root, and CERTIFIES a point by evaluating
; every generator to zero over Q(i).  Everything is exact: a + b i with rational a, b, no floating point and no
; surd, for the decidable case where the relevant imaginary parts are rational (im2 a perfect square).  When im2
; is not a perfect square the coordinate lies in Q(i, sqrt(im2)) and is reported 'not-gaussian rather than forced.
;
; Public (a "gaussian" is (gr a b) meaning a + b*i with a,b rational; coordinate lists low->high):
;   gr-make a b / gr-re / gr-im / gr-add / gr-sub / gr-mul / gr-pow / gr-zero? / gr-equal?  -- Q(i) arithmetic
;   cxt-roots-of (complex re im2)  -> the two Gaussian roots ((gr re s) (gr re -s)) when im2=s^2, else 'not-gaussian
;   cxt-eval-poly coeffs g         -> a real-coefficient univariate polynomial evaluated at a Gaussian number g
;   cxt-tuple re-im2 hs            -> the complex point (alpha, h_1(alpha), ...) for the + root, over Q(i)
;   cxt-eval-gen gen tuple         -> a 2-variable generator evaluated at a Gaussian tuple, as a Gaussian number
;   cxt-certify gens tuple         -> #t iff every generator vanishes (is zero in Q(i)) at the Gaussian tuple
;
; Verified: <x^2+1, y-x> gives the Gaussian point (i, i) certified to make both generators vanish in Q(i); the
; conjugate (-i, -i) likewise; <x^2+1, y-x^2> gives (i, -1) since i^2 = -1; a non-perfect-square imaginary part
; (x^2+x+1) is reported 'not-gaussian; a wrong point fails the certificate.
;
; Builds on poly.lisp and cplxroots.lisp.

(import "cas/poly.lisp")
(import "cas/cplxroots.lisp")

; ----- Gaussian rational arithmetic: (gr a b) = a + b i -----
(define (gr-make a b) (list (quote gr) a b))
(define (gr-re g) (car (cdr g)))
(define (gr-im g) (car (cdr (cdr g))))
(define (gr-add x y) (gr-make (+ (gr-re x) (gr-re y)) (+ (gr-im x) (gr-im y))))
(define (gr-sub x y) (gr-make (- (gr-re x) (gr-re y)) (- (gr-im x) (gr-im y))))
(define (gr-mul x y) (gr-make (- (* (gr-re x) (gr-re y)) (* (gr-im x) (gr-im y)))
                              (+ (* (gr-re x) (gr-im y)) (* (gr-im x) (gr-re y)))))
(define (gr-pow g e) (if (<= e 0) (gr-make 1 0) (gr-mul g (gr-pow g (- e 1)))))
(define (gr-zero? g) (if (= (gr-re g) 0) (= (gr-im g) 0) #f))
(define (gr-equal? x y) (if (= (gr-re x) (gr-re y)) (= (gr-im x) (gr-im y)) #f))
(define (gr-from-rational r) (gr-make r 0))

; ----- rational perfect-square root (or #f) -----
(define (cxt-int-sqrt n) (if (< n 0) #f (cxt-isq n 0)))
(define (cxt-isq n k) (cond ((> (* k k) n) #f) ((= (* k k) n) k) (else (cxt-isq n (+ k 1)))))
(define (cxt-rat-sqrt c) (if (< c 0) #f (cxt-rs (cxt-int-sqrt (numerator c)) (cxt-int-sqrt (denominator c)))))
(define (cxt-rs ns ds) (if ns (if ds (/ ns ds) #f) #f))

; ----- the two Gaussian roots of (complex re im2), when im2 is a perfect square -----
(define (cxt-roots-of cx) (cxt-roots-go (car (cdr cx)) (cxt-rat-sqrt (car (cdr (cdr cx))))))
(define (cxt-roots-go re s) (if s (list (gr-make re s) (gr-make re (- 0 s))) (quote not-gaussian)))

; ----- evaluate a real-coefficient univariate polynomial at a Gaussian number (Horner) -----
(define (cxt-eval-poly coeffs g) (cxt-ep coeffs g 0 (gr-make 0 0)))
(define (cxt-ep coeffs g k acc) (if (null? coeffs) acc (cxt-ep (cdr coeffs) g (+ k 1) (gr-add acc (gr-mul (gr-from-rational (car coeffs)) (gr-pow g k))))))

; ----- assemble the complex point (alpha, h_1(alpha), ...) for the + root -----
(define (cxt-tuple cx hs) (cxt-tuple-go (cxt-roots-of cx) hs))
(define (cxt-tuple-go roots hs) (if (equal? roots (quote not-gaussian)) (quote not-gaussian) (cons (car roots) (cxt-map-h (car roots) hs))))
(define (cxt-map-h alpha hs) (if (null? hs) (quote ()) (cons (cxt-eval-poly (car hs) alpha) (cxt-map-h alpha (cdr hs)))))

; ----- evaluate a 2-variable mpoly generator (coeff . (e0 e1)) at a Gaussian tuple -----
(define (cxt-eval-gen gen tuple) (cxt-sum gen tuple))
(define (cxt-sum gen tuple) (if (null? gen) (gr-make 0 0) (gr-add (cxt-term (car gen) tuple) (cxt-sum (cdr gen) tuple))))
(define (cxt-term term tuple) (gr-mul (gr-from-rational (car term)) (cxt-mono (cdr term) tuple)))
(define (cxt-mono exps tuple) (cxt-mono-go exps tuple 0 (gr-make 1 0)))
(define (cxt-mono-go exps tuple j acc) (if (null? exps) acc (cxt-mono-go (cdr exps) tuple (+ j 1) (gr-mul acc (gr-pow (cxt-nth tuple j) (car exps))))))
(define (cxt-nth l k) (if (= k 0) (car l) (cxt-nth (cdr l) (- k 1))))

; ----- certify: every generator vanishes in Q(i) at the Gaussian tuple -----
(define (cxt-certify gens tuple) (if (equal? tuple (quote not-gaussian)) #f (cxt-all-zero gens tuple)))
(define (cxt-all-zero gens tuple) (cond ((null? gens) #t) ((gr-zero? (cxt-eval-gen (car gens) tuple)) (cxt-all-zero (cdr gens) tuple)) (else #f)))
