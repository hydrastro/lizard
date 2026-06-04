; -*- lisp -*-
; lib/cas/hypergenus.lisp -- the general higher-genus DECISION for INT P(x)/sqrt(q(x)) dx with q squarefree of
; ANY degree >= 7 (genus >= 3): decide elementarity, certify the algebraic antiderivative when it exists, and
; otherwise report a genuine higher-genus hyperelliptic integral, non-elementary by exact reduction
; (docs/CAS.md -- frontier b: genus >= 3 radicands).
;
; The elliptic.lisp reduction d/dx[x^k sqrt q] = (k x^{k-1} q + x^k q'/2)/sqrt q is degree-general, so reducing
; the numerator P below degree deg(q) - 1 works for every degree.  The curve y^2 = q has genus
; g = floor((deg q - 1)/2); the holomorphic differentials x^i dx / sqrt q for 0 <= i < g are non-elementary, so
; a nonzero reduced remainder is a first/second-kind higher-genus differential and INT P/sqrt q is NON-ELEMENTARY.
; A zero remainder gives the elementary answer A sqrt(q), certified inside K = Q(x)[y]/(y^2 - q).
;
; This generalizes hyperelliptic.lisp (which gated degree 5/6, genus 2) to all higher degrees, reusing the same
; reducer and certificate; it is sound both ways, and reports the genus.  As before, a third-kind logarithmic
; part could in principle make a remainder elementary; the firm non-elementary call is for the reduced
; first/second-kind remainder, and other situations are reported 'inconclusive.
;
; Public:
;   hg-genus q                 -> floor((deg q - 1)/2), the genus of y^2 = q for squarefree q (any degree)
;   hg-applicable? q           -> #t iff q is squarefree of degree >= 7 (genus >= 3; lower genus is handled by
;                                 elliptic.lisp / hyperelliptic.lisp)
;   hg-integrate P q           -> (list 'elementary A) | (list 'non-elementary (list 'hyperelliptic-genus g))
;                                 | (list 'inconclusive ..) : the decision for INT P/sqrt(q)
;   hg-split P q               -> (list 'split A rem) : elementary part A sqrt q + named higher-genus remainder
;   hg-certify P q r           -> #t iff an 'elementary result differentiates back to P/sqrt q inside K
;
; Verified: INT (7x^6/2)/sqrt(x^7+1) = sqrt(x^7+1) and INT 4x^7/sqrt(x^8+1) = sqrt(x^8+1) (elementary, certified,
; genus 3); INT 1/sqrt(x^7+1) and INT x/sqrt(x^7+1) (genus-3 hyperelliptic, non-elementary); a degree-9 case
; (genus 4); the genus reported correctly; INT sqrt(x^7+1) split into elementary part + higher-genus remainder.
;
; Builds on elliptic.lisp (the degree-general reducer and the K-certificate) and poly.lisp / msqfree.lisp.

(import "cas/elliptic.lisp")
(import "cas/poly.lisp")
(import "cas/msqfree.lisp")

(define (hg-len l) (if (null? l) 0 (+ 1 (hg-len (cdr l)))))
(define (hg-nth l k) (if (= k 0) (car l) (hg-nth (cdr l) (- k 1))))
(define (hg-coeff p k) (if (if (< k 0) #t (>= k (hg-len p))) 0 (hg-nth p k)))
(define (hg-trimlen p) (hg-tl p (hg-len p)))
(define (hg-tl p n) (cond ((= n 0) 0) ((= (hg-coeff p (- n 1)) 0) (hg-tl p (- n 1))) (else n)))
(define (hg-deg p) (- (hg-trimlen p) 1))
(define (hg-zero? p) (= (hg-trimlen p) 0))

; ----- genus and applicability -----
(define (hg-genus q) (quotient (- (hg-deg q) 1) 2))
(define (hg-applicable? q) (if (>= (hg-deg q) 7) (hg-squarefree? q) #f))
(define (hg-squarefree? q) (= (hg-deg (poly-gcd q (poly-deriv q))) 0))

; ----- the decision (reusing the degree-general reduction) -----
(define (hg-integrate P q) (if (hg-applicable? q) (hg-decide P q (ell-reduce P q)) (list (quote inconclusive) (quote not-genus-ge-3-squarefree))))
(define (hg-decide P q red) (hg-verdict P q (car red) (car (cdr red))))
(define (hg-verdict P q A rem)
  (if (hg-zero? rem)
      (hg-elem P q A)
      (list (quote non-elementary) (list (quote hyperelliptic-genus) (hg-genus q)))))
(define (hg-elem P q A) (if (ell-certify-raw P q A) (list (quote elementary) A) (list (quote inconclusive) (quote uncertified-reduction))))

; ----- integrate as far as possible -----
(define (hg-split P q) (if (hg-applicable? q) (hg-split-go P q (ell-reduce P q)) (list (quote inconclusive) (quote not-genus-ge-3-squarefree))))
(define (hg-split-go P q red) (hg-split-chk P q (car red) (car (cdr red))))
(define (hg-split-chk P q A rem) (if (ell-split-certify P q A rem) (list (quote split) A rem) (list (quote inconclusive) (quote uncertified-split))))

; ----- certificate inside K (delegated) -----
(define (hg-certify P q r) (if (equal? (car r) (quote elementary)) (ell-certify-raw P q (car (cdr r))) #f))
