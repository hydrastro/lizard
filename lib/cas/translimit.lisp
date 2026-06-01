; -*- lisp -*-
; lib/cas/translimit.lisp -- limits of indeterminate forms via Taylor series.
;
; A 0/0 (or related) limit of f(x)/g(x) as x -> 0 is resolved exactly by expanding
; numerator and denominator as power series over Q and comparing leading orders -- the
; series form of L'Hopital's rule, with no derivatives and no floating point.  Where the
; rational-limit module handles only ratios of polynomials, this works for combinations of
; the elementary functions exp, sin, cos, log(1+x), and tan (the last as sin/cos), built
; from their exact Taylor coefficients in series.lisp.
;
; The value is independently certified: for a finite limit L the series of f - L*g must
; vanish to strictly higher order than g (so f/g -> L); for the limit 0 the numerator must
; vanish faster than the denominator; for an infinite limit, slower.  This re-derives the
; limit without reusing the leading-coefficient quotient, so a wrong value is caught.
;
; Builds on series.lisp (exact Taylor series over Q).

(import "cas/series.lisp")

(define (zeros k) (if (= k 0) '() (cons 0 (zeros (- k 1)))))
(define (id-series N) (ser-trunc (list 0 1) N))            ; x
(define (const-series c N) (ser-trunc (list c) N))         ; c
(define (xpow k N) (ser-trunc (append (zeros k) (list 1)) N))  ; x^k
(define (tan-series N) (ser-div (sin-series N) (cos-series N) N))

; the limit of g/h as x -> 0
(define (slimit g h) (limit-ratio g h))

; ---------- certificate ----------
(define (slimit-ok? g h)
  (let ((L (limit-ratio g h)) (a (ser-order g)) (b (ser-order h)))
    (cond ((equal? L 'undefined) (< b 0))
          ((equal? L 'infinite) (and (>= a 0) (< a b)))
          ((equal? L 0) (or (< a 0) (> a b)))
          (else (> (ser-order (ser-sub g (ser-scale L h))) b)))))

; ---------- display ----------
(define (limit->string v)
  (cond ((equal? v 'infinite) "infinite")
        ((equal? v 'undefined) "undefined")
        ((integer? v) (number->string v))
        (else (string-append (number->string (numerator v)) "/" (number->string (denominator v))))))
