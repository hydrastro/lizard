; 236-complete-log-integration.lisp -- the COMPLETE integrator for a rational function of a single
; logarithm: INT A/D dx with A, D polynomials in theta = log x over Q(x).  This is the capstone of
; the logarithmic direction, integrating an arbitrary such A/D (rational residues) in one call.
;
; Polynomial division in theta gives A = Q D + Rem, so INT A/D = INT Q + INT Rem/D: the polynomial
; part Q goes to the primitive-case polynomial integrator (primint.lisp, with logarithm absorption),
; the proper part Rem/D to Hermite reduction plus tower Rothstein-Trager (towerrt.lisp).  Because the
; split A = Q D + Rem is an exact identity and differentiation is linear, certifying each part in its
; own module certifies the whole; the integral is elementary exactly when both parts are.  `must`
; raises on failure.

(import "cas/intlog.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'intlog-check-failed)))

(display "Complete integration of rational functions of log x") (newline) (newline)

(display "1. a pure polynomial in log x") (newline)
(must "INT (log x)^3 dx certified" (int-log-rational-verify (rfpoly-monomial (rat-one) 3) (rf-const (rat-one))))
(newline)

(display "2. a pure proper part with two distinct residues") (newline)
(display "    INT ((3/x+1) log x - (3x+1)) / ((log x)^2 - x^2) dx = 2 log(log x + x) + log(log x - x)") (newline)
(define a2 (list (rat-from-poly (list -1 -3)) (rat-make (list 3 1) (list 0 1))))
(define d2 (list (rat-from-poly (list 0 0 -1)) (rat-zero) (rat-one)))
(must "certified" (int-log-rational-verify a2 d2))
(newline)

(display "3. a MIXED integrand: polynomial part AND proper part, in one call") (newline)
(display "    A/D with A = (log x)^2 (D) + [the two-residue numerator], D = (log x)^2 - x^2") (newline)
(define A3 (rfpoly-add (rfpoly-mul (rfpoly-monomial (rat-one) 2) d2) a2))
(display "    polynomial part Q recovered = ") (display (car (cdr (int-log-rational A3 d2)))) (display "  (= (log x)^2)") (newline)
(must "mixed case certified"     (int-log-rational-verify A3 d2))
(must "mixed case elementary"    (int-log-rational-elementary? A3 d2))
(newline)

(display "4. polynomial part plus a logarithm-absorbing proper part") (newline)
(display "    INT ((log x)^3 + 1/x) / log x dx") (newline)
(define D4 (list (rat-zero) (rat-one)))
(define A4 (rfpoly-add (rfpoly-monomial (rat-one) 3) (rf-const (rat-make (list 1) (list 0 1)))))
(must "certified" (int-log-rational-verify A4 D4))
(newline)

(display "5. non-elementarity of either part propagates") (newline)
(define D5 (list (rat-from-poly (list 0 1)) (rat-zero) (rat-one)))
(must "INT (log x)/((log x)^2 + x) dx reported non-elementary" (not (int-log-rational-elementary? (list (rat-zero) (rat-one)) D5)))
(newline)

(display "all complete-log-integration checks passed.") (newline)
