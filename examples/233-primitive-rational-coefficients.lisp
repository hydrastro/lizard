; 233-primitive-rational-coefficients.lisp -- the primitive (logarithmic) polynomial integration
; problem with RATIONAL coefficients: INT (sum_{k=0..n} a_k(x) (log x)^k) dx for a_k in Q(x).  This
; generalizes example 232 (polynomial coefficients) to the case where intermediate logarithms must
; be absorbed into higher coefficients.
;
; Matching the coefficient of (log x)^k in the derivative of sum b_k (log x)^k gives
; a_k = b_k' + (k+1) b_{k+1}/x.  Top down, b_k is the rational antiderivative of
; R_k = a_k - (k+1) b_{k+1}/x, which exists exactly when R_k integrates to rational + lambda_k log x
; with no other logarithm; the free constant of b_{k+1} is fixed one level up by lambda_k/(k+1), so
; the log x produced at level k is absorbed into the (log x)^{k+1} coefficient.  A logarithm of
; anything but x, or an algebraic residue, is a genuine non-elementary obstruction and is reported.
; Each answer is certified by differentiating in the tower.  `must` raises on failure.

(import "cas/primint.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'primint-check-failed)))
(define (R n d) (rde-rmake n d))                 ; rational n/d, coefficients are (num.den)
(define Z (R (list 0) (list 1)))

(display "Primitive-case polynomial integration with rational coefficients (theta = log x)") (newline) (newline)

(display "1. logarithm absorption: INT (1/x) log x dx = (1/2)(log x)^2") (newline)
(display "    coefficients (b_0 b_1 b_2) = ") (display (car (cdr (int-prim-poly (list Z (R (list 1) (list 0 1))))))) (newline)
(display "    i.e. 0, 0, 1/2 -- the log x produced at level 1 became the (log x)^2 coefficient") (newline)
(must "certified" (int-prim-poly-verify (list Z (R (list 1) (list 0 1)))))
(must "INT (2/x) log x dx = (log x)^2"        (int-prim-poly-verify (list Z (R (list 2) (list 0 1)))))
(must "INT (log x)^2 / x dx = (1/3)(log x)^3" (int-prim-poly-verify (list Z Z (R (list 1) (list 0 1)))))
(newline)

(display "2. rational coefficients with no absorption") (newline)
(display "    INT log x / x^2 dx = -1/x - (log x)/x;  (b_0 b_1) = ")
(display (car (cdr (int-prim-poly (list Z (R (list 1) (list 0 0 1))))))) (newline)
(must "INT log x / x^2 dx certified" (int-prim-poly-verify (list Z (R (list 1) (list 0 0 1)))))
(must "INT log x / x^3 dx certified" (int-prim-poly-verify (list Z (R (list 1) (list 0 0 0 1)))))
(newline)

(display "3. subsumes the polynomial case and base-field log integrals") (newline)
(must "INT x log x dx certified (agrees with logpoly)" (int-prim-poly-verify (list Z (R (list 0 1) (list 1)))))
(must "INT 1/x dx = log x"                              (int-prim-poly-verify (list (R (list 1) (list 0 1)))))
(must "INT 1/x^2 dx = -1/x"                             (int-prim-poly-verify (list (R (list 1) (list 0 0 1)))))
(newline)

(display "4. non-elementary obstructions, correctly reported") (newline)
(must "INT log x/(x^2+1) dx declined (algebraic residue)" (not (int-prim-poly-elementary? (list Z (R (list 1) (list 1 0 1))))))
(must "INT log x/(x-1) dx declined (logarithm of x-1 cannot be absorbed)" (not (int-prim-poly-elementary? (list Z (R (list 1) (list -1 1))))))
(newline)

(display "all primitive rational-coefficient checks passed.") (newline)
