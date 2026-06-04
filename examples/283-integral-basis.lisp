; The INTEGRAL BASIS of an algebraic function field -- the construction that lifts algebraic integration past
; the hyperelliptic restriction to general curves (docs/TRAGER_ROADMAP.md, Rung 4).
;
; For F(x,y) = 0 defining K(x)[y]/(F), the integral closure of K[x] is a free K[x]-module of rank n = deg_y F.
; The naive powers 1, y, ..., y^{n-1} span only an ORDER; at the singular places the closure is strictly larger,
; and Trager's integration algorithm needs a basis of the closure (the regular differentials).  That is the
; integral basis.  This example demonstrates the two capabilities of intbasis.lisp:
;
; (1) the Puiseux-based LOCAL INTEGRALITY ENGINE: an element is integral at x=a iff it has no pole on any branch
;     of the curve over a; using the branch expansions y = Y(t), the valuation of an element on a branch is the
;     ord_t of its substitution, and N/(x-a)^k is integral iff ord_t(N) >= q*k on every branch.
;
; (2) the explicit, certified integral basis for y^2 = D(x): with the square-free factorization
;     D = c prod p_i^{e_i}, the closure is {1, y/g} with g = prod p_i^{floor(e_i/2)}, since (y/g)^2 = D/g^2 is a
;     polynomial (the integrality witness).
(import "cas/intbasis.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The integral basis: when is an algebraic function regular at a singular place, and the closure of y^2=D.") (newline) (newline)

(display "the explicit integral basis for y^2 = D(x), basis {1, y/g}, g = prod p_i^{floor(e_i/2)}:") (newline)

(define D1 (list 0 0 1 1))   ; x^2(x+1) -- nodal cubic
(display "  y^2 = x^2(x+1): g = ") (display (car (cdr (ib-quadratic D1)))) (display "  -> basis {1, y/x}; witness (y/x)^2 = ") (display (ib-quadratic-certify D1)) (display " = x+1") (newline)
(chk "nodal cubic: g = x, (y/x)^2 = x+1 integral, extends naive order" (if (equal? (car (cdr (ib-quadratic D1))) (list 0 1)) (if (equal? (ib-quadratic-certify D1) (list 1 1)) (ib-quadratic-extends? D1) #f) #f))

(define D2 (list 0 0 0 1))   ; x^3 -- cusp
(display "  y^2 = x^3 (cusp): g = ") (display (car (cdr (ib-quadratic D2)))) (display "  -> (y/x)^2 = ") (display (ib-quadratic-certify D2)) (display " = x") (newline)
(chk "cusp: g = x, (y/x)^2 = x" (if (equal? (car (cdr (ib-quadratic D2))) (list 0 1)) (equal? (ib-quadratic-certify D2) (list 0 1)) #f))

(define D4 (list 0 0 0 0 1 1))   ; x^4(x+1)
(display "  y^2 = x^4(x+1): g = ") (display (car (cdr (ib-quadratic D4)))) (display "  -> (y/x^2)^2 = ") (display (ib-quadratic-certify D4)) (display " = x+1") (newline)
(chk "x^4(x+1): g = x^2, (y/x^2)^2 = x+1" (if (equal? (car (cdr (ib-quadratic D4))) (list 0 0 1)) (equal? (ib-quadratic-certify D4) (list 1 1)) #f))

(define D3 (list 1 0 0 1))   ; x^3+1 squarefree -- elliptic, no extension
(display "  y^2 = x^3+1 (squarefree): g = ") (display (car (cdr (ib-quadratic D3)))) (display "  -> naive basis {1, y}, integral closure already the order") (newline)
(chk "squarefree D: g = 1, no extension needed" (not (ib-quadratic-extends? D3)))

(newline)
(display "the Puiseux-based integrality engine on the node F = y^2 - x^2 - x^3:") (newline)
(define F (list (list 0 0 -1 -1) (list) (list 1)))
(define br (pg-branches F 6))
(display "  two branches over x=0: y = +-x*sqrt(1+x)") (newline)
(define Ny (list (list 0) (list 1)))     ; the element y
(chk "y/x is integral at the node (regular on both branches)" (ib-integral-at0? Ny 1 br 6))
(chk "y/x^2 is NOT integral at the node (would have a pole)" (not (ib-integral-at0? Ny 2 br 6)))
(define N1 (list (list 1)))              ; the element 1
(chk "1/x is NOT integral (a genuine pole at 0)" (not (ib-integral-at0? N1 1 br 6)))
(chk "1 is integral everywhere" (ib-integral-at0? N1 0 br 6))

(newline)
(display "Integral basis: the y^2=D closure is certified ((y/g)^2 a polynomial), and the general Puiseux") (newline)
(display "valuation engine decides integrality at a singular place on every branch -- the core of Rung 4.") (newline)
