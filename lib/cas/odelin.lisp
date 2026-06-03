; -*- lisp -*-
; lib/cas/odelin.lisp -- closed-form solutions of constant-coefficient linear ODEs.
;
; ode1 solves separable and autonomous first-order equations; this handles the constant-coefficient
; linear equation of any order,  a_0 y + a_1 y' + ... + a_m y^(m) = 0, whose characteristic polynomial is
; exactly that coefficient list read in r:  char(r) = a_0 + a_1 r + ... + a_m r^m.  For each rational root
; r of multiplicity mu the basis solutions are  x^j e^{r x},  j = 0 .. mu-1, and the general solution is
; their span (this captures the full solution space when char splits over Q -- the ODE analogue of linrec).
; Each basis solution is CERTIFIED by an exact polynomial identity rather than by trusting the root: writing
; the k-th derivative of x^j e^{rx} as p_k(x) e^{rx}, the operator p_0 = x^j, p_{k+1} = p_k' + r p_k turns
; the ODE into  ( sum_k a_k p_k(x) ) e^{rx},  and we check that the polynomial sum_k a_k p_k is identically
; zero.  (For j = 0 this is exactly char(r) = 0; for j >= 1 it is the multiplicity condition on char and its
; derivatives -- all verified symbolically over Q.)  Builds on solve.lisp (rational roots) and poly.lisp.

(import "cas/solve.lisp")

(define (odelin-xj j) (if (= j 0) (list 1) (cons 0 (odelin-xj (- j 1)))))          ; x^j (low->high)
(define (odelin-Dr r p) (poly-add (poly-deriv p) (poly-scale r p)))                 ; p -> p' + r p
(define (odelin-op-sum coeffs r p acc)                                              ; sum_k a_k p_k
  (if (null? coeffs) acc
      (odelin-op-sum (cdr coeffs) r (odelin-Dr r p) (poly-add acc (poly-scale (car coeffs) p)))))
(define (odelin-certify coeffs j r) (poly-zero? (odelin-op-sum coeffs r (odelin-xj j) (quote ()))))

; rational roots of the characteristic polynomial, as (list q multiplicity)
(define (odelin-filter-rat roots)
  (if (null? roots) (quote ())
      (let ((r (car roots)))
        (if (equal? (car (car r)) (quote rat))
            (cons (list (car (cdr (car r))) (car (cdr r))) (odelin-filter-rat (cdr roots)))
            (odelin-filter-rat (cdr roots))))))
(define (odelin-ratroots coeffs) (odelin-filter-rat (solve-poly coeffs)))

; basis solutions as (list j r certified?), one per (root, j<mult)
(define (odelin-basis-mult coeffs r mult j)
  (if (>= j mult) (quote ()) (cons (list j r (odelin-certify coeffs j r)) (odelin-basis-mult coeffs r mult (+ j 1)))))
(define (odelin-basis-go coeffs rr)
  (if (null? rr) (quote ())
      (append (odelin-basis-mult coeffs (car (car rr)) (car (cdr (car rr))) 0) (odelin-basis-go coeffs (cdr rr)))))
(define (odelin-basis coeffs) (odelin-basis-go coeffs (odelin-ratroots coeffs)))

(define (odelin-sum-mults rr) (if (null? rr) 0 (+ (car (cdr (car rr))) (odelin-sum-mults (cdr rr)))))
(define (odelin-all-certified? basis) (if (null? basis) #t (if (car (cdr (cdr (car basis)))) (odelin-all-certified? (cdr basis)) #f)))
(define (odelin-order coeffs) (- (length coeffs) 1))
(define (odelin-fully-solvable? coeffs)                ; char splits over Q AND every basis solution certifies
  (let ((b (odelin-basis coeffs)))
    (if (= (odelin-sum-mults (odelin-ratroots coeffs)) (odelin-order coeffs)) (odelin-all-certified? b) #f)))
