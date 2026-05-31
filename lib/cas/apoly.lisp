; -*- lisp -*-
; lib/cas/apoly.lisp — polynomials whose coefficients live in an algebraic
; number field Q(alpha) = Q[x]/(minpoly).
;
; Same dense low-to-high representation as lib/cas/poly.lisp, but each
; coefficient is an algebraic number (an `alg` value carrying the shared
; minimal polynomial).  Because Q(alpha) is a field, polynomial long division
; and a monic Euclidean GCD work exactly as over Q.  This is the coefficient
; layer the algebraic-residue case of Rothstein-Trager needs: computing
; gcd(p - c q', q) when the residue c is a genuine algebraic number.
;
; Top-level helpers only; builds on lib/cas/algnum.lisp.

(import "cas/algnum.lisp")

(define (alg-zero-like x) (alg-zero (alg-min x)))
(define (alg-zeros-list n z) (if (= n 0) '() (cons z (alg-zeros-list (- n 1) z))))

(define (atrim-rev rc)                     ; rc reversed (high->low): drop leading alg-zeros
  (cond ((null? rc) '()) ((alg-zero? (car rc)) (atrim-rev (cdr rc))) (else rc)))
(define (apoly-norm p) (reverse (atrim-rev (reverse p))))
(define (apoly-zero? p) (null? (apoly-norm p)))
(define (apoly-deg p) (- (length (apoly-norm p)) 1))    ; -1 for the zero polynomial
(define (apoly-lead p) (car (reverse (apoly-norm p))))

(define (apoly-add a b)
  (cond ((null? a) b) ((null? b) a)
        (else (cons (alg-add (car a) (car b)) (apoly-add (cdr a) (cdr b))))))
(define (apoly-neg p) (map alg-neg p))
(define (apoly-sub a b) (apoly-add a (apoly-neg b)))
(define (apoly-scale c p) (map (lambda (x) (alg-mul c x)) p))   ; c is an algebraic number

(define (apoly-mul a b)
  (if (or (null? a) (null? b)) '()
    (apoly-add (apoly-scale (car a) b)
               (cons (alg-zero-like (car b)) (apoly-mul (cdr a) b)))))

(define (apoly-monomial c k) (append (alg-zeros-list k (alg-zero-like c)) (list c)))   ; c x^k

(define (apoly-divmod-loop r d q)
  (if (< (apoly-deg r) (apoly-deg d)) (list (apoly-norm q) (apoly-norm r))
    (let ((c (alg-div (apoly-lead r) (apoly-lead d))) (k (- (apoly-deg r) (apoly-deg d))))
      (let ((t (apoly-monomial c k)))
        (apoly-divmod-loop (apoly-sub r (apoly-mul t d)) d (apoly-add q t))))))
(define (apoly-divmod num den) (apoly-divmod-loop (apoly-norm num) (apoly-norm den) '()))
(define (apoly-rem a b) (car (cdr (apoly-divmod a b))))
(define (apoly-div a b) (car (apoly-divmod a b)))

(define (apoly-monic p) (if (apoly-zero? p) p (apoly-scale (alg-inv (apoly-lead p)) p)))
(define (apoly-gcd a b) (if (apoly-zero? b) (apoly-monic a) (apoly-gcd b (apoly-rem a b))))

(define (apoly-deriv-terms p i)            ; p low->high, multiply coeff i by i
  (if (null? p) '() (cons (alg-scale i (car p)) (apoly-deriv-terms (cdr p) (+ i 1)))))
(define (apoly-deriv p) (if (or (null? p) (null? (cdr p))) '() (apoly-deriv-terms (cdr p) 1)))

; embed a polynomial over Q into Q(alpha)[x]
(define (apoly-embed qpoly minp) (map (lambda (c) (alg-from-q minp c)) qpoly))

; render Q(alpha)[x] poly in variable v (coefficients via alg->string)
(define (apoly->string p v) (apoly-terms (reverse (apoly-norm p)) (- (length (apoly-norm p)) 1) v #t))
(define (apoly-terms rc deg v first)
  (if (null? rc) (if first "0" "")
    (if (alg-zero? (car rc)) (apoly-terms (cdr rc) (- deg 1) v first)
      (string-append (if first "" " + ")
                     "(" (alg->string (car rc)) ")"
                     (cond ((= deg 0) "") ((= deg 1) (string-append "*" v))
                           (else (string-append "*" v "^" (number->string deg))))
                     (apoly-terms (cdr rc) (- deg 1) v #f)))))
