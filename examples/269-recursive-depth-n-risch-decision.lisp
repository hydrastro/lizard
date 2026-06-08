; THE recursive Risch decision driver, on a uniform tower of ARBITRARY depth.  Every prior integrator was
; level-specific (Q(x) at level 0, one extension at level 1, two at level 2); nothing recursed.  ntower.lisp
; gives one representation that nests to any depth -- coefficients at level n are themselves tower elements at
; level n-1, bottoming out at the trusted base field Q(x) -- and ntrisch.lisp gives ONE integrator,
; nt-integrate(L, specs, p), that runs the same code at every level: it splits the integrand into polynomial
; and proper parts, drives each through the Risch machinery (the Risch differential equation, solved by
; recursion into the level below), and returns EITHER a certified antiderivative OR a proof of non-elementarity.
;
; The demonstration is a single uniform procedure deciding at depths 1, 2, 3 and 4 -- the depth-3 and depth-4
; cases are unreachable by any level-specific code and exist only because the recursion closes on itself.  Every
; elementary verdict is re-verified by differentiating the answer at its level (the soundness invariant: the
; driver never claims an antiderivative it cannot certify), and every non-elementary verdict is a genuine proof
; that the Risch differential equation has no solution.
(import "cas/ntrisch.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))

; ---- build the tower of iterated exponentials: theta_{k+1} = exp(theta_k), theta_1 = e^x ----
(define t1 (nt-monomial 1 (rat-one) 1))                                  ; theta1 = e^x  (a level-1 element)
(define s1 (list (list (quote exp) (rat-one))))                          ; D theta1 = 1 * theta1
(define s2 (list (list (quote exp) (rat-one)) (list (quote exp) t1)))    ; D theta2 = theta1 * theta2
(define s3 (list (list (quote exp) (rat-one)) (list (quote exp) t1)
                 (list (quote exp) (nt-monomial 2 (nt-lift 1 (rat-one)) 1))))                 ; D theta3 = theta2 * theta3
(define s4 (list (list (quote exp) (rat-one)) (list (quote exp) t1)
                 (list (quote exp) (nt-monomial 2 (nt-lift 1 (rat-one)) 1))
                 (list (quote exp) (nt-monomial 3 (nt-lift 2 (rat-one)) 1))))                 ; D theta4 = theta3 * theta4

(display "One recursive driver nt-integrate(L, specs, p) -- same code at every depth:") (newline) (newline)

; ===== level 0 (base field Q(x)) =====
(must "L0  INT 2x dx = x^2                                  certified" (nt-integrate-verify 0 (quote ()) (rat-from-poly (list 0 2))))
; ===== level 1 (theta1 = e^x) =====
(must "L1  INT e^x dx = e^x                                 certified" (nt-integrate-verify 1 s1 (nt-monomial 1 (rat-one) 1)))
(must "L1  INT 2x e^x dx = (2x-2)e^x   (Risch DE)           certified" (nt-integrate-verify 1 s1 (nt-monomial 1 (rat-from-poly (list 0 2)) 1)))
; ===== level 2 (theta2 = exp(e^x)) =====
(must "L2  INT e^x exp(e^x) dx = exp(e^x)                   certified" (nt-integrate-verify 2 s2 (nt-monomial 2 t1 1)))
(must "L2  INT exp(e^x) dx                          PROVEN non-elementary" (equal? (car (nt-integrate 2 s2 (nt-monomial 2 (nt-lift 1 (rat-one)) 1))) (quote non-elementary)))
; ===== level 3 (theta3 = exp(exp(e^x))) -- unreachable without recursion =====
(must "L3  INT exp(e^x) exp(exp(e^x)) dx = exp(exp(e^x))    certified  [DEPTH 3]" (nt-integrate-verify 3 s3 (nt-monomial 3 (nt-monomial 2 (nt-lift 1 (rat-one)) 1) 1)))
(must "L3  INT exp(exp(e^x)) dx                     PROVEN non-elementary  [DEPTH 3]" (equal? (car (nt-integrate 3 s3 (nt-monomial 3 (nt-lift 2 (rat-one)) 1))) (quote non-elementary)))
; ===== level 4 (theta4 = exp(exp(exp(e^x)))) =====
(must "L4  INT exp(exp(e^x)) exp(exp(exp(e^x))) dx          certified  [DEPTH 4]" (nt-integrate-verify 4 s4 (nt-monomial 4 (nt-monomial 3 (nt-lift 2 (rat-one)) 1) 1)))
(must "L4  INT exp(exp(exp(e^x))) dx                PROVEN non-elementary  [DEPTH 4]" (equal? (car (nt-integrate 4 s4 (nt-monomial 4 (nt-lift 3 (rat-one)) 1))) (quote non-elementary)))

(newline)
; ===== soundness invariant =====
(display "soundness: every 'elementary verdict is differentiation-certified at its level.") (newline)
(must "  no false positive on the depth-3 elementary case" (nt-integrate-verify 3 s3 (nt-monomial 3 (nt-monomial 2 (nt-lift 1 (rat-one)) 1) 1)))

(newline)
; ===== primitive (logarithmic) tower: same recursive driver, theta = log x (D theta = 1/x) =====
; The primitive monomial uses the integration-by-parts recurrence b_k = INT(a_k - (k+1) b_{k+1} darg) at the
; level below -- the SAME recursion, a different chain rule.  These were 'not-handled before this driver wired
; the primitive polynomial part into the recursion.
(define sLOG (list (list (quote prim) (rat-make (list 1) (list 0 1)))))      ; theta = log x, D theta = 1/x
(display "primitive (logarithmic) tower, theta = log x -- same recursive driver:") (newline)
(must "Llog INT log(x) dx = x log x - x                       certified" (nt-integrate-verify 1 sLOG (nt-monomial 1 (rat-one) 1)))
(must "Llog INT (log x)^2 dx = x(log x)^2 - 2x log x + 2x     certified" (nt-integrate-verify 1 sLOG (nt-monomial 1 (rat-one) 2)))
(must "Llog INT (log x)^3 dx                                  certified" (nt-integrate-verify 1 sLOG (nt-monomial 1 (rat-one) 3)))
; a genuinely mixed polynomial in log x with an x-dependent constant term
(must "Llog INT (x + 2 log x + (log x)^2) dx                  certified"
      (nt-integrate-verify 1 sLOG (nt-add 1 (nt-add 1 (list (rat-from-poly (list 0 1))) (nt-monomial 1 (nt-cscale-scalar 0 2 (rat-one)) 1)) (nt-monomial 1 (rat-one) 2))))
; primitive monomial AT DEPTH 2: D theta2 = theta1 = e^x (a genuine level-1 derivative)
(define sP2 (list (list (quote exp) (rat-one)) (list (quote prim) (nt-monomial 1 (rat-one) 1))))
(must "Llog INT theta2 dx at DEPTH 2 (D theta2 = e^x)         certified" (nt-integrate-verify 2 sP2 (nt-monomial 2 (nt-lift 1 (rat-one)) 1)))

(newline)
(display "recursive Risch decision procedure: one driver, arbitrary depth, exponential AND logarithmic towers, certified answers or proofs of non-elementarity.") (newline)
