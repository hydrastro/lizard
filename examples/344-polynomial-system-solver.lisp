; Solving multivariate polynomial systems by Groebner-basis elimination, built on the existing Buchberger
; implementation (docs/CAS.md -- frontier 4, multivariate: an application on the Groebner machinery).
;
; For a system f_1 = ... = f_k = 0 over Q, a Groebner basis G of the ideal <f_1,...,f_k> makes the structure
; decidable: the system is CONSISTENT iff 1 is not in the ideal (G is not {1}, Weak Nullstellensatz); under lex
; order G is TRIANGULAR (the eliminated / solved form); the ideal is ZERO-DIMENSIONAL (finitely many solutions)
; iff every variable has a pure-power leading monomial; and a polynomial is a CONSEQUENCE of the system iff it
; reduces to 0 modulo G.  Each verdict is gated by reduction modulo the basis -- the Groebner certificate.
(import "cas/polysolve.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Multivariate polynomial systems, solved by Groebner elimination -- consistency, dimension, eliminants.") (newline) (newline)

(display "the circle meets the line: x^2 + y^2 = 1, x = y  (solutions x = y = +-1/sqrt2):") (newline)
(define f1 (list (cons 1 (list 2 0)) (cons 1 (list 0 2)) (cons -1 (list 0 0))))   ; x^2 + y^2 - 1
(define f2 (list (cons 1 (list 1 0)) (cons -1 (list 0 1))))                       ; x - y
(define S1 (list f1 f2))
(chk "the system is consistent (has a common zero)" (psys-consistent? S1))
(chk "and zero-dimensional -- finitely many solutions" (psys-zero-dim? S1 2))
(display "  the lex Groebner basis triangularizes to {x - y, 2y^2 - 1}; the eliminant in y is 2y^2 - 1.") (newline)
(chk "the eliminant in y exists (univariate, 2y^2 - 1)" (if (null? (psys-eliminant S1 2 1)) #f #t))
(chk "x^2 + y^2 - 1 is a consequence of the system (reduces to 0 modulo the basis)" (psys-consequence? f1 S1))
(chk "the polynomial x is NOT a consequence (x is not forced to vanish)" (if (psys-consequence? (list (cons 1 (list 1 0))) S1) #f #t))

(display "an INCONSISTENT system: x + y = 2 and x + y = 3  (forces 0 = 1):") (newline)
(define g1 (list (cons 1 (list 1 0)) (cons 1 (list 0 1)) (cons -2 (list 0 0))))
(define g2 (list (cons 1 (list 1 0)) (cons 1 (list 0 1)) (cons -3 (list 0 0))))
(chk "reported inconsistent -- the Groebner basis collapses to {1}, no common zero" (if (psys-consistent? (list g1 g2)) #f #t))

(display "a POSITIVE-dimensional system: xy = 1  (a hyperbola, infinitely many solutions):") (newline)
(define h1 (list (cons 1 (list 1 1)) (cons -1 (list 0 0))))
(chk "consistent but NOT zero-dimensional (a curve of solutions)" (if (psys-consistent? (list h1)) (if (psys-zero-dim? (list h1) 2) #f #t) #f))

(display "a triangular 3-variable system: x = 1, y = 2, z = 3  (a unique point):") (newline)
(define e1 (list (cons 1 (list 1 0 0)) (cons -1 (list 0 0 0))))
(define e2 (list (cons 1 (list 0 1 0)) (cons -2 (list 0 0 0))))
(define e3 (list (cons 1 (list 0 0 1)) (cons -3 (list 0 0 0))))
(chk "consistent and zero-dimensional (the unique solution (1,2,3))" (if (psys-consistent? (list e1 e2 e3)) (psys-zero-dim? (list e1 e2 e3) 3) #f))

(newline)
(display "Multivariate polynomial systems are now solved structurally by Groebner elimination: consistency, finite-") (newline)
(display "ness of the solution set, the triangular eliminated form, and ideal-membership consequences -- each backed") (newline)
(display "by reduction modulo the basis.  Naming or approximating the individual roots is the natural next step.") (newline)
