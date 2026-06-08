; -*- lisp -*-
; lib/cas/vanhoeijmb.lisp -- the MULTI-BRANCH combined-correction integral element on y^2 = f: building and
; certifying an integral-basis element w = (A(x) + B(x) y)/d(x) that is integral at a place where SEVERAL branches
; of the curve meet at once, the case vanhoeij.lisp honestly defers as 'needs-place-combination (docs/CAS.md --
; summit S2, degree>2 / general integral closure: the combined correction across branches).
;
; vanhoeij builds the correction for a SINGLE branch: w = (y - c(x))/(x-a)^k with c the low-order part of that one
; branch.  At a singular place where F(a, y) = 0 has several roots -- e.g. the node y^2 = x^2(x+1) at the origin,
; whose two branches are y = +-(x + x^2/2 - ...) -- no single-branch correction is integral at all of them, so
; vanhoeij returns 'needs-place-combination.  The combined element is instead found and verified by the exact
; integral-closure criterion in the quadratic field K = Q(x)[y]/(y^2 - f): the element w = (A + B y)/d has
; conjugate (A - B y)/d, hence minimal polynomial over Q[x]
;     w^2 - (2A/d) w + (A^2 - B^2 f)/d^2 = 0,
; and w is INTEGRAL over Q[x] exactly when both coefficients are polynomials -- the trace 2A/d and the norm
; (A^2 - B^2 f)/d^2 each divide out with zero remainder.  This is the same monic-minimal-polynomial test the
; superelliptic integral basis uses, specialized to the candidate's denominator, and it certifies integrality at
; EVERY branch simultaneously (the norm sees all of them).  For the node y^2 = x^2(x+1) the element y/x has trace 0
; and norm -(x+1), both polynomials, so y/x is integral and is the combined-branch basis element the single-branch
; construction could not produce.  The module also reports the explicit monic minimal polynomial as the
; certificate, and returns 'not-integral when the trace or norm fails to be a polynomial (never a forced answer).
;
; This handles the combined correction for the quadratic (hyperelliptic) case -- the case where the field norm is
; the exact A^2 - B^2 f and the test is a polynomial division.  The general-degree combined correction (n branches
; over a degree-n field) is the remaining work; here the soundness boundary is explicit and certified.
;
; Public (f the curve polynomial y^2=f; A, B, d polynomials with w = (A + B y)/d; all coefficient lists low->high):
;   mbc-trace f A B d          -> the trace 2A/d as (quotient . remainder) of the polynomial division
;   mbc-norm f A B d           -> the norm (A^2 - B^2 f)/d^2 as (quotient . remainder)
;   mbc-is-integral? f A B d   -> #t iff both the trace and the norm divide out exactly (w is integral over Q[x])
;   mbc-minpoly f A B d        -> (list 'monic trace-poly norm-poly): w^2 - trace*w + norm = 0, when integral
;   mbc-certify f A B d        -> #t iff the constructed minimal polynomial is monic with polynomial coefficients
;                                 AND reproduces w (the trace/norm recomputed from A,B,d match), the full certificate
;   mbc-element f A B d        -> (list 'integral A B d) | 'not-integral, the combined-branch element or honest no
;
; Verified: on the node y^2 = x^2(x+1) = x^3 + x^2 the element y/x (A=0, B=1, d=x) is integral with trace 0 and
; norm -(x+1); the element y (d=1) is trivially integral (trace 0, norm -f); a non-integral candidate like y/x^2
; (norm -(x+1)/x^2, not a polynomial) is correctly rejected; on y^2 = x(x-1)(x-2) the element y is integral and
; y/(x-1) is rejected (norm has a remainder), matching that x=1 is a smooth branch point with no combined element.
;
; Builds on poly.lisp.

(import "cas/poly.lisp")

(define (mbc-zero? p) (null? (mbc-trim p)))
(define (mbc-trim p) (mbc-trim-go p (mbc-len p)))
(define (mbc-len l) (if (null? l) 0 (+ 1 (mbc-len (cdr l)))))
(define (mbc-trim-go p k) (cond ((= k 0) (quote ())) ((= (mbc-nth p (- k 1)) 0) (mbc-trim-go p (- k 1))) (else (mbc-take p k))))
(define (mbc-nth l k) (if (= k 0) (car l) (mbc-nth (cdr l) (- k 1))))
(define (mbc-take l k) (if (= k 0) (quote ()) (cons (car l) (mbc-take (cdr l) (- k 1)))))

; ----- the trace 2A/d as (quotient . remainder) -----
(define (mbc-trace f A B d) (poly-divmod (poly-scale 2 A) d))

; ----- the norm (A^2 - B^2 f)/d^2 as (quotient . remainder) -----
(define (mbc-norm f A B d) (poly-divmod (poly-sub (poly-mul A A) (poly-mul (poly-mul B B) f)) (poly-mul d d)))

; ----- w is integral iff both trace and norm have zero remainder -----
(define (mbc-is-integral? f A B d) (if (mbc-zero? (car (cdr (mbc-trace f A B d)))) (mbc-zero? (car (cdr (mbc-norm f A B d)))) #f))

; ----- the monic minimal polynomial w^2 - trace*w + norm (the certificate) -----
(define (mbc-minpoly f A B d) (if (mbc-is-integral? f A B d) (list (quote monic) (car (mbc-trace f A B d)) (car (mbc-norm f A B d))) (quote not-integral)))

; ----- full certificate: integral AND the trace/norm quotients reproduce 2A and A^2-B^2 f exactly -----
; (compare via mbc-trim so the two representations of the zero polynomial, () and (0), count as equal)
(define (mbc-certify f A B d) (if (mbc-is-integral? f A B d) (mbc-cert-check f A B d) #f))
(define (mbc-cert-check f A B d)
  (if (mbc-peq? (poly-mul (car (mbc-trace f A B d)) d) (poly-scale 2 A))
      (mbc-peq? (poly-mul (car (mbc-norm f A B d)) (poly-mul d d)) (poly-sub (poly-mul A A) (poly-mul (poly-mul B B) f)))
      #f))
(define (mbc-peq? p q) (equal? (mbc-trim p) (mbc-trim q)))

; ----- the combined-branch element, or honest no -----
(define (mbc-element f A B d) (if (mbc-is-integral? f A B d) (list (quote integral) A B d) (quote not-integral)))
