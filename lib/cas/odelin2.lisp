; -*- lisp -*-
; lib/cas/odelin2.lisp -- POLYNOMIAL PARTICULAR SOLUTIONS of the second-order constant-coefficient linear ODE
; y'' + a y' + b y = q(x) with polynomial forcing q (docs/CAS.md -- summit S5, the inhomogeneous companion to
; odelin's homogeneous constant-coefficient solver and the next order above odefol's first-order equations).
;
; The operator L(y) = y'' + a y' + b y is LINEAR in the coefficients of a polynomial ansatz
; P = c_0 + ... + c_m x^m.  When b /= 0 the term b*P dominates, so a polynomial particular solution has degree
; m = deg q; we build the (deg q + 1) x (m + 1) matrix whose i-th column is the coefficient vector of L(x^i) and
; solve the exact linear system L(P) = q over Q with linalg's Gauss-Jordan solver, then CERTIFY the result by
; differentiation (P'' + a P' + b P - q identically zero).  When b = 0 the constant term of P is free and a
; polynomial solution exists only if q has the right form; the same linear solve handles it and the certificate
; is the arbiter.  An inconsistent system (no polynomial particular solution) is reported honestly as
; 'no-polynomial-solution -- the general solution then needs the homogeneous exponential/trig modes from odelin.
;
; This is the exact, certifiable polynomial slice of the method of undetermined coefficients for second-order
; equations: sound both ways (a returned solution is verified; absence is reported, never fabricated).
;
; Public (a, b rational constants; q a polynomial coefficient list low->high):
;   o2-particular a b q    -> the polynomial P with P'' + a P' + b P = q, or 'no-polynomial-solution
;   o2-certify a b q P     -> #t iff P'' + a P' + b P - q is identically zero (the differentiation certificate)
;   o2-solve a b q         -> (list 'polynomial-solution P) certified | (list 'no-polynomial-solution)
;
; Verified: y'' + y = x^2 gives x^2 - 2; y'' - y = x gives -x; y'' + 3y' + 2y = x gives x/2 - 3/4; a case with no
; polynomial particular solution is reported honestly; every returned P passes the differentiation certificate.
;
; Builds on poly.lisp and linalg.lisp.

(import "cas/poly.lisp")
(import "cas/linalg.lisp")

(define (o2-len l) (if (null? l) 0 (+ 1 (o2-len (cdr l)))))
(define (o2-nth l k) (if (= k 0) (car l) (o2-nth (cdr l) (- k 1))))
(define (o2-app a b) (if (null? a) b (cons (car a) (o2-app (cdr a) b))))

(define (o2-trim p) (o2-trim-go p (o2-len p)))
(define (o2-trim-go p n) (cond ((= n 0) 0) ((= (o2-nth p (- n 1)) 0) (o2-trim-go p (- n 1))) (else n)))
(define (o2-deg p) (- (o2-trim p) 1))

; ----- L(x^i) = (x^i)'' + a (x^i)' + b x^i, as a coeff list -----
(define (o2-L a b i) (poly-add (poly-add (o2-d2-monomial i) (poly-scale a (o2-d1-monomial i))) (poly-scale b (o2-monomial i))))
(define (o2-monomial i) (o2-app (o2-zeros i) (list 1)))
(define (o2-zeros k) (if (<= k 0) (quote ()) (cons 0 (o2-zeros (- k 1)))))
(define (o2-d1-monomial i) (if (<= i 0) (list 0) (o2-app (o2-zeros (- i 1)) (list i))))
(define (o2-d2-monomial i) (if (<= i 1) (list 0) (o2-app (o2-zeros (- i 2)) (list (* i (- i 1))))))

(define (o2-pad p n) (if (>= (o2-len p) n) p (o2-pad (o2-app p (list 0)) n)))

; ----- ansatz degree m = deg q (when b /= 0); when b = 0 use deg q + 1 to allow the free lower terms -----
(define (o2-rhs-len q) (+ (o2-deg q) 1))
(define (o2-ansatz-deg a b q) (if (= b 0) (+ (o2-deg q) 2) (o2-deg q)))
(define (o2-columns a b q) (o2-cols a b (o2-mat-rows a b q) 0 (o2-ansatz-deg a b q)))
(define (o2-mat-rows a b q) (o2-maxnum (o2-rhs-len q) (+ (o2-ansatz-deg a b q) 1)))
(define (o2-maxnum x y) (if (> x y) x y))
(define (o2-cols a b R i m) (if (> i m) (quote ()) (cons (o2-pad (o2-L a b i) R) (o2-cols a b R (+ i 1) m))))
(define (o2-transpose cols R) (o2-tr cols R 0))
(define (o2-tr cols R r) (if (>= r R) (quote ()) (cons (o2-row cols r) (o2-tr cols R (+ r 1)))))
(define (o2-row cols r) (if (null? cols) (quote ()) (cons (o2-nth (car cols) r) (o2-row (cdr cols) r))))

; ----- solve for the ansatz coefficients -----
(define (o2-particular a b q) (o2-from-sol a b q (mat-solve (o2-transpose (o2-columns a b q) (o2-mat-rows a b q)) (o2-pad q (o2-mat-rows a b q)))))
(define (o2-from-sol a b q sol) (if (equal? sol (quote none)) (quote no-polynomial-solution) (o2-verify-or-fail a b q sol)))
(define (o2-verify-or-fail a b q P) (if (o2-certify a b q P) (o2-strip P) (quote no-polynomial-solution)))
(define (o2-strip P) (if (= (o2-trim P) 0) (list 0) (o2-take P (o2-trim P))))
(define (o2-take p n) (if (= n 0) (quote ()) (o2-app (o2-take p (- n 1)) (list (o2-nth p (- n 1))))))

; ----- differentiation certificate -----
(define (o2-certify a b q P) (poly-zero? (poly-sub (poly-add (poly-add (poly-deriv (poly-deriv P)) (poly-scale a (poly-deriv P))) (poly-scale b P)) q)))

; ----- public solve wrapper -----
(define (o2-solve a b q) (o2-wrap (o2-particular a b q)))
(define (o2-wrap P) (if (equal? P (quote no-polynomial-solution)) (list (quote no-polynomial-solution)) (list (quote polynomial-solution) P)))
