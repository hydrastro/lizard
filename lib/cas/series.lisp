; -*- lisp -*-
; lib/cas/series.lisp — truncated power series over Q (Taylor expansion).
;
; A series to order N is the coefficient list (c_0 c_1 ... c_{N-1}), low-to-high,
; representing c_0 + c_1 x + ... + c_{N-1} x^{N-1} + O(x^N).  Everything is exact
; over Q.  The point of a CAS series module is that the results are checkable:
;   * the series S of a rational function p/q satisfies q*S = p exactly mod x^N;
;   * exp's series satisfies S' = S;  log(1+x) satisfies (1+x) S' = 1;
;   * sin/cos satisfy S'' = -S;  (1+x)^a satisfies (1+x) S' = a S.
; These defining identities, truncated, are the certificates (see examples).
;
; Self-contained: only list arithmetic over Q.

(define (nth lst i) (if (= i 0) (car lst) (nth (cdr lst) (- i 1))))
(define (iota a b) (if (> a b) '() (cons a (iota (+ a 1) b))))
(define (factorial n) (if (= n 0) 1 (* n (factorial (- n 1)))))

; ---------- truncation / basic arithmetic ----------
(define (ser-trunc s N) (cond ((= N 0) '()) ((null? s) (cons 0 (ser-trunc '() (- N 1)))) (else (cons (car s) (ser-trunc (cdr s) (- N 1))))))
(define (ser-coeff s i) (if (< i (length s)) (nth s i) 0))
(define (ser-len s) (length s))
(define (s2 f a b) (if (null? a) '() (cons (f (car a) (car b)) (s2 f (cdr a) (cdr b)))))
(define (ser-add a b) (let ((n (max (length a) (length b)))) (s2 + (ser-trunc a n) (ser-trunc b n))))
(define (ser-neg s) (map (lambda (x) (- 0 x)) s))
(define (ser-sub a b) (ser-add a (ser-neg b)))
(define (ser-scale c s) (map (lambda (x) (* c x)) s))

; ---------- multiplication, inverse, division ----------
(define (conv a b k i acc) (if (> i k) acc (conv a b k (+ i 1) (+ acc (* (ser-coeff a i) (ser-coeff b (- k i)))))))
(define (ser-mul a b N) (map (lambda (k) (conv a b k 0 0)) (iota 0 (- N 1))))
; 1/s, requires c_0 /= 0: b_0 = 1/a_0; b_k = -(1/a_0) sum_{i=1}^k a_i b_{k-i}
(define (conv-from1 s b k i acc) (if (> i k) acc (conv-from1 s b k (+ i 1) (+ acc (* (ser-coeff s i) (ser-coeff b (- k i)))))))
(define (sinv-go s N inv0 acc) (if (>= (length acc) N) acc (sinv-go s N inv0 (append acc (list (* (- 0 inv0) (conv-from1 s acc (length acc) 1 0)))))))
(define (ser-inverse s N) (let ((inv0 (/ 1 (ser-coeff s 0)))) (sinv-go s N inv0 (list inv0))))
(define (ser-div a b N) (ser-mul (ser-trunc a N) (ser-inverse b N) N))

; ---------- calculus ----------
(define (sderiv s k) (if (null? s) '() (cons (* k (car s)) (sderiv (cdr s) (+ k 1)))))
(define (ser-deriv s) (if (or (null? s) (null? (cdr s))) '() (sderiv (cdr s) 1)))
(define (sint s k) (if (null? s) '() (cons (/ (car s) k) (sint (cdr s) (+ k 1)))))
(define (ser-integrate s) (cons 0 (sint s 1)))      ; antiderivative with zero constant term

; ---------- composition a(b(x)), requires b_0 = 0 ----------
(define (scomp ra b N acc) (if (null? ra) acc (scomp (cdr ra) b N (ser-add (ser-mul acc b N) (list (car ra))))))
(define (ser-compose a b N) (ser-trunc (scomp (reverse (ser-trunc a N)) b N (list 0)) N))

; ---------- from polynomials / rational functions ----------
(define (poly->series p N) (ser-trunc p N))
(define (ratfun->series p q N) (ser-div (ser-trunc p N) (ser-trunc q N) N))   ; needs q(0) /= 0

; ---------- elementary series to order N ----------
(define (rem4 k) (- k (* 4 (floor (/ k 4)))))
(define (rem2 k) (- k (* 2 (floor (/ k 2)))))
(define (exp-series N) (map (lambda (k) (/ 1 (factorial k))) (iota 0 (- N 1))))
(define (geometric-series N) (map (lambda (k) 1) (iota 0 (- N 1))))                 ; 1/(1-x)
(define (log1p-series N) (cons 0 (map (lambda (k) (/ (if (= (rem2 k) 1) 1 -1) k)) (iota 1 (- N 1)))))  ; log(1+x)
(define (sin-coeff k) (if (= (rem2 k) 0) 0 (/ (if (= (rem4 k) 1) 1 -1) (factorial k))))
(define (cos-coeff k) (if (= (rem2 k) 1) 0 (/ (if (= (rem4 k) 0) 1 -1) (factorial k))))
(define (sin-series N) (map sin-coeff (iota 0 (- N 1))))
(define (cos-series N) (map cos-coeff (iota 0 (- N 1))))
(define (ff a k) (if (= k 0) 1 (* (- a (- k 1)) (ff a (- k 1)))))                   ; falling factorial
(define (binom-series a N) (map (lambda (k) (/ (ff a k) (factorial k))) (iota 0 (- N 1))))  ; (1+x)^a

; compose elementary series with a series f having f_0 = 0
(define (exp-of f N) (ser-compose (exp-series N) f N))
(define (log1p-of f N) (ser-compose (log1p-series N) f N))
(define (sin-of f N) (ser-compose (sin-series N) f N))
(define (cos-of f N) (ser-compose (cos-series N) f N))

; ---------- display ----------
(define (ser->string s var) (let ((r (sterms s var 0 ""))) (if (equal? r "") "0" (string-append r " + O(" var "^" (number->string (length s)) ")"))))
(define (sterms s var k acc)
  (if (null? s) acc
    (sterms (cdr s) var (+ k 1)
      (if (= (car s) 0) acc
        (let ((t (sterm (car s) var k))) (if (equal? acc "") t (string-append acc " + " t)))))))
(define (sterm c var k)
  (cond ((= k 0) (qstr c)) ((= k 1) (string-append (qstr c) "*" var)) (else (string-append (qstr c) "*" var "^" (number->string k)))))
(define (qstr c) (if (integer? c) (number->string c) (string-append (number->string (numerator c)) "/" (number->string (denominator c)))))

; ---------- limits at x -> 0 via exact series ----------
; order of vanishing (index of first nonzero coeff); -1 if identically zero to order
(define (sord s k) (cond ((null? s) -1) ((not (= (car s) 0)) k) (else (sord (cdr s) (+ k 1)))))
(define (ser-order s) (sord s 0))
(define (limit-at-0 s) (ser-coeff s 0))                 ; lim_{x->0} f(x) = c_0
; lim_{x->0} g(x)/h(x), resolving 0/0 by comparing series orders (exact L'Hopital)
(define (limit-ratio g h)
  (let ((a (ser-order g)) (b (ser-order h)))
    (cond ((< b 0) 'undefined)                          ; denominator identically zero
          ((< a 0) 0)
          ((> a b) 0)
          ((= a b) (/ (ser-coeff g a) (ser-coeff h b)))
          (else 'infinite))))
