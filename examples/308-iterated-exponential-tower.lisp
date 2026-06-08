; RUNG 5, going deeper: the ARBITRARY-DEPTH iterated exponential tower.  Define the iterated exponentials
; E_0 = x, E_1 = exp(x), E_2 = exp(exp(x)), ..., E_n = exp(E_{n-1}).  This generalizes the depth-2 nested
; exponential (nestexp.lisp) to a tower of arbitrary height n -- the first time the system handles nesting of
; unbounded depth (docs/TRAGER_ROADMAP.md, Rung 5).
;
; The key derivative law, proved by induction from E_k = exp(E_{k-1}) (so E_k' = E_{k-1}' E_k):
;     E_k' = E_k * (E_1 E_2 ... E_{k-1}).
; In particular d/dx(E_n) = E_1 E_2 ... E_n, the product of the WHOLE tower, so
;     INT (E_1 E_2 ... E_n) dx = E_n.
; A tower element is a sum of monomials c(x) E_1^{a_1} ... E_n^{a_n}; the derivation of a monomial scales by a_k
; and raises the exponents of E_1..E_{k-1} by one for each k with a_k > 0, plus the c' term.  Every result is
; certified by differentiating in the tower.
(import "cas/itexp.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The arbitrary-depth iterated exponential tower: E_k = exp(E_{k-1}), with INT(E_1...E_n) = E_n.") (newline) (newline)

(display "the derivative law E_k' = E_k (E_1 ... E_{k-1}):") (newline)
(display "  E_1' has exponent vector ") (display (ie-mvec (car (ie-Ek-deriv 3 1)))) (display "  (= E_1)") (newline)
(display "  E_2' has exponent vector ") (display (ie-mvec (car (ie-Ek-deriv 3 2)))) (display "  (= E_1 E_2)") (newline)
(display "  E_3' has exponent vector ") (display (ie-mvec (car (ie-Ek-deriv 3 3)))) (display "  (= E_1 E_2 E_3)") (newline)
(chk "E_1' = E_1, E_2' = E_1 E_2, E_3' = E_1 E_2 E_3" (if (ie-vec-eq? (ie-mvec (car (ie-Ek-deriv 3 1))) (list 1 0 0)) (if (ie-vec-eq? (ie-mvec (car (ie-Ek-deriv 3 2))) (list 1 1 0)) (ie-vec-eq? (ie-mvec (car (ie-Ek-deriv 3 3))) (list 1 1 1)) #f) #f))

(display "depth 2: INT exp(x) exp(exp x) dx = exp(exp x):") (newline)
(chk "d/dx(E_2) = E_1 E_2, so INT (E_1 E_2) = E_2" (ie-certify 2 (ie-top 2) (ie-full-product 2)))

(display "depth 3: INT exp(x) exp(exp x) exp(exp(exp x)) dx = exp(exp(exp x)):") (newline)
(display "  d/dx(E_3) = ") (display (ie-mvec (car (ie-deriv 3 (ie-top 3))))) (display "  (the full product E_1 E_2 E_3)") (newline)
(chk "INT (E_1 E_2 E_3) = E_3" (ie-certify 3 (ie-top 3) (ie-full-product 3)))

(display "depth 4 and depth 5 (the same law at greater height):") (newline)
(chk "INT (E_1 E_2 E_3 E_4) = E_4" (ie-certify 4 (ie-top 4) (ie-full-product 4)))
(chk "INT (E_1 E_2 E_3 E_4 E_5) = E_5" (ie-certify 5 (ie-top 5) (ie-full-product 5)))

(display "the full Leibniz expansion is handled too -- d/dx(E_1 E_2 E_3) has three monomials:") (newline)
(display "  ") (display (ie-deriv 3 (ie-full-product 3))) (newline)

(display "soundness -- a wrong answer is rejected by the certificate:") (newline)
(chk "d/dx(E_3) is NOT E_1 E_2, so claiming INT(E_1 E_2) = E_3 is rejected" (not (ie-certify 3 (ie-top 3) (ie-full-product 2))))

(newline)
(display "Iterated exponentials of arbitrary depth: the derivative law E_k' = E_k (E_1...E_{k-1}) gives") (newline)
(display "INT (E_1 ... E_n) dx = E_n at any height n, each instance certified by differentiating in the tower.") (newline)
