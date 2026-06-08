; Closed-form solutions of the EULER (equidimensional / Cauchy-Euler) ODE x^2 y'' + a x y' + b y = 0 -- the
; variable-coefficient companion to the constant-coefficient solver, a new ODE class (docs/CAS.md -- summit S5).
;
; The substitution y = x^r gives the indicial polynomial r^2 + (a-1) r + b, and the discriminant D = (a-1)^2 - 4b
; decides the regime: D > 0 two real exponents (y = c1 x^r1 + c2 x^r2), D = 0 a repeated exponent with x^r log x,
; D < 0 a complex pair giving x^alpha cos/sin(beta log x).  Integer exponents are certified by direct
; substitution; irrational exponents are named exactly as algebraic numbers.
(import "cas/odeuler.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Euler equidimensional equations x^2 y'' + a x y' + b y = 0: exponents from the indicial polynomial.") (newline) (newline)

(display "x^2 y'' + x y' - y = 0: indicial r^2 - 1, exponents 1 and -1, so y = x and y = 1/x:") (newline)
(chk "the indicial polynomial is r^2 - 1" (equal? (oe-indicial 1 -1) (list -1 0 1)))
(chk "the regime is two distinct real exponents" (equal? (oe-case 1 -1) (quote two-real)))
(chk "the exponents are 1 and -1" (equal? (oe-exponents 1 -1) (list 1 -1)))
(chk "exponent 1 is certified: x^2 y'' + x y' - y vanishes for y = x" (oe-certify-int 1 -1 1))
(chk "exponent -1 is certified: it vanishes for y = 1/x" (oe-certify-int 1 -1 -1))
(chk "a non-root exponent (r = 2) is correctly rejected" (if (oe-certify-int 1 -1 2) #f #t))

(display "x^2 y'' + 3x y' + y = 0: discriminant zero, a repeated exponent -1 with a logarithmic second solution:") (newline)
(chk "the discriminant is zero" (= (oe-discriminant 3 1) 0))
(chk "the regime is a repeated exponent" (equal? (oe-case 3 1) (quote repeated)))
(chk "the repeated exponent is -1" (= (oe-repeated-exponent 3 1) -1))
(chk "and it is certified by substitution" (oe-certify-int 3 1 -1))

(display "x^2 y'' + x y' + y = 0: negative discriminant, the complex regime x^alpha cos/sin(beta log x):") (newline)
(chk "the regime is complex" (equal? (oe-case 1 1) (quote complex)))

(display "x^2 y'' + x y' - 2y = 0: irrational exponents +-sqrt(2), named exactly as algebraic numbers:") (newline)
(chk "two algebraic exponents are named" (= (length (oe-exponents 1 -2)) 2))

(newline)
(display "The Euler equation is solved exactly across all three regimes -- two real exponents, a repeated exponent") (newline)
(display "with a log term, or a complex pair -- with integer exponents certified by substitution and irrational") (newline)
(display "exponents named as algebraic numbers.  Variable-coefficient equations beyond the equidimensional form, and") (newline)
(display "nonlinear ODEs, remain the open territory.") (newline)
