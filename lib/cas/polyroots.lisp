; -*- lisp -*-
; lib/cas/polyroots.lisp -- naming the solutions of a solved polynomial system: take the univariate eliminant
; produced by the Groebner solver (polysolve.lisp), convert it to a dense coefficient polynomial, and count and
; isolate its real roots exactly with Sturm sequences (sturm.lisp).  This closes frontier 4(d) -- "root-naming
; for the polynomial-system solver" -- turning the structural decision (consistency, dimension, eliminant) into
; an exact count of real solutions and rational isolating intervals (docs/CAS.md).
;
; The eliminant for a variable in a zero-dimensional system is a univariate polynomial whose real roots are the
; possible real values of that coordinate.  polysolve returns it in the multivariate monomial representation
; (terms (coeff . exponent-vector) with all exponents zero except in the eliminated variable); here we project it
; to a dense low-to-high coefficient list in that one variable, then:
;   - num-real-roots gives the exact number of DISTINCT real roots (Sturm, over the Cauchy bound);
;   - isolate-refined gives rational intervals each containing exactly one real root, refined to a tolerance.
; Both are exact and certified by Sturm's theorem (sign-change counts of the canonical chain on a squarefree
; part); no floating point is used and no root is ever miscounted.  Complex (non-real) roots are not named -- the
; count is of real solutions, which is the decidable, exact quantity.
;
; Public:
;   pr-eliminant->coeffs e i nv  -> dense coeff list (low->high) of the eliminant e in variable i (nv variables)
;   pr-real-solution-count F nv i-> number of distinct REAL values the i-th coordinate takes over the variety
;                                   of the (zero-dimensional) system F; () if there is no univariate eliminant
;   pr-real-intervals F nv i eps -> rational isolating intervals (each holding one real root) for that coordinate
;   pr-solution-report F nv eps  -> per-variable list of (count . intervals), the named real-solution structure
;
; Verified: for <x^2+y^2-1, x-y> the y-eliminant 2y^2-1 has 2 real roots isolated in (-3/2,0) and (0,3/2)
; (the values +-1/sqrt2); for <x^2+1, ...> style eliminants with no real root the count is 0; a triangular linear
; system gives one real value per coordinate.
;
; Builds on polysolve.lisp (and thus groebner.lisp) and sturm.lisp.

(import "cas/polysolve.lisp")
(import "cas/sturm.lisp")

(define (pr-len l) (if (null? l) 0 (+ 1 (pr-len (cdr l)))))
(define (pr-nth l k) (if (= k 0) (car l) (pr-nth (cdr l) (- k 1))))

; ----- project a univariate eliminant (monomial terms over nv variables, nonzero only in slot i) to a dense
; low-to-high coefficient list in that single variable -----
(define (pr-eliminant->coeffs e i nv) (if (null? e) (list 0) (pr-build e i (pr-maxdeg e i 0))))
(define (pr-maxdeg e i acc) (if (null? e) acc (pr-maxdeg (cdr e) i (pr-max acc (pr-nth (cdr (car e)) i)))))
(define (pr-max a b) (if (> a b) a b))
; build coeff list of length d+1: coefficient of x_i^k is the sum of term-coeffs whose exponent in slot i is k
(define (pr-build e i d) (pr-bgo e i 0 d))
(define (pr-bgo e i k d) (if (> k d) (quote ()) (cons (pr-coeff-at e i k) (pr-bgo e i (+ k 1) d))))
(define (pr-coeff-at e i k) (if (null? e) 0 (+ (pr-term-contrib (car e) i k) (pr-coeff-at (cdr e) i k))))
(define (pr-term-contrib t i k) (if (= (pr-nth (cdr t) i) k) (car t) 0))

; ----- the real-solution count for coordinate i -----
(define (pr-real-solution-count F nv i) (pr-count-of (psys-eliminant F nv i) i nv))
(define (pr-count-of e i nv) (if (null? e) (quote ()) (num-real-roots (pr-eliminant->coeffs e i nv))))

; ----- isolating intervals for coordinate i, refined to eps -----
(define (pr-real-intervals F nv i eps) (pr-iv-of (psys-eliminant F nv i) i nv eps))
(define (pr-iv-of e i nv eps) (if (null? e) (quote ()) (isolate-refined (pr-eliminant->coeffs e i nv) eps)))

; ----- a per-variable report: list (over i = 0..nv-1) of (count . intervals) -----
(define (pr-solution-report F nv eps) (pr-rep-go F nv 0 eps))
(define (pr-rep-go F nv i eps) (if (>= i nv) (quote ()) (cons (cons (pr-real-solution-count F nv i) (pr-real-intervals F nv i eps)) (pr-rep-go F nv (+ i 1) eps))))
