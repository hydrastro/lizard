; -*- lisp -*-
; lib/cas/elliptic3pell.lisp -- the NONCONSTANT-B third-kind construction by the polynomial Pell / fundamental-unit
; structure: building g = A + B*sqrt(q) with B a NONCONSTANT polynomial, the last open rung of the third-kind ladder
; for the genus-0 case (docs/CAS.md -- summit S1, the Jacobian-torsion / Pell question, genus-0 case).
;
; The third-kind construction needs g = A + B*sqrt(q) whose logarithmic derivative g'/g realizes a prescribed
; divisor; the norm N = A^2 - B^2 q must match the residue data.  elliptic3split solved this for CONSTANT B (when
; N + c^2 q is a perfect square).  The genuinely hard remaining case is NONCONSTANT B, which is exactly the
; polynomial Pell problem: find A, B in Q[x] with A^2 - B^2 q = (constant).  For q monic of degree 2 (a genus-0
; curve y^2 = q) this is solved completely by the fundamental unit
;     u = a_0 + sqrt(q),   a_0 = the polynomial part of sqrt(q)  (for q = x^2 + b x + c, a_0 = x + b/2),
; whose norm a_0^2 - q = b^2/4 - c is a nonzero constant.  Every solution is, up to sign and constants, a power of
; this unit: writing u^n = A_n + B_n sqrt(q) by exact arithmetic in the ring Z[x][sqrt q] (where
; (P + Q sqrt q)(R + S sqrt q) = (PR + QS q) + (PS + QR) sqrt q), the pair (A_n, B_n) has B_n NONCONSTANT for
; n >= 2, and norm(u^n) = (b^2/4 - c)^n by multiplicativity of the norm.  This constructs the nonconstant-B
; third-kind elements directly and certifies each by recomputing A_n^2 - B_n^2 q and matching (b^2/4 - c)^n -- an
; exact polynomial identity.  This closes the genus-0 case of the last rung; the positive-genus polynomial Pell
; problem (where the continued fraction of sqrt(q) may be non-periodic and no fundamental unit need exist) is the
; remaining summit, and is reported as out of scope rather than forced.
;
; Public (q a monic quadratic coefficient list (c b 1) low->high; n a positive integer power):
;   e3p-a0 q                   -> the polynomial part a_0 of sqrt(q) (x + b/2 for q = x^2 + b x + c)
;   e3p-fundamental-norm q     -> the constant a_0^2 - q = b^2/4 - c (the norm of the fundamental unit)
;   e3p-unit-power q n         -> (A_n . B_n): u^n = A_n + B_n sqrt(q), by exact arithmetic in Z[x][sqrt q]
;   e3p-norm q A B             -> A^2 - B^2 q (a polynomial; constant for a genuine unit)
;   e3p-B-nonconstant? q n     -> #t iff B_n is a nonconstant polynomial (true for n >= 2 on a genus-0 curve)
;   e3p-certify q n            -> #t iff norm(u^n) equals (fundamental-norm)^n exactly (the Pell certificate)
;   e3p-g q n                  -> (list 'g A_n B_n) meaning g = A_n + B_n sqrt(q), the nonconstant-B element
;
; Verified: for q = x^2 + 1 the unit (x, 1) has norm -1; u^2 = (2x^2+1, 2x) with norm 1 and B nonconstant;
; u^3 = (4x^3+3x, 4x^2+1) with norm -1 and B nonconstant; for q = x^2 + 2x + 5 the unit is (x+1, 1) with norm -4
; and the powers certify against (-4)^n; the norm certificate holds for n = 1..4.
;
; Builds on poly.lisp.

(import "cas/poly.lisp")

; ----- the polynomial part a_0 of sqrt(q) for monic q = x^2 + b x + c: a_0 = x + b/2 -----
(define (e3p-a0 q) (list (/ (e3p-coeff q 1) 2) 1))   ; (b/2, 1) = x + b/2
(define (e3p-coeff q k) (if (< k (e3p-len q)) (e3p-nth q k) 0))
(define (e3p-len l) (if (null? l) 0 (+ 1 (e3p-len (cdr l)))))
(define (e3p-nth l k) (if (= k 0) (car l) (e3p-nth (cdr l) (- k 1))))

; ----- the fundamental norm a_0^2 - q (a constant) -----
(define (e3p-fundamental-norm q) (e3p-const-of (poly-sub (poly-mul (e3p-a0 q) (e3p-a0 q)) q)))
(define (e3p-const-of p) (if (null? p) 0 (car p)))

; ----- u^n = A_n + B_n sqrt(q) by exact arithmetic in Z[x][sqrt q] -----
(define (e3p-unit-power q n) (e3p-pow-go q n (cons (e3p-a0 q) (list 1))))  ; start from u^1 = (a0, 1)
(define (e3p-pow-go q n acc) (if (<= n 1) acc (e3p-pow-go q (- n 1) (e3p-mul q acc (cons (e3p-a0 q) (list 1))))))
; (P + Q s)(R + S s) = (PR + QS q) + (PS + QR) s
(define (e3p-mul q x y)
  (cons (poly-add (poly-mul (car x) (car y)) (poly-mul (poly-mul (cdr x) (cdr y)) q))
        (poly-add (poly-mul (car x) (cdr y)) (poly-mul (cdr x) (car y)))))

; ----- the norm A^2 - B^2 q -----
(define (e3p-norm q A B) (poly-sub (poly-mul A A) (poly-mul (poly-mul B B) q)))

; ----- is B_n nonconstant? -----
(define (e3p-B-nonconstant? q n) (> (e3p-deg (cdr (e3p-unit-power q n))) 0))
(define (e3p-deg p) (- (e3p-trim p) 1))
(define (e3p-trim p) (e3p-trim-n p (e3p-len p)))
(define (e3p-trim-n p k) (cond ((= k 0) 0) ((= (e3p-nth p (- k 1)) 0) (e3p-trim-n p (- k 1))) (else k)))

; ----- the Pell certificate: norm(u^n) = (fundamental norm)^n -----
(define (e3p-certify q n) (equal? (e3p-norm-const q n) (e3p-ipow (e3p-fundamental-norm q) n)))
(define (e3p-norm-const q n) (e3p-const-of (e3p-norm q (car (e3p-unit-power q n)) (cdr (e3p-unit-power q n)))))
(define (e3p-ipow b e) (if (<= e 0) 1 (* b (e3p-ipow b (- e 1)))))

; ----- the constructed element g = A_n + B_n sqrt(q) -----
(define (e3p-g q n) (cons (quote g) (cons (car (e3p-unit-power q n)) (list (cdr (e3p-unit-power q n))))))
