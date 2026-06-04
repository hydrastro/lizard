; RUNG 5: the NESTED EXPONENTIAL tower Q(x)(s1)(s2) with s1 = exp(x) and s2 = exp(exp(x)) = exp(s1) -- the
; multiplicative-tower counterpart of the nested logarithm (nestlog.lisp).  Here the second monomial's
; derivative s2' = s1' s2 = s1 s2 MULTIPLIES by the first monomial (rather than dividing, as t2' = 1/(x t1) did
; for nested logs), so the coefficient ring stays polynomial -- a two-variable polynomial ring over Q(x) -- but
; the derivation raises the s1-degree (docs/TRAGER_ROADMAP.md, Rung 5).
;
; A tower element is a polynomial in s2 with s1-polynomial coefficients over Q(x): sum_{m,k} c_{m,k}(x) s1^k s2^m.
; From s1' = s1 and s2' = s1 s2, the derivation of a term is
;   d/dx ( c s1^k s2^m ) = (c' + k c) s1^k s2^m + (m c) s1^{k+1} s2^m,
; so within a fixed s2-degree m the s1-polynomial C_m goes to ds1(C_m) + m * (s1-shift of C_m).  The s2-degree is
; preserved, making the derivation block-diagonal across s2-degrees -- which is what lets the integral be solved
; by undetermined coefficients and certified by differentiating in the tower.
(import "cas/nestexp.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "A nested-exponential tower: s1 = exp(x), s2 = exp(exp(x)), in Q(x)(s1)(s2).") (newline) (newline)

(display "the inner derivation (s1 = exp x, with s1' = s1):") (newline)
(chk "d/dx(exp x) = exp x" (rat-equal? (ne-nth (ne-ds1 (list (rat-zero) (rat-one))) 1) (rat-one)))

(display "the outer monomial s2 = exp(exp x) has derivative s2' = exp(x) exp(exp x) = s1 s2:") (newline)
(define E (list (list) (list (rat-one))))   ; E = s2  (s2-degree 1, s1-degree 0, coefficient 1)
(define B (ne-deriv E))
(display "  d/dx(exp exp x), s2^1 coefficient (an s1-polynomial) = ") (display (ne-scoeff B 1)) (display "  (= s1)") (newline)
(chk "d/dx(exp(exp x)) = exp(x) exp(exp x) = s1 s2" (ne-peq? (ne-scoeff B 1) (list (rat-zero) (rat-one))))

(display "the nested-exp integral INT exp(x) exp(exp(x)) dx = exp(exp(x)):") (newline)
(define Esol (ne-solve B 1 1))
(display "  ne-solve recovers E = ") (display Esol) (display "  = exp(exp x)") (newline)
(chk "the solver recovers E = s2 (the integral is exp(exp x))" (if (equal? Esol (quote none)) #f (ne-eq? Esol E)))
(chk "the top-level integrator returns the elementary answer" (equal? (car (ne-integrate B)) (quote elementary)))

(display "a higher case, INT 2 exp(x) (exp(exp x))^2 dx = (exp(exp x))^2:") (newline)
(define E2 (list (list) (list) (list (rat-one))))   ; s2^2
(define B2 (ne-deriv E2))
(display "  d/dx((exp exp x)^2), s2^2 coefficient = ") (display (ne-scoeff B2 2)) (display "  (= 2 s1)") (newline)
(chk "the solver recovers E = s2^2" (if (equal? (ne-solve B2 2 1) (quote none)) #f (ne-eq? (ne-solve B2 2 1) E2)))

(newline)
(display "A genuinely nested exponential tower: the outer derivative multiplies by the inner exponential and") (newline)
(display "raises its degree, and INT exp(x) exp(exp x) dx = exp(exp x) is solved and certified in the tower.") (newline)
