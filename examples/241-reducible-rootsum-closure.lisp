; 241-reducible-rootsum-closure.lisp -- completing the algebraic-residue RootSum for a REDUCIBLE
; residue polynomial, which finishes the logarithmic part of the single transcendental extension for
; a squarefree denominator.
;
; When the Rothstein-Trager residue polynomial R(z) for INT a/d factors over Q into several
; irreducible pieces of mixed degree, the logarithmic part is a sum of pieces: each linear factor
; gives an ordinary logarithm with rational residue, and each irreducible factor of degree >= 2 gives
; a RootSum over its conjugate class, handled in the number field Q(x)[c]/(P_j).  In the generic case
; each argument v_c = theta - r is linear, and the per-factor norm is the characteristic polynomial of
; r obtained from the power sums tr(r^k) by Newton's identities run backward -- no resultant needed.
; The Rothstein-Trager factors satisfy prod_j N_j = d, so the per-factor derivatives sum over the
; common denominator d, and the whole part is certified by checking the sum of the field traces equals
; a/d.  `must` raises on failure.

(import "cas/algresfull.lisp")
(define LOG (list 'log))
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'algresfull-check-failed)))

(display "Reducible algebraic-residue RootSum closure") (newline) (newline)

(display "1. mixed residues: R(z) = (z - 1)(z^2 - 1/8) -- one rational, one conjugate pair") (newline)
(display "    INT (1/x)(L^2 + L - 3) / ((L - 1)(L^2 - 2)) dx,  L = log x") (newline)
(display "    = log(L - 1) + sqrt(1/8) log((L - sqrt 2)/(L + sqrt 2))") (newline)
(define d (list (rat-from-poly (list 2)) (rat-from-poly (list -2)) (rat-from-poly (list -1)) (rat-one)))   ; L^3 - L^2 - 2L + 2
(define a (list (rat-make (list -3) (list 0 1)) (rat-make (list 1) (list 0 1)) (rat-make (list 1) (list 0 1)))) ; (1/x)(L^2+L-3)
(define r1 (int-prim-rational-full a d LOG))
(must "reported as a (reducible) RootSum"        (equal? (car r1) 'rootsum-full))
(must "certified: sum of field traces equals a/d" (int-prim-rational-full-verify a d LOG))
(newline)

(display "3. genuinely non-elementary integrals are still reported so") (newline)
(define dn (list (rat-from-poly (list 0 -1)) (rat-zero) (rat-one)))            ; L^2 - x, residues depend on x
(must "x-dependent residues remain non-elementary" (not (int-prim-rational-full-elementary? (list (rat-one)) dn LOG)))
(newline)

(display "all reducible-rootsum-closure checks passed.") (newline)
