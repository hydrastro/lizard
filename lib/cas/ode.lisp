; -*- lisp -*-
; lib/cas/ode.lisp — power-series solutions of linear ODEs with polynomial
; (or series) coefficients, by the method of undetermined Taylor coefficients.
;
; For an ODE  p_0(x) y + p_1(x) y' + ... + p_m(x) y^(m) = r(x)  at an ordinary point
; (p_m(0) /= 0), the Taylor coefficients of y are determined by a recurrence: at
; each order k, matching coefficients of x^k gives a linear equation whose only
; unknown is c_{k+m} (the highest-index coefficient), since y^(i) contributes
; (k+i)!/k! * c_{k+i} to x^k.  Given the initial data c_0,...,c_{m-1} (i.e. y(0),
; y'(0)/1!, ... or the derivative values), the rest follow one at a time.
;
; The result is certified the same way the elementary series are: substitute the
; computed series back and check  sum_i p_i(x) y^(i)(x) - r(x) = 0  mod x^{N-m}.
; Cross-checks against known closed forms (exp solves y'=y, sin/cos solve y''=-y,
; 1/(1-x) solves (1-x)y'=y) confirm the recurrence independently.
;
; Coefficient lists are series from series.lisp.

(import "cas/series.lisp")

(define (falling n i) (if (= i 0) 1 (* n (falling (- n 1) (- i 1)))))
(define (ith-deriv y i) (if (= i 0) y (ith-deriv (ser-deriv y) (- i 1))))

; contribution of p_i(x) y^(i) to [x^k], using already-known coefficients in c-acc
; (terms whose c-index is not yet known contribute 0, which is exactly the unknown)
(define (pi-contrib pi i c-acc k j acc)
  (if (>= j (length pi)) acc
    (let ((idx (+ (- k j) i)))
      (pi-contrib pi i c-acc k (+ j 1)
        (if (< idx 0) acc (+ acc (* (ser-coeff pi j) (falling idx i) (ser-coeff c-acc idx))))))))
(define (kp-sum p-list c-acc k i acc)
  (if (null? p-list) acc
    (kp-sum (cdr p-list) c-acc k (+ i 1) (+ acc (pi-contrib (car p-list) i c-acc k 0 0)))))

; next coefficient c_{len} where len = length(c-acc); requires p_m(0) /= 0
(define (ode-next p-list r c-acc m)
  (let ((k (- (length c-acc) m)))
    (let ((KP (kp-sum p-list c-acc k 0 0))
          (lead (* (ser-coeff (nth p-list m) 0) (falling (+ k m) m))))
      (/ (- (ser-coeff r k) KP) lead))))
(define (ode-fill p-list r c-acc m N)
  (if (>= (length c-acc) N) (ser-trunc c-acc N)
    (ode-fill p-list r (append c-acc (list (ode-next p-list r c-acc m))) m N)))
; p-list = (p_0 p_1 ... p_m) coefficient polynomials/series; r RHS; inits = (c_0 ... c_{m-1}); order N
(define (ode-series p-list r inits N)
  (let ((m (- (length p-list) 1))) (ode-fill p-list r (ser-trunc inits m) m N)))

; ---------- certificate: substitute the series back ----------
(define (ode-res-go p-list r y N i acc)
  (if (null? p-list) acc (ode-res-go (cdr p-list) r y N (+ i 1) (ser-add acc (ser-mul (car p-list) (ith-deriv y i) N)))))
(define (ode-residual p-list r y N) (ode-res-go p-list r y N 0 (ser-neg (ser-trunc r N))))
(define (all-zero s) (cond ((null? s) #t) ((= (car s) 0) (all-zero (cdr s))) (else #f)))
(define (ode-ok? p-list r y N) (all-zero (ser-trunc (ode-residual p-list r y N) (- N (- (length p-list) 1)))))
