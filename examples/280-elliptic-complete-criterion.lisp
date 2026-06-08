; The COMPLETE elliptic third-kind integration test, correcting and completing the elliptic rungs
; (docs/TRAGER_ROADMAP.md).
;
; Rung 3b decided INT dx/((x-s) sqrt(p)) elementary <=> the pole lifts to a torsion point.  That condition is
; NECESSARY but NOT SUFFICIENT.  The correct criterion (Trager; Combot, arXiv:2103.04134) is TWO parts:
;   (1) the pole P=(s,rho) is torsion -- this is what makes the logarithmic part L = c log f EXIST; and
;   (2) the remainder I - L, a holomorphic (first-kind) differential lambda * dx/y, must VANISH.
; A nonzero lambda means I = (elementary logarithm) + lambda * (elliptic integral of the first kind) -- NOT
; elementary, since dx/y is the canonical non-elementary elliptic integral.
;
; This module (ellint.lisp) implements the complete test by CONSTRUCTING L and computing the remainder: it
; builds the function g with div(g) = N[P]-N[O] (N = order of P) by interpolation (g = A + B y vanishing to
; order N at P, robust at 2-torsion), forms f = g/conj(g) so c f'/f matches the residues of dx/((x-s)y), and
; checks whether the remainder lambda*dx/y vanishes.  When it does, c log f is certified by differentiation.
;
; The decisive corrected findings: INT dx/(x sqrt(x^3+1)) IS elementary = (1/3) log((y-1)/(y+1)) (lambda = 0),
; but the torsion poles of orders 4, 5, 6 tested all have lambda != 0 and are therefore NON-elementary --
; correcting the earlier over-optimistic "torsion => elementary" verdict.
(import "cas/ellint.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Complete elliptic third-kind test: elementary <=> pole torsion AND first-kind remainder lambda = 0.") (newline) (newline)

(display "the genuinely elementary case (order-3 torsion pole, lambda = 0):") (newline)
(define r3 (ei-integrate (rat-from-poly (list 1 0 0 1)) 0))
(display "  INT dx/(x sqrt(x^3+1)) -> ") (display (car r3)) (display " ; c = ") (display (car (cdr (cdr r3)))) (newline)
(display "  lambda = ") (display (ei-remainder-lambda (rat-from-poly (list 1 0 0 1)) 0)) (display " (zero -> truly elementary)") (newline)
(chk "INT dx/(x sqrt(x^3+1)) = (1/3) log((y-1)/(y+1)), certified elementary" (equal? (car r3) (quote elementary)))

(define r3b (ei-integrate (rat-from-poly (list 4 0 0 1)) 0))
(display "  INT dx/(x sqrt(x^3+4)) [order-3 pole (0,2)] -> ") (display (car r3b)) (newline)
(chk "INT dx/(x sqrt(x^3+4)) certified elementary (lambda=0)" (equal? (car r3b) (quote elementary)))

(newline)
(display "torsion BUT non-elementary (first-kind remainder lambda != 0) -- the corrected verdicts:") (newline)
(define r6 (ei-integrate (rat-from-poly (list 1 0 0 1)) 2))
(display "  INT dx/((x-2) sqrt(x^3+1)) [order-6 pole (2,3)] -> ") (display r6) (newline)
(display "    lambda = ") (display (ei-remainder-lambda (rat-from-poly (list 1 0 0 1)) 2)) (display " (nonzero -> NON-elementary)") (newline)
(chk "order-6 torsion pole is NON-elementary (first-kind remainder), corrected" (equal? (car r6) (quote non-elementary)))
(define r4 (ei-integrate (rat-from-poly (list 0 4 0 1)) 2))
(display "  INT dx/((x-2) sqrt(x^3+4x)) [order-4 pole (2,4)] -> ") (display r4) (newline)
(chk "order-4 torsion pole is NON-elementary (first-kind remainder)" (equal? (car r4) (quote non-elementary)))

(newline)
(display "non-torsion pole (the canonical elliptic obstruction):") (newline)
(define ri (ei-integrate (rat-from-poly (list -2 0 0 1)) 3))
(display "  INT dx/((x-3) sqrt(x^3-2)) [pole (3,5), infinite order] -> ") (display ri) (newline)
(chk "infinite-order pole NON-elementary" (equal? (car ri) (quote non-elementary)))

(newline)
(display "soundness:") (newline)
(define rx (ei-integrate (rat-from-poly (list 1 0 0 1)) 1))
(display "  INT dx/((x-1) sqrt(x^3+1)) [p(1)=2 not a square] -> ") (display (car rx)) (newline)
(chk "non-rational lift honestly reported needs-extension" (equal? (car rx) (quote needs-extension)))

(newline)
(display "COMPLETE elliptic third-kind criterion: torsion + lambda=0, certified log for the elementary case; corrected non-elementary verdicts.") (newline)
