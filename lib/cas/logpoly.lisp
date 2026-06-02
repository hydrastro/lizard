; -*- lisp -*-
; lib/cas/logpoly.lisp -- integration of the primitive (logarithmic) polynomial part of a
; height-one tower: a polynomial sum_{k=0..n} a_k(x) theta^k in theta = log x, with the a_k
; polynomials in x.  This is the logarithmic counterpart of expoly.lisp's exponential case.
;
; The derivation sends theta to 1/x, so for an antiderivative sum b_k theta^k the coefficient of
; theta^k in its derivative is b_k' + (k+1) b_{k+1}/x.  Writing b_k = x c_k (the antiderivative of
; a polynomial-in-log with polynomial coefficients has its coefficients vanish at x = 0) turns the
; matching condition into a_k = c_k + x c_k' + (k+1) c_{k+1}.  Because (x c)' = c + x c', solving
; c_k + x c_k' = R for a polynomial R is immediate: matching x^j gives (j+1) c_{k,j} = R_j, so
; c_k is R with its j-th coefficient divided by j+1.  Processing k from the top down (with
; c_{n+1} = 0) therefore yields every coefficient in closed form, no linear system required, and
; the result is exact -- this class is always elementary.  Each answer is certified by computing
; the tower derivative and comparing to the input.  Builds on poly.lisp.

(import "cas/poly.lisp")

(define (lp-len l) (if (null? l) 0 (+ 1 (lp-len (cdr l)))))
(define (lp-nth l k) (cond ((null? l) '()) ((= k 0) (car l)) (else (lp-nth (cdr l) (- k 1)))))
(define (lp-divx b) (if (null? b) '() (cdr b)))                 ; b / x  (b has zero constant term)
(define (lp-timesx c) (if (null? c) '() (poly-norm (cons 0 c))))  ; x * c

; solve c + x c' = R for polynomial c:  c_j = R_j / (j+1)
(define (lp-solve-go p j) (if (null? p) '() (cons (/ (car p) (+ j 1)) (lp-solve-go (cdr p) (+ j 1)))))
(define (lp-solve R) (poly-norm (lp-solve-go R 0)))

; terms = (a_0 a_1 ... a_n) polynomials -> antiderivative coefficients (b_0 ... b_n), b_k = x c_k
(define (lp-loop rev k cnext bacc)
  (if (null? rev) bacc
      (let ((ck (lp-solve (poly-sub (car rev) (poly-scale (+ k 1) cnext)))))
        (lp-loop (cdr rev) (- k 1) ck (cons (lp-timesx ck) bacc)))))
(define (int-log-poly terms) (lp-loop (lp-reverse terms '()) (- (lp-len terms) 1) '() '()))
(define (lp-reverse l acc) (if (null? l) acc (lp-reverse (cdr l) (cons (car l) acc))))

; --- certificate: tower derivative of sum b_k theta^k, coefficient of theta^k ---
(define (lp-deriv-coeff bs k)
  (poly-add (poly-deriv (lp-nth bs k)) (poly-scale (+ k 1) (lp-divx (lp-nth bs (+ k 1))))))
(define (lp-peq a b) (poly-zero? (poly-sub a b)))
(define (lp-check terms bs k n) (if (= k n) #t (if (lp-peq (lp-nth terms k) (lp-deriv-coeff bs k)) (lp-check terms bs (+ k 1) n) #f)))
(define (int-log-poly-verify terms) (lp-check terms (int-log-poly terms) 0 (lp-len terms)))
