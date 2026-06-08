; The GENERAL Risch differential equation solver with RATIONAL-FUNCTION coefficients: decides and solves
; y' + f y = g for a rational y, where f and g are rational functions over Q.  This lifts the polynomial-
; coefficient RDE (rischtower) to arbitrary rational coefficients -- the step that unlocks the recursive Risch
; decision procedure over arbitrary tower levels (docs/TRAGER_ROADMAP.md, the summit).
;
; Pipeline (Bronstein's weak/SPDE approach over Q(x)): the poles of any rational solution sit only at poles of
; f and g, so y = q/d with d a safe denominator over-bound; substituting reduces to a polynomial RDE
; A q' + B q = C, which is degree-bounded and solved by an exact linear system.  The differentiation
; certificate y' + f y = g is the final arbiter, so the construction is sound: only a y that genuinely satisfies
; the equation is returned.
(import "cas/rischrde.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The general RDE y' + f y = g with rational f, g: solved for rational y, or proven to have none.") (newline) (newline)

(display "y' + y = x  ->  y = x - 1  (f, g polynomial):") (newline)
(define yA (rde-solve (rat-from-poly (list 1)) (rat-from-poly (list 0 1))))
(chk "y' + y = x solved, y = x - 1, certified" (if (equal? yA (quote no-rational-solution)) #f (rde-certify (rat-from-poly (list 1)) (rat-from-poly (list 0 1)) yA)))

(display "y' - (1/x) y = x  ->  y = x^2  (a pole in f, but NONE in y -- the subtle case):") (newline)
(define yB (rde-solve (rat-make (list -1) (list 0 1)) (rat-from-poly (list 0 1))))
(chk "y' - (1/x) y = x solved, y = x^2, certified" (if (equal? yB (quote no-rational-solution)) #f (rde-certify (rat-make (list -1) (list 0 1)) (rat-from-poly (list 0 1)) yB)))

(display "y' + (1/x) y = 1  ->  y = x/2:") (newline)
(define yC (rde-solve (rat-make (list 1) (list 0 1)) (rat-from-poly (list 1))))
(chk "y' + (1/x) y = 1 solved, y = x/2, certified" (if (equal? yC (quote no-rational-solution)) #f (rde-certify (rat-make (list 1) (list 0 1)) (rat-from-poly (list 1)) yC)))

(display "y' + (2/x) y = 1/x^2  ->  y = 1/x  (a genuine pole in y):") (newline)
(define yD (rde-solve (rat-make (list 2) (list 0 1)) (rat-make (list 1) (list 0 0 1))))
(chk "y' + (2/x) y = 1/x^2 solved, y = 1/x, certified" (if (equal? yD (quote no-rational-solution)) #f (rde-certify (rat-make (list 2) (list 0 1)) (rat-make (list 1) (list 0 0 1)) yD)))

(display "y' + y = 1/x  ->  NO rational solution (the exponential-integral obstruction):") (newline)
(display "  ") (display (rde-decide (rat-from-poly (list 1)) (rat-make (list 1) (list 0 1)))) (newline)
(chk "y' + y = 1/x has NO rational solution (this is the Ei obstruction)" (equal? (rde-solve (rat-from-poly (list 1)) (rat-make (list 1) (list 0 1))) (quote no-rational-solution)))

(newline)
(display "The general RDE solver handles rational coefficients with poles: a pole in f need not force one in y") (newline)
(display "(y = x^2 above), poles in y are found when needed (y = 1/x), and the Ei obstruction is proven -- the") (newline)
(display "rational-function-coefficient solver the tower recursion needs at each level.") (newline)
