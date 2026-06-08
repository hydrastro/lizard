; -*- lisp -*-
; lib/cas/vanhoeij.lisp -- VAN HOEIJ CORRECTION TERMS for the integral basis of a GENERAL plane curve F(x,y)=0.
; This closes the last Rung-4 gap (docs/TRAGER_ROADMAP.md): for a superelliptic curve y^n = g the integral
; basis has the pure-power form {y^j / d_j} (handled by intbasis.lisp), but for a general curve a basis element
; is w_j = (y^j + sum_{i<j} c_{j,i}(x) y^i) / d_j(x) -- a LOWER-degree-in-y numerator.  The c_{j,i} are the
; "correction terms" that cancel the poles the naive y^j/d_j would have.
;
; The construction at a rational place x = a (Puiseux ramification q = 1, the common case): a single branch is a
; genuine power series y(x) = sum coeffs[i] (x-a)^{p+i}.  The element (y - c(x))/(x-a)^k is integral there iff
; y - c vanishes to order >= k, so the correction is c(x) = the part of the branch BELOW order k -- "subtract
; the singular part".  Subtracting it raises the valuation from < k up to >= k, exactly cancelling the pole.
; The same idea lifts a level-j element: subtract the lower-order behaviour read from the branch.
;
; Integrality is certified with the general-F Puiseux valuation oracle of intbasis.lisp (ib-integral-at0?,
; which already accepts an arbitrary numerator N(x,y) and the branches from pg-branches).  For a ramified place
; (q > 1) or several branches needing a combined correction, the simple single-branch construction does not
; suffice and this module reports 'needs-place-combination rather than guessing -- preserving soundness.
;
; Public:
;   vh-branches F M             -> the Puiseux branches of F at x = 0 (via pg-branches)
;   vh-integral-at0? N k F M    -> #t iff N(x,y)/x^k is integral at x=0 (reuses the general-F oracle)
;   vh-correction br k          -> c(x): the part of a rational branch below order k (the singular part), or
;                                  'needs-place-combination if the branch is ramified (q>1)
;   vh-correct-level1 F k M     -> (list 'corrected c d-power) giving w = (y - c)/x^k integral, with a witness
;                                  that y/x^k was NOT integral; or 'no-correction-needed / 'needs-... 
;   vh-certify-correction F N k M -> #t iff N/x^k is integral AND (N without its constant-in-y part)/x^k is not
;                                  (i.e. the correction genuinely matters)
;
; Verified: on y = x + x^2 + x^3, (y - x)/x^2 and (y - x - x^2)/x^3 are integral while y/x^2, y/x^3 are not; on
; y^2 - 2xy - x^3, the integrality oracle correctly classifies y/x (integral) vs y/x^2 (pole).
;
; Builds on intbasis.lisp (the valuation oracle) and puiseuxg.lisp (general-F Puiseux), over poly.lisp.

(import "cas/intbasis.lisp")
(import "cas/puiseuxg.lisp")

(define (vh-nth l k) (if (= k 0) (car l) (vh-nth (cdr l) (- k 1))))

; the branches of F at x = 0
(define (vh-branches F M) (pg-branches F M))

; integrality of N(x,y)/x^k at x=0 -- reuse the general-F oracle (works for any numerator and any F)
(define (vh-integral-at0? N k F M) (ib-integral-at0? N k (vh-branches F M) M))

; ----- the correction: the part of a rational branch below order k -----
; branch = (puiseux q p coeffs), y = sum_i coeffs[i] x^{p+i}.  For q = 1 this is an ordinary power series and
; the correction c(x) = sum_{p+i < k} coeffs[i] x^{p+i} (a polynomial).  For q > 1 the branch is ramified and a
; single-branch polynomial correction does not exist -- report 'needs-place-combination.
(define (vh-correction br k)
  (if (not (equal? (car br) (quote puiseux))) (quote needs-place-combination)
      (vh-corr-go br k)))
(define (vh-corr-go br k)
  (let ((q (vh-nth br 1)) (p (vh-nth br 2)) (coeffs (vh-nth br 3)))
    (if (not (= q 1)) (quote needs-place-combination)
        (vh-build-poly p coeffs k))))
; build the polynomial sum_{p+i < k} coeffs[i] x^{p+i}; coefficient of x^e is coeffs[e-p] for p <= e < k
(define (vh-build-poly p coeffs k) (vh-bp-go p coeffs k 0))
(define (vh-bp-go p coeffs k e)                                ; e = current exponent, from 0 upward
  (if (>= e k) (quote ())
      (cons (vh-coeff-at p coeffs e) (vh-bp-go p coeffs k (+ e 1)))))
(define (vh-coeff-at p coeffs e) (if (< e p) 0 (vh-idx coeffs (- e p))))
(define (vh-idx l i) (if (< i (vh-len l)) (vh-nth l i) 0))
(define (vh-len l) (if (null? l) 0 (+ 1 (vh-len (cdr l)))))

; ----- level-1 correction: make (y - c)/x^k integral at x=0 for a single-branch (rational) place -----
; returns (list 'corrected c k) where c makes w = (y - c)/x^k integral, provided the naive y/x^k was not and the
; place is a single rational branch; otherwise an honest verdict.
(define (vh-correct-level1 F k M)
  (vh-cl1-dispatch F k M (vh-branches F M)))
(define (vh-cl1-dispatch F k M branches)
  (cond ((null? branches) (quote no-branches))
        ((not (= (vh-len branches) 1)) (quote needs-place-combination))   ; multiple branches: combined correction
        ((not (equal? (car (car branches)) (quote puiseux))) (quote needs-place-combination))
        ((not (= (vh-nth (car branches) 1) 1)) (quote needs-place-combination))  ; ramified
        (else (vh-cl1-build F k M (car branches)))))
(define (vh-cl1-build F k M br)
  (vh-cl1-result F k M (vh-correction br k)))
(define (vh-cl1-result F k M c)
  (vh-cl1-final F k M c (vh-neg c)))
; numerator N = y - c  has poly-in-y coefficients (N0 N1) = (-c, 1)
(define (vh-cl1-final F k M c negc)
  (if (vh-integral-at0? (list negc (list 1)) k F M)
      (vh-cl1-witness F k M c negc)
      (quote correction-insufficient)))
(define (vh-cl1-witness F k M c negc)
  (if (vh-integral-at0? (list (list 0) (list 1)) k F M)
      (quote no-correction-needed)                             ; y/x^k already integral
      (list (quote corrected) c k)))                           ; genuine correction
(define (vh-neg p) (if (null? p) (quote ()) (cons (- 0 (car p)) (vh-neg (cdr p)))))

; ----- certificate: N/x^k is integral AND dropping the constant-in-y (correction) part breaks integrality -----
; N is a poly-in-y (N0 N1 ...); "dropping the correction" zeroes the y^0 coefficient.
(define (vh-certify-correction F N k M)
  (if (vh-integral-at0? N k F M)
      (not (vh-integral-at0? (cons (list 0) (cdr N)) k F M))
      #f))
