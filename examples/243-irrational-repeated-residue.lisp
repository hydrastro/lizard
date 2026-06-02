; 243-irrational-repeated-residue.lisp -- the final piece of the single-extension logarithmic part:
; a NON-SQUAREFREE residue polynomial whose repeated residue is IRRATIONAL.
;
; The Rothstein-Trager residue polynomial R(z) is factored over Q into distinct irreducible factors
; with multiplicities.  A factor of degree m occurring to multiplicity i is one conjugate class of
; residues whose argument v_c = gcd_theta(d, a - c Dd) has degree i over the number field
; K = Q(x)[c]/(P).  When m >= 2 the per-factor norm N = prod_sigma sigma(v_c) of that higher-degree
; argument is computed as the determinant of the m-by-m multiplication-by-v_c matrix over Q(x)[theta]
; (built by reducing v_c c^j modulo the monic P, so no division is needed), and the per-factor
; derivative is the field trace of c (Dv_c)(d/v_c).  Since the Rothstein-Trager factors multiply back
; to d, the traces sum over the common denominator d.  `must` raises on failure.

(import "cas/algresnsf2.lisp")
(define LOG (list 'log))
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'nsf2-check-failed)))

(display "Non-squarefree residue polynomial with an IRRATIONAL repeated residue") (newline) (newline)

(display "1. R(z) = (z^2 - 1/8)^2 : the conjugate pair +-sqrt(1/8), each with multiplicity two") (newline)
(display "    the argument v_c is degree two over Q(x)(sqrt(1/8)); its norm is a 2x2 determinant") (newline)
(display "    INT [-((log x)^2 + 1)/(4x)] / [(log x)^4 - (17/8)(log x)^2 + 1] dx") (newline)
(display "    = sqrt(1/8) log((log x)^2 + sqrt(1/8) log x - 1) + conjugate") (newline)
(define d4 (list (rat-from-poly (list 1)) (rat-zero) (rat-from-poly (list (/ -17 8))) (rat-zero) (rat-one)))
(define a4 (list (rat-make (list (/ -1 4)) (list 0 1)) (rat-zero) (rat-make (list (/ -1 4)) (list 0 1))))
(must "certified: sum of field traces equals a/d" (int-prim-rational-complete-verify a4 d4 LOG))
(newline)

(display "2. a non-squarefree RATIONAL residue still resolves: R(z) = (z - 1)^2") (newline)
(define d1 (list (rat-from-poly (list -2)) (rat-zero) (rat-one)))
(define a1 (list (rat-zero) (rat-make (list 2) (list 0 1))))
(must "INT (2 log x / x)/((log x)^2 - 2) = log((log x)^2 - 2) certified" (int-prim-rational-complete-verify a1 d1 LOG))
(newline)

(display "3. genuinely non-elementary integrals remain reported so") (newline)
(define dn (list (rat-from-poly (list 0 -1)) (rat-zero) (rat-one)))
(must "x-dependent residues remain non-elementary" (not (int-prim-rational-complete-elementary? (list (rat-one)) dn LOG)))
(newline)

(display "all irrational-repeated-residue checks passed.") (newline)
