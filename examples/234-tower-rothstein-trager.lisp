; 234-tower-rothstein-trager.lisp -- Rothstein-Trager IN THE TOWER, completing the proper
; (fractional) case of integration over a primitive monomial theta = log x with x-dependent
; coefficients.  This is the piece that decides non-elementarity for the logarithmic direction.
;
; After Hermite reduction leaves a squarefree remainder a/d (d squarefree in theta), the primitive-
; case residue criterion says INT a/d is elementary over Q(x)(theta) iff R(z) = Res_theta(d, a - z Dd)
; has constant roots (D the tower derivation), and then INT a/d = sum_i c_i log gcd_theta(d, a - c_i Dd).
; R(z) is built by a resultant over Q(x) interpolated in z; made monic in z it has constant
; coefficients exactly when the integral is elementary.  Rational roots give the logarithms, a
; non-constant coefficient is a genuine obstruction, and constant-but-irrational roots are a deferred
; RootSum.  Every answer is checked by differentiating in the tower.  `must` raises on failure.

(import "cas/towerrt.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'towerrt-check-failed)))
(define LOG (list 'log))
(define (residues r) (map car (car (cdr (cdr r)))))   ; the list of residues c_i found

(display "Rothstein-Trager in the tower: the proper case of INT R(log x) with x-dependent coeffs") (newline) (newline)

(display "1. two distinct rational residues (beyond a single new logarithm)") (newline)
(display "    INT ((3/x+1) log x - (3x+1)) / ((log x)^2 - x^2) dx = 2 log(log x + x) + log(log x - x)") (newline)
(define a1 (list (rat-from-poly (list -1 -3)) (rat-make (list 3 1) (list 0 1))))
(define d1 (list (rat-from-poly (list 0 0 -1)) (rat-zero) (rat-one)))
(display "    residues found = ") (display (residues (int-prim-rational a1 d1 LOG))) (display "  (the coefficients 2 and 1)") (newline)
(must "elementary"  (int-prim-rational-elementary? a1 d1 LOG))
(must "certified by differentiation in the tower" (int-prim-rational-verify a1 d1 LOG))
(newline)

(display "2. another constructed multi-log integrand, residues 3 and 1") (newline)
(define v1 (list (rat-make (list -1) (list 0 1)) (rat-one)))   ; log x - 1/x
(define v2 (list (rat-make (list 1) (list 0 1)) (rat-one)))    ; log x + 1/x
(define a2 (rfpoly-add (rfpoly-mul (rfpoly-cscale (rat-from-poly (list 3)) (Drf v1 LOG)) v2) (rfpoly-mul (Drf v2 LOG) v1)))
(define d2 (rfpoly-mul v1 v2))
(display "    residues found = ") (display (residues (int-prim-rational a2 d2 LOG))) (newline)
(must "certified" (int-prim-rational-verify a2 d2 LOG))
(newline)

(display "3. the proper path recovers the basic new-log case") (newline)
(define a3 (list (rat-make (list 1) (list 0 1)))) (define d3 (list (rat-zero) (rat-one)))   ; (1/x)/theta
(must "INT 1/(x log x) dx = log(log x), certified" (int-prim-rational-verify a3 d3 LOG))
(newline)

(display "4. non-elementarity, decided by the residue criterion") (newline)
(define a4 (list (rat-zero) (rat-one)))                              ; theta
(define d4 (list (rat-from-poly (list 0 1)) (rat-zero) (rat-one)))   ; theta^2 + x
(must "INT (log x)/((log x)^2 + x) dx reported non-elementary (residues depend on x)" (not (int-prim-rational-elementary? a4 d4 LOG)))
(newline)

(display "5. algebraic residues are deferred, never falsely certified") (newline)
(define a5 (list (rat-make (list 1) (list 0 1))))
(define d5 (list (rat-from-poly (list -2)) (rat-zero) (rat-one)))    ; theta^2 - 2
(must "INT (1/x)/((log x)^2 - 2) dx returns 'algebraic" (equal? (car (int-prim-rational a5 d5 LOG)) 'algebraic))
(must "and is not falsely certified" (not (int-prim-rational-verify a5 d5 LOG)))
(newline)

(display "all tower Rothstein-Trager checks passed.") (newline)
