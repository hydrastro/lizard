; Closed-form solutions of constant-coefficient linear ODEs via the characteristic polynomial -- the ODE
; analogue of linrec.  For a_0 y + a_1 y' + ... + a_m y^(m) = 0 the basis solutions are x^j e^{rx} for each
; rational root r (multiplicity mu, j < mu).  Each is certified by an exact polynomial identity, not by
; trusting the root: writing the k-th derivative of x^j e^{rx} as p_k(x) e^{rx} with p_0 = x^j and
; p_{k+1} = p_k' + r p_k, the ODE becomes (sum_k a_k p_k(x)) e^{rx}, and the polynomial sum is checked zero.
(import "cas/odelin.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(display "y'' - 3y' + 2y = 0  basis (j r certified): ") (display (odelin-basis (list 2 -3 1))) (display "  => e^x, e^2x") (newline)
(display "y'' - 2y' + y  = 0  basis: ") (display (odelin-basis (list 1 -2 1))) (display "  => e^x, x e^x") (newline) (newline)
(must "y'' - 3y' + 2y = 0 fully solvable, both solutions certified" (odelin-fully-solvable? (list 2 -3 1)))
(must "y'' - 2y' + y = 0 (repeated root) fully solvable" (odelin-fully-solvable? (list 1 -2 1)))
(must "y''' - 2y'' - y' + 2y = 0 (roots 1,-1,2) fully solvable" (odelin-fully-solvable? (list 2 -1 -2 1)))
(must "x^2 e^x correctly REJECTED for (r-1)^2 ODE (multiplicity 2)" (not (odelin-certify (list 1 -2 1) 2 1)))
(must "y'' + y = 0 reported NOT solvable over Q (needs complex roots)" (not (odelin-fully-solvable? (list 1 0 1))))
(newline) (display "constant-coefficient linear ODEs solved and certified by polynomial identity.") (newline)
