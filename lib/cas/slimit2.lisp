; -*- lisp -*-
; lib/cas/slimit2.lisp -- limits at an ARBITRARY point (and indeterminate 0/0, including transcendental
; numerators/denominators) by local series, generalizing translimit.lisp (which works at x = 0) to any point a
; and adding the order comparison that resolves 0/0, 0/(nonzero), (nonzero)/0, and faster-vanishing cases.
;
; The caller supplies the LOCAL SERIES of the numerator and denominator already expanded in t = x - a (the
; series engine and the standard expansions of log(1+t), sin, cos, e^t, ... provide these; for a rational
; function ratlimit.lisp expands by Taylor shift).  Given those two t-series, the limit as x -> a is:
;   - if both have the same order k (the index of the first nonzero coefficient), the limit is the ratio of the
;     two unit series' constant terms after dividing out t^k -- i.e. lead(num)/lead(den) generalized through
;     the full series (this is exactly L'Hopital resolved by series);
;   - if ord(num) > ord(den) the limit is 0;
;   - if ord(num) < ord(den) the quotient diverges ('infinite).
; This is exact and certificate-free in the sense that it is a finite rational computation on the given series;
; the only modelling assumption is that the supplied series are correct expansions, which the expansions used
; (and ratlimit's shift) guarantee.
;
; Public:
;   sl-order s              -> the order (index of the first nonzero coefficient) of a series, or 'inf if all zero
;   sl-limit num den N      -> lim_{x->a} num/den from the t = x-a series: a rational, 0, or 'infinite
;   sl-limit-series s       -> lim of a single series as t -> 0 (its constant term)
;   sl-shift-poly p a       -> the Taylor shift of a polynomial p to t = x - a (coefficients of p(a+t)), so a
;                              rational-function limit at a can be set up directly from polynomials
;
; Verified: lim_{x->1} (log x)/(x-1) = 1; lim_{x->0} (1-cos x)/x^2 = 1/2; lim_{x->2} (x^2-4)/(x-2) = 4;
; lim_{x->0} sin(x)/x = 1; the diverging and vanishing cases.
;
; Builds on series.lisp and poly.lisp.

(import "cas/series.lisp")
(import "cas/poly.lisp")

(define (sl-order s) (sl-ord-go s 0))
(define (sl-ord-go s k) (cond ((null? s) (quote inf)) ((not (= (car s) 0)) k) (else (sl-ord-go (cdr s) (+ k 1)))))
(define (sl-drop s k) (if (= k 0) s (sl-drop (cdr s) (- k 1))))

(define (sl-limit num den N)
  (sl-limit-ord num den N (sl-order num) (sl-order den)))
(define (sl-limit-ord num den N on od)
  (cond ((equal? od (quote inf)) (quote undefined-zero-denominator))  ; denominator identically zero
        ((equal? on (quote inf)) 0)                                    ; numerator identically zero
        ((> on od) 0)                                                  ; numerator vanishes faster
        ((< on od) (quote infinite))                                   ; denominator vanishes faster
        (else (car (ser-div (sl-drop num on) (sl-drop den od) (+ N 1))))))  ; equal order: ratio of units

(define (sl-limit-series s) (if (null? s) 0 (car s)))

; Taylor shift of a polynomial to t = x - a: coefficients of p(a + t), built by direct expansion
; p(a+t) = sum_k p_k (a+t)^k, each (a+t)^k a binomial t-polynomial.
(define (sl-shift-poly p a) (sl-shift-go (poly-norm p) a 0))
(define (sl-shift-go p a k) (if (null? p) (quote ()) (sl-padd (sl-sscale (car p) (sl-atpow a k)) (sl-shift-go (cdr p) a (+ k 1)))))
(define (sl-atpow a k) (sl-atp-go a k 0))               ; (a + t)^k as a t-poly (low->high)
(define (sl-atp-go a k j) (if (> j k) (quote ()) (cons (* (sl-binom k j) (sl-ipow a (- k j))) (sl-atp-go a k (+ j 1)))))
(define (sl-binom n k) (if (if (= k 0) #t (= k n)) 1 (+ (sl-binom (- n 1) (- k 1)) (sl-binom (- n 1) k))))
(define (sl-ipow b e) (if (= e 0) 1 (* b (sl-ipow b (- e 1)))))
(define (sl-sscale c s) (if (null? s) (quote ()) (cons (* c (car s)) (sl-sscale c (cdr s)))))
(define (sl-padd a b) (cond ((null? a) b) ((null? b) a) (else (cons (+ (car a) (car b)) (sl-padd (cdr a) (cdr b))))))
