; Closed-form POLYNOMIAL solutions of the first-order LINEAR ODE  y' + p(x) y = q(x)  with polynomial coefficients
; -- the variable-coefficient case between ode1's separable equations and odelin's constant-coefficient equations
; (docs/CAS.md -- summit S5, Maxima-territory ODE solving).
;
; The operator L(y) = y' + p y is linear in the coefficients of a polynomial ansatz, so a polynomial solution (if
; one exists) has degree deg q - deg p and is found by solving an exact linear system over Q.  The candidate is
; certified by differentiation; if the system is inconsistent (the genuine solution needs the integrating factor
; exp(INT p) and is not polynomial), an honest 'no-polynomial-solution is returned, never a guess.
(import "cas/odefol.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "First-order linear ODEs y' + p(x) y = q(x): exact polynomial solutions, certified by differentiation.") (newline) (newline)

(display "constant coefficient: y' + y = x  ->  y = x - 1:") (newline)
(define s1 (fol-particular (list 1) (list 0 1)))
(chk "the solution is x - 1" (equal? s1 (list -1 1)))
(chk "it satisfies y' + y - x = 0 (certificate)" (fol-certify (list 1) (list 0 1) s1))

(display "variable coefficient: y' + x y = x  ->  y = 1:") (newline)
(define s2 (fol-particular (list 0 1) (list 0 1)))
(chk "the solution is the constant 1" (equal? (fol-trim s2) 1))
(chk "it satisfies y' + x y - x = 0" (fol-certify (list 0 1) (list 0 1) s2))

(display "higher degree: y' + 2x y = 2x^3  ->  y = x^2 - 1:") (newline)
(define s3 (fol-particular (list 0 2) (list 0 0 0 2)))
(chk "the solution is x^2 - 1" (equal? s3 (list -1 0 1)))
(chk "it satisfies y' + 2x y - 2x^3 = 0" (fol-certify (list 0 2) (list 0 0 0 2) s3))

(display "another variable case: y' + x y = x^3 + 2x  ->  y = x^2:") (newline)
(define s4 (fol-particular (list 0 1) (list 0 2 0 1)))
(chk "the solution is x^2" (equal? s4 (list 0 0 1)))

(display "soundness: y' + x y = 1 has no polynomial solution (it needs exp(x^2/2)) -- reported honestly:") (newline)
(chk "no polynomial solution is returned, not a fabricated one" (equal? (car (fol-solve (list 0 1) (list 1))) (quote no-polynomial-solution)))

(newline)
(display "The first-order linear ODE with variable polynomial coefficient is now solved exactly when a polynomial") (newline)
(display "solution exists -- by an exact linear solve, certified by differentiation -- and reported honestly when") (newline)
(display "the integrating factor is non-elementary.  General non-polynomial integrating factors and nonlinear ODEs") (newline)
(display "remain the open Maxima-territory summit.") (newline)
