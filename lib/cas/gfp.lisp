; -*- lisp -*-
; lib/cas/gfp.lisp -- arithmetic in the finite field GF(p^n).
;
; The field with p^n elements is F_p[x] / (m), where m is any monic irreducible of degree
; n; this module finds the smallest such m with the irreducibility test from the
; finite-field factoriser, then represents elements as polynomials of degree < n and does
; all arithmetic modulo m.  Addition is coefficientwise mod p, multiplication is polynomial
; multiplication reduced mod m, and the inverse uses Fermat: a^(p^n - 2) = a^(-1) since the
; multiplicative group has order p^n - 1.
;
; Four independent facts certify the construction: the chosen modulus is irreducible (so
; the quotient really is a field), a * a^(-1) = 1 for every nonzero element, the Frobenius
; identity a^(p^n) = a holds for every element, and a primitive element's successive powers
; enumerate all p^n - 1 nonzero elements exactly once (the multiplicative group is cyclic).
; So GF(8), GF(16), GF(9), GF(25), GF(27) are built and exercised.  Builds on ffactor.lisp.

(import "cas/ffactor.lisp")

(define (q-size p n) (expt p n))

; ---------- find a monic irreducible of degree n over F_p ----------
(define (mono-from k p n) (append (low-coeffs k p n) (list 1)))   ; degree-n monic: n low coeffs then 1
(define (low-coeffs k p n) (if (= n 0) '() (cons (imod k p) (low-coeffs (quotient k p) p (- n 1)))))
(define (find-irred p n) (fi p n 0))
(define (fi p n k) (let ((m (trim (pnorm (mono-from k p n) p)))) (if (irreducible? m p) m (fi p n (+ k 1)))))
(define (gf-modulus p n) (find-irred p n))

; ---------- field arithmetic modulo m ----------
(define (gf-add a b p) (padd a b p))
(define (gf-sub a b p) (psub a b p))
(define (gf-mul a b m p) (pmod (pmul a b p) m p))
(define (gf-pow a e m p) (ppowmod a e m p))
(define (gf-inv a m p) (ppowmod a (- (expt p (- (pdeg m) 0)) 2) m p))   ; a^(p^n - 2),  n = deg m
(define (gf-one) (list 1))
(define (gf-zero? a) (pzero? a))

; ---------- enumerate field elements (as degree < n polynomials) ----------
(define (gf-elt k p n) (trim (pnorm (low-coeffs k p n) p)))           ; k in 0..p^n-1 -> element
(define (gf-range p n) (gr 0 (q-size p n) p n))
(define (gr k hi p n) (if (>= k hi) '() (cons (gf-elt k p n) (gr (+ k 1) hi p n))))

; ---------- element order and primitive elements ----------
(define (gf-order a m p) (gord a (gf-mul a a m p) m p 1))             ; smallest k>0 with a^k = 1
(define (gord a cur m p k) (if (equal? cur (gf-one)) (+ k 1) (gord a (gf-mul cur a m p) m p (+ k 1))))
(define (gf-primitive? a m p) (= (gf-order a m p) (- (q-size p (pdeg m)) 1)))
(define (find-primitive m p) (fp-go m p 1))
(define (fp-go m p k) (let ((a (gf-elt k p (pdeg m)))) (if (and (not (gf-zero? a)) (gf-primitive? a m p)) a (fp-go m p (+ k 1)))))

; ---------- certificates ----------
(define (gf-field-ok? p n) (irreducible? (gf-modulus p n) p))        ; modulus irreducible => field
(define (inv-ok-one a m p) (or (gf-zero? a) (equal? (gf-mul a (gf-inv a m p) m p) (gf-one))))
(define (gf-inverses-ok? p n) (let ((m (gf-modulus p n))) (all-inv (gf-range p n) m p)))
(define (all-inv es m p) (cond ((null? es) #t) ((inv-ok-one (car es) m p) (all-inv (cdr es) m p)) (else #f)))
(define (frob-ok-one a m p) (equal? (gf-pow a (q-size p (pdeg m)) m p) (pmod a m p)))   ; a^(p^n) = a
(define (gf-frobenius-ok? p n) (let ((m (gf-modulus p n))) (all-frob (gf-range p n) m p)))
(define (all-frob es m p) (cond ((null? es) #t) ((frob-ok-one (car es) m p) (all-frob (cdr es) m p)) (else #f)))
; a primitive element's powers hit every nonzero element exactly once
(define (powers-of g cnt m p) (po g (gf-one) cnt m p '()))
(define (po g cur cnt m p acc) (if (= cnt 0) acc (po g (gf-mul cur g m p) (- cnt 1) m p (cons cur acc))))
(define (no-dups? l) (cond ((null? l) #t) ((member-eq? (car l) (cdr l)) #f) (else (no-dups? (cdr l)))))
(define (member-eq? x l) (cond ((null? l) #f) ((equal? x (car l)) #t) (else (member-eq? x (cdr l)))))
(define (gf-primitive-generates? p n)
  (let ((m (gf-modulus p n)))
    (let ((g (find-primitive m p)))
      (let ((ps (powers-of g (- (q-size p n) 1) m p)))
        (and (= (length ps) (- (q-size p n) 1)) (no-dups? ps))))))

; ---------- display ----------
(define (gf->string a) (poly->string a))
(define (gf-modulus->string p n) (poly->string (gf-modulus p n)))
