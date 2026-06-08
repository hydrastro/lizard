; -*- lisp -*-
; lib/cas/ramplace.lisp -- the RAMIFIED-PLACE integral element on y^2 = f: building and certifying the local
; integral-closure generator at a place where the cover y^2 = f ramifies (a root of f of multiplicity >= 2, where
; the branch is a Puiseux series in a fractional power, not an ordinary power series), the case intbasis.lisp and
; vanhoeij.lisp defer (docs/CAS.md -- general integral closure: the ramified places, harder than the multi-branch
; node because the local parameter is fractional).
;
; At x = a with (x - a)^m || f (multiplicity m >= 1), the branch is y = (x-a)^{m/2} * unit, so over Q[x] the
; valuations at the place are v(x - a) = 2 and v(y) = m.  When m >= 2 the order Q[x]<1, y> is NOT maximal there:
; the element
;     w = y / (x - a)^{floor(m/2)}
; has valuation m - 2*floor(m/2) = (m mod 2) >= 0, so it is integral, and it generates the local integral closure
; (for m odd it is the ramified uniformizer's partner with v = 1; for m even it is a unit that splits the place).
; Integrality is exact and CERTIFIED by the monic minimal polynomial: w^2 = f / (x-a)^{2 floor(m/2)} is a polynomial
; (the division has zero remainder because 2 floor(m/2) <= m <= the multiplicity of (x-a) in f), so w satisfies the
; monic w^2 - (f/(x-a)^{2k}) = 0 over Q[x].  The module computes the multiplicity, the two valuations, the element,
; and the certificate, and reports the place as ramified (m odd) or split/unramified (m even) -- the Riemann-Hurwitz
; local data.  A point that is NOT a root of f (m = 0) is reported 'unramified-regular with no new element, and a
; non-integral candidate is never forced.
;
; This handles a single ramified place of the quadratic (hyperelliptic) cover, by exact polynomial division.  The
; general-degree ramification (a degree-n cover with e | n and a genuine Puiseux cycle of length e) is the
; remaining work; here the local parameter is the square root and the valuations are exact.
;
; Public (f the curve polynomial y^2 = f; a a rational point; coefficient lists low->high):
;   ram-mult f a               -> the multiplicity m of (x - a) in f (0 if a is not a root)
;   ram-val-x f a              -> v(x - a) at the place = 2 when m >= 1 (the cover is degree 2 over x)
;   ram-val-y f a              -> v(y) at the place = m (the multiplicity)
;   ram-is-ramified? f a       -> #t iff the place is ramified (m odd), #f if split/unramified (m even or m = 0)
;   ram-element f a            -> (list 'elt k) meaning w = y/(x-a)^k with k = floor(m/2), or 'no-new-element (m<2)
;   ram-element-sq f a         -> the polynomial w^2 = f/(x-a)^{2k} (the monic minimal polynomial's constant part)
;   ram-certify f a            -> #t iff w^2 = f/(x-a)^{2k} divides out exactly (w integral over Q[x]) AND
;                                 (x-a)^{2k} * (that quotient) = f (the division reproduces f) -- the full certificate
;   ram-place-type f a         -> 'ramified | 'split | 'unramified-regular (the local place classification)
;
; Verified: y^2 = x^3 (m=3 at 0) gives w = y/x with w^2 = x, ramified, certified; y^2 = x^5 (m=5) gives w = y/x^2
; with w^2 = x; y^2 = (x-1)^3 (m=3 at 1) gives w = y/(x-1) with w^2 = x-1; y^2 = x^2 (m=2) gives w = y/x with
; w^2 = 1 (split, a unit); a non-root point (a where f(a) != 0) gives 'unramified-regular and no new element.
;
; Builds on poly.lisp.

(import "cas/poly.lisp")

; ----- the multiplicity of (x - a) in f, by repeated exact division -----
(define (ram-mult f a) (ram-mult-go f a 0))
(define (ram-mult-go f a acc) (if (ram-divides? f a) (ram-mult-go (car (poly-divmod f (list (- 0 a) 1))) a (+ acc 1)) acc))
(define (ram-divides? f a) (if (ram-zero? f) #f (ram-zero? (car (cdr (poly-divmod f (list (- 0 a) 1)))))))
(define (ram-zero? p) (null? (ram-trim p)))
(define (ram-trim p) (ram-trim-go p (ram-len p)))
(define (ram-len l) (if (null? l) 0 (+ 1 (ram-len (cdr l)))))
(define (ram-trim-go p k) (cond ((= k 0) (quote ())) ((= (ram-nth p (- k 1)) 0) (ram-trim-go p (- k 1))) (else (ram-take p k))))
(define (ram-nth l k) (if (= k 0) (car l) (ram-nth (cdr l) (- k 1))))
(define (ram-take l k) (if (= k 0) (quote ()) (cons (car l) (ram-take (cdr l) (- k 1)))))

; ----- the valuations at the place -----
(define (ram-val-x f a) (if (>= (ram-mult f a) 1) 2 0))      ; v(x-a) = 2 (degree-2 cover) at any point on the curve
(define (ram-val-y f a) (ram-mult f a))                       ; v(y) = multiplicity

; ----- ramified (m odd) vs split/unramified (m even) -----
(define (ram-is-ramified? f a) (ram-odd? (ram-mult f a)))
(define (ram-odd? m) (= (remainder m 2) 1))

; ----- the integral element w = y/(x-a)^k, k = floor(m/2) -----
(define (ram-k f a) (ram-half (ram-mult f a)))
(define (ram-half m) (quotient m 2))
(define (ram-element f a) (if (< (ram-mult f a) 2) (quote no-new-element) (list (quote elt) (ram-k f a))))

; ----- w^2 = f/(x-a)^{2k} -----
(define (ram-denom f a) (ram-pow (list (- 0 a) 1) (* 2 (ram-k f a))))
(define (ram-pow p e) (if (<= e 0) (list 1) (poly-mul p (ram-pow p (- e 1)))))
(define (ram-element-sq f a) (car (poly-divmod f (ram-denom f a))))

; ----- the certificate: w^2 divides out exactly AND reproduces f -----
(define (ram-certify f a) (if (< (ram-mult f a) 2) #f (ram-cert-go f a)))
(define (ram-cert-go f a)
  (if (ram-zero? (car (cdr (poly-divmod f (ram-denom f a)))))
      (equal? (ram-trim (poly-mul (ram-element-sq f a) (ram-denom f a))) (ram-trim f))
      #f))

; ----- the place classification -----
(define (ram-place-type f a) (ram-classify (ram-mult f a)))
(define (ram-classify m) (cond ((= m 0) (quote unramified-regular)) ((ram-odd? m) (quote ramified)) (else (quote split))))
