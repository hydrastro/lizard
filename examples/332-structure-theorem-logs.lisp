; The LOGARITHMIC PART of the Risch-Trager structure theorem, constructive and certified: given an integrand f
; and candidate factors g_1, ..., g_m, decide whether f = sum_i c_i (g_i'/g_i) for CONSTANTS c_i, so that
; INT f = sum_i c_i log(g_i) -- a sum of logarithms.  This recognizes when an integral is a sum of logarithms
; with constant coefficients, the heart of the structure theorem's logarithmic content
; (docs/TRAGER_ROADMAP.md, the summit, "the structure theorem").
;
; Liouville's theorem: INT f is elementary iff f = v' + sum c_i u_i'/u_i with the c_i constants.  This module
; handles the logarithmic sum by solving sum c_i (g_i'/g_i) = f for rational constants c_i (an exact linear solve
; over Q via sampling at pole-free points), then CERTIFYING symbolically; a returned decomposition is genuine and
; non-cases yield an honest 'no-log-decomposition.
(import "cas/rischstruct.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The structure theorem's logarithmic part: f = sum c_i (g_i'/g_i) for constants c_i, so INT f = sum c_i log(g_i).") (newline) (newline)

(display "INT 2x/(x^2-1) dx = log(x^2-1): with factor x^2-1, the coefficient is the constant 1:") (newline)
(define f (rat-make (list 0 2) (list -1 0 1)))
(define g1 (rat-from-poly (list -1 0 1)))
(define r (struct-log-solve f (list g1)))
(chk "2x/(x^2-1) = 1 * (x^2-1)'/(x^2-1)" (if (equal? (car r) (quote logs)) (rat-equal? (car (car (cdr r))) (rat-one)) #f))

(display "the same integrand as a sum of two simple logarithms, INT = log(x-1) + log(x+1):") (newline)
(define gA (rat-from-poly (list -1 1)))
(define gB (rat-from-poly (list 1 1)))
(define r2 (struct-log-solve f (list gA gB)))
(chk "coefficients are (1, 1)" (if (equal? (car r2) (quote logs)) (if (rat-equal? (car (car (cdr r2))) (rat-one)) (rat-equal? (car (cdr (car (cdr r2)))) (rat-one)) #f) #f))
(chk "and the decomposition certifies" (if (equal? (car r2) (quote logs)) (struct-log-certify f (list gA gB) (car (cdr r2))) #f))

(display "non-unit residues: INT (3/(x-1) + 5/(x+2)) dx = 3 log(x-1) + 5 log(x+2), constants (3, 5):") (newline)
(define f3 (rat-add (rat-make (list 3) (list -1 1)) (rat-make (list 5) (list 2 1))))
(define gC (rat-from-poly (list 2 1)))
(define r3 (struct-log-solve f3 (list gA gC)))
(chk "recovers the constants (3, 5)" (if (equal? (car r3) (quote logs)) (if (rat-equal? (car (car (cdr r3))) (rat-from-poly (list 3))) (rat-equal? (car (cdr (car (cdr r3)))) (rat-from-poly (list 5))) #f) #f))

(display "a non-decomposable integrand returns an honest no-log-decomposition:") (newline)
(define f4 (rat-make (list 1) (list 1 0 1)))
(chk "1/(x^2+1) is not a sum of the given log-derivatives" (equal? (struct-log-solve f4 (list gA)) (quote no-log-decomposition)))

(newline)
(display "The structure theorem's logarithmic part is now constructive and certified: the constant residues are") (newline)
(display "found by an exact linear solve over Q and checked by differentiation -- recognizing integrals that are") (newline)
(display "sums of logarithms, the core decidable content of Liouville's theorem at the logarithmic level.") (newline)
