; POLYNOMIAL PARTICULAR SOLUTIONS of the second-order constant-coefficient linear ODE y'' + a y' + b y = q(x) with
; polynomial forcing (docs/CAS.md -- summit S5, the inhomogeneous companion to odelin's homogeneous solver, one
; order above odefol's first-order equations).
;
; The operator L(y) = y'' + a y' + b y is linear in a polynomial ansatz, so a polynomial particular solution
; (degree deg q when b /= 0) is found by an exact linear solve over Q and certified by differentiation.
; Inconsistent cases are reported honestly; the general solution then adds the homogeneous modes from odelin.
(import "cas/odelin2.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Second-order linear ODEs y'' + a y' + b y = q(x): exact polynomial particular solutions, certified.") (newline) (newline)

(display "y'' + y = x^2  ->  y_p = x^2 - 2:") (newline)
(define s1 (o2-particular 0 1 (list 0 0 1)))
(chk "the particular solution is x^2 - 2" (equal? s1 (list -2 0 1)))
(chk "it satisfies y'' + y - x^2 = 0" (o2-certify 0 1 (list 0 0 1) s1))

(display "y'' - y = x  ->  y_p = -x:") (newline)
(define s2 (o2-particular 0 -1 (list 0 1)))
(chk "the particular solution is -x" (equal? s2 (list 0 -1)))
(chk "it satisfies y'' - y - x = 0" (o2-certify 0 -1 (list 0 1) s2))

(display "with a first-derivative term: y'' + 3y' + 2y = x  ->  y_p = x/2 - 3/4:") (newline)
(define s3 (o2-particular 3 2 (list 0 1)))
(chk "the particular solution is x/2 - 3/4" (equal? s3 (list (/ -3 4) (/ 1 2))))
(chk "it satisfies y'' + 3y' + 2y - x = 0" (o2-certify 3 2 (list 0 1) s3))

(display "a cubic forcing: y'' + y = x^3 - 2x  ->  y_p = x^3 - 8x:") (newline)
(chk "the particular solution is x^3 - 8x" (equal? (o2-particular 0 1 (list 0 -2 0 1)) (list 0 -8 0 1)))

(display "the doubly-integrated case y'' = 1  ->  y_p = x^2/2:") (newline)
(chk "the particular solution is x^2/2" (equal? (o2-particular 0 0 (list 1)) (list 0 0 (/ 1 2))))

(newline)
(display "Second-order constant-coefficient linear ODEs with polynomial forcing are now solved exactly for the") (newline)
(display "particular solution -- by an exact linear solve, certified by differentiation -- joining the first-order") (newline)
(display "solver.  Non-polynomial forcing (resonance, exponential/trig right-hand sides) is the next step.") (newline)
