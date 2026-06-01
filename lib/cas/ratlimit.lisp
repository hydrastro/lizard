; -*- lisp -*-
; lib/cas/ratlimit.lisp — exact limits of rational functions at any point and at
; infinity, by local Taylor expansion.
;
; To evaluate lim_{x->a} p(x)/q(x), expand both polynomials about a (the coefficients
; of p(a+t) are the Taylor coefficients of p at a, obtained exactly by repeated
; synthetic division by x-a).  The limit is then lim_{t->0} P(t)/Q(t), resolved by
; comparing the orders of vanishing -- which handles ordinary points, removable 0/0
; singularities (the value is the ratio of the leading nonzero coefficients, an exact
; L'Hopital), and poles (reported as infinite).  At infinity the answer is read off
; the degrees: 0 if deg p < deg q, the ratio of leading coefficients if equal, and
; infinite if deg p > deg q.
;
; Everything is exact over Q; the local expansion itself is the certificate.  Builds
; on poly.lisp and reuses the series order/limit machinery from series.lisp.

(import "cas/poly.lisp")
(import "cas/series.lisp")

; Taylor coefficients of p at a (low to high): p(a+t) = c_0 + c_1 t + ...
(define (taylor-at p a)
  (if (poly-zero? p) '()
    (cons (poly-eval p a) (taylor-at (car (poly-divmod p (list (- 0 a) 1))) a))))

; lim_{x->a} p/q  (a rational)
(define (ratfun-limit p q a) (limit-ratio (taylor-at p a) (taylor-at q a)))

; lim_{x->inf} p/q
(define (ratfun-limit-inf p q)
  (let ((dp (poly-deg p)) (dq (poly-deg q)))
    (cond ((< dp dq) 0)
          ((= dp dq) (/ (poly-lead p) (poly-lead q)))
          (else 'infinite))))

; pretty value
(define (limit->string v) (cond ((equal? v 'infinite) "infinite") ((equal? v 'undefined) "undefined") ((integer? v) (number->string v)) (else (string-append (number->string (numerator v)) "/" (number->string (denominator v))))))
