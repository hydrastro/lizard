; Height-two primitive integrals with rational coefficients, by reduction to rational-function integration.
; For a primitive second monomial theta2 (D2 theta2 = Dtheta2 in K1), the chain rule gives
; d/dx F(theta2) = F'(theta2) Dtheta2, so for Abar, Dbar in Q[theta2],
;     INT Dtheta2 Abar(theta2)/Dbar(theta2) dx = [ INT Abar(t)/Dbar(t) dt ]_{t = theta2}.
; The whole Rothstein-Trager logarithmic part is then the trusted rational-function integrator over Q
; (rat-integrate), with NO Sylvester resultant over K1 and none of its memory blow-up.  This integrates
;     INT (Dtheta2)(6 theta2^2 - 10 theta2 + 2)/(theta2^3 - 3 theta2^2 + 2 theta2) dx
;        = log(theta2) + 2 log(theta2 - 1) + 3 log(theta2 - 2)
; (a cubic denominator with three rational residues -- exactly the case the determinant-based resultant
; could not evaluate) and a quartic INT (Dtheta2) D*'/D* dx = log(D*).  The reduction is gated by
; reconstructing each coefficient (A_i = Dtheta2 * Abar_i exactly in K1, D_i = Dbar_i), so substitution is
; certified to apply; rat-integrate then supplies its own differentiation certificate over Q.
(import "cas/tower2primrat.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define EXP1 (list 'exp (list 0 1)))
(define Dth2 (list (list (rat-zero) (rat-one)) (list (rat-one) (rat-one))))   ; D theta2 = e^x/(e^x+1), theta2 = log(e^x+1)
; --- cubic denominator: three rational residues (the previously-blocked case) ---
(define A3 (list (k1-iscale 2 Dth2) (k1-iscale -10 Dth2) (k1-iscale 6 Dth2)))                 ; Dtheta2 (6 theta2^2 - 10 theta2 + 2)
(define D3 (list (k1-from-int 0) (k1-from-int 2) (k1-from-int -3) (k1-from-int 1)))           ; theta2^3 - 3 theta2^2 + 2 theta2
(display "INT (Dtheta2)(6 theta2^2-10 theta2+2)/(theta2^3-3 theta2^2+2 theta2) dx") (newline)
(define r3 (int-h2-prim-rat A3 D3 Dth2 EXP1))
(must "reduces to rational-function integration in theta2" (equal? (car r3) 'ok))
(must "three logarithmic terms (residues 1, 2, 3)" (= (length (car (cdr (cdr (car (cdr (cdr (cdr r3)))))))) 3))
(must "elementary: all residues rational" (int-h2-prim-rat-elementary? A3 D3 Dth2 EXP1))
(must "CERTIFIED: D2(antiderivative) = integrand A/D" (int-h2-prim-rat-verify A3 D3 Dth2 EXP1))
; --- quartic denominator: INT (Dtheta2) D*'/D* dx = log(D*) ---
(define D4 (list (k1-from-int 0) (k1-from-int -6) (k1-from-int 11) (k1-from-int -6) (k1-from-int 1)))   ; theta2(theta2-1)(theta2-2)(theta2-3)
(define A4 (list (k1-iscale -6 Dth2) (k1-iscale 22 Dth2) (k1-iscale -18 Dth2) (k1-iscale 4 Dth2)))      ; Dtheta2 * D*'
(display "INT (Dtheta2) D*'/D* dx = log(D*)   [quartic denominator]") (newline)
(must "quartic case reduces and is elementary" (int-h2-prim-rat-elementary? A4 D4 Dth2 EXP1))
(must "CERTIFIED: D2(log D*) = integrand" (int-h2-prim-rat-verify A4 D4 Dth2 EXP1))
(newline) (display "height-two primitive rational integrals certified (cubic and quartic), no resultant.") (newline)
