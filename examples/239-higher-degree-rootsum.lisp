; 239-higher-degree-rootsum.lisp -- the algebraic-residue RootSum closure for an IRREDUCIBLE residue
; polynomial of ANY degree (generalizing the quadratic case of example 238 to the
; Lazard-Rioboo-Trager-style general situation).
;
; When the Rothstein-Trager residue polynomial R(z) for INT a/d is monic over Q and irreducible of
; degree m, the logarithmic part is a RootSum over its m conjugate roots: sum_{R(c)=0} c log(v_c).
; The argument v_c = gcd_theta(d, a - c Dd) is computed over the number field K = Q(x)[c]/(R(c)) --
; whose elements are simply polynomials in c over Q(x), so ordinary polynomial arithmetic reduced
; mod R suffices.  The answer is certified WITHOUT extracting any radical: since the RT factors
; satisfy prod_{R(c)=0} v_c = d, the derivative of the RootSum is (trace of c Dv_c (d/v_c)) / d, and
; the field trace down to Q(x) is taken via the power sums of the roots from Newton's identities, so
; no conjugate is ever named.  `must` raises on failure.

(import "cas/algresn.lisp")
(define LOG (list 'log))
(define EXP (list 'exp (list 0 1)))
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'algresn-check-failed)))

(display "Higher-degree algebraic-residue RootSum closure") (newline) (newline)

(display "1. CUBIC residues: R(z) = z^3 - 2 is irreducible over Q") (newline)
(display "    INT (6/x) / ((log x)^3 - 2) dx = sum_{c^3 = 2} c log(log x - c)") (newline)
(define dc (list (rat-from-poly (list -2)) (rat-zero) (rat-zero) (rat-one)))   ; (log x)^3 - 2
(define ac (list (rat-make (list 6) (list 0 1))))                              ; 6/x
(must "reported as a RootSum"        (equal? (car (int-prim-rational-algn ac dc LOG)) 'rootsum))
(must "certified by the field trace" (int-prim-rational-algn-verify ac dc LOG))
(newline)

(display "3. the general path subsumes the quadratic case and preserves existing behaviour") (newline)
(define dq (list (rat-from-poly (list -2)) (rat-zero) (rat-one)))              ; (log x)^2 - 2
(define aq (list (rat-make (list 1) (list 0 1))))
(must "quadratic INT (1/x)/((log x)^2 - 2) still certified" (int-prim-rational-algn-verify aq dq LOG))
(define dn (list (rat-from-poly (list 0 -1)) (rat-zero) (rat-one)))            ; (log x)^2 - x
(must "x-dependent residues still reported non-elementary" (not (int-prim-rational-algn-elementary? (list (rat-one)) dn LOG)))
(newline)

(display "all higher-degree-rootsum checks passed.") (newline)
