; -*- lisp -*-
; lib/cas/primint.lisp -- the primitive (logarithmic) polynomial integration problem with RATIONAL
; coefficients: INT (sum_{k=0..n} a_k(x) (log x)^k) dx for a_k in Q(x).  This generalizes
; logpoly.lisp (polynomial coefficients) and is where intermediate logarithms must be absorbed.
;
; The antiderivative is sum_{k=0..n+1} b_k (log x)^k with b_k in Q(x).  Matching the coefficient of
; (log x)^k in the derivative gives a_k = b_k' + (k+1) b_{k+1}/x.  Processing k from the top down,
; b_k is the rational antiderivative of R_k = a_k - (k+1) b_{k+1}/x, which exists exactly when R_k
; integrates to (rational) + lambda_k log x with NO other logarithm; the free constant in b_{k+1}
; is then fixed one level up by C_{k+1} = lambda_k/(k+1), so the log x produced at level k is
; absorbed into the (log x)^{k+1} coefficient.  If any level needs a logarithm of something other
; than x, or has an algebraic (non-rational) residue, the integral is not elementary in this form
; and the procedure reports so -- a genuine obstruction, since those logs cannot be absorbed.  Each
; answer is certified by forming the tower derivative and comparing to the input.  This recovers,
; e.g., INT (1/x) log x dx = (1/2)(log x)^2, INT log x / x^2 dx = -1/x - (log x)/x, and the
; polynomial-coefficient cases of logpoly.lisp.  Builds on rderat.lisp and ratfull.lisp.

(import "cas/rderat.lisp")
(import "cas/ratfull.lisp")

(define (pr-len l) (if (null? l) 0 (+ 1 (pr-len (cdr l)))))
(define (pr-reverse l acc) (if (null? l) acc (pr-reverse (cdr l) (cons (car l) acc))))
(define (pr-invx) (rde-rmake (list 1) (list 0 1)))
(define (pr-const c) (rde-rmake (list c) (list 1)))
(define (pr-zero) (rde-rmake (list 0) (list 1)))
(define (pr-nth l k) (cond ((null? l) (pr-zero)) ((= k 0) (car l)) (else (pr-nth (cdr l) (- k 1)))))

; --- read a rat-integrate-full result (polypart ratnum ratden logterms complete?) ---
(define (pr-polypart res) (car res))
(define (pr-ratnum res) (car (cdr res)))
(define (pr-ratden res) (car (cdr (cdr res))))
(define (pr-logs res) (car (cdr (cdr (cdr res)))))
(define (pr-complete res) (car (cdr (cdr (cdr (cdr res))))))
(define (pr-ratpart res) (rde-radd (cons (pr-polypart res) (list 1)) (cons (pr-ratnum res) (pr-ratden res))))
(define (pr-isx v) (equal? (poly-monic v) (list 0 1)))
(define (pr-logx-go ts) (cond ((null? ts) 0) ((pr-isx (car (cdr (car ts)))) (car (car ts))) (else (pr-logx-go (cdr ts)))))
(define (pr-logx res) (pr-logx-go (pr-logs res)))
(define (pr-onlyx-go ts) (cond ((null? ts) #t) ((pr-isx (car (cdr (car ts)))) (pr-onlyx-go (cdr ts))) (else #f)))
(define (pr-ok res) (if (pr-complete res) (pr-onlyx-go (pr-logs res)) #f))

; --- one level: a_k, k, beta_{k+1} -> (ok beta_k lambda_k b_{k+1}) | (fail) ---
(define (pr-level ak k bnext)
  (let ((Rk (rde-rsub ak (rde-rmul (rde-rmul bnext (pr-invx)) (pr-const (+ k 1))))))
    (let ((res (rat-integrate-full (car Rk) (cdr Rk))))
      (if (pr-ok res)
          (list 'ok (pr-ratpart res) (pr-logx res) (rde-radd bnext (pr-const (/ (pr-logx res) (+ k 1)))))
          (list 'fail)))))

; --- top-down loop; emits b_{n+1}..b_1, then b_0 = beta_0 ---
(define (pr-loop rev k bnext bacc)
  (if (null? rev) (cons bnext bacc)
      (let ((lvl (pr-level (car rev) k bnext)))
        (if (equal? (car lvl) 'fail) 'fail
            (pr-loop (cdr rev) (- k 1) (car (cdr lvl)) (cons (car (cdr (cdr (cdr lvl)))) bacc))))))

; terms = (a_0 a_1 ... a_n), each a rational (num.den) -> (list 'elementary bs) | (list 'non-elementary)
(define (int-prim-poly terms)
  (let ((bs (pr-loop (pr-reverse terms '()) (- (pr-len terms) 1) (pr-zero) '())))
    (if (equal? bs 'fail) (list 'non-elementary) (list 'elementary bs))))

; --- certificate: tower derivative, coefficient of (log x)^k is b_k' + (k+1) b_{k+1}/x ---
(define (pr-coeff bs k)
  (rde-radd (rde-rderiv (pr-nth bs k)) (rde-rmul (rde-rmul (pr-nth bs (+ k 1)) (pr-invx)) (pr-const (+ k 1)))))
(define (pr-check terms bs k top)
  (if (> k top) #t
      (if (rde-rzero? (rde-rsub (pr-coeff bs k) (pr-nth terms k))) (pr-check terms bs (+ k 1) top) #f)))
(define (int-prim-poly-verify terms)
  (let ((r (int-prim-poly terms)))
    (if (equal? (car r) 'non-elementary) #f (pr-check terms (car (cdr r)) 0 (- (pr-len (car (cdr r))) 1)))))
(define (int-prim-poly-elementary? terms) (equal? (car (int-prim-poly terms)) 'elementary))
