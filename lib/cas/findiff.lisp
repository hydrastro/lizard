; -*- lisp -*-
; lib/cas/findiff.lisp -- finite differences and Newton forward-difference interpolation.
;
; The discrete analogue of calculus.  Given values y_0, ..., y_n at the integer nodes
; 0, ..., n, the forward difference operator (D y)_i = y_{i+1} - y_i builds a difference
; table whose leading entries D^k y_0 are the Newton coefficients.  The unique interpolating
; polynomial of degree <= n is then
;
;     P(x) = sum_k  (D^k y_0) * C(x,k),     C(x,k) = x (x-1) ... (x-k+1) / k! ,
;
; which this module returns in ordinary monomial form over the rationals.  Summation is the
; discrete integral: since sum_{x=0}^{m-1} C(x,k) = C(m,k+1) (the hockey-stick identity),
;
;     sum_{i=0}^{m-1} P(i) = sum_k (D^k y_0) * C(m, k+1),
;
; a closed form, and the antidifference Q (with Q(x+1) - Q(x) = P(x)) is obtained by
; shifting the Newton coefficients up one index.  Faulhaber's power-sum polynomials fall
; out by interpolating x^p.  Everything is checked independently: the polynomial reproduces
; the data at every node, and the closed-form sums equal brute-force sums.  Self-contained.

; ---------- minimal polynomial arithmetic (coefficients low-to-high) ----------
(define (fd-pow x i) (if (= i 0) 1 (* x (fd-pow x (- i 1)))))
(define (fd-eval p x) (fd-ev p x 0 0))
(define (fd-ev p x i acc) (if (null? p) acc (fd-ev (cdr p) x (+ i 1) (+ acc (* (car p) (fd-pow x i))))))
(define (fd-add a b) (cond ((null? a) b) ((null? b) a) (else (cons (+ (car a) (car b)) (fd-add (cdr a) (cdr b))))))
(define (fd-scale c p) (map (lambda (k) (* c k)) p))
(define (fd-mullin p j) (fd-add (cons 0 p) (fd-scale (- 0 j) p)))     ; (x - j) * p
(define (fd-trim p) (cond ((null? p) '()) ((null? (cdr p)) (if (= (car p) 0) '() p)) (else (let ((r (fd-trim (cdr p)))) (if (and (null? r) (= (car p) 0)) '() (cons (car p) r))))))
(define (fd-deg p) (let ((q (fd-trim p))) (if (null? q) 0 (- (length q) 1))))
(define (fd-range a b) (if (> a b) '() (cons a (fd-range (+ a 1) b))))

; ---------- combinatorial helpers ----------
(define (fd-fact n) (if (= n 0) 1 (* n (fd-fact (- n 1)))))
(define (fd-binom n k) (cond ((< k 0) 0) ((> k n) 0) (else (fd-bl n (if (< (- n k) k) (- n k) k) 1 1))))
(define (fd-bl n k i acc) (if (> i k) acc (fd-bl n k (+ i 1) (quotient (* acc (+ (- n i) 1)) i))))
(define (fd-falling k) (fd-fall k 0 (list 1)))                       ; x(x-1)...(x-k+1)
(define (fd-fall k j acc) (if (>= j k) acc (fd-fall k (+ j 1) (fd-mullin acc j))))

; ---------- difference table and Newton coefficients ----------
(define (deltas l) (if (or (null? l) (null? (cdr l))) '() (cons (- (car (cdr l)) (car l)) (deltas (cdr l)))))
(define (newton-coeffs ys) (if (null? ys) '() (cons (car ys) (newton-coeffs (deltas ys)))))

; ---------- the interpolating polynomial (monomial form) ----------
(define (newton-poly ys) (np-go (newton-coeffs ys) 0 '()))
(define (np-go a k acc) (if (null? a) acc (np-go (cdr a) (+ k 1) (fd-add acc (fd-scale (/ (car a) (fd-fact k)) (fd-falling k))))))

; ---------- closed-form summation and antidifference ----------
(define (sum-from-newton a m) (sfn a m 0 0))
(define (sfn a m k acc) (if (null? a) acc (sfn (cdr a) m (+ k 1) (+ acc (* (car a) (fd-binom m (+ k 1)))))))
(define (sum-values ys m) (sum-from-newton (newton-coeffs ys) m))    ; sum_{i=0}^{m-1} P(i)
(define (antidiff p) (ad-go (newton-coeffs (map (lambda (i) (fd-eval p i)) (fd-range 0 (fd-deg p)))) 0 '()))
(define (ad-go b k acc) (if (null? b) acc (ad-go (cdr b) (+ k 1) (fd-add acc (fd-scale (/ (car b) (fd-fact (+ k 1))) (fd-falling (+ k 1)))))))

; ---------- Faulhaber: power-sum polynomial for sum of p-th powers ----------
(define (pow-list p) (map (lambda (i) (fd-pow i p)) (fd-range 0 p)))
(define (power-sum p n) (sum-from-newton (newton-coeffs (pow-list p)) (+ n 1)))   ; sum_{i=0}^{n} i^p

; ---------- certificates ----------
(define (all-true l) (cond ((null? l) #t) ((car l) (all-true (cdr l))) (else #f)))
(define (interp-ok? ys) (let ((p (newton-poly ys))) (all-true (map (lambda (i) (= (fd-eval p (car i)) (car (cdr i)))) (fd-zip-index ys)))))
(define (fd-zip-index ys) (fz ys 0))
(define (fz ys i) (if (null? ys) '() (cons (list i (car ys)) (fz (cdr ys) (+ i 1)))))
(define (degree-ok? ys) (<= (fd-deg (newton-poly ys)) (- (length ys) 1)))
(define (sum-direct ys m) (let ((p (newton-poly ys))) (sd p m 0 0)))
(define (sd p m i acc) (if (>= i m) acc (sd p m (+ i 1) (+ acc (fd-eval p i)))))
(define (sum-ok? ys m) (= (sum-values ys m) (sum-direct ys m)))
(define (antidiff-ok? p) (let ((q (antidiff p))) (all-true (map (lambda (x) (= (- (fd-eval q (+ x 1)) (fd-eval q x)) (fd-eval p x))) (fd-range 0 (+ (fd-deg p) 2))))))
(define (faulhaber-direct p n) (fh p n 0 0))
(define (fh p n i acc) (if (> i n) acc (fh p n (+ i 1) (+ acc (fd-pow i p)))))
(define (faulhaber-ok? p n) (= (power-sum p n) (faulhaber-direct p n)))

; ---------- display ----------
(define (qstr x) (if (integer? x) (number->string x) (string-append (number->string (numerator x)) "/" (number->string (denominator x)))))
(define (terms->string p i) (cond ((null? p) "") ((= (car p) 0) (terms->string (cdr p) (+ i 1))) ((= i 0) (string-append (qstr (car p)) (rest->string (cdr p) 1))) (else (string-append (qstr (car p)) "*x^" (number->string i) (rest->string (cdr p) (+ i 1))))))
(define (rest->string p i) (cond ((null? p) "") ((= (car p) 0) (rest->string (cdr p) (+ i 1))) (else (string-append " + " (qstr (car p)) (if (= i 0) "" (string-append "*x^" (number->string i))) (rest->string (cdr p) (+ i 1))))))
(define (newton-poly->string ys) (let ((p (fd-trim (newton-poly ys)))) (if (null? p) "0" (terms->string p 0))))
