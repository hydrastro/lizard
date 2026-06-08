; -*- lisp -*-
; lib/cas/superelliptic.lisp -- the GENUS of SUPERELLIPTIC curves y^n = f(x) for any n >= 2, generalizing the
; hyperelliptic (n = 2) genus of infplaces.lisp/hypergenus.lisp to higher cyclic covers (docs/CAS.md -- summit S2,
; integral bases / genus beyond degree-2-in-y).
;
; For y^n = f(x) with f squarefree (separable) of degree d, the x-map is a degree-n cyclic cover of P^1, and the
; genus is given by the classical superelliptic formula
;     g = (1/2) [ (n - 1)(d - 1) - (gcd(n, d) - 1) ].
; This module computes that genus and INDEPENDENTLY confirms it by Riemann-Hurwitz: the cover ramifies totally
; over each of the d roots of f (index n, contributing n - 1 apiece) and over infinity at gcd(n, d) places of
; index n/gcd(n, d) (contributing n - gcd(n, d) in total), so the ramification number is
;     R = d (n - 1) + (n - gcd(n, d)),
; and 2g - 2 = -2n + R gives g = R/2 - n + 1, which equals the formula above on every case.  Both computations are
; exact integer arithmetic; for n = 2 they reduce to floor((d - 1)/2), agreeing with the hyperelliptic modules.
; The number and ramification of the places over infinity are also reported.
;
; Public (n the root index >= 2; f a polynomial coefficient list low->high, assumed squarefree):
;   sup-degree f               -> deg f
;   sup-gcd a b                -> gcd of two nonnegative integers
;   sup-genus n f              -> the superelliptic genus via the closed formula
;   sup-ramification n f       -> the total ramification number R of the x-cover
;   sup-genus-rh n f           -> the genus via Riemann-Hurwitz, R/2 - n + 1
;   sup-genus-agrees? n f      -> #t iff the formula and the Riemann-Hurwitz genus coincide
;   sup-infinite-places n f    -> the number of places over x = infinity, gcd(n, deg f)
;   sup-reduces-to-hyperelliptic? f -> #t iff the n=2 genus equals floor((deg f - 1)/2) (the consistency check)
;
; Verified: y^2 = f matches floor((d-1)/2) for d = 3..6; y^3 = f of degree 4 has genus 3, degree 5 genus 4;
; y^4 = f of degree 3 genus 3, degree 5 genus 6; y^3 = (degree 3) genus 1; the formula and Riemann-Hurwitz agree
; for n = 2..6 across these degrees; the infinite-place count is gcd(n, d).
;
; Builds on poly.lisp.

(import "cas/poly.lisp")

(define (sup-len l) (if (null? l) 0 (+ 1 (sup-len (cdr l)))))
(define (sup-nth l k) (if (= k 0) (car l) (sup-nth (cdr l) (- k 1))))

; ----- degree on a trimmed coefficient list -----
(define (sup-trim f) (sup-trim-go f (sup-len f)))
(define (sup-trim-go f n) (cond ((= n 0) 0) ((= (sup-nth f (- n 1)) 0) (sup-trim-go f (- n 1))) (else n)))
(define (sup-degree f) (- (sup-trim f) 1))

; ----- gcd -----
(define (sup-gcd a b) (if (= b 0) a (sup-gcd b (remainder a b))))

; ----- the closed superelliptic genus formula -----
(define (sup-genus n f) (sup-g-formula n (sup-degree f)))
(define (sup-g-formula n d) (quotient (- (* (- n 1) (- d 1)) (- (sup-gcd n d) 1)) 2))

; ----- ramification and the Riemann-Hurwitz genus -----
(define (sup-ramification n f) (sup-ram n (sup-degree f)))
(define (sup-ram n d) (+ (* d (- n 1)) (- n (sup-gcd n d))))
(define (sup-genus-rh n f) (+ (- (quotient (sup-ramification n f) 2) n) 1))

; ----- the cross-check -----
(define (sup-genus-agrees? n f) (= (sup-genus n f) (sup-genus-rh n f)))

; ----- places over infinity -----
(define (sup-infinite-places n f) (sup-gcd n (sup-degree f)))

; ----- consistency with the hyperelliptic modules at n = 2 -----
(define (sup-reduces-to-hyperelliptic? f) (= (sup-genus 2 f) (quotient (- (sup-degree f) 1) 2)))
