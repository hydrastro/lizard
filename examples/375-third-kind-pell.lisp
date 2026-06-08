; The NONCONSTANT-B third-kind construction by the polynomial Pell / fundamental-unit structure: building
; g = A + B*sqrt(q) with B a NONCONSTANT polynomial -- the last open rung of the third-kind ladder, for the genus-0
; case (docs/CAS.md -- summit S1, the Jacobian-torsion / Pell question).
;
; elliptic3split solved the third-kind construction for constant B; the hard remaining case is nonconstant B, which
; is the polynomial Pell problem A^2 - B^2 q = constant.  For q monic of degree 2 it is solved by the fundamental
; unit u = a_0 + sqrt(q) (a_0 the polynomial part of sqrt(q)); every solution is a power u^n = A_n + B_n sqrt(q),
; computed by exact arithmetic in Z[x][sqrt q], with B_n nonconstant for n >= 2 and norm (a_0^2 - q)^n.  Each is
; certified by recomputing the norm.
(import "cas/elliptic3pell.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The last rung: nonconstant-B third-kind elements g = A + B*sqrt(q), via the polynomial Pell structure.") (newline) (newline)

(define q (list 1 0 1))   ; y^2 = x^2 + 1, a genus-0 curve

(display "the fundamental unit of y^2 = x^2 + 1 is u = x + sqrt(q), with constant norm -1:") (newline)
(must "the polynomial part a_0 of sqrt(q) is x" (equal? (e3p-a0 q) (list 0 1)))
(must "the fundamental norm a_0^2 - q is -1" (= (e3p-fundamental-norm q) -1))
(must "u^1 = (x, 1) -- the fundamental unit has constant B" (if (e3p-B-nonconstant? q 1) #f #t))

(display "u^2 = (2x^2 + 1) + (2x) sqrt(q): a genuine NONCONSTANT B, with norm (-1)^2 = 1:") (newline)
(display "  ") (display (e3p-unit-power q 2)) (newline)
(must "u^2 = (2x^2+1, 2x)" (equal? (e3p-unit-power q 2) (cons (list 1 0 2) (list 0 2))))
(must "B = 2x is nonconstant" (e3p-B-nonconstant? q 2))
(must "the Pell certificate holds: norm(u^2) = (-1)^2" (e3p-certify q 2))

(display "u^3 = (4x^3 + 3x) + (4x^2 + 1) sqrt(q): nonconstant B, norm (-1)^3 = -1:") (newline)
(display "  ") (display (e3p-unit-power q 3)) (newline)
(must "u^3 = (3x+4x^3, 1+4x^2)" (equal? (e3p-unit-power q 3) (cons (list 0 3 0 4) (list 1 0 4))))
(must "B = 4x^2 + 1 is nonconstant" (e3p-B-nonconstant? q 3))
(must "the Pell certificate holds: norm(u^3) = (-1)^3" (e3p-certify q 3))
(must "u^4 certifies as well" (e3p-certify q 4))

(display "a general monic quadratic y^2 = x^2 + 2x + 5: fundamental unit (x + 1) + sqrt(q), norm -4:") (newline)
(define q2 (list 5 2 1))
(must "a_0 is x + 1" (equal? (e3p-a0 q2) (list 1 1)))
(must "the fundamental norm is -4" (= (e3p-fundamental-norm q2) -4))
(must "u^2 certifies against (-4)^2 = 16" (e3p-certify q2 2))
(must "u^3 certifies against (-4)^3 = -64" (e3p-certify q2 3))
(must "u^2 has nonconstant B" (e3p-B-nonconstant? q2 2))

(newline)
(display "The last rung of the third-kind ladder is now built for the genus-0 case: nonconstant-B elements") (newline)
(display "g = A + B*sqrt(q) are constructed as powers of the fundamental Pell unit and certified by the norm relation.") (newline)
(display "The positive-genus polynomial Pell problem -- where sqrt(q) may have a non-periodic continued fraction and") (newline)
(display "no fundamental unit need exist -- is the remaining summit, and is left out of scope rather than forced.") (newline)
