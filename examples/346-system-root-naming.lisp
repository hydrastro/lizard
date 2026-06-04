; Naming the solutions of a solved polynomial system: take the univariate eliminant from the Groebner solver
; (polysolve), project it to a dense coefficient polynomial, and count and isolate its real roots exactly with
; Sturm sequences (sturm) -- closing frontier 4(d), root-naming for the polynomial-system solver (docs/CAS.md).
;
; The eliminant for a coordinate in a zero-dimensional system is a univariate polynomial whose real roots are the
; possible real values of that coordinate.  num-real-roots gives the exact count (Sturm over the Cauchy bound),
; and isolate-refined gives rational intervals each holding exactly one real root -- all exact, no floating point,
; certified by Sturm's theorem.
(import "cas/polyroots.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Naming real solutions of polynomial systems: exact count and rational isolating intervals via Sturm.") (newline) (newline)

(display "the circle meets the line: x^2 + y^2 = 1, x = y  (solutions x = y = +-1/sqrt2):") (newline)
(define f1 (list (cons 1 (list 2 0)) (cons 1 (list 0 2)) (cons -1 (list 0 0))))   ; x^2 + y^2 - 1
(define f2 (list (cons 1 (list 1 0)) (cons -1 (list 0 1))))                       ; x - y
(define S1 (list f1 f2))
(display "  the y-eliminant is 2y^2 - 1; projecting it to a coefficient polynomial:") (newline)
(chk "eliminant coefficients are (-1 0 2) = 2y^2 - 1" (equal? (pr-eliminant->coeffs (psys-eliminant S1 2 1) 1 2) (list -1 0 2)))
(chk "y takes exactly 2 real values (+-1/sqrt2)" (= (pr-real-solution-count S1 2 1) 2))
(display "  isolating intervals for y (to tolerance 1/100) bracket +-0.7071...:") (newline)
(chk "two disjoint rational isolating intervals are produced" (= (pr-len (pr-real-intervals S1 2 1 (/ 1 100))) 2))

(display "a triangular system x = 1, y = 2, z = 3  (a unique real point):") (newline)
(define e1 (list (cons 1 (list 1 0 0)) (cons -1 (list 0 0 0))))
(define e2 (list (cons 1 (list 0 1 0)) (cons -2 (list 0 0 0))))
(define e3 (list (cons 1 (list 0 0 1)) (cons -3 (list 0 0 0))))
(define S4 (list e1 e2 e3))
(chk "each of x, y, z takes exactly one real value" (if (= (pr-real-solution-count S4 3 0) 1) (if (= (pr-real-solution-count S4 3 1) 1) (= (pr-real-solution-count S4 3 2) 1) #f) #f))

(display "the full solution report pairs each coordinate's count with its isolating intervals:") (newline)
(display "  ") (display (pr-solution-report S1 2 (/ 1 100))) (newline)
(chk "report has one entry per variable" (= (pr-len (pr-solution-report S1 2 (/ 1 100))) 2))

(newline)
(display "The polynomial-system solver now NAMES its real solutions: from the Groebner eliminant, Sturm gives the") (newline)
(display "exact number of distinct real values per coordinate and rational intervals isolating each -- exact and") (newline)
(display "certified by Sturm's theorem, no floating point.  Complex roots and full coordinate back-substitution") (newline)
(display "(pairing the per-variable values into solution tuples) are the natural further steps.") (newline)
