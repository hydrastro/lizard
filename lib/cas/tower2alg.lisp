; -*- lisp -*-
; lib/cas/tower2alg.lisp -- height-two integration with an irreducible-quadratic residue.
;
; When the Rothstein-Trager residue polynomial of a height-two logarithmic part is an irreducible
; quadratic over Q, the two residues are a conjugate pair of algebraic numbers and the antiderivative
; is a RootSum of two logarithms whose arguments live in the quadratic extension K1(alpha), where
; K1 = Q(x)(theta1) and alpha is a root of the (monic) residue polynomial z^2 + c1 z + c0.  Rather than
; carry the algebraic closure, we use that the RootSum's logarithmic derivative is a trace over the
; conjugate pair and therefore descends to K1.  For a squarefree D* of degree two in theta2 the second
; Rothstein-Trager argument A* - z D2(D*) is linear, so each argument is v_alpha = theta2 - rho with
; rho = -(constant)/(leading) of A* - alpha D2(D*) in K1(alpha); its conjugate is v_albar, and the two
; multiply back to D* = v_alpha v_albar.  The derivative of alpha log(v_alpha) + albar log(v_albar) is
;
;     [ Tr( alpha D2(v_alpha) v_albar ) ] / ( v_alpha v_albar ) = num / D*,
;
; whose numerator num, a trace, is a polynomial over K1.  The integral is certified by checking that
; num equals A* over K1[theta2] -- a fully rational identity that requires only the degree-two extension
; arithmetic of K1(alpha) (pairs over K1, reduced by alpha^2 = -c1 alpha - c0) and the descent by trace.
; With theta1 = e^x and theta2 = log(e^x + 1) the integrator certifies
;
;     INT (D theta2) / (theta2^2 + 1) dx = arctan(theta2) = arctan(log(e^x + 1)),
;
; whose residue polynomial 4 z^2 + 1 is irreducible over Q (residues plus/minus i/2).  The minimal
; polynomial coefficients c1, c0 are supplied as the globals ac1, ac0 (elements of K1) read off from the
; monic residue polynomial.  Builds on tower2rt.lisp.

(import "cas/tower2rt.lisp")

; ----- K1(alpha) = K1[alpha]/(alpha^2 + ac1 alpha + ac0): elements (list ka kb) = ka + kb alpha -----
(define (k1a-make a b) (list a b))
(define (k1a-re x) (car x))
(define (k1a-im x) (car (cdr x)))
(define (k1a-zero) (list (k1-zero) (k1-zero)))
(define (k1a-one) (list (k1-one) (k1-zero)))
(define (k1a-alpha) (list (k1-zero) (k1-one)))
(define (k1a-lift x) (list x (k1-zero)))
(define (k1a-add x y) (list (k1-add (k1a-re x) (k1a-re y)) (k1-add (k1a-im x) (k1a-im y))))
(define (k1a-neg x) (list (k1-neg (k1a-re x)) (k1-neg (k1a-im x))))
(define (k1a-sub x y) (k1a-add x (k1a-neg y)))
(define (k1a-iscale n x) (list (k1-iscale n (k1a-re x)) (k1-iscale n (k1a-im x))))
(define (k1a-mul x y)                          ; (a+b@)(c+d@), @^2 = -ac1 @ - ac0
  (let ((a (k1a-re x)) (b (k1a-im x)) (c (k1a-re y)) (d (k1a-im y)))
    (let ((bd (k1-mul b d)))
      (list (k1-sub (k1-mul a c) (k1-mul bd ac0))
            (k1-sub (k1-add (k1-mul a d) (k1-mul b c)) (k1-mul bd ac1))))))
(define (k1a-conj x)                            ; conj(a+b@) = (a - b ac1) - b @     (since @bar = -ac1 - @)
  (list (k1-sub (k1a-re x) (k1-mul (k1a-im x) ac1)) (k1-neg (k1a-im x))))
(define (k1a-trace x) (k1-sub (k1-iscale 2 (k1a-re x)) (k1-mul (k1a-im x) ac1)))   ; x + conj(x) in K1
(define (k1a-norm x) (k1a-re (k1a-mul x (k1a-conj x))))                            ; x conj(x) in K1
(define (k1a-inv x)
  (let ((nn (k1a-norm x)) (cx (k1a-conj x)))
    (list (k1-div (k1a-re cx) nn) (k1-div (k1a-im cx) nn))))
