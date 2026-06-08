; The SUPERELLIPTIC FUNCTION FIELD K = Q(x)[y]/(y^n - g(x)) for arbitrary n, with its derivation and a certified
; logarithm constructor -- the algebraic foundation generalizing algfunc.lisp (fixed at n = 2) to any degree,
; on which the rest of the Rung-4 superelliptic integration is built (docs/TRAGER_ROADMAP.md).
;
; An element is a length-n list of rational functions (a_0 ... a_{n-1}) = a_0 + a_1 y + ... + a_{n-1} y^{n-1}.
; The relation y^n = g reduces products via y^{i+j} = g^{floor((i+j)/n)} y^{(i+j) mod n}.  The derivation uses
; y' = g' y/(n g), giving d/dx (sum a_j y^j) = sum [a_j' + a_j (j/n) g'/g] y^j (which stays within the field).
;
; For a field element u, d/dx log u = u'/u; rather than invert u, the identity INT f dx = c log u is certified
; by clearing the denominator -- f * u = c * u' as a field identity (needing only multiply and derive, both
; exact).  This constructs and verifies genuinely algebraic logarithms for any n.
(import "cas/sefield.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The superelliptic field Q(x)[y]/(y^n - g): arithmetic, derivation, and certified logarithms, any n.") (newline) (newline)

(define g (list 1 0 0 1))   ; g = x^3 + 1, n = 3

(display "the defining relation y^n = g, computed by repeated multiplication:") (newline)
(define y (sf-y 3))
(define y3 (sf-product g 3 (sf-product g 3 y y) y))
(display "  y^3 in Q(x)[y]/(y^3 - g) = ") (display y3) (display "  (sector 0 is g, the others zero)") (newline)
(chk "y^3 = g = x^3 + 1, and y^1, y^2 parts vanish" (if (rat-equal? (sf-nth y3 0) (rat-from-poly g)) (if (rat-zero? (sf-nth y3 1)) (rat-zero? (sf-nth y3 2)) #f) #f))

(display "the derivation y' = g' y/(n g):") (newline)
(define dy (sf-deriv g 3 y))
(display "  d/dx y = (x^2/(x^3+1)) y  ->  ") (display dy) (newline)
(chk "y' = (x^2/(x^3+1)) y in the y^1 sector" (rat-equal? (sf-nth dy 1) (rat-make (list 0 0 1) (list 1 0 0 1))))

(display "differentiating a genuine field element u = x + y:") (newline)
(define u (sf-set (sf-set (sf-zeros 3) 0 (rat-from-poly (list 0 1))) 1 (rat-one)))
(define du (sf-deriv g 3 u))
(display "  u' = d/dx(x + y) = 1 + (x^2/(x^3+1)) y  ->  ") (display du) (newline)
(chk "u' = 1 + (x^2/(x^3+1)) y" (if (rat-equal? (sf-nth du 0) (rat-one)) (rat-equal? (sf-nth du 1) (rat-make (list 0 0 1) (list 1 0 0 1))) #f))

(display "a certified logarithm, INT g'/g dx = log g (cleared identity f * g = (g)'):") (newline)
(define f_gg (sf-set (sf-zeros 3) 0 (rat-make (poly-deriv g) g)))   ; f = g'/g
(define ug (sf-set (sf-zeros 3) 0 (rat-from-poly g)))               ; u = g as a field element
(chk "INT (g'/g) dx = log g, certified by f * g = d/dx g" (sf-log-certify g 3 f_gg ug (rat-one)))
(display "  the superelliptic log y = (1/n) log g lives here as well (y = g^(1/n))") (newline)
(define f_y (sf-set (sf-zeros 3) 0 (rat-make (list 0 0 1) (list 1 0 0 1))))   ; x^2/(x^3+1)
(chk "INT (x^2/(x^3+1)) ... = log y, certified by f * y = y'" (sf-log-certify g 3 f_y y (rat-one)))

(display "the n = 2 specialization reproduces algfunc's derivation (y^2 = x^2 + 1):") (newline)
(define g2 (list 1 0 1))
(define dy2 (sf-deriv g2 2 (sf-y 2)))
(display "  d/dx y = (x/(x^2+1)) y  ->  ") (display dy2) (newline)
(chk "n=2: y' = (x/(x^2+1)) y, matching algfunc" (rat-equal? (sf-nth dy2 1) (rat-make (list 0 1) (list 1 0 1))))

(newline)
(display "The superelliptic field for any n: y^n = g reduction, the closed-form derivation, and certified") (newline)
(display "logarithms via a cleared-denominator identity -- the foundation generalizing algfunc beyond n = 2.") (newline)
