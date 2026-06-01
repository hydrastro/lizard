; -*- lisp -*-
; lib/cas/pade.lisp — Pade approximants of a power series over Q.
;
; The [m/n] Pade approximant of a series S is the rational function P(x)/Q(x) with
; deg P <= m, deg Q <= n, Q(0) = 1, that matches S to order x^(m+n+1):
;
;       S(x) * Q(x) - P(x)  =  O(x^(m+n+1)).
;
; The n coefficients q_1..q_n of the denominator are determined by the n equations in
; degrees m+1..m+n (the coefficients of S*Q that must vanish), an exact rational linear
; system solved with the Gauss-Jordan solver from gosper.lisp; the numerator is then a
; convolution of Q with S.  The result is verified directly: S*Q - P, expanded as a
; power series and truncated at order m+n+1, must be identically zero.  This both
; accelerates/condenses series and recovers a rational function exactly from its
; expansion (the [d/d] approximant of a degree-d rational series is the function back).
;
; Builds on gosper.lisp (the exact linear solver) and series.lisp (series arithmetic
; for the certificate).

(import "cas/gosper.lisp")
(import "cas/series.lisp")

(define (cidx c i) (if (and (>= i 0) (< i (length c))) (nth c i) 0))   ; c_i, 0 outside range

; denominator system: row r (r=1..n) is the degree-(m+r) coefficient equation
(define (pade-row c m n r) (append (map (lambda (j) (cidx c (- (+ m r) j))) (iota 1 n)) (list (- 0 (cidx c (+ m r))))))
(define (pade-rows c m n) (map (lambda (r) (pade-row c m n r)) (iota 1 n)))

; numerator: p_k = sum_{j=0}^{min(k,n)} Q_j c_{k-j}, for k = 0..m
(define (conv-pk Q c k j acc) (if (> j k) acc (conv-pk Q c k (+ j 1) (+ acc (* (cidx Q j) (cidx c (- k j)))))))
(define (pade-num Q c m) (map (lambda (k) (conv-pk Q c k 0 0)) (iota 0 m)))

(define (drop0 r) (cond ((null? r) '()) ((= (car r) 0) (drop0 (cdr r))) (else r)))
(define (trim0 p) (reverse (drop0 (reverse p))))

; [m/n] Pade approximant of the series c -> (list P Q) (coeff lists, low->high, Q[0]=1)
(define (pade-approx c m n)
  (if (= n 0) (list (trim0 (pade-num (list 1) c m)) (list 1))
    (let ((q (lin-solve (pade-rows c m n) n)))
      (if (equal? q 'none) 'singular
          (let ((Q (cons 1 q))) (list (trim0 (pade-num Q c m)) (trim0 Q)))))))

; ---------- certificate: S*Q - P = O(x^(m+n+1)) ----------
(define (all-zero? s) (cond ((null? s) #t) ((= (car s) 0) (all-zero? (cdr s))) (else #f)))
(define (pade-ok? c P Q m n)
  (let ((N (+ (+ m n) 1)))
    (all-zero? (ser-trunc (ser-sub (ser-mul (ser-trunc c N) Q N) P) N))))

; ---------- display ----------
(define (qn c) (if (integer? c) (number->string c) (string-append (number->string (numerator c)) "/" (number->string (denominator c)))))
(define (term->s coef k var)
  (cond ((= k 0) (qn coef))
        ((= k 1) (if (= coef 1) var (string-append (qn coef) "*" var)))
        (else (if (= coef 1) (string-append var "^" (number->string k)) (string-append (qn coef) "*" var "^" (number->string k))))))
(define (poly->s p var) (if (null? p) "0" (ps p var 0 "")))
(define (ps p var k acc)
  (if (null? p) (if (equal? acc "") "0" acc)
    (if (= (car p) 0) (ps (cdr p) var (+ k 1) acc)
        (ps (cdr p) var (+ k 1) (if (equal? acc "") (term->s (car p) k var) (string-append acc " + " (term->s (car p) k var)))))))
(define (pade->string ans var)
  (if (equal? ans 'singular) "singular (non-normal Pade)"
      (string-append "(" (poly->s (car ans) var) ") / (" (poly->s (car (cdr ans)) var) ")")))
