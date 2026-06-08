; 171-ode-firstorder.lisp — closed-form solutions of separable first-order ODEs,
; certified by the rational-function integrator's own FTC certificate.
;
; y' = f(x) g(y) separates to INT (1/g(y)) dy = INT f(x) dx + C.  Each side is a
; rational-function integral; integrate-rational returns the antiderivative AND
; verifies it by differentiating back over Q.  So the implicit solution G(y)=F(x)+C
; is certified whenever both antiderivatives are: differentiating implicitly gives
; G'(y) y' = F'(x), i.e. (1/g(y)) y' = f(x), i.e. y' = f(x) g(y) -- the original ODE.
; `must` raises on failure.

(import "cas/ode1.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'ode1-check-failed)))
(define (chk label r expect)
  (display "    ") (display label) (display "  ->  ") (display (ode1-result->string r)) (newline)
  (must "certified" (ode1-certified? r))
  (must "matches expected closed form" (equal? (ode1-result->string r) expect)))

(display "Separable first-order ODEs (closed form, certified)") (newline) (newline)

(display "1. autonomous y' = g(y)") (newline)
(chk "y' = y"       (solve-autonomous (list 0 1) (list 1))     "log(y) = x + C")
(chk "y' = y^2"     (solve-autonomous (list 0 0 1) (list 1))   "(-1)/(y) = x + C")
(chk "y' = 1 + y^2" (solve-autonomous (list 1 0 1) (list 1))   "arctan(y) = x + C")
(newline)

(display "2. separable y' = f(x) g(y)") (newline)
(chk "y' = x*y"        (solve-separable (list 0 1) (list 1) (list 0 1) (list 1))     "log(y) = 1/2*x^2 + C")
(chk "y' = x/y"        (solve-separable (list 0 1) (list 1) (list 1) (list 0 1))     "1/2*y^2 = 1/2*x^2 + C")
(chk "y' = (1+y^2)/x"  (solve-separable (list 1) (list 0 1) (list 1 0 1) (list 1))   "arctan(y) = log(x) + C")
(chk "y' = y/x^2"      (solve-separable (list 1) (list 0 0 1) (list 0 1) (list 1))   "log(y) = (-1)/(x) + C")
(newline)

(display "all first-order ODE checks passed.") (newline)
