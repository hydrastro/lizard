; -*- lisp -*-
; lib/cas/linrec.lisp -- closed forms for constant-coefficient linear recurrences over Q.
;
; A C-finite recurrence  a_n = c_1 a_{n-1} + ... + c_d a_{n-d}  with initial values
; a_0..a_{d-1} has characteristic polynomial  x^d - c_1 x^{d-1} - ... - c_d.  When that
; polynomial splits into linear factors over Q (all roots rational), the closed form is
;
;     a_n = sum_i  P_i(n) * r_i^n ,   deg P_i < multiplicity of the root r_i,
;
; and the d coefficients of the P_i are fixed by the d initial conditions -- a square
; rational linear system, solved with the Gauss-Jordan solver.  Repeated roots bring the
; polynomial factors n, n^2, ...  The result is verified by evaluating the closed form
; and comparing it to the directly iterated sequence over many terms, so a wrong closed
; form is never reported.  If the characteristic polynomial has an irreducible factor of
; degree >= 2 the roots are irrational/complex; we then report the polynomial and its
; irreducible factorization (the minimal polynomials of the roots) and decline a rational
; closed form rather than guess.  This complements Zeilberger, which DISCOVERS such
; recurrences; here we SOLVE them.
;
; Builds on factor.lisp (factorization over Q), poly.lisp, and gosper.lisp.

(import "cas/factor.lisp")
(import "cas/gosper.lisp")

(define (take-n l n) (if (= n 0) '() (cons (car l) (take-n (cdr l) (- n 1)))))
(define (drop-n l n) (if (= n 0) l (drop-n (cdr l) (- n 1))))
(define (lastn lst d) (drop-n lst (- (length lst) d)))
(define (dot a b) (if (or (null? a) (null? b)) 0 (+ (* (car a) (car b)) (dot (cdr a) (cdr b)))))

; characteristic polynomial of (c_1..c_d): x^d - c_1 x^{d-1} - ... - c_d, dense low->high
(define (crec-charpoly cs) (append (reverse (map (lambda (c) (- 0 c)) cs)) (list 1)))

; ---------- iterate the recurrence: a_0..a_N ----------
(define (crec-terms cs inits N) (ct cs inits N))
(define (ct cs acc N) (if (>= (- (length acc) 1) N) acc (ct cs (append acc (list (dot cs (reverse (lastn acc (length cs)))))) N)))

; ---------- rational roots with multiplicities ----------
(define (linear? f) (= (poly-deg f) 1))
(define (all-linear? fl) (cond ((null? fl) #t) ((linear? (car (cdr (car fl)))) (all-linear? (cdr fl))) (else #f)))
(define (lin-root f) (- 0 (/ (poly-coeff f 0) (poly-coeff f 1))))
(define (rat-roots fl) (map (lambda (e) (cons (lin-root (car (cdr e))) (car e))) fl))   ; list of (root . mult)

; ---------- closed-form solve ----------
(define (npow n j) (if (= j 0) 1 (expt n j)))
(define (rpow r n) (if (= n 0) 1 (expt r n)))
(define (basis-cols roots) (if (null? roots) '() (append (map (lambda (j) (cons (car (car roots)) j)) (iota 0 (- (cdr (car roots)) 1))) (basis-cols (cdr roots)))))
(define (basis-val col k) (* (npow k (cdr col)) (rpow (car col) k)))
(define (lr-rows cols inits d) (map (lambda (k) (append (map (lambda (col) (basis-val col k)) cols) (list (nth inits k)))) (iota 0 (- d 1))))
(define (group-closed roots c) (if (null? roots) '() (cons (cons (car (car roots)) (poly-norm (take-n c (cdr (car roots))))) (group-closed (cdr roots) (drop-n c (cdr (car roots)))))))
(define (crec-solve cs inits)
  (let ((fl (car (cdr (factor-Q (crec-charpoly cs))))))
    (if (not (all-linear? fl)) 'not-rational
      (let ((roots (rat-roots fl)))
        (let ((c (lin-solve (lr-rows (basis-cols roots) inits (length cs)) (length cs))))
          (if (equal? c 'none) 'singular (group-closed roots c)))))))

; ---------- evaluate the closed form at n ----------
(define (peval poly n) (pe (reverse poly) n 0))
(define (pe rc n acc) (if (null? rc) acc (pe (cdr rc) n (+ (* acc n) (car rc)))))
(define (eval-closed form n) (if (null? form) 0 (+ (* (peval (cdr (car form)) n) (rpow (car (car form)) n)) (eval-closed (cdr form) n))))

; ---------- certificate: closed form reproduces the iterated sequence ----------
(define (cf-check form terms k K) (cond ((> k K) #t) ((= (eval-closed form k) (nth terms k)) (cf-check form terms (+ k 1) K)) (else #f)))
(define (crec-ok? cs inits form) (if (or (equal? form 'not-rational) (equal? form 'singular)) #f (cf-check form (crec-terms cs inits (* 2 (length cs))) 0 (* 2 (length cs)))))

; ---------- display ----------
(define (rstr r) (if (integer? r) (number->string r) (string-append (number->string (numerator r)) "/" (number->string (denominator r)))))
(define (cterm->string t) (string-append "[" (poly->string (cdr t) "n") "] * [" (rstr (car t)) "]^n"))
(define (cg form) (cond ((null? form) "0") ((null? (cdr form)) (cterm->string (car form))) (else (string-append (cterm->string (car form)) " + " (cg (cdr form))))))
(define (crec->string form) (if (equal? form 'not-rational) "no rational closed form" (if (equal? form 'singular) "singular" (cg form))))
(define (crec-charpoly->string cs) (poly->string (crec-charpoly cs) "x"))
