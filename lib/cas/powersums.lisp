; -*- lisp -*-
; lib/cas/powersums.lisp -- power sums of polynomial roots via Newton's identities.
;
; For a polynomial with roots a_1..a_n, the power sums p_k = sum_i a_i^k are determined by
; the coefficients alone, with no root finding.  Writing the monic polynomial as
; prod (x - a_i) = x^n - e_1 x^{n-1} + e_2 x^{n-2} - ..., the elementary symmetric
; functions are e_j = (-1)^j * (coefficient of x^{n-j}).  Newton's identities then give
;
;     p_k = sum_{i=1}^{k-1} (-1)^{i-1} e_i p_{k-i} + (-1)^{k-1} k e_k ,
;
; (with e_i = 0 for i > n), which we iterate to any order.  The result is checked by a
; round trip: reconstructing e_1..e_n back from p_1..p_n via the inverse identity must
; return the original symmetric functions, and p_0 must equal the degree.
;
; This computes the sums exactly even when the roots are irrational or complex: the roots
; of x^2 - x - 1 give the Lucas numbers 2, 1, 3, 4, 7, 11, ...; the roots +-i of x^2 + 1
; give the real sums 2, 0, -2, 0, 2, ...  Self-contained over poly.lisp.

(import "cas/poly.lisp")

(define (pnth l i) (if (= i 0) (car l) (pnth (cdr l) (- i 1))))
(define (prange a b) (if (> a b) '() (cons a (prange (+ a 1) b))))
(define (to-monic p) (poly-scale (/ 1 (poly-lead p)) p))

; elementary symmetric e_j of the roots, from the monic coefficients (e_0 = 1)
(define (e-of monic n i) (cond ((= i 0) 1) ((> i n) 0) (else (* (expt -1 i) (pnth monic (- n i))))))
(define (e-list p) (let ((monic (to-monic p))) (let ((n (poly-deg p))) (map (lambda (j) (e-of monic n j)) (prange 1 n)))))

; ---------- power sums p_1..p_K via Newton ----------
(define (newton-sum monic n prevs k i acc)
  (if (> i (- k 1)) acc (newton-sum monic n prevs k (+ i 1) (+ acc (* (expt -1 (- i 1)) (e-of monic n i) (pnth prevs (- (- k i) 1)))))))
(define (newton-pk monic n prevs k) (+ (newton-sum monic n prevs k 1 0) (* (expt -1 (- k 1)) k (e-of monic n k))))
(define (ps-build monic n K k acc) (if (> k K) acc (ps-build monic n K (+ k 1) (append acc (list (newton-pk monic n acc k))))))
(define (power-sums p K) (ps-build (to-monic p) (poly-deg p) K 1 '()))
(define (power-sum p k) (if (= k 0) (poly-deg p) (pnth (power-sums p k) (- k 1))))

; ---------- reconstruct e_1..e_n from p_1..p_n (inverse Newton) ----------
(define (re-sum ps eacc k i acc) (if (> i k) acc (re-sum ps eacc k (+ i 1) (+ acc (* (expt -1 (- i 1)) (pnth eacc (- k i)) (pnth ps (- i 1)))))))
(define (recon-ek ps eacc k) (* (/ 1 k) (re-sum ps eacc k 1 0)))
(define (re-build ps n k eacc) (if (> k n) (cdr eacc) (re-build ps n (+ k 1) (append eacc (list (recon-ek ps eacc k))))))
(define (recon-e ps n) (re-build ps n 1 (list 1)))

; ---------- certificate ----------
(define (power-sums-ok? p)
  (let ((n (poly-deg p)))
    (and (= (power-sum p 0) n) (equal? (recon-e (power-sums p n) n) (e-list p)))))

; ---------- display ----------
(define (qn x) (if (integer? x) (number->string x) (string-append (number->string (numerator x)) "/" (number->string (denominator x)))))
(define (list->string l) (if (null? l) "" (lg l)))
(define (lg l) (if (null? (cdr l)) (qn (car l)) (string-append (qn (car l)) ", " (lg (cdr l)))))
(define (power-sums->string p K) (list->string (power-sums p K)))
