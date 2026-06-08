; -*- lisp -*-
; lib/cas/elliptic3norm.lisp -- CONSTRUCT the norm N of a third-kind logarithm from its residue data: the first
; constructive step of building g = A + B*sqrt(q) from an integrand omega = a + b*y, recovering N = A^2 - B^2 q
; from the integer residues of 2a and verifying it reproduces a (docs/CAS.md -- summit S1, turning the residue
; DECISION of elliptic3residue into a partial CONSTRUCTION).
;
; elliptic3residue established that the rational part a of omega satisfies a = (1/2) N'/N, so 2a = N'/N has an
; INTEGER residue m_i at each pole p_i equal to the order of the norm N there.  Given that residue data -- a list
; of (pole . multiplicity) pairs -- the norm is reconstructed (monic) as
;     N = prod_i (x - p_i)^{m_i},
; and the construction is VERIFIED by checking that (1/2) N'/N equals the original rational part a exactly as
; rational functions.  This recovers the norm of g from the residues; it is the first half of the constructive
; third-kind problem (the remaining half -- splitting N = A^2 - B^2 q into the actual A and B, the Jacobian/torsion
; step -- stays open).  Everything is exact over Q; the verification is an exact rational-function equality, so a
; reconstructed N either provably reproduces a or is rejected.
;
; Public (residues = list of (pole . multiplicity) with integer multiplicity; a a rational function (num den)):
;   e3n-build-norm residues    -> the monic norm N = prod (x - pole)^mult as a polynomial (low->high)
;   e3n-logder-half N          -> (1/2) N'/N as a rational function (the predicted rational part a)
;   e3n-verifies? residues a   -> #t iff (1/2) N'/N of the reconstructed N equals a exactly
;   e3n-degree-from-residues residues -> the total degree of N, sum of the multiplicities
;
; Verified: residues {(1,1),(2,2)} reconstruct N = (x-1)(x-2)^2 = x^3 - 5x^2 + 8x - 4, and (1/2) N'/N matches the
; a built from that N; residues {(0,1)} reconstruct N = x with (1/2) N'/N = 1/(2x); a wrong a is rejected.
;
; Builds on poly.lisp and ratfun.lisp.

(import "cas/poly.lisp")
(import "cas/ratfun.lisp")

; ----- reconstruct the monic norm N = prod (x - p)^m from (pole . mult) pairs -----
(define (e3n-build-norm residues) (e3n-build-go residues (list 1)))
(define (e3n-build-go residues acc) (if (null? residues) acc (e3n-build-go (cdr residues) (poly-mul acc (e3n-pow-linear (car (car residues)) (cdr (car residues)))))))
(define (e3n-pow-linear p m) (if (<= m 0) (list 1) (poly-mul (list (- 0 p) 1) (e3n-pow-linear p (- m 1)))))

; ----- (1/2) N'/N as a rational function -----
(define (e3n-logder-half N) (rat-make (poly-scale (/ 1 2) (poly-deriv N)) N))

; ----- verify the reconstructed N reproduces the given rational part a -----
(define (e3n-verifies? residues a) (rat-equal? (e3n-logder-half (e3n-build-norm residues)) a))

; ----- total degree of N from the residues -----
(define (e3n-degree-from-residues residues) (if (null? residues) 0 (+ (cdr (car residues)) (e3n-degree-from-residues (cdr residues)))))