(define (k1a-deriv x mono1) (list (tr-deriv (k1a-re x) mono1) (tr-deriv (k1a-im x) mono1)))   ; D2 on K1(alpha)
(define (k1a-zero? x) (if (k1-zero? (k1a-re x)) (k1-zero? (k1a-im x)) #f))

; ----- h2a: polynomials in theta2 with K1(alpha) coefficients (low->high) -----
(define (h2a-zero? p) (if (null? p) #t (if (k1a-zero? (car p)) (h2a-zero? (cdr p)) #f)))
(define (h2a-norm p) (reverse (h2a-strip (reverse p))))
(define (h2a-strip p) (if (null? p) '() (if (k1a-zero? (car p)) (h2a-strip (cdr p)) p)))
(define (h2a-deg p) (- (length (h2a-norm p)) 1))
(define (h2a-coeff p i) (if (>= i (length p)) (k1a-zero) (k1z-nth p i)))
(define (h2a-add p q) (if (null? p) q (if (null? q) p (cons (k1a-add (car p) (car q)) (h2a-add (cdr p) (cdr q))))))
(define (h2a-cscale s p) (if (null? p) '() (cons (k1a-mul s (car p)) (h2a-cscale s (cdr p)))))
(define (h2a-neg p) (if (null? p) '() (cons (k1a-neg (car p)) (h2a-neg (cdr p)))))
(define (h2a-sub p q) (h2a-add p (h2a-neg q)))
(define (h2a-conj p) (if (null? p) '() (cons (k1a-conj (car p)) (h2a-conj (cdr p)))))
(define (h2a-trace p) (if (null? p) '() (cons (k1a-trace (car p)) (h2a-trace (cdr p)))))   ; -> h2poly over K1
(define (h2a-shift p) (cons (k1a-zero) p))
(define (h2a-mul p q) (if (null? p) '() (h2a-add (h2a-cscale (car p) q) (h2a-shift (h2a-mul (cdr p) q)))))
(define (h2-to-h2a p) (if (null? p) '() (cons (k1a-lift (car p)) (h2-to-h2a (cdr p)))))

; D2 over K1(alpha)[theta2] for a primitive theta2 (D theta2 in K1): coefficientwise + chain
(define (t2a-deriv-cw p mono1) (if (null? p) '() (cons (k1a-deriv (car p) mono1) (t2a-deriv-cw (cdr p) mono1))))
(define (t2a-sdk p k) (if (null? p) '() (cons (k1a-iscale k (car p)) (t2a-sdk (cdr p) (+ k 1)))))
(define (t2a-deriv p Dth2 mono1)
  (if (null? p) '()
      (h2a-add (t2a-deriv-cw p mono1) (h2a-cscale (k1a-lift Dth2) (t2a-sdk (cdr p) 1)))))

; ----- the irreducible-quadratic logarithmic part -----
(define (h2alg-respoly As Ds Dth2 mono1)        ; residue polynomial q(z) in Q[z] via the ratio trick
  (let ((D2D (t2-deriv Ds Dth2 mono1)) (N (h2-deg Ds)))
    (let ((rs (h2rt-Rvals As Ds D2D 0 N '())))
      (let ((k0 (h2rt-first-nonzero rs 0)))
        (q-lagrange (h2rt-int-list N) (h2rt-ratios rs (k1z-nth rs k0) mono1 '()))))))
(define (h2alg-irreducible-quadratic? q) (if (= (poly-deg q) 2) (null? (ros-rational-roots q)) #f))
(define (h2alg-num As Ds Dth2 mono1)            ; Tr[ alpha D2(v_alpha) v_albar ]  in K1[theta2]
  (let ((D2D (h2-to-h2a (t2-deriv Ds Dth2 mono1))) (Asa (h2-to-h2a As)))
    (let ((g (h2a-sub Asa (h2a-cscale (k1a-alpha) D2D))))
      (let ((rho (k1a-neg (k1a-mul (h2a-coeff g 0) (k1a-inv (h2a-coeff g 1))))))
        (let ((va (list (k1a-neg rho) (k1a-one))))
          (h2a-trace (h2a-mul (h2a-cscale (k1a-alpha) (t2a-deriv va Dth2 mono1)) (h2a-conj va))))))))
(define (h2alg-verify As Ds Dth2 mono1) (h2-equal? (h2-norm (h2alg-num As Ds Dth2 mono1)) (h2-norm As)))
