; RUNG 5: a GENERAL integrator for the iterated LOGARITHM tower (itlog.lisp), the reciprocal-mirror counterpart
; of the iterated-exponential solver (itexpsolve.lisp).  Given an arbitrary Laurent element B = sum of monomials
; c(x) L_1^{a_1}...L_n^{a_n} (integer exponents), find an antiderivative in the tower by undetermined
; coefficients and certify it by differentiating in the tower (docs/TRAGER_ROADMAP.md, Rung 5).
;
; il-deriv lowers the first k exponents (scaled by a_k/x), so the candidate ANSWER monomials are B's monomials
; with one-step prefixes RAISED (the inverse); positing E over that support and matching d/dx(E) = B gives an
; exact linear system, solved with Gauss-Jordan and confirmed by the certificate.
(import "cas/itlogsolve.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))
(define (mono c v) (cons (rat-from-poly (list c)) v))

(display "General integration in the iterated-logarithm tower, beyond the structured nested-log identity.") (newline) (newline)

(display "INT (2 log(log x)/(x log x)) dx = (log(log x))^2:") (newline)
(define B1 (il-deriv 2 (list (mono 1 (list 0 2)))))   ; d/dx(L_2^2)
(define S1 (ils-solve 2 B1))
(display "  ils-solve recovers exponent vector ") (display (ils-vecs S1)) (display "  (= L_2^2)") (newline)
(chk "INT (2 log log x/(x log x)) = (log log x)^2" (if (equal? S1 (quote none)) #f (il-certify 2 S1 B1)))

(display "INT (1/x + 1/(x log x)) dx = log x + log(log x)  (a genuine two-term answer):") (newline)
(define ans2 (list (mono 1 (list 1 0)) (mono 1 (list 0 1))))   ; L_1 + L_2
(define B2 (il-deriv 2 ans2))
(display "  the integrand d/dx(L_1 + L_2) has monomials ") (display (ils-vecs B2)) (display "  (1/x and 1/(x L_1))") (newline)
(define S2 (ils-solve 2 B2))
(display "  ils-solve recovers ") (display (ils-vecs S2)) (display "  (log x and log log x)") (newline)
(chk "INT (1/x + 1/(x log x)) = log x + log log x" (if (equal? S2 (quote none)) #f (il-certify 2 S2 B2)))

(display "the structured nested-log identity recovered by the general solver (depth 3):") (newline)
(define B3 (il-deriv 3 (il-top 3)))   ; d/dx(L_3) = 1/(x L_1 L_2)
(chk "INT 1/(x log x log log x) = log log log x via the general solver" (if (equal? (ils-solve 3 B3) (quote none)) #f (il-certify 3 (ils-solve 3 B3) B3)))

(newline)
(display "General log-tower integration: arbitrary Laurent elements of the iterated-logarithm tower integrated") (newline)
(display "by undetermined coefficients over a monomial support, each answer certified by differentiation.") (newline)
