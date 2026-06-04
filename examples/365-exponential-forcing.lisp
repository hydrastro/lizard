; PARTICULAR SOLUTIONS of y'' + a y' + b y = q(x) e^{r x}, including the RESONANT case where r is a characteristic
; root (docs/CAS.md -- summit S5, non-polynomial forcing beyond polynomial right-hand sides).
;
; Substituting y = u(x) e^{r x} turns the equation into u'' + (2r+a) u' + (r^2+ar+b) u = q(x), a constant-
; coefficient polynomial ODE solved exactly by odelin2.  The constant term r^2+ar+b is the characteristic value at
; r; when r is a root (resonance) it vanishes and the ansatz degree rises automatically, so resonance -- including
; a double root -- needs no special handling.  Every solution is certified by symbolic differentiation.
(import "cas/odeexp.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Second-order ODEs with exponential forcing y'' + a y' + b y = q(x) e^{r x}, resonance included.") (newline) (newline)

(display "non-resonant: y'' + y = e^x (r = 1 is not a root of r^2 + 1), so y = (1/2) e^x:") (newline)
(chk "r = 1 is not resonant here" (if (oef-resonant? 0 1 1) #f #t))
(chk "the coefficient polynomial u is 1/2" (equal? (oef-u 0 1 1 (list 1)) (list (/ 1 2))))
(chk "y = (1/2) e^x is certified" (oef-certify 0 1 1 (list 1) (oef-u 0 1 1 (list 1))))

(display "resonant: y'' - y = e^x (r = 1 IS a root of r^2 - 1), so the solution gains a factor of x: y = (x/2) e^x:") (newline)
(chk "r = 1 is resonant" (oef-resonant? 0 -1 1))
(chk "the coefficient polynomial u is x/2" (equal? (oef-u 0 -1 1 (list 1)) (list 0 (/ 1 2))))
(chk "y = (x/2) e^x is certified" (oef-certify 0 -1 1 (list 1) (oef-u 0 -1 1 (list 1))))

(display "double-root resonance: y'' - 2y' + y = e^x (r = 1 a DOUBLE root), so y = (x^2/2) e^x:") (newline)
(chk "r = 1 is resonant for the double root" (oef-resonant? -2 1 1))
(chk "the coefficient polynomial u is x^2/2" (equal? (oef-u -2 1 1 (list 1)) (list 0 0 (/ 1 2))))
(chk "y = (x^2/2) e^x is certified" (oef-certify -2 1 1 (list 1) (oef-u -2 1 1 (list 1))))

(display "polynomial times exponential, resonant: y'' - y = x e^x is solved and certified:") (newline)
(chk "the x e^x forcing is handled and certified" (oef-certify 0 -1 1 (list 0 1) (oef-u 0 -1 1 (list 0 1))))

(display "a shifted exponent: y'' - 3y' + 2y = e^{3x} (r = 3), so y = (1/2) e^{3x}:") (newline)
(chk "the e^{3x} case is certified" (oef-certify -3 2 3 (list 1) (oef-u -3 2 3 (list 1))))

(newline)
(display "Exponential forcing is now solved exactly, with resonance -- including a double characteristic root --") (newline)
(display "handled automatically by the reduction to a polynomial equation in the coefficient, certified by") (newline)
(display "differentiation.  Trigonometric forcing and genuinely nonlinear ODEs remain the open territory.") (newline)
