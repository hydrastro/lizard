; 242-nonsquarefree-rootsum.lisp -- the non-squarefree residue-polynomial case (rational residues).
;
; The Rothstein-Trager residue polynomial R(z) for INT a/d (d squarefree in theta) is squarefree for
; a generic integrand, but it acquires repeated factors when several roots of d share one residue.
; Writing the squarefree factorization R = prod_i R_i^i, a residue that is a root of R_i comes with an
; argument v_c = gcd_theta(d, a - c Dd) of degree i in theta rather than a linear one.  This example
; covers the common branch where all residues are rational: each rational residue c0 contributes
; c0 log(v_c) with a higher-degree v_c formed entirely in Q(x)[theta], and the part is certified by
; checking that the sum of c0 (Dv_c)/v_c over the residues equals a/d (with the exponential
; base-field correction).  Integrals are supplied in proper form.  `must` raises on failure.

(import "cas/algresnsf.lisp")
(define LOG (list 'log))
(define EXP (list 'exp (list 0 1)))
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'nsf-check-failed)))

(display "Non-squarefree residue polynomial (rational residues)") (newline) (newline)

(display "1. residue 1 with multiplicity two: R(z) = (z - 1)^2, argument of degree two") (newline)
(display "    INT (2 log x / x) / ((log x)^2 - 2) dx = log((log x)^2 - 2)") (newline)
(define d1 (list (rat-from-poly (list -2)) (rat-zero) (rat-one)))    ; (log x)^2 - 2
(define a1 (list (rat-zero) (rat-make (list 2) (list 0 1))))         ; (2/x) log x
(must "reported as a non-squarefree RootSum" (equal? (car (int-prim-rational-nsf a1 d1 LOG)) 'rootsum-nsf))
(must "certified"                            (int-prim-rational-nsf-verify a1 d1 LOG))
(newline)

(display "3. the exponential tower, with base-field correction") (newline)
(display "    INT (-e^x + 2) / (e^(2x) + e^x - 1) dx = log(e^(2x) + e^x - 1) - 2x") (newline)
(define de (list (rat-from-poly (list -1)) (rat-from-poly (list 1)) (rat-one)))    ; e^(2x)+e^x-1
(define ae (list (rat-from-poly (list 2)) (rat-from-poly (list -1))))              ; -e^x+2
(must "reported as a non-squarefree RootSum" (equal? (car (int-prim-rational-nsf ae de EXP)) 'rootsum-nsf))
(must "certified"                            (int-prim-rational-nsf-verify ae de EXP))
(newline)

(display "4. squarefree residue polynomials still resolve, and non-elementary stays so") (newline)
(define dc (list (rat-from-poly (list -2)) (rat-zero) (rat-zero) (rat-one)))       ; z^3 - 2 irreducible
(define ac (list (rat-make (list 6) (list 0 1))))
(must "squarefree irreducible cubic still certified" (int-prim-rational-nsf-verify ac dc LOG))
(define dn (list (rat-from-poly (list 0 -1)) (rat-zero) (rat-one)))                ; (log x)^2 - x
(must "x-dependent residues remain non-elementary" (not (int-prim-rational-nsf-elementary? (list (rat-one)) dn LOG)))
(newline)

(display "all nonsquarefree-rootsum checks passed.") (newline)
