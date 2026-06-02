; Height-two primitive integrals with ALGEBRAIC residues (arctangents), by reduction to rational integration.
; For a primitive theta2 (D2 theta2 = Dtheta2 in K1), the chain rule gives, for Abar, Dbar in Q[theta2],
;     INT Dtheta2 Abar(theta2)/Dbar(theta2) dx = [ INT Abar/Dbar d(theta2) ]_{t = theta2}.
; Routed through the complete rational-function integrator integrate-rational over Q, an irreducible
; quadratic denominator factor (negative discriminant) yields a genuine arctangent -- with NO Sylvester
; resultant over K1 and none of its memory blow-up:
;     INT (Dtheta2)/(theta2^2 + 1) dx = arctan(theta2) = arctan(log(e^x + 1)),
; and a mixed INT (Dtheta2)(theta2^2 + theta2 + 1)/(theta2^3 + theta2) dx = log(theta2) + arctan(theta2).
; The reduction is gated by reconstructing each coefficient (A_i = Dtheta2 * Abar_i exactly in K1), so
; substitution is certified to apply; integrate-verify then certifies d/dtheta2 = Abar/Dbar over Q.
(import "cas/tower2primfull.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define EXP1 (list 'exp (list 0 1)))
(define Dth2 (list (list (rat-zero) (rat-one)) (list (rat-one) (rat-one))))   ; D theta2 = e^x/(e^x+1), theta2 = log(e^x+1)
; --- arctangent: irreducible-quadratic denominator ---
(define A1 (list Dth2))                                                  ; Dtheta2 * 1
(define D1 (list (k1-from-int 1) (k1-from-int 0) (k1-from-int 1)))       ; theta2^2 + 1
(display "INT (Dtheta2)/(theta2^2+1) dx = arctan(theta2) = arctan(log(e^x+1))") (newline)
(define r1 (int-h2-prim-full A1 D1 Dth2 EXP1))
(define res1 (car (cdr (cdr (cdr r1)))))
(must "reduces to rational-function integration in theta2" (equal? (car r1) 'ok))
(must "one arctangent term, no logarithms" (if (= (length (acc-arctans (cdr res1))) 1) (= (length (acc-logs (cdr res1))) 0) #f))
(must "elementary (arctangent closed form)" (int-h2-prim-full-elementary? A1 D1 Dth2 EXP1))
(must "CERTIFIED: D2(arctan theta2) = integrand A/D" (int-h2-prim-full-verify A1 D1 Dth2 EXP1))
; --- mixed logarithm + arctangent ---
(define A2 (list (k1-iscale 1 Dth2) (k1-iscale 1 Dth2) (k1-iscale 1 Dth2)))           ; Dtheta2 (theta2^2+theta2+1)
(define D2 (list (k1-from-int 0) (k1-from-int 1) (k1-from-int 0) (k1-from-int 1)))    ; theta2^3 + theta2 = theta2(theta2^2+1)
(display "INT (Dtheta2)(theta2^2+theta2+1)/(theta2^3+theta2) dx = log(theta2) + arctan(theta2)") (newline)
(define r2 (int-h2-prim-full A2 D2 Dth2 EXP1))
(define res2 (car (cdr (cdr (cdr r2)))))
(must "one logarithm and one arctangent" (if (= (length (acc-logs (cdr res2))) 1) (= (length (acc-arctans (cdr res2))) 1) #f))
(must "elementary" (int-h2-prim-full-elementary? A2 D2 Dth2 EXP1))
(must "CERTIFIED: D2(log theta2 + arctan theta2) = integrand" (int-h2-prim-full-verify A2 D2 Dth2 EXP1))
(newline) (display "height-two arctangent primitive integrals certified (pure and mixed), no resultant.") (newline)
