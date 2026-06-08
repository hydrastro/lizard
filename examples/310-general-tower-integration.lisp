; RUNG 5, going deeper: a GENERAL integrator for the iterated exponential tower (itexp.lisp), beyond the single
; full-product identity.  Given an arbitrary element B = sum of monomials c(x) E_1^{a_1}...E_n^{a_n}, find an
; antiderivative E in the tower by undetermined coefficients and certify it by differentiating in the tower
; (docs/TRAGER_ROADMAP.md, Rung 5).
;
; ie-deriv sends each answer monomial to a small set of monomials, acting linearly on the finite monomial
; support; positing E over the candidate support (B's monomials plus their prefix-lowered "one-derivative-down"
; forms) and matching d/dx(E) = B gives an exact linear system, solved with Gauss-Jordan and confirmed by the
; certificate -- so a spurious candidate simply receives coefficient 0.
(import "cas/itexpsolve.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))
(define (mono c v) (cons (rat-from-poly (list c)) v))

(display "General integration in the iterated-exponential tower, beyond the full-product identity.") (newline) (newline)

(display "INT exp(x) dx = exp(x)  (E_1):") (newline)
(define B1 (ie-deriv 2 (list (mono 1 (list 1 0)))))   ; d/dx(E_1) = E_1
(define S1 (ies-solve 2 B1))
(chk "INT E_1 = E_1" (if (equal? S1 (quote none)) #f (ie-certify 2 S1 B1)))

(display "INT (exp(x) + exp(x)exp(exp x)) dx = exp(x) + exp(exp x)  (E_1 + E_2):") (newline)
(define ans2 (list (mono 1 (list 1 0)) (mono 1 (list 0 1))))   ; E_1 + E_2
(define B2 (ie-deriv 2 ans2))
(display "  the integrand d/dx(E_1 + E_2) has monomials ") (display (ies-vecs B2)) (display "  (E_1 and E_1 E_2)") (newline)
(define S2 (ies-solve 2 B2))
(display "  ies-solve recovers ") (display (ies-vecs S2)) (display "  (E_1 and E_2)") (newline)
(chk "INT (E_1 + E_1 E_2) = E_1 + E_2 (a genuine two-term answer)" (if (equal? S2 (quote none)) #f (ie-certify 2 S2 B2)))

(display "INT (exp(x)exp(exp x) + exp(x)^2 exp(exp x)) dx = exp(x) exp(exp x)  (E_1 E_2):") (newline)
(define B3 (ie-deriv 2 (list (mono 1 (list 1 1)))))   ; d/dx(E_1 E_2)
(define S3 (ies-solve 2 B3))
(chk "INT (E_1 E_2 + E_1^2 E_2) = E_1 E_2" (if (equal? S3 (quote none)) #f (ie-certify 2 S3 B3)))

(display "the full-product identity recovered by the general solver (depth 3):") (newline)
(define B4 (ie-deriv 3 (list (mono 1 (list 0 0 1)))))   ; d/dx(E_3) = E_1 E_2 E_3
(chk "INT (E_1 E_2 E_3) = E_3 via the general solver" (if (equal? (ies-solve 3 B4) (quote none)) #f (ie-certify 3 (ies-solve 3 B4) B4)))

(newline)
(display "General tower integration: arbitrary elements of the iterated-exponential tower integrated by") (newline)
(display "undetermined coefficients over a monomial support, each answer certified by differentiation.") (newline)
