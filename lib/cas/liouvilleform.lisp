; -*- lisp -*-
; lib/cas/liouvilleform.lisp -- the LIOUVILLE STRUCTURE THEOREM made explicit for rational functions.  Liouville's
; theorem says INT f dx is elementary iff f = v' + sum_i c_i u_i'/u_i with v and the u_i in the base field and
; the c_i constants; this module returns that decomposition as an explicit WITNESS for a rational f, and
; certifies it (docs/TRAGER_ROADMAP.md, the summit).  For a rational function every such integral is elementary,
; so the structure form is the certificate of HOW.
;
; For f = N/D with D squarefree, deg N < deg D, and the (given) simple roots a_1..a_r of D, the partial-fraction
; / residue decomposition gives
;     f = sum_i res_i / (x - a_i) ,   res_i = N(a_i) / D'(a_i) ,
; which is exactly v' + sum c_i u_i'/u_i with v = 0, c_i = res_i, u_i = x - a_i (so u_i'/u_i = 1/(x - a_i)); the
; antiderivative is sum res_i log(x - a_i).  We take the roots as input (root finding for a general D is a
; separate, already-available concern) and we CERTIFY the decomposition by checking that sum res_i/(x - a_i)
; equals N/D at many sample points (an exact rational identity), so a wrong residue set is rejected.
;
; Public:
;   lf-residue N D a        -> the residue N(a)/D'(a) at a simple root a of D
;   lf-residues N D roots   -> the list of (a_i . res_i) over the given roots
;   lf-form N D roots       -> (list 'liouville-form 0 pairs): f = 0' + sum res_i (x-a_i)'/(x-a_i), the witness,
;                              pairs = ((a_1 . c_1) ...), each c_i the residue and the log argument u_i = x - a_i
;   lf-certify N D roots    -> #t iff sum res_i/(x - a_i) = N/D (checked at sample points)
;   lf-antiderivative-form N D roots -> the antiderivative as a list of (c_i . a_i): sum c_i log(x - a_i)
;
; Verified: f = 1/(x^2-1) -> (1/2) log(x-1) - (1/2) log(x+1); f = 1/(x^2+ ... ) cases; f = (2x)/(x^2-1) ->
; log(x-1) + log(x+1); and the certificate that the residue sum reproduces f.
;
; Builds on poly.lisp.

(import "cas/poly.lisp")

(define (lf-nth l k) (if (= k 0) (car l) (lf-nth (cdr l) (- k 1))))
(define (lf-len l) (if (null? l) 0 (+ 1 (lf-len (cdr l)))))

; ----- residue at a simple root a: N(a)/D'(a) -----
(define (lf-residue N D a) (/ (poly-eval N a) (poly-eval (poly-deriv D) a)))
(define (lf-residues N D roots) (if (null? roots) (quote ()) (cons (cons (car roots) (lf-residue N D (car roots))) (lf-residues N D (cdr roots)))))

; ----- the structure-theorem witness -----
(define (lf-form N D roots) (list (quote liouville-form) 0 (lf-residues N D roots)))

; ----- the antiderivative as (c_i . a_i) pairs: sum c_i log(x - a_i) -----
(define (lf-antiderivative-form N D roots) (lf-af-go (lf-residues N D roots)))
(define (lf-af-go pairs) (if (null? pairs) (quote ()) (cons (cons (cdr (car pairs)) (car (car pairs))) (lf-af-go (cdr pairs)))))

; ----- certificate: sum res_i/(x - a_i) = N/D, checked at sample points (avoiding the roots) -----
(define (lf-points) (list 2 3 5 7 11 13 17 19 23 29 31 37))
(define (lf-certify N D roots) (lf-cert-go N D (lf-residues N D roots) (lf-good-points (lf-points) roots D)))
(define (lf-cert-go N D pairs pts)
  (cond ((null? pts) #t)
        ((lf-close? (lf-sum-at pairs (car pts)) (/ (poly-eval N (car pts)) (poly-eval D (car pts)))) (lf-cert-go N D pairs (cdr pts)))
        (else #f)))
(define (lf-sum-at pairs x) (if (null? pairs) 0 (+ (/ (cdr (car pairs)) (- x (car (car pairs)))) (lf-sum-at (cdr pairs) x))))
(define (lf-close? a b) (= a b))
; keep only sample points where D is nonzero and which are not roots (exact: D(x) != 0)
(define (lf-good-points pts roots D) (cond ((null? pts) (quote ())) ((= (poly-eval D (car pts)) 0) (lf-good-points (cdr pts) roots D)) (else (cons (car pts) (lf-good-points (cdr pts) roots D)))))
