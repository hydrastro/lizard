; -*- lisp -*-
; lib/cas/algfuncint.lisp -- INT P(x)/sqrt(R) dx for ARBITRARY polynomial P and monic quadratic R.
;
; algfunc integrated only a LINEAR numerator over sqrt(quadratic).  The general polynomial numerator is
; reduced by the classical ansatz
;     INT P(x)/sqrt(R) dx = Q(x) sqrt(R) + lambda * INT dx/sqrt(R),    R = x^2 + b x + c  (monic),
; with Q a polynomial of degree deg(P)-1 and lambda a constant.  Differentiating and clearing sqrt(R)
; turns this into the exact polynomial identity  P = Q' R + Q R'/2 + lambda, which is TRIANGULAR in the
; coefficients of Q and is solved by one descending pass (no linear-algebra needed):
;     q_{n-1} = p_n / n,   q_{k-1} = ( p_k - b(k+1/2) q_k - c(k+1) q_{k+1} ) / k,   lambda = p_0 - (b/2)q_0 - c q_1.
; The remaining INT dx/sqrt(R) = log(x + b/2 + sqrt(R)) is algfunc's basic radical.  The result is CERTIFIED
; by differentiation inside K = Q(x)[sqrt(R)] exactly as before:  D(Q sqrt(R) + lambda log(g)) = P/sqrt(R).
; Builds on algfunc.lisp.

(import "cas/algfunc.lisp")

; solve the triangular system for Q (low->high) and lambda; R monic = x^2 + b x + c
(define (afi-go P b c k qk qk1 acc)
  (if (= k 0) acc
      (let ((qkm1 (/ (- (poly-coeff P k) (+ (* b (+ k (/ 1 2)) qk) (* c (+ k 1) qk1))) k)))
        (afi-go P b c (- k 1) qkm1 qk (cons qkm1 acc)))))
(define (afi-build P R)
  (let ((n (poly-deg P)) (b (poly-coeff R 1)) (c (poly-coeff R 0)))
    (if (<= n 0) (list (quote ()) (poly-coeff P 0))
        (let ((qn1 (/ (poly-coeff P n) n)))
          (let ((qs (afi-go P b c (- n 1) qn1 0 (list qn1))))
            (list qs (- (poly-coeff P 0) (+ (* (/ b 2) (car qs)) (* c (if (>= n 2) (car (cdr qs)) 0))))))))))

; INT P/sqrt(R) dx -> (list alg-part clog logarg) in K = Q(x)[sqrt R]
(define (int-poly-sqrt P R)
  (let ((sol (afi-build P R)))
    (list (af-make (rat-zero) (rat-from-poly (car sol)))           ; Q * sqrt(R)
          (rat-from-poly (list (car (cdr sol))))                   ; lambda
          (af-logarg (rat-from-poly R)))))                         ; x + b/2 + sqrt(R)
(define (int-poly-sqrt-integrand P R) (af-mul (rat-from-poly R) (af-from-rat (rat-from-poly P)) (af-inv (rat-from-poly R) (af-y))))
(define (int-poly-sqrt-certify P R)
  (let ((r (int-poly-sqrt P R)))
    (af-certify (rat-from-poly R) (car r) (car (cdr r)) (car (cdr (cdr r))) (int-poly-sqrt-integrand P R))))
