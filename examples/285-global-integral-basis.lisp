; The GLOBAL integral basis -- combining the local integral bases across ALL singular places of an algebraic
; curve, continuing Rung 4 of the Trager-Bronstein climb (docs/TRAGER_ROADMAP.md).
;
; The previous step computed the local integral basis at x = 0.  A general curve is singular at several places;
; this step finds them all (the repeated roots of g, for the superelliptic model y^n = g(x)), computes the local
; basis at each (by shifting the curve F(x+a, y) so the place sits at the origin), and combines the local
; denominators into the GLOBAL basis {y^j / d_j(x)}, d_j(x) = prod_a (x-a)^{k_j(a)}.  Each k_j(a) is the maximal
; exponent for which y^j/(x-a)^{k_j} is regular on every branch over a (the van-Hoeij triangular basis, with the
; lower-degree correction terms vanishing for these superelliptic curves).
;
; SOUNDNESS: a place can have an irrational tangent -- its branches then live over an extension of Q and the
; rational engine cannot certify integrality there.  In that case the routine returns needs-extension rather
; than a wrong basis; it never claims an integral element it cannot witness over Q.  (Quadratic nodes generically
; have irrational tangents; the example below uses a degree-3 curve whose cusps have rational tangents -- the
; cube roots of 1 are rational -- so the global basis is fully certifiable over Q.)
(import "cas/intbasis.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The global integral basis: local contributions at every singular place combined into {y^j / d_j(x)}.") (newline) (newline)

(display "a curve singular at TWO places, F = y^3 - x^4(x-1)^2 (cusps at x=0 and x=1, both rational tangents):") (newline)
(define g (list 0 0 0 0 1 -2 1))                   ; x^4(x-1)^2 = x^6 - 2x^5 + x^4
(define F (list (poly-neg g) (list) (list) (list 1)))
(display "  singular places: ") (display (ib-sing-places-superelliptic g 6)) (display "  (x=0 and x=1)") (newline)
(define gb (ib-global-basis-superelliptic F g 3 4))
(display "  global denominators d_0, d_1, d_2 = ") (display gb) (newline)
(display "  -> integral basis {1,  y/x,  y^2/(x^2(x-1))}") (newline)
(chk "d_1 = x (local k=1 at x=0, k=0 at x=1)" (equal? (ib-nth gb 1) (list 0 1)))
(chk "d_2 = x^2(x-1) (local k=2 at x=0, k=1 at x=1) -- combines BOTH places" (equal? (ib-nth gb 2) (list 0 0 -1 1)))

(display "  certify the basis elements are integral over Q[x] (w^3 a polynomial, since y^3 = g):") (newline)
(display "    w_1 = y/x  ->  w_1^3 = g/x^3 = x(x-1)^2 = ") (display (car (poly-divmod g (list 0 0 0 1)))) (newline)
(chk "w_1 = y/x integral (g/x^3 is a polynomial)" (equal? (car (cdr (poly-divmod g (list 0 0 0 1)))) (quote ())))
(define d2 (list 0 0 -1 1))
(chk "w_2 = y^2/(x^2(x-1)) integral (g^2/d_2^3 is a polynomial)" (equal? (car (cdr (poly-divmod (poly-mul g g) (poly-mul d2 (poly-mul d2 d2))))) (quote ())))

(newline)
(display "agreement with the certified quadratic case (single multi-root factor handles its places together):") (newline)
(define D (list 0 0 1 1))    ; x^2(x+1), nodal cubic y^2 = D
(display "  y^2 = x^2(x+1): quadratic g = ") (display (car (cdr (ib-quadratic D)))) (display " = x, basis {1, y/x}") (newline)
(chk "quadratic closure g = x (matches local assembly)" (equal? (car (cdr (ib-quadratic D))) (list 0 1)))

(newline)
(display "soundness -- a place with an irrational tangent is honestly deferred:") (newline)
(define D2 (list 0 0 1 -1 -1 1))    ; x^2(x-1)^2(x+1): node at x=1 has tangent sqrt(2), irrational over Q
(define Fq (list (poly-neg D2) (list) (list 1)))
(display "  y^2 = x^2(x-1)^2(x+1) (node at x=1 has tangent sqrt(2)) -> ") (display (ib-global-basis-superelliptic Fq D2 3 4)) (newline)
(chk "irrational tangent -> needs-extension (never a wrong basis)" (equal? (ib-global-basis-superelliptic Fq D2 3 4) (quote needs-extension)))

(newline)
(display "Global integral basis: local bases combined across all singular places, certified over Q, with an") (newline)
(display "honest needs-extension when a place lives over an extension -- the multi-place core of Rung 4.") (newline)
