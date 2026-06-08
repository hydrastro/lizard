; The GENERAL exponential-integral decider, built on the rational-coefficient RDE solver (rischrde): decides
; INT R(x) e^{g(x)} dx for R an ARBITRARY rational function and g a polynomial, by the exact reduction to a
; Risch differential equation.  This SUBSUMES the polynomial-R decider (liouville) and the pole-at-origin
; rational decider (liouvillerat) into a single rational-coefficient procedure -- the rational-function-
; coefficient solver the recursive Risch procedure needs at each level (docs/TRAGER_ROADMAP.md, the summit).
;
; INT R e^g = y e^g iff (y e^g)' = (y' + g' y) e^g = R e^g, i.e. y solves the RDE  y' + g' y = R, decided in the
; rationals by rischrde: a solution proves elementarity, its absence proves non-elementarity.
(import "cas/rischrde2.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "INT R(x) e^{g(x)} dx for arbitrary rational R, by reduction to the RDE y' + g' y = R.") (newline) (newline)

(display "INT x e^x dx = (x-1) e^x:") (newline)
(define d1 (re2-decide (rat-from-poly (list 0 1)) (list 0 1)))
(chk "INT x e^x = (x-1) e^x elementary, certified" (if (equal? (car d1) (quote elementary)) (re2-certify (rat-from-poly (list 0 1)) (list 0 1) (car (cdr d1))) #f))

(display "INT e^x/x dx -- the exponential integral Ei, PROVEN non-elementary:") (newline)
(display "  ") (display (re2-decide (rat-make (list 1) (list 0 1)) (list 0 1))) (newline)
(chk "INT e^x/x is PROVEN non-elementary (Ei)" (equal? (car (re2-decide (rat-make (list 1) (list 0 1)) (list 0 1))) (quote non-elementary)))

(display "INT (1/x - 1/x^2) e^x dx = (1/x) e^x  (arbitrary rational R with poles):") (newline)
(define d3 (re2-decide (rat-make (list -1 1) (list 0 0 1)) (list 0 1)))
(chk "INT (1/x - 1/x^2) e^x = (1/x) e^x elementary, certified" (if (equal? (car d3) (quote elementary)) (re2-certify (rat-make (list -1 1) (list 0 0 1)) (list 0 1) (car (cdr d3))) #f))

(display "INT x e^{x^2} dx = (1/2) e^{x^2}  (g = x^2, so the RDE is y' + 2x y = x):") (newline)
(define d4 (re2-decide (rat-from-poly (list 0 1)) (list 0 0 1)))
(chk "INT x e^{x^2} = (1/2) e^{x^2} elementary, certified" (if (equal? (car d4) (quote elementary)) (re2-certify (rat-from-poly (list 0 1)) (list 0 0 1) (car (cdr d4))) #f))

(display "INT e^{x^2} dx -- the error function erf, PROVEN non-elementary (the RDE y' + 2x y = 1 has no rational y):") (newline)
(chk "INT e^{x^2} is PROVEN non-elementary (erf)" (equal? (car (re2-decide (rat-from-poly (list 1)) (list 0 0 1))) (quote non-elementary)))

(newline)
(display "A single rational-coefficient procedure decides INT R e^g for arbitrary rational R and polynomial g,") (newline)
(display "subsuming the polynomial and pole-at-origin deciders -- erf and Ei proven non-elementary through the RDE.") (newline)
