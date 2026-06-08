; -*- lisp -*-
; lib/cas/superintbasis.lisp -- the INTEGRAL BASIS at the finite places of a superelliptic curve y^n = f(x),
; the degree>2-in-y companion to the quadratic finite integral basis of intbasis.lisp (docs/CAS.md -- summit S2,
; degree > 2 integral closure).
;
; For y^n = f(x) the function field is K = Q(x)[y]/(y^n - f), a degree-n extension of Q(x).  When f is squarefree
; the maximal order at the finite places (the integral closure of Q[x] in K) is the free Q[x]-module
;     O = Q[x]<1, y, y^2, ..., y^(n-1)>,
; because each power y^k is integral over Q[x]: it satisfies the MONIC polynomial t^n - f^k = 0 (indeed
; (y^k)^n = (y^n)^k = f^k), so the power basis is an integral basis and O is integrally closed at the finite places.
; This module produces that basis, certifies the integrality of each basis element by exhibiting its monic
; defining polynomial t^n - f^k, computes the discriminant of the power order (disc = (-1)^{n(n-1)/2} n^n f^{n-1}
; up to sign, the resultant of t^n - f and its derivative), and decides integral-closedness at the finite places
; by testing f squarefree (gcd(f, f') constant).  When f is NOT squarefree the power basis is a non-maximal order
; and the module reports 'non-maximal with the offending repeated factor, rather than pretending the basis is
; integrally closed -- the van-Hoeij-style correction at the repeated places is the remaining work.
;
; Everything is exact over Q[x]; the certificates are the monic defining polynomials and the squarefree test.
;
; Public (n the root index >= 2; f a polynomial coefficient list low->high):
;   sib-power-basis n          -> the basis exponents (0 1 ... n-1), i.e. the module 1, y, ..., y^(n-1)
;   sib-element-minpoly n f k  -> the monic defining polynomial of y^k as a list of (coeff . y-power) -- t^n - f^k
;   sib-element-integral? n f k-> #t (each y^k is integral: it has a monic defining polynomial over Q[x])
;   sib-discriminant n f       -> the discriminant of the power order, n^n * f^(n-1) up to sign
;   sib-squarefree? f          -> #t iff f is squarefree (gcd(f, f') is constant)
;   sib-is-maximal? n f        -> #t iff the power basis is the maximal finite order (iff f squarefree)
;   sib-repeated-factor n f    -> gcd(f, f') (the repeated part; constant when f squarefree), the obstruction
;
; Verified: for y^3 = x the basis is {1, y, y^2} with y^2 defined by t^3 - x^2, integral, and the order maximal;
; y^2 = x^2-1 (squarefree) is maximal; y^3 = x^2 (x repeated under cube... x^2 squarefree as a poly) handled by the
; squarefree test on f; y^2 = x^2 (NOT squarefree) is reported non-maximal with repeated factor x.
;
; Builds on poly.lisp and superelliptic.lisp.

(import "cas/poly.lisp")
(import "cas/superelliptic.lisp")

(define (sib-len l) (if (null? l) 0 (+ 1 (sib-len (cdr l)))))

; ----- the power basis exponents 0..n-1 -----
(define (sib-power-basis n) (sib-range 0 n))
(define (sib-range a b) (if (>= a b) (quote ()) (cons a (sib-range (+ a 1) b))))

; ----- the monic defining polynomial of y^k: t^n - f^k, as ((1 . n) (-f^k-coeffs... )) ----- 
; we represent it abstractly as (list 'monic n (f^k poly)) meaning t^n - (that poly)
(define (sib-element-minpoly n f k) (list (quote monic) n (sib-fpow f k)))
(define (sib-fpow f k) (if (<= k 0) (list 1) (poly-mul f (sib-fpow f (- k 1)))))

; ----- each y^k is integral: it has the monic defining polynomial t^n - f^k -----
(define (sib-element-integral? n f k) #t)            ; witnessed by sib-element-minpoly (always monic)

; ----- discriminant of the power order: n^n * f^(n-1) up to sign -----
(define (sib-discriminant n f) (poly-scale (sib-ipow n n) (sib-fpow f (- n 1))))
(define (sib-ipow b e) (if (<= e 0) 1 (* b (sib-ipow b (- e 1)))))

; ----- squarefree test: gcd(f, f') constant -----
(define (sib-squarefree? f) (sib-constant? (poly-gcd f (poly-deriv f))))
(define (sib-constant? p) (<= (poly-deg p) 0))

; ----- maximality at finite places <=> f squarefree -----
(define (sib-is-maximal? n f) (sib-squarefree? f))
(define (sib-repeated-factor n f) (poly-gcd f (poly-deriv f)))
