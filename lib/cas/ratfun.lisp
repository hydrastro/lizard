; -*- lisp -*-
; lib/cas/ratfun.lisp — rational functions over Q and partial-fraction
; decomposition, built on the exact polynomial layer and the factorizer.
;
; A rational function is (list num den) with num, den coefficient lists over Q.
; Partial fractions decompose p/q into a polynomial part plus a sum of terms
; A / f^j, where each f is an irreducible factor of q (from `factor-Q`) and
; deg A < deg f.  The numerators come from the Chinese-remainder formula
;   P_i = p * (q/m_i)^{-1}  (mod m_i),     m_i = f_i^{e_i},
; followed by the f_i-adic expansion of each P_i.  Every decomposition is
; checked by recombining the terms over a common denominator (`pf-verify`).
;
; Top-level helpers only.

(import "cas/factor.lisp")

; ============================================================
;  extended Euclid over Q:  s*a + t*b = gcd,  then the coprime inverse
; ============================================================
(define (peea old-r r old-s s old-t t)
  (if (poly-zero? r) (list old-r old-s old-t)
    (let ((q (poly-div old-r r)))
      (peea r (poly-sub old-r (poly-mul q r))
            s (poly-sub old-s (poly-mul q s))
            t (poly-sub old-t (poly-mul q t))))))

(define (poly-bezout a b) (peea a b (list 1) '() '() (list 1)))   ; -> (g s t)

(define (poly-bezout1 a b)                ; a,b coprime -> (s t) with s*a + t*b = 1
  (let ((res (poly-bezout a b)))
    (let ((c (poly-coeff (car res) 0)))
      (list (poly-scale (/ 1 c) (car (cdr res)))
            (poly-scale (/ 1 c) (car (cdr (cdr res))))))))

(define (mod-inverse a m) (poly-rem (car (poly-bezout1 a m)) m))   ; a^{-1} mod m

; ============================================================
;  rational-function arithmetic (normalised: gcd-reduced, monic denominator)
; ============================================================
(define (rat-make num den)
  (if (poly-zero? num) (list '() (list 1))
    (let ((g (poly-gcd num den)))
      (let ((n (poly-div num g)) (d (poly-div den g)))
        (let ((lc (poly-lead d)))
          (list (poly-scale (/ 1 lc) n) (poly-scale (/ 1 lc) d)))))))

(define (rat-num r) (car r))
(define (rat-den r) (car (cdr r)))
(define (rat-add r1 r2)
  (rat-make (poly-add (poly-mul (rat-num r1) (rat-den r2))
                      (poly-mul (rat-num r2) (rat-den r1)))
            (poly-mul (rat-den r1) (rat-den r2))))
(define (rat-mul r1 r2)
  (rat-make (poly-mul (rat-num r1) (rat-num r2)) (poly-mul (rat-den r1) (rat-den r2))))
(define (rat-equal? r1 r2)
  (and (equal? (rat-num r1) (rat-num r2)) (equal? (rat-den r1) (rat-den r2))))
(define (rat-add-many base rs) (if (null? rs) base (rat-add-many (rat-add base (car rs)) (cdr rs))))

; ============================================================
;  partial fractions
;  result: (list poly-part terms),  terms = list of (list A f j) meaning A / f^j
; ============================================================
; CRT numerators: P_i = p * (q/m_i)^{-1} mod m_i
(define (pf-numerators p q ms)
  (map (lambda (mi) (poly-rem (poly-mul p (mod-inverse (poly-div q mi) mi)) mi)) ms))

; f-adic digits of P: (d_0 d_1 ... d_{count-1}) with P = sum d_k f^k, deg d_k < deg f
(define (fadic-digits P f count)
  (if (= count 0) '()
    (let ((qr (poly-divmod P f)))
      (cons (car (cdr qr)) (fadic-digits (car qr) f (- count 1))))))

; digit d_k contributes A_{e-k}/f^{e-k}
(define (digits->terms digits f e k)
  (if (null? digits) '()
    (let ((rest (digits->terms (cdr digits) f e (+ k 1))))
      (if (poly-zero? (car digits)) rest (cons (list (car digits) f (- e k)) rest)))))

(define (pf-build-terms Ps irrs)
  (if (null? Ps) '()
    (append (digits->terms (fadic-digits (car Ps) (car (cdr (car irrs))) (car (car irrs)))
                           (car (cdr (car irrs))) (car (car irrs)) 0)
            (pf-build-terms (cdr Ps) (cdr irrs)))))

(define (partial-fractions num den)
  (let ((qr (poly-divmod num den)))
    (let ((Q (car qr)) (R (car (cdr qr))))
      (let ((fz (factor-Q den)))
        (let ((R2 (poly-scale (/ 1 (car fz)) R))
              (ms (map (lambda (mf) (poly-pow (car (cdr mf)) (car mf))) (car (cdr fz)))))
          (list Q (pf-build-terms (pf-numerators R2 (prod-polys ms) ms) (car (cdr fz)))))))))

; recombine and compare to num/den  (the certificate)
(define (term->rat t) (rat-make (car t) (poly-pow (car (cdr t)) (car (cdr (cdr t))))))
(define (pf-verify num den pf)
  (rat-equal? (rat-add-many (rat-make (car pf) (list 1)) (map term->rat (car (cdr pf))))
              (rat-make num den)))

; ============================================================
;  pretty printing:  "x + 2 + 1/(x - 1) - 1/(x - 1)^2 - x/(x^2 + 1)"
; ============================================================
(define (denom-string f j var)
  (if (= j 1) (string-append "(" (poly->string f var) ")")
    (string-append "(" (poly->string f var) ")^" (number->string j))))

(define (term-string t var)               ; A / f^j
  (let ((A (car t)) (f (car (cdr t))) (j (car (cdr (cdr t)))))
    (if (poly-const? A)
        (string-append (rat->string (poly-coeff A 0)) "/" (denom-string f j var))
        (string-append "(" (poly->string A var) ")/" (denom-string f j var)))))

(define (terms-string terms var)
  (if (null? terms) ""
    (let ((rest (terms-string (cdr terms) var)))
      (if (equal? rest "")
          (term-string (car terms) var)
          (string-append (term-string (car terms) var) " + " rest)))))

(define (pf->string pf var)
  (let ((Q (car pf)) (terms (car (cdr pf))))
    (cond ((null? terms) (poly->string Q var))
          ((poly-zero? Q) (terms-string terms var))
          (else (string-append (poly->string Q var) " + " (terms-string terms var))))))
