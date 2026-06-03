; Height-two RootSum with an irreducible-CUBIC algebraic residue -- the degree-three generalisation of
; tower2alg's quadratic arctangent case.  The integral
;     INT 6 (D theta2) / (theta2^3 - 2) dx = sum_{r^3 = 2} r log(theta2 - r),   theta2 = log(e^x + 1),
; has residues equal to the three cube roots of 2: a full conjugate set of algebraic numbers of degree
; three, with residue polynomial q(z) = z^3 - 2 irreducible over Q.  The residue polynomial is recovered
; by the fraction-free ratio trick of tower2ff (the ratios q(z_k)/q(z_0) are rational because q in Q[z] is
; evaluated at integers).  The antiderivative lives in K1(alpha)[theta2] with alpha a root of q, but we
; never split the algebraic closure: the RootSum's logarithmic derivative is the TRACE over the conjugates
; and descends to K1[theta2].  K1(alpha) = K1[alpha]/(q) is realised as h2polys over K1 reduced modulo q;
; the log argument v_alpha = gcd(D*, A* - alpha D2(D*)) is taken by Euclid over K1(alpha)[theta2]; and the
; certificate checks
;     Tr_{K1(alpha)/K1} ( alpha D2(v_alpha) (D*/v_alpha) )  =  A*    exactly in K1[theta2],
; with the trace computed from the power sums of q's roots (Newton's identities).  Computed once.
(import "cas/tower2algn.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define MEXP (list 'exp (list 0 1)))
(define Dth2 (list (list (rat-zero) (rat-one)) (list (rat-one) (rat-one))))   ; D theta2 = e^x/(e^x+1)
(define Ds (list (k1-from-int -2) (k1-zero) (k1-zero) (k1-one)))              ; D* = theta2^3 - 2
(define As (list (k1-iscale 6 Dth2)))                                         ; A* = 6 D theta2
(display "INT 6 (D theta2)/(theta2^3 - 2) dx = sum_{r^3=2} r log(theta2 - r)") (newline) (newline)
(define q (h2algn-respoly As Ds Dth2 MEXP))                                   ; residue polynomial, computed ONCE
(must "residue polynomial recovered as z^3 - 2"     (equal? q (list -2 0 0 1)))
(must "irreducible over Q (degree-3 algebraic residues)" (h2algn-irreducible? q))
(must "CERTIFIED: Tr[alpha D2(v) (D*/v)] = A* in K1[theta2]"
      (h2-equal? (h2-norm (h2algn-num As Ds Dth2 MEXP q)) (h2-norm As)))
(newline) (display "cubic (degree-3) algebraic-residue RootSum certified at height two via the conjugate trace.") (newline)
