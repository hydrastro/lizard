; -*- lisp -*-
; lib/cas/intexp.lisp -- the COMPLETE integrator for a rational function of a single exponential:
; INT A/D dx with A, D polynomials in theta = e^x over Q(x).  The exponential capstone, the mirror
; of intlog.lisp.
;
; A rational function of theta = e^x splits into a Laurent polynomial part (powers of theta, positive
; and negative, since theta is a unit) and a proper part whose denominator is coprime to theta.
; Writing D = theta^j D0 with D0 coprime to theta and choosing S theta^j + T D0 = 1 by Bezout,
;   A/D = A T / theta^j + A S / D0,
; and dividing A S = Q' D0 + R gives the proper part R/D0 (degree of R below D0) plus a polynomial Q'.
; The entire Laurent part is then (A T + Q' theta^j)/theta^j, a single polynomial in theta shifted
; down by j, whose coefficient of theta^k is the coefficient of theta^{k+j} in that polynomial -- so
; the Laurent coefficients are read off with no overlap.  The Laurent part is integrated by
; expoly.lisp (a differential equation per power) and the proper part by expnrt.lisp (Hermite plus
; tower Rothstein-Trager with the base-field correction).  As in intlog.lisp, correctness follows by
; linearity: the decomposition A = Lnum D0 + R theta^j is an exact identity, and certifying each part
; in its own module certifies the whole.  This integrates, e.g., 1/(e^x(e^x+1)) (a negative power
; plus a proper part) and a mixed e^(2x) plus a two-residue proper part, all certified.
; Builds on expoly.lisp and expnrt.lisp.

(import "cas/expoly.lisp")
(import "cas/expnrt.lisp")

(define (iexp-content-go D i) (cond ((null? D) i) ((rat-zero? (car D)) (iexp-content-go (cdr D) (+ i 1))) (else i)))
(define (iexp-content D) (iexp-content-go D 0))                   ; multiplicity of theta in D
(define (iexp-drop D j) (if (= j 0) D (iexp-drop (cdr D) (- j 1))))

; decompose A/D into a Laurent part Lnum/theta^j and a proper part R/D0
(define (iexp-decompose A D)
  (let ((j (iexp-content D)))
    (let ((D0 (iexp-drop D j)) (thj (rfpoly-monomial (rat-one) j)))
      (let ((bz (rfpoly-bezout thj D0)))
        (let ((gc (rat-inv (rfpoly-lead (car bz)))))
          (let ((S (rfpoly-cscale gc (car (cdr bz)))) (T (rfpoly-cscale gc (car (cdr (cdr bz))))))
            (let ((qr (rfpoly-divmod (rfpoly-mul A S) D0)))
              (list (rfpoly-add (rfpoly-mul A T) (rfpoly-mul (car qr) thj)) j (car (cdr qr)) D0))))))))

; Laurent coefficients of Lnum/theta^j as expoly terms (k anum aden), k = power of theta
(define (iexp-lterms Lnum j i acc)
  (cond ((null? Lnum) (reverse acc))
        ((rat-zero? (car Lnum)) (iexp-lterms (cdr Lnum) j (+ i 1) acc))
        (else (iexp-lterms (cdr Lnum) j (+ i 1) (cons (list (- i j) (car (car Lnum)) (car (cdr (car Lnum)))) acc)))))

(define (iexp-pieces A D) (iexp-decompose A D))                   ; (Lnum j R D0)
(define (iexp-Lnum d) (car d))
(define (iexp-j d) (car (cdr d)))
(define (iexp-R d) (car (cdr (cdr d))))
(define (iexp-D0 d) (car (cdr (cdr (cdr d)))))
(define (iexp-expo) (list 0 1))                                   ; exponent x, so theta = e^x

(define (iexp-split-ok? A d)
  (rfpoly-zero? (rfpoly-sub A (rfpoly-add (rfpoly-mul (iexp-Lnum d) (iexp-D0 d))
                                          (rfpoly-mul (iexp-R d) (rfpoly-monomial (rat-one) (iexp-j d)))))))
(define (int-exp-rational-full-verify A D)
  (let ((d (iexp-decompose A D)))
    (if (iexp-split-ok? A d)
        (if (int-exp-poly-verify (iexp-lterms (iexp-Lnum d) (iexp-j d) 0 '()) (iexp-expo))
            (if (rfpoly-zero? (iexp-R d)) #t (int-exp-rational-verify (iexp-R d) (iexp-D0 d)))
            #f)
        #f)))
(define (int-exp-rational-full-elementary? A D)
  (let ((d (iexp-decompose A D)))
    (if (int-exp-poly-elementary? (iexp-lterms (iexp-Lnum d) (iexp-j d) 0 '()) (iexp-expo))
        (if (rfpoly-zero? (iexp-R d)) #t (int-exp-rational-elementary? (iexp-R d) (iexp-D0 d)))
        #f)))
