; RUNG 5, the LOGARITHMIC (primitive) case of mixed transcendental-over-algebraic integration:
; INT (P_1 t + P_0) dx where t = log(h) is a primitive monomial (t' = h'/h) and the coefficients P_1, P_0 are
; ALGEBRAIC functions -- field elements of K = Q(x)[y]/(y^2 - p).  Together with the exponential case
; (mixedexp.lisp) this builds out Rung 5: a transcendental monomial over an algebraic coefficient field.
;
; An element of the tower K(t) is a list of field elements (c_0 c_1 ... c_d) = c_0 + c_1 t + ... + c_d t^d.
; The derivation is d/dx (sum c_i t^i) = sum (c_i' + (i+1) c_{i+1} t') t^i (the t' couples adjacent degrees).
; Integrating a degree-1 input P_1 t + P_0 gives a degree-2 answer Q_2 t^2 + Q_1 t + Q_0 with Q_2' = 0,
; 2 Q_2 t' + Q_1' = P_1, Q_1 t' + Q_0' = P_0 -- two field-antiderivative solves; for Q of bounded degree it is
; one exact linear system, and the answer is certified by differentiating in K(t) and matching the integrand.
;
; This example works over the canonical field y^2 = x (y = sqrt x) with t = log x (so t' = 1/x).
(import "cas/mixedlog.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Mixed log-over-algebraic integration: INT (P_1 log x + P_0) dx with algebraic coefficients, over K.") (newline) (newline)

(define p (rat-from-poly (list 0 1)))        ; the field y^2 = x
(define tp (rat-make (list 1) (list 0 1)))   ; t' = 1/x, i.e. t = log x

(display "the mixed integral INT ((1/(2 sqrt x)) log x + 1/sqrt x) dx = sqrt(x) log(x):") (newline)
(define y (af-make (rat-zero) (rat-from-poly (list 1))))   ; y = sqrt x
(define Q (list (af-zero) y))   ; Q = y t  (sqrt x times log x)
(define B (ml-deriv p tp Q))
(display "  the integrand d/dx(sqrt x log x) has t-coefficients ") (display B) (newline)
(display "  (t^1 part 1/(2 sqrt x), t^0 part 1/sqrt x)") (newline)
(chk "the constructive identity INT B dx = sqrt(x) log(x) certifies in K(t)" (ml-certify p tp Q B))

(display "solving the tower system -- recover Q from the integrand B:") (newline)
(define Qsol (ml-solve-sqrt tp B 2 1))
(display "  ml-solve-sqrt recovers Q = ") (display Qsol) (display "  = sqrt(x) log(x)") (newline)
(chk "the solver recovers Q = sqrt x . log x" (if (equal? Qsol (quote none)) #f (ml-eq? Qsol Q)))
(define res (ml-integrate-sqrt tp B 2))
(chk "the top-level integrator returns the elementary answer" (equal? (car res) (quote elementary)))

(display "a rational-coefficient case, INT log x dx = x log x - x  (here d/dx(x log x) = log x + 1):") (newline)
(define Qx (list (af-zero) (af-from-rat (rat-from-poly (list 0 1)))))   ; x t
(define Bx (ml-deriv p tp Qx))
(display "  d/dx(x log x) has t-coefficients ") (display Bx) (display "  (t^1 part 1, t^0 part 1)") (newline)
(chk "the solver recovers Q = x log x from log x + 1" (if (equal? (ml-solve-sqrt tp Bx 2 1) (quote none)) #f (ml-eq? (ml-solve-sqrt tp Bx 2 1) Qx)))

(display "the genuine t^2 case, INT (log x)/x dx = (1/2)(log x)^2:") (newline)
(define Qt2 (list (af-zero) (af-zero) (af-from-rat (rat-make (list 1) (list 2)))))   ; (1/2) t^2
(define Bt2 (ml-deriv p tp Qt2))
(display "  d/dx((1/2)(log x)^2) = (1/x) log x  ->  ") (display Bt2) (newline)
(chk "the solver recovers Q = (1/2)(log x)^2 (answer one t-degree higher)" (if (equal? (ml-solve-sqrt tp Bt2 2 0) (quote none)) #f (ml-eq? (ml-solve-sqrt tp Bt2 2 0) Qt2)))

(newline)
(display "Rung 5 logarithmic case: a primitive monomial log(h) integrated over an algebraic coefficient field,") (newline)
(display "solving the tower system Q_2 t^2 + Q_1 t + Q_0 inside K = Q(x)[sqrt x], every answer differentiate-certified.") (newline)
