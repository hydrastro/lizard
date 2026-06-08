; 166-ode-series.lisp — power-series solutions of linear ODEs with polynomial
; coefficients, by undetermined Taylor coefficients, certified by substitution.
;
; For p_0 y + p_1 y' + ... + p_m y^(m) = r at an ordinary point, the Taylor
; coefficients of y satisfy a one-step recurrence; given y(0),...,y^(m-1)(0) the
; rest follow.  The solution is certified by plugging it back: the residual
; sum_i p_i y^(i) - r must vanish to the truncation order.  We recover the familiar
; closed forms and then solve the Airy equation, whose solution is not elementary.
; Each ODE is given as a list of coefficient polynomials (p_0 ... p_m).  `must` raises.

(import "cas/ode.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'ode-check-failed)))
(define N 9)

(display "Power-series solutions of linear ODEs (certified by substitution)") (newline) (newline)

(display "1. y' = y,  y(0)=1") (newline)
(define expo (list (list -1) (list 1)))
(define y1 (ode-series expo (list) (list 1) N))
(must "solution is the exp series" (equal? y1 (exp-series N)))
(must "residual y'-y vanishes"     (ode-ok? expo (list) y1 N))
(newline)

(display "2. y'' + y = 0") (newline)
(define osc (list (list 1) (list 0) (list 1)))
(must "y(0)=0,y'(0)=1 gives sin"   (equal? (ode-series osc (list) (list 0 1) N) (sin-series N)))
(must "y(0)=1,y'(0)=0 gives cos"   (equal? (ode-series osc (list) (list 1 0) N) (cos-series N)))
(must "residual y''+y vanishes (sin)" (ode-ok? osc (list) (ode-series osc (list) (list 0 1) N) N))
(newline)

(display "3. (1-x) y' = y,  y(0)=1") (newline)
(define geo (list (list -1) (list 1 -1)))
(define y3 (ode-series geo (list) (list 1) N))
(must "solution is 1/(1-x)"        (equal? y3 (geometric-series N)))
(must "residual (1-x)y'-y vanishes" (ode-ok? geo (list) y3 N))
(newline)

(display "4. Airy equation  y'' = x y,  y(0)=1, y'(0)=0  (non-elementary solution)") (newline)
(define airy (list (list 0 -1) (list 0) (list 1)))
(define y4 (ode-series airy (list) (list 1 0) N))
(display "    series: ") (display y4) (newline)
(must "coefficients match Airy (c3=1/6, c6=1/180)" (and (= (ser-coeff y4 3) (/ 1 6)) (= (ser-coeff y4 6) (/ 1 180)) (= (ser-coeff y4 1) 0) (= (ser-coeff y4 2) 0)))
(must "residual y''-xy vanishes"   (ode-ok? airy (list) y4 N))
(newline)

(display "5. Hermite-type  y'' - 2x y' + 4 y = 0,  y(0)=1, y'(0)=0  (degree-2 polynomial solution)") (newline)
(define herm (list (list 4) (list 0 -2) (list 1)))
(define y5 (ode-series herm (list) (list 1 0) N))
(display "    series: ") (display y5) (newline)
(must "solution is the polynomial 1 - 2x^2 (Hermite H2 up to scale)" (and (= (ser-coeff y5 0) 1) (= (ser-coeff y5 2) -2) (= (ser-coeff y5 4) 0) (= (ser-coeff y5 6) 0)))
(must "residual vanishes"          (ode-ok? herm (list) y5 N))
(newline)

(display "all ODE series checks passed.") (newline)
