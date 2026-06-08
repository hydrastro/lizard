; -*- lisp -*-
; lib/cas/infplaces.lisp -- the INFINITE PLACES of the hyperelliptic curve y^2 = q(x): how many points lie over
; x = infinity, whether the cover ramifies there, and an INDEPENDENT genus computation by Riemann-Hurwitz, which
; together complete the place/genus picture that the finite integral basis (intbasis.lisp) and the genus decision
; (hypergenus.lisp) assume (docs/CAS.md -- summit S2: integral bases at infinite places).
;
; The x-coordinate gives a degree-2 cover phi : C -> P^1.  Over x = infinity (substitute x = 1/t), the behavior
; is governed by the parity of d = deg q and the leading coefficient:
;   - d ODD: there is ONE place at infinity and the cover RAMIFIES there (infinity is a branch point);
;   - d EVEN: the cover is unramified at infinity, with TWO places when the leading coefficient is a perfect
;     square (the two sheets stay separate) and ONE place (with residue field a quadratic extension) otherwise.
; Counting ramification R (the d simple finite branch points -- the roots of squarefree q -- plus infinity when
; d is odd) and applying Riemann-Hurwitz 2g - 2 = 2(-2) + R gives g = (R - 2)/2, which equals floor((d-1)/2):
; an INDEPENDENT check of the hyperelliptic genus, agreeing with hypergenus on every degree.  Everything is exact
; integer arithmetic on d plus a perfect-square test on the leading coefficient; no analysis or approximation.
;
; Public (q a polynomial coefficient list, low->high, squarefree):
;   ip-degree q                -> deg q
;   ip-infinite-ramified? q    -> #t iff the cover ramifies at infinity (iff deg q is odd)
;   ip-num-infinite-places q   -> number of places over x = infinity (1 if d odd or lead non-square; else 2)
;   ip-ramification q          -> total ramification number R of the x-cover (finite branch points + infinity)
;   ip-genus-rh q              -> the genus via Riemann-Hurwitz, (R - 2)/2
;   ip-genus-agrees? q         -> #t iff the Riemann-Hurwitz genus equals floor((deg q - 1)/2)
;
; Verified: y^2 = x^3+1 (d=3) has one ramified infinite place and RH-genus 1; y^2 = x^4+1 (d=4, lead 1 square)
; has two infinite places, unramified, RH-genus 1; y^2 = x^5+1 RH-genus 2 (one ramified place); y^2 = x^6+1 two
; infinite places, RH-genus 2; the RH genus agrees with floor((d-1)/2) for degrees 3 through 9.
;
; Builds on poly.lisp.

(import "cas/poly.lisp")

(define (ip-len l) (if (null? l) 0 (+ 1 (ip-len (cdr l)))))
(define (ip-nth l k) (if (= k 0) (car l) (ip-nth (cdr l) (- k 1))))

; ----- degree and leading coefficient (on the trimmed poly) -----
(define (ip-trim q) (ip-trim-go q (ip-len q)))
(define (ip-trim-go q n) (cond ((= n 0) 0) ((= (ip-nth q (- n 1)) 0) (ip-trim-go q (- n 1))) (else n)))
(define (ip-degree q) (- (ip-trim q) 1))
(define (ip-lead q) (ip-nth q (- (ip-trim q) 1)))

; ----- parity-driven classification at infinity -----
(define (ip-odd? n) (= (remainder n 2) 1))
(define (ip-infinite-ramified? q) (ip-odd? (ip-degree q)))
(define (ip-num-infinite-places q) (if (ip-odd? (ip-degree q)) 1 (if (ip-perfect-square? (ip-lead q)) 2 1)))

; perfect-square test for a rational leading coefficient (num and den both perfect squares of integers)
(define (ip-perfect-square? c) (if (ip-rational-int? c) (ip-int-square? c) (ip-rat-square? c)))
(define (ip-rational-int? c) (if (= (denominator c) 1) #t #f))
(define (ip-int-square? n) (if (< n 0) #f (ip-is-sq (numerator n))))
(define (ip-rat-square? c) (if (< c 0) #f (if (ip-is-sq (numerator c)) (ip-is-sq (denominator c)) #f)))
(define (ip-is-sq n) (ip-sq-search n 0))
(define (ip-sq-search n k) (cond ((> (* k k) n) #f) ((= (* k k) n) #t) (else (ip-sq-search n (+ k 1)))))

; ----- Riemann-Hurwitz: R = (finite branch points = deg q, q squarefree) + (1 if infinity ramified else 0) -----
(define (ip-ramification q) (+ (ip-degree q) (if (ip-infinite-ramified? q) 1 0)))
(define (ip-genus-rh q) (quotient (- (ip-ramification q) 2) 2))

; ----- cross-check against floor((d-1)/2) -----
(define (ip-genus-floor q) (quotient (- (ip-degree q) 1) 2))
(define (ip-genus-agrees? q) (= (ip-genus-rh q) (ip-genus-floor q)))
