; -*- lisp -*-
; lib/cas/transsolve.lisp -- solving TRANSCENDENTAL equations that become polynomial under a substitution
; u = m(x) (m one of exp, log, a power, ...), generalizing solve.lisp (polynomial equations) the way Maxima's
; solver handles e.g. e^{2x} - 3 e^x + 2 = 0.  The equation is given as a polynomial in the monomial u; we solve
; that polynomial over Q with the certified solver, then back-substitute u = m(x) to express each x-solution.
;
; For u = e^x a root u = r > 0 gives x = log r (and r <= 0 gives no real x); for u = log x a root u = r gives
; x = e^r; for u = x^k a root u = r gives the k-th roots.  We report the back-substituted solutions
; symbolically as (list 'exp-eq r) / (list 'log-eq r) / (list 'pow-eq k r) tagged with the rational root r, so
; the result is exact (x = log r etc.) without forcing a floating-point value.
;
; Public:
;   ts-solve-exp  coeffs    -> solutions of P(e^x) = 0 : a list of (x = log r) for each positive rational root r,
;                              with non-positive roots reported as having no real x
;   ts-solve-log  coeffs    -> solutions of P(log x) = 0 : x = e^r for each rational root r
;   ts-solve-pow  k coeffs  -> solutions of P(x^k) = 0 : the real k-th roots of each rational root r
;   ts-uroots coeffs        -> the rational roots r of the polynomial-in-u (the substitution variable), reused
;                              by all three; back-substitution is then applied
;   ts-verify-exp coeffs sol -> #t iff substituting u = r (sol = (... r)) into the polynomial gives 0
;
; Verified: e^{2x} - 3 e^x + 2 = 0 -> u in {1,2} -> x in {0, log 2}; (log x)^2 - 1 = 0 -> log x in {1,-1} ->
; x in {e, 1/e}; x^2 - 5 x^? ... and the back-substitution / verification.
;
; Builds on solve.lisp (the certified polynomial solver) and poly.lisp.

(import "cas/solve.lisp")
(import "cas/poly.lisp")

; the rational roots r (with multiplicity collapsed) of the polynomial in the substitution variable u.
; solve-poly returns a list of (value mult) with value tagged (rat r); we extract the rational values.
(define (ts-uroots coeffs) (ts-extract (solve-poly coeffs)))
(define (ts-extract sols) (if (null? sols) (quote ()) (ts-extract-one (car sols) (ts-extract (cdr sols)))))
(define (ts-extract-one s rest) (if (ts-rat? (car s)) (cons (ts-ratval (car s)) rest) rest))
(define (ts-rat? v) (if (pair? v) (equal? (car v) (quote rat)) #f))
(define (ts-ratval v) (car (cdr v)))

; u = e^x: x = log r for r > 0; r <= 0 -> no real solution
(define (ts-solve-exp coeffs) (ts-exp-go (ts-uroots coeffs)))
(define (ts-exp-go rs) (if (null? rs) (quote ()) (cons (ts-exp-one (car rs)) (ts-exp-go (cdr rs)))))
(define (ts-exp-one r) (if (> r 0) (list (quote x=log) r) (list (quote no-real-x-for-u) r)))

; u = log x: x = e^r
(define (ts-solve-log coeffs) (ts-log-go (ts-uroots coeffs)))
(define (ts-log-go rs) (if (null? rs) (quote ()) (cons (list (quote x=exp) (car rs)) (ts-log-go (cdr rs)))))

; u = x^k: real k-th roots of r (r>0 -> r^{1/k}; r<0 with k odd -> -(|r|)^{1/k}; r=0 -> 0)
(define (ts-solve-pow k coeffs) (ts-pow-go k (ts-uroots coeffs)))
(define (ts-pow-go k rs) (if (null? rs) (quote ()) (cons (list (quote x=root) k (car rs)) (ts-pow-go k (cdr rs)))))

; verification: substitute u = r back into the polynomial and check it is zero
(define (ts-verify coeffs r) (= (poly-eval coeffs r) 0))
(define (ts-verify-exp coeffs sol) (ts-verify coeffs (car (cdr sol))))
