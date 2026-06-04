; -*- lisp -*-
; lib/cas/odeexp.lisp -- PARTICULAR SOLUTIONS of the second-order constant-coefficient linear ODE with
; EXPONENTIAL POLYNOMIAL forcing  y'' + a y' + b y = q(x) e^{r x}, including the RESONANT case where r is a root
; of the characteristic polynomial (docs/CAS.md -- summit S5, non-polynomial forcing beyond odelin2).
;
; Substituting y = u(x) e^{r x} gives y' = (u' + r u) e^{rx} and y'' = (u'' + 2r u' + r^2 u) e^{rx}, so
;     L(y) = e^{rx} [ u'' + (2r + a) u' + (r^2 + a r + b) u ].
; Dividing by e^{rx}, a polynomial particular solution u solves the constant-coefficient equation
;     u'' + (2r + a) u' + (r^2 + a r + b) u = q(x),
; which is exactly the problem odelin2 already solves by an exact linear system over Q.  The constant coefficient
; r^2 + a r + b is the characteristic polynomial evaluated at r: when it is nonzero the ansatz degree equals deg q,
; and when r is a characteristic ROOT (RESONANCE) the constant term vanishes and odelin2's b = 0 branch raises the
; ansatz degree automatically -- so resonance is handled with no special casing.  The full solution is y = u e^{rx},
; and it is CERTIFIED by symbolic differentiation: writing the derivative of (polynomial)*e^{rx} in closed form,
; the module checks that y'' + a y' + b y - q e^{rx} is identically zero (the e^{rx} factors out and a polynomial
; identity remains).  An inconsistent system is reported honestly as 'no-solution.
;
; Public (a, b, r rational constants; q a polynomial coefficient list low->high; a "solution" is the polynomial u
; with the meaning y = u(x) e^{r x}):
;   oef-char a b r            -> the characteristic value r^2 + a r + b
;   oef-resonant? a b r       -> #t iff r is a root of the characteristic polynomial (resonance)
;   oef-u a b r q             -> the polynomial u with (u e^{rx}) solving the ODE, or 'no-solution
;   oef-certify a b r q u     -> #t iff y = u e^{rx} satisfies y'' + a y' + b y = q e^{rx} identically
;   oef-solve a b r q         -> (list 'solution-times-exp u r) certified | (list 'no-solution)
;
; Verified: y'' - y = e^x (r = 1 resonant) gives u = x/2, so y = (x/2) e^x; y'' + y = e^x (r = 1 non-resonant)
; gives u = 1/2; y'' - 3y' + 2y = e^{3x} gives u = 1/2 (r = 3 non-resonant); the resonant y'' - 2y' + y = e^x
; (double root r = 1) gives u = x^2/2; every returned u passes the differentiation certificate.
;
; Builds on poly.lisp and odelin2.lisp.

(import "cas/poly.lisp")
(import "cas/odelin2.lisp")

; ----- the characteristic value and the resonance test -----
(define (oef-char a b r) (+ (+ (* r r) (* a r)) b))
(define (oef-resonant? a b r) (= (oef-char a b r) 0))

; ----- reduce to the polynomial equation u'' + (2r+a) u' + (r^2+ar+b) u = q and solve via odelin2 -----
(define (oef-u a b r q) (o2-particular (+ (* 2 r) a) (oef-char a b r) q))

; ----- certificate: y = u e^{rx}, so y' = (u' + r u) e^{rx}, y'' = (u'' + 2r u' + r^2 u) e^{rx}.
; y'' + a y' + b y - q e^{rx} = e^{rx} [ (u'' + 2r u' + r^2 u) + a(u' + r u) + b u - q ].  The bracket must be the
; zero polynomial; we form it directly and test. -----
(define (oef-certify a b r q u)
  (poly-zero? (poly-sub (oef-bracket a b r u) q)))
(define (oef-bracket a b r u)
  (poly-add (poly-add (poly-add (poly-deriv (poly-deriv u))
                                (poly-scale (* 2 r) (poly-deriv u)))
                      (poly-scale (* r r) u))
            (poly-add (poly-scale a (poly-add (poly-deriv u) (poly-scale r u)))
                      (poly-scale b u))))

; ----- public solve wrapper -----
(define (oef-solve a b r q) (oef-wrap (oef-u a b r q) r a b q))
(define (oef-wrap u r a b q) (if (equal? u (quote no-polynomial-solution)) (list (quote no-solution)) (oef-finish u r a b q)))
(define (oef-finish u r a b q) (if (oef-certify a b r q u) (list (quote solution-times-exp) u r) (list (quote no-solution))))
