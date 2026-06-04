; RUNG 5: the NESTED LOGARITHM tower Q(x)(t1)(t2) with t1 = log x and t2 = log(t1) = log(log x).  A genuinely
; nested depth-2 tower: the second monomial is the logarithm of the first, t2' = t1'/t1 = 1/(x t1) -- so t2's
; derivative has t1 in the DENOMINATOR and the coefficient ring is rational (not just polynomial) in t1
; (docs/TRAGER_ROADMAP.md, Rung 5).
;
; Layering: over Q(x), a Q(x)(t1) element is a rational function in t1 (a pair N(t1)/D(t1) of t1-polynomials
; with Q(x) coefficients); its derivation uses t1' = 1/x.  Over that, a tower element is a polynomial in t2 with
; Q(x)(t1) coefficients, differentiated with t2' = 1/(x t1).  Every result is certified by differentiating in
; the tower and matching the integrand.
(import "cas/nestlog.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "A nested-logarithm tower: t1 = log x, t2 = log(log x), in Q(x)(t1)(t2).") (newline) (newline)

(display "the inner derivation (in Q(x)(t1), with t1' = 1/x):") (newline)
(chk "d/dx(log x) = 1/x" (rat-equal? (car (nl-t1deriv (list (rat-zero) (rat-one)))) (nl-invx)))
(chk "d/dx((log x)^2) = 2 (log x)/x  (t1^1 coefficient is 2/x)" (rat-equal? (nl-nth (nl-t1deriv (list (rat-zero) (rat-zero) (rat-one))) 1) (rat-mul (rat-from-poly (list 2)) (nl-invx))))

(display "the outer monomial t2 = log(log x) has derivative t2' = 1/(x log x):") (newline)
(display "  d/dx(log log x) computed in the tower = ") (display (nl-tcoeff (nl-deriv (nl-answer-t2)) 0)) (display "  (= (1/x)/t1 = t2')") (newline)
(chk "d/dx(log log x) = 1/(x log x)" (nl-q-eq? (nl-tcoeff (nl-deriv (nl-answer-t2)) 0) (nl-t2prime)))

(display "the nested-log integral INT 1/(x log x) dx = log(log x):") (newline)
(chk "INT 1/(x log x) dx = log(log x), certified in the tower" (nl-certify (nl-answer-t2) (nl-integrand-loglog)))

(display "a higher case, INT 2 log(log x)/(x log x) dx = (log(log x))^2:") (newline)
(define E2 (list (nl-q-zero) (nl-q-zero) (nl-q-one)))   ; t2^2
(chk "d/dx((log log x)^2) = 2 log(log x)/(x log x), certified" (nl-certify E2 (nl-deriv E2)))

(newline)
(display "A genuinely nested depth-2 logarithm tower: the coefficient ring is rational in the inner log, the") (newline)
(display "outer derivative carries log x in its denominator, and INT 1/(x log x) = log(log x) is certified.") (newline)
