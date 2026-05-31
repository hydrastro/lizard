; 153-rational-integration.lisp — integration of rational functions, with each
; antiderivative CERTIFIED by differentiating it back to the integrand.
;
; The integrand p/q is partial-fractioned (which factors q), and each term is
; integrated in closed form: polynomial part by the power rule, linear factors
; to logs and rational terms, complex-root quadratic factors to logs + arctan.
; The answer is then DIFFERENTIATED and checked, exactly as rational functions
; over Q, to equal the integrand -- so a wrong antiderivative cannot pass.  The
; arctan's irrational constant sqrt(D) never enters the check: the derivative of
; its term is the rational function mu/f by a closed-form identity.
;
; Cases needing algebraic numbers beyond Q (quadratics with real irrational
; roots, repeated quadratics with an arctan part, degree>=3 irreducibles) are
; reported as 'cannot -- never integrated wrongly.  `must` raises on failure.

(import "cas/integrate.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'integrate-check-failed)))

(display "certified integration of rational functions") (newline) (newline)

; ============================================================
; 1. Each integral is verified by differentiating the answer back
; ============================================================
(display "1. d/dx(answer) = integrand, checked exactly over Q") (newline)
(define (int-ok? num den) (integrate-verify num den (integrate-rational num den)))
(must "INT x^2 dx               (polynomial)"        (int-ok? (list 0 0 1) (list 1)))
(must "INT 1/(x^2-1) dx          (logs)"             (int-ok? (list 1) (list -1 0 1)))
(must "INT x^3/(x^2-1) dx        (poly part + logs)" (int-ok? (list 0 0 0 1) (list -1 0 1)))
(must "INT 1/(x-1)^2 dx          (rational)"         (int-ok? (list 1) (poly-pow (list -1 1) 2)))
(must "INT 1/(x^2+1) dx          (arctan)"           (int-ok? (list 1) (list 1 0 1)))
(must "INT 1/(x^2+x+1) dx        (arctan, sqrt 3)"   (int-ok? (list 1) (list 1 1 1)))
(must "INT 1/(x^3+x) dx          (log + log)"        (int-ok? (list 1) (list 0 1 0 1)))
(must "INT (5x+4)/((x-1)(x^2+4)) (log + log + arctan)"
      (int-ok? (list 4 5) (poly-mul (list -1 1) (list 4 0 1))))
(must "INT (2x^2+3)/(x-1)^3 dx   (rationals)"
      (int-ok? (list 3 0 2) (poly-pow (list -1 1) 3)))
(newline)

; ============================================================
; 2. Specific closed forms the integrator must produce exactly
; ============================================================
(display "2. exact closed forms") (newline)
(must "INT 1/(x^2+1) = arctan(x)"
      (equal? (integral->string (integrate-rational (list 1) (list 1 0 1)) "x")
              "arctan(x) + C"))
(must "INT 1/(x-1)^2 = -1/(x-1)"
      (equal? (integral->string (integrate-rational (list 1) (poly-pow (list -1 1) 2)) "x")
              "(-1)/(x - 1) + C"))
(must "INT x^2 = x^3/3"
      (equal? (integral->string (integrate-rational (list 0 0 1) (list 1)) "x")
              "1/3*x^3 + C"))
(newline)

; ============================================================
; 3. HONEST deferral: cases beyond Q are reported, never faked
; ============================================================
(display "3. honest deferral (no wrong/partial answers)") (newline)
(must "INT 1/(x^2-2)   real irrational roots -> cannot"
      (equal? (car (integrate-rational (list 1) (list -2 0 1))) 'cannot))
(must "INT 1/(x^2+1)^2 repeated quadratic    -> cannot"
      (equal? (car (integrate-rational (list 1) (poly-pow (list 1 0 1) 2))) 'cannot))
(must "INT 1/(x^3+2)   degree-3 irreducible  -> cannot"
      (equal? (car (integrate-rational (list 1) (list 2 0 0 1))) 'cannot))
(newline)

(display "all integration checks passed (every closed form differentiates back).")
(newline)
