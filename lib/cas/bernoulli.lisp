; -*- lisp -*-
; lib/cas/bernoulli.lisp -- Bernoulli numbers and Faulhaber's power-sum formula.
;
; The Bernoulli numbers B_0, B_1, B_2, ... over Q come from the recurrence
;     sum_{k=0}^{m} C(m+1, k) B_k = 0    (B_0 = 1),
; which gives B_1 = -1/2, B_2 = 1/6, B_4 = -1/30, B_6 = 1/42, ...  Faulhaber's formula
; then expresses the power sum as a polynomial in n,
;     S_k(n) = sum_{i=1}^{n} i^k = (1/(k+1)) sum_{j=0}^{k} C(k+1, j) B_j^{+} n^{k+1-j},
; using the convention B_1^{+} = +1/2.  The result is a degree k+1 polynomial with zero
; constant term.
;
; Each Faulhaber polynomial is certified exactly: evaluated at m = 0, 1, ..., k+2 it must
; equal the directly computed sum 1^k + ... + m^k.  Agreement at more than k+2 points
; pins down the unique degree k+1 polynomial, so a wrong formula is never returned.
;
; Builds on poly.lisp (poly-eval, poly->string).

(import "cas/poly.lisp")

(define (pnth l i) (if (= i 0) (car l) (pnth (cdr l) (- i 1))))
(define (prange a b) (if (> a b) '() (cons a (prange (+ a 1) b))))
(define (factorial n) (if (= n 0) 1 (* n (factorial (- n 1)))))
(define (binom n k) (/ (factorial n) (* (factorial k) (factorial (- n k)))))

; ---------- Bernoulli numbers ----------
(define (bsum m prev k acc) (if (> k (- m 1)) acc (bsum m prev (+ k 1) (+ acc (* (binom (+ m 1) k) (pnth prev k))))))
(define (bern-m m prev) (if (= m 0) 1 (* (/ -1 (+ m 1)) (bsum m prev 0 0))))
(define (bl m M acc) (if (> m M) acc (bl (+ m 1) M (append acc (list (bern-m m acc))))))
(define (bern-list M) (bl 0 M '()))
(define (bernoulli n) (pnth (bern-list n) n))

; ---------- Faulhaber polynomial S_k(n) ----------
(define (bplus j B) (if (= j 1) (/ 1 2) (pnth B j)))
(define (fa-coeff k B deg)
  (if (= deg 0) 0
    (let ((j (- (+ k 1) deg)))
      (if (or (< j 0) (> j k)) 0 (* (/ 1 (+ k 1)) (binom (+ k 1) j) (bplus j B))))))
(define (faulhaber-poly k) (let ((B (bern-list k))) (map (lambda (deg) (fa-coeff k B deg)) (prange 0 (+ k 1)))))

; ---------- direct sum and certificate ----------
(define (sp i n k acc) (if (> i n) acc (sp (+ i 1) n k (+ acc (expt i k)))))
(define (sumpow n k) (sp 1 n k 0))
(define (fok P k m M) (cond ((> m M) #t) ((= (poly-eval P m) (sumpow m k)) (fok P k (+ m 1) M)) (else #f)))
(define (faulhaber-ok? k) (fok (faulhaber-poly k) k 0 (+ k 2)))

; ---------- display ----------
(define (faulhaber->string k) (poly->string (faulhaber-poly k) "n"))
(define (qn x) (if (integer? x) (number->string x) (string-append (number->string (numerator x)) "/" (number->string (denominator x)))))
(define (bern->string n) (qn (bernoulli n)))
