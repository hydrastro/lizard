; Assembling the INTEGRAL BASIS at a singular place, continuing Rung 4 of the Trager-Bronstein climb
; (docs/TRAGER_ROADMAP.md).  The previous step gave the local integrality ENGINE (decide whether an element is
; regular at a place via its valuation on every Puiseux branch).  This step ASSEMBLES the local integral basis:
; for each power y^j (j = 0 .. deg_y(F) - 1), it finds the largest exponent k_j such that y^j / x^{k_j} is still
; integral at x = 0, yielding the basis {y^j / x^{k_j}} of the integral closure localized at 0.
;
; This is the van-Hoeij triangular basis in the (common) case where the lower-degree correction terms vanish --
; true for superelliptic curves and the examples below.  Each basis element's integrality is witnessed by the
; branch valuations, and maximality (k_j cannot be increased) is checked by the same engine.  The sum of the
; k_j is a local measure of the singularity (the x=0 contribution to the gap between the integral closure and
; the naive order K[x][y]/(F)).
(import "cas/intbasis.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The local integral basis at x=0: {y^j / x^{k_j}} with each k_j maximal for integrality, via branches.") (newline) (newline)

(display "a degree-3 superelliptic curve, F = y^3 - x^4 (one branch y = x^(4/3)):") (newline)
(define F1 (list (list 0 0 0 0 -1) (list) (list) (list 1)))
(define b1 (ib-local-basis-at0 F1 3 4))
(display "  basis exponents (j . k_j) = ") (display b1) (display "  -> {1, y/x, y^2/x^2}") (newline)
(chk "y^3=x^4 integral basis is {1, y/x, y^2/x^2}" (equal? b1 (list (cons 0 0) (cons 1 1) (cons 2 2))))
(display "  singularity measure delta at 0 = sum k_j = ") (display (ib-delta-at0 b1)) (newline)
(chk "delta = 3" (= (ib-delta-at0 b1) 3))

(display "the nodal cubic, F = y^2 - x^2(x+1) (two branches y = +-x*sqrt(1+x)):") (newline)
(define F2 (list (list 0 0 -1 -1) (list) (list 1)))
(define b2 (ib-local-basis-at0 F2 3 5))
(display "  basis exponents = ") (display b2) (display "  -> {1, y/x}") (newline)
(chk "nodal cubic integral basis {1, y/x} (agrees with the certified quadratic g = x)" (equal? b2 (list (cons 0 0) (cons 1 1))))
(chk "agrees with ib-quadratic: g = x" (equal? (car (cdr (ib-quadratic (list 0 0 1 1)))) (list 0 1)))

(display "a smooth place, F = y^2 - x^3 - 1 (the elliptic curve y^2 = x^3+1, nonsingular at x=0):") (newline)
(define F3 (list (list -1 0 0 -1) (list) (list 1)))
(define b3 (ib-local-basis-at0 F3 2 4))
(display "  basis exponents = ") (display b3) (display "  -> {1, y}: no extension, the order is already integral here") (newline)
(chk "smooth place: no extension, basis {1, y}" (equal? b3 (list (cons 0 0) (cons 1 0))))
(chk "delta = 0 at a smooth place" (= (ib-delta-at0 b3) 0))

(newline)
(display "Integral-basis assembly: the local closure {y^j / x^{k_j}} is built from branch valuations, matching") (newline)
(display "the certified quadratic case on the nodal cubic and correctly finding no extension at a smooth place.") (newline)
