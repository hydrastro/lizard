; -*- lisp -*-
; lib/cas/hyperelliptic.lisp -- the genus-2 (and general higher-genus) DECISION for INT P(x)/sqrt(q(x)) dx with
; q squarefree of degree 5 or 6: decide elementarity, certify the algebraic antiderivative when it exists, and
; otherwise report a genuine HYPERELLIPTIC integral -- non-elementary by exact reduction
; (docs/TRAGER_ROADMAP.md, frontier 2: genus >= 2 hyperelliptic curves).
;
; The Hermite-style reduction for the radical (built in elliptic.lisp) is already degree-general: since
; d/dx[x^k sqrt q] has numerator degree k + deg(q) - 1, repeatedly subtracting multiples reduces the numerator P
; below degree deg(q) - 1, accumulating the algebraic part A with P/sqrt q = d/dx[A sqrt q] + rem/sqrt q.  The
; only genus-dependent fact is the meaning of a surviving remainder: the curve y^2 = q has genus
; g = floor((deg(q) - 1)/2), and the holomorphic (first-kind) differentials x^i dx / sqrt(q), 0 <= i < g, are
; non-elementary; for deg(q) in {5, 6} the genus is 2, so a nonzero reduced remainder (degree <= deg(q) - 2)
; spans first/second-kind hyperelliptic differentials and the integral is NON-ELEMENTARY.  Remainder zero gives
; the elementary answer A sqrt(q), certified inside K = Q(x)[y]/(y^2 - q).
;
; This module wraps that reduction for the genus-2 case (and reports the genus), reusing elliptic.lisp's reducer
; and certificate; it is sound both ways exactly as the elliptic decision is.  As with genus 1, a third-kind
; logarithmic part (rational residue at a pole) could make a remainder elementary; this decision makes the firm
; non-elementary call for the reduced first/second-kind remainder and reports 'inconclusive otherwise.
;
; Public:
;   he-genus q                 -> floor((deg q - 1)/2), the genus of y^2 = q for squarefree q
;   he-disc-ok? q              -> #t iff q is squarefree of degree 5 or 6 (genus 2)
;   he-integrate P q           -> (list 'elementary A) | (list 'non-elementary 'hyperelliptic-genus-2)
;                                 | (list 'inconclusive ..) : the decision for INT P/sqrt(q)
;   he-split P q               -> (list 'split A rem) : the elementary part A sqrt q and the named hyperelliptic
;                                 remainder rem/sqrt q (integrate as far as possible)
;   he-certify P q r           -> #t iff an 'elementary result differentiates back to P/sqrt q inside K
;
; Verified: INT (5x^4/2)/sqrt(x^5+1) = sqrt(x^5+1) (elementary, certified); INT 1/sqrt(x^5+1) and INT x/sqrt(x^5+1)
; (genus-2 hyperelliptic, non-elementary); INT sqrt(x^5+1) split into elementary part + hyperelliptic remainder;
; a sextic case; genus reported as 2 for degree 5 and 6.
;
; Builds on elliptic.lisp (the degree-general reducer ell-reduce, ell-certify-raw) and poly.lisp / msqfree.lisp.

(import "cas/elliptic.lisp")
(import "cas/poly.lisp")
(import "cas/msqfree.lisp")

(define (he-len l) (if (null? l) 0 (+ 1 (he-len (cdr l)))))
(define (he-nth l k) (if (= k 0) (car l) (he-nth (cdr l) (- k 1))))
(define (he-coeff p k) (if (if (< k 0) #t (>= k (he-len p))) 0 (he-nth p k)))
(define (he-trimlen p) (he-tl p (he-len p)))
(define (he-tl p n) (cond ((= n 0) 0) ((= (he-coeff p (- n 1)) 0) (he-tl p (- n 1))) (else n)))
(define (he-deg p) (- (he-trimlen p) 1))
(define (he-zero? p) (= (he-trimlen p) 0))

; ----- genus and applicability -----
(define (he-genus q) (quotient (- (he-deg q) 1) 2))
(define (he-disc-ok? q) (if (if (= (he-deg q) 5) #t (= (he-deg q) 6)) (he-squarefree? q) #f))
(define (he-squarefree? q) (= (he-deg (poly-gcd q (poly-deriv q))) 0))

; ----- the decision (reusing elliptic.lisp's degree-general reduction) -----
(define (he-integrate P q) (if (he-disc-ok? q) (he-decide P q (ell-reduce P q)) (list (quote inconclusive) (quote not-genus-2-squarefree))))
(define (he-decide P q red) (he-verdict P q (car red) (car (cdr red))))
(define (he-verdict P q A rem)
  (if (he-zero? rem)
      (he-elem P q A)
      (list (quote non-elementary) (quote hyperelliptic-genus-2))))
(define (he-elem P q A) (if (ell-certify-raw P q A) (list (quote elementary) A) (list (quote inconclusive) (quote uncertified-reduction))))

; ----- integrate as far as possible: elementary part + named hyperelliptic remainder -----
(define (he-split P q) (if (he-disc-ok? q) (he-split-go P q (ell-reduce P q)) (list (quote inconclusive) (quote not-genus-2-squarefree))))
(define (he-split-go P q red) (he-split-chk P q (car red) (car (cdr red))))
(define (he-split-chk P q A rem) (if (ell-split-certify P q A rem) (list (quote split) A rem) (list (quote inconclusive) (quote uncertified-split))))

; ----- certificate inside K (delegated to the elliptic field certificate) -----
(define (he-certify P q r) (if (equal? (car r) (quote elementary)) (ell-certify-raw P q (car (cdr r))) #f))
