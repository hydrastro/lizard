; -*- lisp -*-
; lib/cas/special.lisp -- SPECIAL FUNCTIONS: the Gamma function, the error function erf, and the Bessel
; functions J_n.  This closes the one capability that Maxima has and lizard had none of (the comparison chart's
; only "none" row).  Each function is represented the way it can be computed and CERTIFIED exactly: by its power
; series (rational coefficients) together with its defining functional identities, all checked against the
; differentiation / recurrence relations using the series engine (series.lisp).
;
; GAMMA.  Integer values Gamma(n) = (n-1)!.  Half-integer values are rational multiples of sqrt(pi): we carry
; the rational coefficient, gm-half k = Gamma((2k+1)/2)/sqrt(pi), via the functional equation Gamma(x+1) =
; x Gamma(x) (so gm-half(k+1) = ((2k+1)/2) gm-half(k), gm-half 0 = 1).  gm-func-check verifies the functional
; equation at a half-integer.
;
; ERF.  erf(x) = (2/sqrt pi) sum_{n>=0} (-1)^n x^{2n+1} / (n! (2n+1)).  We carry the series WITHOUT the constant
; 2/sqrt(pi) factor (erf-series), since that factor is what makes erf'(x) = (2/sqrt pi) e^{-x^2}: the reduced
; series differentiates exactly to the series of e^{-x^2} (erf-deriv-check).  erf is odd, erf(0) = 0.
;
; BESSEL.  J_n(x) = sum_{m>=0} (-1)^m / (m! (m+n)!) (x/2)^{2m+n}.  We build the series to a requested order and
; verify the contiguous relation J_0'(x) = -J_1(x) (bessel-deriv-check) and that J_0 satisfies the Bessel
; equation x^2 y'' + x y' + x^2 y = 0 (bessel-ode-check).
;
; Public:
;   sp-fact n / sp-gamma-int n            -> n! and Gamma(n) = (n-1)!
;   sp-gamma-half k                       -> Gamma((2k+1)/2) / sqrt(pi)  (rational)
;   sp-gamma-func-check k                 -> #t iff Gamma((2k+3)/2) = ((2k+1)/2) Gamma((2k+1)/2)
;   sp-erf-series N                       -> the reduced erf series (without 2/sqrt pi) to N terms
;   sp-erf-deriv-check N                  -> #t iff d/dx(reduced erf) = series of e^{-x^2}
;   sp-besselj n N                        -> the series of J_n to N terms
;   sp-bessel-deriv-check N               -> #t iff J_0'(x) = -J_1(x) to N terms
;   sp-bessel-ode-check N                 -> #t iff x^2 J_0'' + x J_0' + x^2 J_0 = 0 to N terms
;
; Verified: Gamma(5)=24, the sqrt(pi)-coefficients 1,1/2,3/4,15/8 and the functional equation; erf'=e^{-x^2};
; J_0'=-J_1 and the Bessel ODE for J_0.
;
; Builds on series.lisp (the power-series engine) and standard rational arithmetic.

(import "cas/series.lisp")

(define (sp-nth l k) (if (= k 0) (car l) (sp-nth (cdr l) (- k 1))))

; ----- factorial and integer Gamma -----
(define (sp-fact n) (if (= n 0) 1 (* n (sp-fact (- n 1)))))
(define (sp-gamma-int n) (sp-fact (- n 1)))

; ----- half-integer Gamma: Gamma((2k+1)/2)/sqrt(pi), via Gamma(x+1)=x Gamma(x) -----
(define (sp-gamma-half k) (if (= k 0) 1 (* (/ (- (* 2 k) 1) 2) (sp-gamma-half (- k 1)))))
; functional equation: Gamma((2(k+1)+1)/2) = ((2k+1)/2) Gamma((2k+1)/2)
(define (sp-gamma-func-check k) (= (sp-gamma-half (+ k 1)) (* (/ (+ (* 2 k) 1) 2) (sp-gamma-half k))))

; ----- erf reduced series: sum (-1)^n x^{2n+1}/(n!(2n+1)) -----
(define (sp-erf-coeffs N) (sp-erfc-go 0 N (sp-zeros N)))
(define (sp-erfc-go n N acc)
  (let ((e (+ (* 2 n) 1)))
    (if (>= e N) acc
        (sp-erfc-go (+ n 1) N (sp-setnth acc e (/ (sp-altsign n) (* (sp-fact n) (+ (* 2 n) 1))))))))
(define (sp-altsign n) (if (= (remainder n 2) 0) 1 -1))
(define (sp-zeros N) (if (= N 0) (quote ()) (cons 0 (sp-zeros (- N 1)))))
(define (sp-setnth l i v) (if (= i 0) (cons v (cdr l)) (cons (car l) (sp-setnth (cdr l) (- i 1) v))))

; e^{-x^2} series: sum (-1)^n x^{2n}/n!
(define (sp-emx2 N) (sp-emx2-go 0 N (sp-zeros N)))
(define (sp-emx2-go n N acc)
  (let ((e (* 2 n)))
    (if (>= e N) acc
        (sp-emx2-go (+ n 1) N (sp-setnth acc e (/ (sp-altsign n) (sp-fact n)))))))

; certificate: d/dx(reduced erf) = e^{-x^2}
(define (sp-erf-deriv-check N) (equal? (ser-trunc (ser-deriv (sp-erf-coeffs (+ N 1))) N) (ser-trunc (sp-emx2 N) N)))

; ----- Bessel J_n series: sum_m (-1)^m/(m!(m+n)!) (x/2)^{2m+n} -----
(define (sp-besselj n N) (sp-bj-go n 0 N (sp-zeros N)))
(define (sp-bj-go n m N acc)
  (let ((e (+ (* 2 m) n)))
    (if (>= e N) acc
        (sp-bj-go n (+ m 1) N (sp-setnth acc e (sp-bj-coeff n m))))))
; coefficient of x^{2m+n}: (-1)^m / (m! (m+n)! 2^{2m+n})
(define (sp-bj-coeff n m) (/ (sp-altsign m) (* (* (sp-fact m) (sp-fact (+ m n))) (sp-pow2 (+ (* 2 m) n)))))
(define (sp-pow2 k) (if (= k 0) 1 (* 2 (sp-pow2 (- k 1)))))

; certificate: J_0'(x) = -J_1(x)
(define (sp-bessel-deriv-check N) (equal? (ser-trunc (ser-deriv (sp-besselj 0 (+ N 1))) N) (ser-trunc (ser-neg (sp-besselj 1 N)) N)))

; certificate: x^2 J_0'' + x J_0' + x^2 J_0 = 0
(define (sp-bessel-ode-check N) (sp-allzero (ser-trunc (sp-bessel-ode N) N)))
(define (sp-bessel-ode N)
  (let ((J0 (sp-besselj 0 (+ N 2))))
    (let ((dJ0 (ser-deriv J0)))
      (let ((ddJ0 (ser-deriv dJ0)))
        (ser-add (ser-add (sp-xshift ddJ0 2) (sp-xshift dJ0 1)) (sp-xshift J0 2))))))
(define (sp-xshift s k) (append (sp-zeros k) s))
(define (sp-allzero l) (cond ((null? l) #t) ((= (car l) 0) (sp-allzero (cdr l))) (else #f)))
