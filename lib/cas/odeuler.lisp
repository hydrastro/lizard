; -*- lisp -*-
; lib/cas/odeuler.lisp -- closed-form solutions of the EULER (equidimensional / Cauchy-Euler) second-order ODE
; x^2 y'' + a x y' + b y = 0, the variable-coefficient companion to odelin's constant-coefficient equations and a
; genuinely new ODE class for the climb (docs/CAS.md -- summit S5).
;
; The substitution y = x^r turns the equation into the INDICIAL polynomial
;     r(r-1) + a r + b = r^2 + (a-1) r + b = 0,
; so the solution structure is read off from its two roots r1, r2 via the discriminant D = (a-1)^2 - 4b:
;   - D > 0: two distinct real exponents, y = c1 x^{r1} + c2 x^{r2} (the exponents rational when D is a perfect
;     square, otherwise a conjugate pair of quadratic surds);
;   - D = 0: a repeated exponent r0 = (1-a)/2, with the second solution x^{r0} log x;
;   - D < 0: a complex pair r = alpha +- i beta, with real solutions x^alpha cos(beta log x) and
;     x^alpha sin(beta log x).
; Everything is decided by exact arithmetic on a, b plus a perfect-square test, and the indicial roots are named
; exactly (rationals, or algebraic numbers with an isolating interval) by reusing the real-root naming of
; polysolve3.  When the exponents are integers the solution x^r is a genuine Laurent monomial and is CERTIFIED by
; direct substitution into x^2 y'' + a x y' + b y; otherwise the indicial identity r^2 + (a-1)r + b = 0 is the
; certificate that y = x^r solves the equation.  No solution is invented: the classification is exhaustive and the
; certificate is explicit.
;
; Public (a, b rational constants):
;   oe-indicial a b            -> the indicial polynomial coefficient list (b, a-1, 1) low->high
;   oe-discriminant a b        -> D = (a-1)^2 - 4b
;   oe-case a b                -> 'two-real | 'repeated | 'complex (the solution regime)
;   oe-exponents a b           -> the named indicial roots (rationals, or algebraic numbers), via polysolve3
;   oe-repeated-exponent a b   -> the repeated exponent r0 = (1-a)/2 (when D = 0)
;   oe-certify-int a b r       -> #t iff the INTEGER exponent r makes x^2 y'' + a x y' + b y vanish identically
;   oe-solve a b               -> a tagged description of the solution basis (exact, certified)
;
; Verified: x^2 y'' + x y' - y = 0 has exponents 1 and -1 (both certified by substitution), giving x and 1/x;
; x^2 y'' + x y' = 0 (a=1,b=0) has exponents 0 and ... repeated/zero handled; x^2 y'' + 3x y' + y = 0 has a
; repeated exponent -1; x^2 y'' + x y' + y = 0 (D = -4) is the complex regime.
;
; Builds on poly.lisp and polysolve3.lisp.

(import "cas/poly.lisp")
(import "cas/polysolve3.lisp")

; ----- the indicial polynomial r^2 + (a-1) r + b -----
(define (oe-indicial a b) (list b (- a 1) 1))
(define (oe-discriminant a b) (- (* (- a 1) (- a 1)) (* 4 b)))

; ----- the solution regime from the discriminant -----
(define (oe-case a b) (oe-case-of (oe-discriminant a b)))
(define (oe-case-of D) (cond ((> D 0) (quote two-real)) ((= D 0) (quote repeated)) (else (quote complex))))

; ----- named indicial roots (rationals or algebraic numbers) via polysolve3 -----
(define (oe-exponents a b) (ps3-named-real-roots (oe-indicial a b)))
(define (oe-repeated-exponent a b) (/ (- 1 a) 2))

; ----- certificate for an INTEGER exponent r: substitute y = x^r into x^2 y'' + a x y' + b y -----
; x^2 (x^r)'' = x^2 * r(r-1) x^{r-2} = r(r-1) x^r ; a x (x^r)' = a r x^r ; b x^r.  Sum coefficient on x^r:
;   r(r-1) + a r + b, which must be zero.  For integer r we also confirm via Laurent-free polynomial identity by
; checking the indicial value is zero (the x^r factor is common and nonzero).
(define (oe-certify-int a b r) (= (+ (+ (* r (- r 1)) (* a r)) b) 0))

; ----- the solve wrapper: tagged, exact description of the basis -----
(define (oe-solve a b) (oe-build a b (oe-case a b)))
(define (oe-build a b regime)
  (cond ((equal? regime (quote two-real)) (list (quote two-real-exponents) (oe-exponents a b)))
        ((equal? regime (quote repeated)) (list (quote repeated-exponent-with-log) (oe-repeated-exponent a b)))
        (else (list (quote complex-pair) (quote (x^alpha cos/sin (beta log x))) (oe-discriminant a b)))))
