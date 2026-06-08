; The field NORM, INVERSE, and rationalized LOGARITHMIC DERIVATIVE in the superelliptic field
; K = Q(x)[y]/(y^n - g), the layer that presents third-kind logarithms with ordinary polynomial denominators
; and opens the door to Rothstein-Trager residue analysis over the field (Rung 4, docs/TRAGER_ROADMAP.md).
;
; For u in K the Norm N(u) = product of the n conjugates (y -> zeta^k g^{1/n}) is a rational function of x,
; computed here as the determinant of the multiplication-by-u matrix on the basis {1, y, ..., y^{n-1}} (cofactor
; expansion over rational functions, reusing the field multiplication).  The adjugate gives the conjugate
; product ubar with u * ubar = N(u), hence the inverse u^{-1} = ubar / N(u), and this rationalizes the
; logarithmic derivative u'/u = u' * ubar / N(u) -- a field element over a scalar denominator N(u) in Q(x).
(import "cas/senorm.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The superelliptic field Norm, inverse, and rationalized logarithmic derivative, for any n.") (newline) (newline)

(define g (list 1 0 0 1))   ; g = x^3 + 1, n = 3
(define y (sf-y 3))

(display "the Norm as the determinant of the multiplication matrix:") (newline)
(display "  N(y) = ") (display (sn-norm g 3 y)) (display "  = g (product of the three conjugates zeta^k g^(1/3))") (newline)
(chk "N(y) = g = x^3 + 1" (rat-equal? (sn-norm g 3 y) (rat-from-poly g)))
(define u (list (rat-from-poly (list 0 1)) (rat-one) (rat-from-poly (list 2))))   ; x + y + 2 y^2
(display "  N(x + y + 2 y^2) = ") (display (sn-norm g 3 u)) (display "  = 8x^6 - 6x^4 + 18x^3 - 6x + 9") (newline)
(chk "N(x+y+2y^2) matches the cubic norm form a^3 + b^3 g + c^3 g^2 - 3abc g" (rat-equal? (sn-norm g 3 u) (rat-from-poly (list 9 -6 0 18 -6 0 8))))

(display "the conjugate-product ubar with u * ubar = N(u):") (newline)
(define ub (sn-ubar g 3 y))
(display "  ubar(y) = ") (display ub) (display "  = y^2, and y * y^2 = y^3 = g = N(y)") (newline)
(chk "ubar(y) = y^2 and y * ubar(y) = g" (if (sf-equal? ub (list (rat-zero) (rat-zero) (rat-one))) (sf-equal? (sf-product g 3 y ub) (list (rat-from-poly g) (rat-zero) (rat-zero))) #f))

(display "the field inverse u^{-1} = ubar / N(u):") (newline)
(define yi (sn-inverse g 3 y))
(display "  y^{-1} = y^2 / g  ->  ") (display yi) (newline)
(chk "y * y^{-1} = 1" (sf-equal? (sf-product g 3 y yi) (sf-one 3)))
(define u2 (list (rat-from-poly (list 0 1)) (rat-one) (rat-zero)))   ; x + y
(chk "(x + y) * (x + y)^{-1} = 1" (sf-equal? (sf-product g 3 u2 (sn-inverse g 3 u2)) (sf-one 3)))

(display "the rationalized logarithmic derivative u'/u = (u' * ubar) / N(u):") (newline)
(chk "for u = x + y, the rationalization satisfies u * F = N * u'" (sn-logderiv-check g 3 u2))
(chk "for u = x + y + 2 y^2 as well" (sn-logderiv-check g 3 u))
(display "  -- this presents log(x + y) etc. with a polynomial-in-x denominator, ready for residue analysis") (newline)

(display "the n = 2 specialization reproduces the classical norm a^2 - b^2 g:") (newline)
(define g2 (list 1 0 1))
(define u3 (list (rat-from-poly (list 0 1)) (rat-one)))   ; x + y on y^2 = x^2 + 1
(display "  N(x + y) on y^2 = x^2+1 = x^2 - (x^2+1) = -1  ->  ") (display (sn-norm g2 2 u3)) (newline)
(chk "n=2 Norm(x+y) = -1" (rat-equal? (sn-norm g2 2 u3) (rat-from-poly (list -1))))

(newline)
(display "The Norm, inverse, and rationalized log derivative for any n: third-kind logarithms can now be") (newline)
(display "presented over polynomial denominators -- the gateway to residue (Rothstein-Trager) analysis in K.") (newline)
