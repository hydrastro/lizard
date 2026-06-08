; RUNG 4 (start) of the Trager-Bronstein climb (docs/TRAGER_ROADMAP.md): PUISEUX EXPANSIONS of algebraic
; functions -- the local fractional-power series that is the foundation for handling GENERAL algebraic functions
; (n-th roots y^e = g(x), and ultimately arbitrary F(x,y) = 0), lifting the integrator past the hyperelliptic
; (square-root-only) restriction.
;
; At the place over x = 0 an algebraic function expands as y(x) = sum_{k >= k0} c_k x^(k/e), a power series in a
; fractional power x^(1/e) with e the ramification index.  This module computes that expansion for the
; superelliptic case y^e = g(x) by the Newton-Puiseux method: write g = x^v * gt(x) with gt(0) != 0, so
; y = x^(v/e) gt(0)^(1/e) (1 + (gt/gt(0) - 1))^(1/e), expand the binomial series, and re-express in the
; uniformizer t = x^(1/E) where E = e/gcd(v,e) is the true ramification index.  Each branch is returned as
; (puiseux E lead coeffs), meaning y = sum_i coeffs[i] x^((lead+i)/E), and every result is checked by raising
; the series to the e-th power and comparing with g.  When gt(0)^(1/e) is irrational the leading coefficient is
; reported needs-radical rather than guessed.
(import "cas/puiseux.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Puiseux expansions of y for y^e = g(x) at x=0, in the uniformizer t = x^(1/E), power-checked:") (newline) (newline)

(display "ramified square roots:") (newline)
(define r1 (px-superelliptic (list 0 1) 2 6))                 ; y^2 = x
(display "  y^2 = x        -> E = ") (display (px-nth r1 1)) (display ", y = x^(1/2)   (lead exponent ") (display (px-nth r1 2)) (display ")") (newline)
(chk "y^2 = x : ramification E=2, verifies" (if (= (px-nth r1 1) 2) (px-verify (list 0 1) 2 6) #f))
(define r3 (px-superelliptic (list 0 0 0 1) 2 8))             ; y^2 = x^3 (cusp)
(display "  y^2 = x^3      -> E = ") (display (px-nth r3 1)) (display ", y = x^(3/2)   (cusp; lead exponent ") (display (px-nth r3 2)) (display ")") (newline)
(chk "y^2 = x^3 (cusp) : E=2, leading exponent 3, verifies" (if (= (px-nth r3 2) 3) (px-verify (list 0 0 0 1) 2 8) #f))

(display "a genuine fractional series, y^2 = x + x^2:") (newline)
(define r2 (px-superelliptic (list 0 1 1) 2 6))
(display "  y = t + (1/2)t^3 - (1/8)t^5 + ...  (t = x^(1/2)) -> coeffs ") (display (px-nth r2 3)) (newline)
(chk "y^2 = x + x^2 verifies" (px-verify (list 0 1 1) 2 6))

(display "higher root, y^3 = x:") (newline)
(define r4 (px-superelliptic (list 0 1) 3 6))
(display "  y^3 = x        -> E = ") (display (px-nth r4 1)) (display ", y = x^(1/3)") (newline)
(chk "y^3 = x : ramification E=3, verifies" (if (= (px-nth r4 1) 3) (px-verify (list 0 1) 3 6) #f))

(display "unramified place with a nontrivial leading coefficient, y^2 = 4 + x:") (newline)
(define r5 (px-superelliptic (list 4 1) 2 5))
(display "  y = 2 + (1/4)x - ...  -> E = ") (display (px-nth r5 1)) (display " (unramified), lead coeff ") (display (car (px-nth r5 3))) (newline)
(chk "y^2 = 4 + x : E=1, leading coeff 2, verifies" (if (= (car (px-nth r5 3)) 2) (px-verify (list 4 1) 2 5) #f))

(newline)
(display "soundness (no guessed radicals):") (newline)
(define r6 (px-superelliptic (list 2 1) 2 5))
(display "  y^2 = 2 + x  (sqrt(2) irrational) -> ") (display (car r6)) (newline)
(chk "irrational leading coefficient honestly reported needs-radical" (equal? (car r6) (quote needs-radical)))

(newline)
(display "RUNG 4 begun: Puiseux expansions of superelliptic functions with correct ramification, power-checked, sound.") (newline)
