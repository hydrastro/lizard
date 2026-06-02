; 238-algebraic-rootsum-closure.lisp -- closing the algebraic-residue case for the single-extension
; proper integral.  When the Rothstein-Trager residue polynomial R(z) for INT a/d is monic over Q
; with constant but IRRATIONAL roots, the logarithmic part is a RootSum over a conjugate set of
; algebraic residues.  This closes the irreducible-quadratic case z^2 + p z + q: the argument
; v_c = gcd_theta(d, a - c Dd) is computed over Q(x)(c) with c^2 = -p c - q, and the answer is
; certified WITHOUT extracting a radical by checking the trace sum_{R(c)=0} c Dv_c/v_c = a/d (plus
; the exponential base-field correction), which is rational because the conjugate residues cancel.
; Every answer is gated by that certificate, so cases that do not certify are still reported
; 'algebraic -- the closure only tightens what towerrt and expnrt already report.  `must` raises on
; failure.

(import "cas/algres.lisp")
(define LOG (list 'log))
(define EXP (list 'exp (list 0 1)))
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'algres-check-failed)))

(display "Algebraic-residue RootSum closure (irreducible quadratic)") (newline) (newline)

(display "1. logarithmic tower, irrational residues +-sqrt(1/8)") (newline)
(display "    INT (1/x) / ((log x)^2 - 2) dx = sqrt(1/8) log((log x - sqrt 2)/(log x + sqrt 2))") (newline)
(define dl (list (rat-from-poly (list -2)) (rat-zero) (rat-one)))   ; (log x)^2 - 2
(define al (list (rat-make (list 1) (list 0 1))))                   ; 1/x
(display "    residue polynomial R(z) = z^2 - 1/8 is irreducible over Q; argument v_c = log x - sqrt 2") (newline)
(must "reported as a RootSum" (equal? (car (int-prim-rational-alg al dl LOG)) 'rootsum))
(must "certified by the trace" (int-prim-rational-alg-verify al dl LOG))
(newline)

(display "3. the closure does not disturb existing behaviour") (newline)
(define dr (list (rat-from-poly (list -1)) (rat-zero) (rat-one)))   ; (log x)^2 - 1, rational residues
(must "rational-residue case still certified" (int-prim-rational-alg-verify al dr LOG))
(define dn (list (rat-from-poly (list 0 -1)) (rat-zero) (rat-one))) ; (log x)^2 - x, residues depend on x
(must "x-dependent residues still reported non-elementary" (not (int-prim-rational-alg-elementary? (list (rat-one)) dn LOG)))
(newline)

(display "all algebraic-rootsum-closure checks passed.") (newline)
