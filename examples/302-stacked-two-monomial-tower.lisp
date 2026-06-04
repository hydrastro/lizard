; RUNG 5: the first genuinely STACKED two-monomial tower.  An element lives in Q(x)(theta)(t) with TWO
; independent transcendental monomials: theta = exp(x) (theta' = theta) and t = log(x) (t' = 1/x) -- the
; structure where the recursive Risch algorithm really operates, one monomial stacked over another over the
; rational base (docs/TRAGER_ROADMAP.md, Rung 5).
;
; An element is a t-polynomial of theta-polynomials of rational functions: sum_{j,k} c_{j,k}(x) theta^k t^j.
; The derivation, from theta' = theta and t' = 1/x, is
;   d/dx (c theta^k t^j) = (c' + k c) theta^k t^j  +  (j c / x) theta^k t^{j-1}.
; The system integrates genuinely mixed two-monomial integrands and decides elementarity within the bounded
; ansatz, the answer found by undetermined coefficients and certified by differentiating in the tower.
(import "cas/twotower.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "A stacked two-monomial tower: theta = exp(x) and t = log(x), in Q(x)(theta)(t).") (newline) (newline)

(define one (rat-one))
(define zero (rat-zero))

(display "the integral INT (exp(x) log x + exp(x)/x) dx = exp(x) log(x):") (newline)
(display "  -- each summand alone is non-elementary (an exponential-integral Ei), but the combination is elementary --") (newline)
(define E (list (list) (list zero one)))   ; E = theta * t  (t-degree 1, theta^1, coefficient 1)
(define B (tt-deriv E))
(display "  the integrand d/dx(exp(x) log x): t^1 part = ") (display (tt-tcoeff B 1)) (display "  (= exp(x)),  t^0 part = ") (display (tt-tcoeff B 0)) (display "  (= exp(x)/x)") (newline)
(chk "the constructive identity INT B dx = exp(x) log(x) certifies in the tower" (tt-certify E B))

(display "solving the tower system -- recover the answer from the integrand:") (newline)
(define Esol (tt-solve B 1 1 0))
(display "  tt-solve recovers E = ") (display Esol) (display "  = exp(x) log(x)") (newline)
(chk "the solver recovers E = exp(x) log(x)" (if (equal? Esol (quote none)) #f (tt-eq? Esol E)))
(define res (tt-integrate B))
(chk "the top-level integrator decides the combined integral is elementary" (equal? (car res) (quote elementary)))

(display "the base exponential alone, INT exp(x) dx = exp(x):") (newline)
(define Ee (list (list zero one)))   ; theta
(define Be (tt-deriv Ee))
(display "  d/dx(exp(x)) = ") (display (tt-tcoeff Be 0)) (display "  (= exp(x))") (newline)
(chk "the solver recovers E = exp(x)" (if (equal? (tt-solve Be 0 1 0) (quote none)) #f (tt-eq? (tt-solve Be 0 1 0) Ee)))

(newline)
(display "A stacked tower with two transcendental monomials: the derivation couples theta (in place) and t (down") (newline)
(display "in degree, weight 1/x), and the integrator finds the elementary combination exp(x) log(x), certified.") (newline)
