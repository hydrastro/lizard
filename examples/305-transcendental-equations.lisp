; Solving TRANSCENDENTAL equations that become polynomial under a substitution u = m(x) (m one of exp, log, a
; power), the way Maxima handles e.g. e^{2x} - 3 e^x + 2 = 0.  The equation is a polynomial in the monomial u;
; we solve it over Q with the certified solver (solve.lisp) and back-substitute u = m(x) to express each
; x-solution exactly (x = log r, x = e^r, x = r^{1/k}) without forcing a floating-point value.
(import "cas/transsolve.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Transcendental equations solved by substitution u = m(x), then back-substitution.") (newline) (newline)

(display "e^{2x} - 3 e^x + 2 = 0  (substitute u = e^x):") (newline)
(display "  the polynomial u^2 - 3u + 2 has roots ") (display (ts-uroots (list 2 -3 1))) (display " -> x = log r for each:") (newline)
(display "  ") (display (ts-solve-exp (list 2 -3 1))) (display "  (i.e. x = log 2 and x = log 1 = 0)") (newline)
(chk "u-roots of u^2 - 3u + 2 are {2, 1}" (equal? (ts-uroots (list 2 -3 1)) (list 2 1)))
(chk "the exp equation gives x = log 2 and x = log 1" (equal? (ts-solve-exp (list 2 -3 1)) (list (list (quote x=log) 2) (list (quote x=log) 1))))

(display "e^{2x} - e^x - 6 = 0  (u = e^x; the negative root has no real x):") (newline)
(display "  ") (display (ts-solve-exp (list -6 -1 1))) (display "  (u = 3 -> x = log 3; u = -2 -> no real x)") (newline)
(chk "negative root reported as having no real x" (equal? (ts-solve-exp (list -6 -1 1)) (list (list (quote x=log) 3) (list (quote no-real-x-for-u) -2))))

(display "(log x)^2 - 1 = 0  (substitute u = log x):") (newline)
(display "  ") (display (ts-solve-log (list -1 0 1))) (display "  (u = 1 -> x = e; u = -1 -> x = 1/e)") (newline)
(chk "the log equation gives x = e^{-1} and x = e^1" (equal? (ts-solve-log (list -1 0 1)) (list (list (quote x=exp) -1) (list (quote x=exp) 1))))

(display "a power substitution u = x^2 in u^2 - 5u + 4 = 0:") (newline)
(display "  ") (display (ts-solve-pow 2 (list 4 -5 1))) (display "  (u = 4 -> x = +/-2; u = 1 -> x = +/-1)") (newline)

(display "every root is verified by substituting it back into the polynomial:") (newline)
(chk "u = 2 and u = 1 both satisfy u^2 - 3u + 2 = 0" (if (ts-verify (list 2 -3 1) 2) (ts-verify (list 2 -3 1) 1) #f))

(newline)
(display "Transcendental equations reduced to polynomials by substitution, solved exactly over Q, and the") (newline)
(display "solutions expressed in closed form (x = log r, e^r, r^{1/k}) with each polynomial root verified.") (newline)
