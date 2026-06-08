; INT (D theta2)/(theta2^2 + 1) dx with theta1 = e^x and theta2 = log(e^x + 1):
; the Rothstein-Trager residue polynomial 4 z^2 + 1 is irreducible over Q (residues +- i/2), so the
; antiderivative arctan(log(e^x + 1)) is a RootSum of two logarithms over the quadratic extension
; K1(alpha).  Because the residues are a conjugate pair the logarithmic derivative is a trace that
; descends to K1, and the integral is certified by Tr[alpha D2(v_alpha) v_albar] = A* over K1[theta2].
(import "cas/tower2alg.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define EXP1 (list 'exp (list 0 1)))
(define Dth2 (list (list (rat-zero) (rat-one)) (list (rat-one) (rat-one))))     ; e^x/(e^x + 1)
(define A (list Dth2))                                                          ; A* = D theta2
(define D (list (k1-one) (k1-zero) (k1-one)))                                  ; theta2^2 + 1
(display "INT (D theta2)/(theta2^2 + 1) dx  [theta1 = e^x, theta2 = log(e^x + 1)]") (newline)
(define q (h2alg-respoly A D Dth2 EXP1))
(must "Rothstein-Trager residue polynomial has degree two" (= (poly-deg q) 2))
(must "residues are irreducible over Q (no rational roots)" (h2alg-irreducible-quadratic? q))
(define lead (poly-lead q))
(define ac1 (k1-from-rat (/ (poly-coeff q 1) lead)))                            ; minimal polynomial of the
(define ac0 (k1-from-rat (/ (poly-coeff q 0) lead)))                            ; residue: alpha^2 + ac1 alpha + ac0
(must "RootSum certified: Tr[alpha D2(v_alpha) v_albar] = A* over K1[theta2]" (h2alg-verify A D Dth2 EXP1))
(newline) (display "all irreducible-quadratic height-two checks passed.") (newline)
