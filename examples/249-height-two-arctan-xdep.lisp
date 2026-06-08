; A height-two irreducible-quadratic residue with an x-DEPENDENT argument.  Differentiating the RootSum
; (i/2) log(theta2 - i theta1) + (-i/2) log(theta2 + i theta1) with theta1 = e^x and theta2 = log(e^x+1)
; gives INT (theta1 theta2 - theta1 D theta2)/(theta2^2 + theta1^2) dx.  The residues are still +- i/2
; (residue polynomial 4 z^2 + 1, irreducible over Q), but the gcd argument rho = i theta1 now depends on
; x, so D2(rho) is nonzero -- unlike the arctan case where rho = 1/(2 alpha) was constant.  The same
; trace certificate Tr[alpha D2(v_alpha) v_albar] = A* over K1[theta2] carries through, because the
; height-two derivation t2a-deriv differentiates the K1(alpha) coefficients of v_alpha coefficientwise.
(import "cas/tower2alg.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define EXP1 (list 'exp (list 0 1)))
(define t1   (list (list (rat-zero) (rat-one)) (list (rat-one))))               ; theta1 = e^x
(define Dth2 (list (list (rat-zero) (rat-one)) (list (rat-one) (rat-one))))     ; D theta2 = e^x/(e^x+1)
(define A (list (k1-neg (k1-mul t1 Dth2)) t1))                                  ; A* = theta1 theta2 - theta1 D theta2
(define D (list (k1-mul t1 t1) (k1-zero) (k1-one)))                            ; D* = theta2^2 + theta1^2
(display "INT (theta1 theta2 - theta1 D theta2)/(theta2^2 + theta1^2) dx  [rho = i theta1 depends on x]") (newline)
(define q (h2alg-respoly A D Dth2 EXP1))
(must "Rothstein-Trager residue polynomial has degree two" (= (poly-deg q) 2))
(must "residues are irreducible over Q (residues +- i/2)" (h2alg-irreducible-quadratic? q))
(define lead (poly-lead q))
(define ac1 (k1-from-rat (/ (poly-coeff q 1) lead)))
(define ac0 (k1-from-rat (/ (poly-coeff q 0) lead)))
(must "RootSum certified with x-dependent argument: Tr[alpha D2(v_alpha) v_albar] = A*" (h2alg-verify A D Dth2 EXP1))
(newline) (display "all x-dependent irreducible-quadratic height-two checks passed.") (newline)
