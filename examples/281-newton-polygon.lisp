; The NEWTON POLYGON for a general plane algebraic curve F(x,y) = 0, continuing Rung 4 of the Trager-Bronstein
; climb (docs/TRAGER_ROADMAP.md).  puiseux.lisp handled the superelliptic case y^e = g(x); this module handles
; GENERAL F by computing the Newton polygon at x = 0, whose lower-left hull edges give the leading exponents
; (slopes) and leading coefficients of ALL the Puiseux branches -- the decisive first step of Newton-Puiseux
; for arbitrary algebraic functions, and the branching analyzer the integral basis will consume.
;
; F is a list of y-coefficients, each a polynomial in x (low-to-high): F = (F0 F1 ... Fd) means
; sum_j Fj(x) y^j.  Each hull edge from (i1,j1) to (i2,j2) yields a branch leading exponent mu = (i2-i1)/(j2-j1)
; (returned as a reduced (num . den)) and an EDGE POLYNOMIAL in c whose nonzero roots are the branch leading
; coefficients (y ~ c x^mu).
(import "cas/newton.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Newton polygon of F(x,y)=0 at x=0: leading exponents (slopes) and edge polynomials for the branches.") (newline) (newline)

(display "a node, F = y^2 - x^2 - x^3  (two tangents):") (newline)
(define node (list (list 0 0 -1 -1) (list) (list 1)))
(display "  Newton polygon -> ") (display (nw-newton-polygon node)) (newline)
(display "  one edge of slope mu = 1, edge polynomial c^2 - 1 -> roots +-1: the two branches y ~ +-x") (newline)
(chk "node: single slope 1, edge poly c^2-1" (if (equal? (nw-slopes node) (list (cons 1 1))) (equal? (car (cdr (car (nw-newton-polygon node)))) (list -1 0 1)) #f))

(display "a ramified place, F = y^2 - x:") (newline)
(define se (list (list 0 -1) (list) (list 1)))
(display "  slope mu = ") (display (car (nw-slopes se))) (display "  -> y ~ x^(1/2) (ramification index 2)") (newline)
(chk "y^2 = x: slope 1/2" (equal? (nw-slopes se) (list (cons 1 2))))

(display "a cusp, F = y^2 - x^3:") (newline)
(define cusp (list (list 0 0 0 -1) (list) (list 1)))
(display "  slope mu = ") (display (car (nw-slopes cusp))) (display "  -> y ~ x^(3/2)") (newline)
(chk "cusp y^2 = x^3: slope 3/2" (equal? (nw-slopes cusp) (list (cons 3 2))))

(display "two branches of DIFFERENT slopes, F = (y - x)(y - x^2) = y^2 - (x + x^2) y + x^3:") (newline)
(define two (list (list 0 0 0 1) (list 0 -1 -1) (list 1)))
(display "  Newton polygon -> ") (display (nw-newton-polygon two)) (newline)
(display "  slopes ") (display (nw-slopes two)) (display ": branches y ~ x^2 (slope 2) and y ~ x (slope 1)") (newline)
(chk "(y-x)(y-x^2): slopes 2 and 1 (two distinct branches)" (equal? (nw-slopes two) (list (cons 2 1) (cons 1 1))))

(display "a tacnode-type place, F = y^2 - x^4  (two branches y ~ +-x^2):") (newline)
(define tac (list (list 0 0 0 0 -1) (list) (list 1)))
(display "  slope mu = ") (display (car (nw-slopes tac))) (display ", edge polynomial c^2 - 1 -> y ~ +-x^2") (newline)
(chk "y^2 = x^4: slope 2, edge poly c^2-1" (if (equal? (nw-slopes tac) (list (cons 2 1))) (equal? (car (cdr (car (nw-newton-polygon tac)))) (list -1 0 1)) #f))

(display "a smooth branch, F = y - x - x^2:") (newline)
(define smooth (list (list 0 -1 -1) (list 1)))
(chk "y = x + x^2 (smooth): slope 1" (equal? (nw-slopes smooth) (list (cons 1 1))))

(newline)
(display "Newton polygon working for general F(x,y): nodes, cusps, ramified and tacnode places, multiple") (newline)
(display "distinct slopes, and smooth branches -- the branching/ramification analyzer for general Puiseux.") (newline)
