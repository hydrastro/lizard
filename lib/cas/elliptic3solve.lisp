; -*- lisp -*-
; lib/cas/elliptic3solve.lisp -- the third-kind SEARCH: given an integrand omega on the curve y^2 = q(x),
; constructively FIND g = u(x) + sqrt(q) (u a polynomial of bounded degree) such that omega = g'/g, so that
; INT omega dx = log(g) -- the inverse of last arc's recognizer, which only certified a supplied g
; (docs/TRAGER_ROADMAP.md, frontier 1: full third-kind decision -- finding g from the integrand).
;
; Why a polynomial-u family is the right sound slice.  For g = u + sqrt(q) with u a polynomial,
;     g'/g = (u' + q'/(2 sqrt q)) / (u + sqrt q) = (u' (u - sqrt q) + (q'/2)(u - sqrt q)/sqrt q) / (u^2 - q),
; and the key simplification is that the cases of interest have u^2 - q a CONSTANT (e.g. q = x^2 + c, u = x give
; u^2 - q = -c, the arcsinh/arccosh family) or a simple factor, keeping g'/g a manageable K-element.  Rather than
; solve the (nonlinear) matching equations symbolically, we run a BOUNDED CONSTRUCTIVE SEARCH over a finite
; family of candidate u (monomials and small integer combinations up to a degree bound), compute g'/g in
; K = Q(x)[y]/(y^2-q) for each, and test equality against omega.  This is sound: every reported g is CERTIFIED by
; differentiation (e3-certify), and exhausting the family without a match yields an honest 'not-found rather than
; a false negative about elementarity in general.
;
; Public:
;   e3s-search q omega deg     -> (list 'found g) | (list 'not-found ..) : search u of degree <= deg (with small
;                                 integer coefficients) for g = u + sqrt(q) with g'/g = omega; g is certified
;   e3s-integrate q omega deg  -> (list 'elementary-log g) | (list 'unknown ..) : if found, INT omega = log(g)
;   e3s-candidates deg         -> the finite candidate list of u-polynomials searched (for inspection)
;
; Verified: for q = x^2-1, omega = d/dx log(x + sqrt(x^2-1)) is solved with g = x + sqrt(x^2-1) (FOUND, certified);
; q = x^2+1 with omega = d/dx log(x + sqrt(x^2+1)) found; a genus-1 q = x^3+1 log-derivative of (x + sqrt q) found;
; an omega with no polynomial-u solution in the family returns not-found honestly.
;
; Builds on elliptic3.lisp (e3-logderiv, e3-certify) and algfunc.lisp / tower.lisp / poly.lisp.

(import "cas/elliptic3.lisp")
(import "cas/algfunc.lisp")
(import "cas/tower.lisp")
(import "cas/poly.lisp")

(define (e3s-app a b) (if (null? a) b (cons (car a) (e3s-app (cdr a) b))))

; ----- candidate u-polynomials: all integer-coefficient polys of degree <= deg with coefficients in {-2..2}
; and a nonzero leading term, plus the constant and monomial cases.  Kept small and finite. -----
(define (e3s-candidates deg) (e3s-cand-go deg))
(define (e3s-cand-go deg) (e3s-filter-nonzero (e3s-coeffvecs (+ deg 1))))
; all coefficient vectors of length n over {-1,0,1} (kept small so the bounded search stays tractable; the
; third-kind g of interest have small integer u-coefficients).  For wider coefficients, raise this set.
(define (e3s-coeffvecs n) (if (= n 0) (list (quote ())) (e3s-prefix (list -1 0 1) (e3s-coeffvecs (- n 1)))))
(define (e3s-prefix vals rest) (if (null? vals) (quote ()) (e3s-app (e3s-map-cons (car vals) rest) (e3s-prefix (cdr vals) rest))))
(define (e3s-map-cons v rest) (if (null? rest) (quote ()) (cons (cons v (car rest)) (e3s-map-cons v (cdr rest)))))
; drop the all-zero vector (g = sqrt q alone is a degenerate case we skip)
(define (e3s-filter-nonzero vs) (cond ((null? vs) (quote ())) ((e3s-allzero? (car vs)) (e3s-filter-nonzero (cdr vs))) (else (cons (car vs) (e3s-filter-nonzero (cdr vs))))))
(define (e3s-allzero? v) (cond ((null? v) #t) ((= (car v) 0) (e3s-allzero? (cdr v))) (else #f)))

; ----- the search: for each candidate u, build g = u + sqrt q, test g'/g = omega -----
(define (e3s-search q omega deg) (e3s-loop q omega (e3s-candidates deg)))
(define (e3s-loop q omega cands)
  (cond ((null? cands) (list (quote not-found) (quote no-polynomial-u-in-family)))
        ((e3s-try q omega (car cands)) (list (quote found) (e3s-gof q (car cands))))
        (else (e3s-loop q omega (cdr cands)))))
(define (e3s-gof q u) (af-make (rat-from-poly u) (rat-one)))      ; g = u + sqrt q
(define (e3s-try q omega u) (e3-certify q (e3s-gof q u) omega))

; ----- integrate via the search -----
(define (e3s-integrate q omega deg) (e3s-result (e3s-search q omega deg)))
(define (e3s-result s) (if (equal? (car s) (quote found)) (list (quote elementary-log) (car (cdr s))) (list (quote unknown) (quote not-found-in-family))))
