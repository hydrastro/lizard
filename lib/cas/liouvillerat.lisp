; -*- lisp -*-
; lib/cas/liouvillerat.lisp -- the RATIONAL-coefficient extension of the Liouville exponential decider: decides
; INT R(x) e^x dx for R a RATIONAL function (liouville.lisp handled polynomial R).  By Liouville's theorem this
; is elementary iff there is a RATIONAL S with S' + S = R, in which case the antiderivative is S e^x.  Part of
; the decider suite that proves elementarity verdicts both ways (docs/TRAGER_ROADMAP.md, the summit).
;
; We restrict to R with a single finite pole at the origin for a clean, exact decision (R = poly(x) + sum_{j=1}^{m}
; r_j / x^j), which already exhibits the essential phenomenon -- the exponential-integral obstruction.  Writing
; S = poly_S(x) + sum_{j=1}^{M} s_j / x^j, the equation S' + S = R splits:
;   * polynomial part: solved exactly by the polynomial Liouville decider (liouville.lisp, g = x);
;   * principal (pole) part: S' lowers nothing but RAISES the pole order, S' = sum -j s_j / x^{j+1}, so
;     (S' + S) has, at order k (for 1 <= k <= M+1), coefficient  -(k-1) s_{k-1} + s_k  (with s_0 = s_{M+1} = 0).
;     Matching this triangular system to R's principal part (coefficients r_k, with r_k = 0 for k > m) is solvable
;     iff a consistent set of s_j exists; the lowest level (k = 1) gives s_1 = r_1, and the top level
;     (k = M+1) gives -(M) s_M = r_{M+1} = 0, forcing the chain.  The KEY obstruction: a nonzero residue r_1
;     (a simple pole, the Ei case) makes the system inconsistent -- INT e^x/x is non-elementary -- and likewise
;     a lone 1/x^m with no compensating terms fails (INT e^x/x^2 reduces to Ei).  When R's principal part is
;     itself of the form S'+S for a rational pole part (e.g. R = 1/x - 1/x^2 from S = 1/x), it is elementary.
;
; Public:
;   lr-poles R-prin         -> attempt to solve the pole-part system for S's principal coeffs | 'none
;   lr-decide poly r-prin   -> (list 'elementary polyS sPrin) : INT (poly + r-prin) e^x dx = (polyS + sPrin) e^x,
;                              | (list 'non-elementary 'no-rational-S) : the proven verdict
;     (poly is the polynomial part of R as a coeff list; r-prin is the principal part as a list (r_1 r_2 ... r_m),
;      r_j the coefficient of 1/x^j)
;   lr-certify poly r-prin polyS sPrin -> #t iff (S' + S) = R with S = polyS + sum sPrin_j/x^j
;
; Verified: INT e^x/x (r-prin (1)) NON-elementary (Ei); INT e^x/x^2 (r-prin (0 1)) NON-elementary; the designed
; elementary INT (1/x - 1/x^2) e^x = e^x/x (r-prin (1 -1) -> S-prin (0 1)); INT x e^x = (x-1) e^x (polynomial).
;
; Builds on liouville.lisp (the polynomial part) and poly.lisp.

(import "cas/liouville.lisp")
(import "cas/poly.lisp")

(define (lr-nth l k) (if (= k 0) (car l) (lr-nth (cdr l) (- k 1))))
(define (lr-len l) (if (null? l) 0 (+ 1 (lr-len (cdr l)))))
(define (lr-nthor l k) (if (< k 0) 0 (if (< k (lr-len l)) (lr-nth l k) 0)))

; ----- pole-part decision.  r-prin = (r_1 r_2 ... r_m) (coeff of 1/x^j at index j-1).  Solve for
; s-prin = (s_1 ... s_M) with the relation, at each order k (1..M+1):  -(k-1) s_{k-1} + s_k = r_k.
; We take M = m (S's principal part has the same top order as R's), then VERIFY; if the top-order forcing makes
; it inconsistent the verdict is non-elementary.  The recurrence solves downward:
;   k=1:  s_1 = r_1
;   k=2:  -1 s_1 + s_2 = r_2  ->  s_2 = r_2 + s_1
;   k:    s_k = r_k + (k-1) s_{k-1}
; then the top closing equation at k=m+1: -(m) s_m = r_{m+1} = 0 must hold, i.e. s_m = 0.  If the forward
; recurrence yields s_m = 0 the system is consistent (elementary); otherwise inconsistent (non-elementary). -----
(define (lr-poles r-prin) (lr-poles-go r-prin 1 (lr-len r-prin) 0 (quote ())))
(define (lr-poles-go r-prin k m sprev acc)
  (if (> k m) (lr-poles-close acc sprev)
      (lr-poles-step r-prin k m acc (+ (lr-nthor r-prin (- k 1)) (* (- k 1) sprev)))))
(define (lr-poles-step r-prin k m acc sk) (lr-poles-go r-prin (+ k 1) m sk (lr-append acc sk)))
; closing: the top equation -(m) s_m = 0 requires s_m = 0 (sprev holds s_m here)
(define (lr-poles-close acc sm) (if (= sm 0) acc (quote none)))
(define (lr-append l v) (if (null? l) (list v) (cons (car l) (lr-append (cdr l) v))))

; ----- the decision -----
(define (lr-decide poly r-prin) (lr-decide-go poly r-prin (lr-poles r-prin) (lv-decide-or-poly poly)))
(define (lv-decide-or-poly poly) (if (lr-allzero? poly) (list) (lr-poly-S poly)))
(define (lr-allzero? p) (cond ((null? p) #t) ((= (car p) 0) (lr-allzero? (cdr p))) (else #f)))
; polynomial part S solves polyS' + polyS = poly, by the polynomial Liouville decider with g = x
(define (lr-poly-S poly) (lr-extract (lv-decide poly (list 0 1))))
(define (lr-extract v) (if (equal? (car v) (quote elementary)) (car (cdr v)) (quote none)))
(define (lr-decide-go poly r-prin sPrin polyS)
  (if (if (equal? sPrin (quote none)) #t (equal? polyS (quote none)))
      (list (quote non-elementary) (quote no-rational-S))
      (list (quote elementary) polyS sPrin)))

; ----- certificate: (S' + S) = R, S = polyS + sum sPrin_j / x^j, R = poly + sum r-prin_j / x^j -----
; polynomial side: polyS' + polyS =? poly.  pole side: at order k, -(k-1) sPrin_{k-1} + sPrin_k =? r-prin_k.
(define (lr-certify poly r-prin polyS sPrin)
  (if (lr-poly-side? poly polyS) (lr-pole-side? r-prin sPrin) #f))
(define (lr-poly-side? poly polyS) (lr-peq? (poly-add (poly-deriv polyS) polyS) poly))
(define (lr-pole-side? r-prin sPrin) (lr-pole-go r-prin sPrin 1 (lr-maxlen r-prin sPrin)))
(define (lr-maxlen a b) (if (> (lr-len a) (lr-len b)) (lr-len a) (lr-len b)))
(define (lr-pole-go r-prin sPrin k n)
  (if (> k n) #t
      (if (= (+ (* (- 0 (- k 1)) (lr-nthor sPrin (- k 2))) (lr-nthor sPrin (- k 1))) (lr-nthor r-prin (- k 1)))
          (lr-pole-go r-prin sPrin (+ k 1) n) #f)))
(define (lr-peq? a b) (lr-veq? (poly-norm a) (poly-norm b)))
(define (lr-veq? a b) (cond ((null? a) (null? b)) ((null? b) (lr-veq? a (quote ()))) (else (if (= (car a) (lr-h b)) (lr-veq? (cdr a) (lr-t b)) #f))))
(define (lr-h b) (if (null? b) 0 (car b)))
(define (lr-t b) (if (null? b) (quote ()) (cdr b)))
