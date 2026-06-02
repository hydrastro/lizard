; 226-risch-de-rational.lisp -- the Risch differential equation with rational data, and the
; integration of R(x) e^{p(x)} for rational R.
;
; risch.lisp solves y' + f y = g only for polynomial f, g.  This extends it to a polynomial f
; with a RATIONAL g in Q(x) -- exactly what integrating R(x) e^{p(x)} needs, since that integral
; is h e^p with h' + p' h = R, where p' is a polynomial but R is rational.  A pole of R of order
; m forces a pole of h of order m-1 (p' has none), so the solution denominator divides
; gcd(E, E') for E = denominator(R); writing h = U/gcd(E,E') reduces everything to one linear
; equation for a polynomial U.  Every elementary answer is differentiated and checked, and for
; this class the bound is tight, so "non-elementary" is a proof.  `must` raises on failure.

(import "cas/rischde.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'rde-check-failed)))
(define (rat B E) (cons B E))

(display "Risch DE over Q(x), and INT R(x) e^{p(x)} dx") (newline) (newline)

(display "1. the Risch DE y' + f y = g with f polynomial, g rational") (newline)
(display "    e.g. y' + y = 1/x + ... has rational solutions only when the poles are compatible") (newline)
(must "y' + 0*y = 3x^2  solves to y = x^3"        (equal? (rischde '() (rat (list 0 0 3) (list 1))) (cons (list 0 0 0 1) (list 1))))
(must "the solver certifies its own output"       (rischde-verify (list 1) (rat (list 1 1) (list 1)) (rischde (list 1) (rat (list 1 1) (list 1)))))
(newline)

(display "2. INT R(x) e^{p(x)} dx with R rational -- elementary cases, all certified") (newline)
(display "    INT (x-1)/x^2 e^x dx  = e^x / x") (newline)
(must "  h = 1/x, certified"  (and (equal? (car (int-rat-exp (rat (list -1 1) (list 0 0 1)) (list 0 1))) 'elementary)
                                   (int-rat-exp-verify (rat (list -1 1) (list 0 0 1)) (list 0 1))))
(display "    INT x/(x+1)^2 e^x dx  = e^x / (x+1)") (newline)
(must "  h = 1/(x+1), certified" (int-rat-exp-verify (rat (list 0 1) (list 1 2 1)) (list 0 1)))
(display "    INT (1 + 2x^2) e^{x^2} dx  = x e^{x^2}") (newline)
(must "  h = x, certified"    (int-rat-exp-verify (rat (list 1 0 2) (list 1)) (list 0 0 1)))
(must "  INT 2x e^{x^2} dx = e^{x^2}, certified" (int-rat-exp-verify (rat (list 0 2) (list 1)) (list 0 0 1)))
(newline)

(display "3. non-elementary integrals -- PROVEN, no elementary antiderivative exists") (newline)
(must "INT e^{x^2} dx is non-elementary"  (equal? (car (int-rat-exp (rat (list 1) (list 1)) (list 0 0 1))) 'non-elementary))
(must "INT e^x / x dx is non-elementary"  (equal? (car (int-rat-exp (rat (list 1) (list 0 1)) (list 0 1))) 'non-elementary))
(newline)

(display "all Risch-DE checks passed.") (newline)
