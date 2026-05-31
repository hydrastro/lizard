; -*- lisp -*-
; lib/cas/algnum.lisp — arithmetic in an algebraic number field Q(alpha).
;
; An element is (list 'alg minpoly rep): `minpoly` is a monic irreducible
; polynomial over Q (the minimal polynomial of the field generator alpha), and
; `rep` is a polynomial over Q of degree < deg(minpoly) representing the element
; as rep(alpha) in Q[x]/(minpoly).  Because minpoly is irreducible, Q[x]/(minpoly)
; is a field, so inverses exist (extended Euclid mod minpoly).
;
; This gives exact arithmetic with numbers like sqrt(2) or the golden ratio, and
; the ability to evaluate a rational polynomial at an algebraic number -- the
; substitution check used to certify roots in the solver, and the coefficient
; arithmetic the Rothstein-Trager log part will need.
;
; Top-level helpers only; builds on lib/cas/poly.lisp.

(import "cas/poly.lisp")

(define (alg-min a) (car (cdr a)))
(define (alg-rep a) (car (cdr (cdr a))))
(define (alg-make minp rep) (list 'alg minp (poly-rem rep minp)))

(define (alg-from-q minp q) (alg-make minp (const-poly q)))
(define (alg-gen minp) (alg-make minp (list 0 1)))          ; alpha itself
(define (alg-zero minp) (list 'alg minp '()))
(define (alg-one minp) (alg-make minp (list 1)))

(define (alg-add a b) (alg-make (alg-min a) (poly-add (alg-rep a) (alg-rep b))))
(define (alg-sub a b) (alg-make (alg-min a) (poly-sub (alg-rep a) (alg-rep b))))
(define (alg-mul a b) (alg-make (alg-min a) (poly-mul (alg-rep a) (alg-rep b))))
(define (alg-neg a)   (list 'alg (alg-min a) (poly-neg (alg-rep a))))
(define (alg-scale c a) (alg-make (alg-min a) (poly-scale c (alg-rep a))))
(define (alg-zero? a) (poly-zero? (alg-rep a)))
(define (alg-equal? a b) (equal? (alg-rep a) (alg-rep b)))

; inverse via extended Euclid: t*rep + s*minpoly = gcd (a nonzero constant),
; so rep^{-1} = t / gcd  (mod minpoly).  Tracks the cofactor of `rep`.
(define (alg-eea old-r r old-t t)
  (if (poly-zero? r) (list old-r old-t)
    (let ((q (poly-div old-r r)))
      (alg-eea r (poly-sub old-r (poly-mul q r)) t (poly-sub old-t (poly-mul q t))))))
(define (alg-inv a)
  (let ((minp (alg-min a)))
    (let ((res (alg-eea minp (alg-rep a) '() (list 1))))
      (alg-make minp (poly-scale (/ 1 (poly-coeff (car res) 0)) (car (cdr res)))))))
(define (alg-div a b) (alg-mul a (alg-inv b)))

; evaluate a polynomial over Q at an algebraic ELEMENT al (Horner in Q(alpha))
(define (alg-horner hi-lo al acc)
  (if (null? hi-lo) acc
    (alg-horner (cdr hi-lo) al (alg-add (alg-mul acc al) (alg-from-q (alg-min al) (car hi-lo))))))
(define (alg-eval p al) (alg-horner (reverse (poly-norm p)) al (alg-zero (alg-min al))))
; is `al` a root of the rational polynomial p?  (substitute and check zero in Q(alpha))
(define (alg-root? p al) (alg-zero? (alg-eval p al)))

; simplify a surd:  sqrt(D) = s * sqrt(d) with d a squarefree integer (signed), s rational
(define (sf-extract n k s)
  (cond ((> (* k k) n) (list s n))
        ((= (remainder n (* k k)) 0) (sf-extract (quotient n (* k k)) k (* s k)))
        (else (sf-extract n (+ k 1) s))))
(define (simplify-surd D)
  (let ((num (numerator D)) (den (denominator D)))
    (let ((sd (sf-extract (* (abs num) den) 2 1)))
      (list (/ (car sd) den) (if (negative? D) (- 0 (car (cdr sd))) (car (cdr sd)))))))

; ============================================================
;  display:  sqrt-form for quadratic fields, RootOf otherwise
; ============================================================
(define (int-sqrt-or n k) (cond ((< n 0) #f) ((> (* k k) n) #f) ((= (* k k) n) k) (else (int-sqrt-or n (+ k 1)))))
(define (sqrt-rat-or D)                      ; rational sqrt of D, or #f
  (if (negative? D) #f
    (let ((sn (int-sqrt-or (numerator D) 0)) (sd (int-sqrt-or (denominator D) 0)))
      (if (and sn sd) (/ sn sd) #f))))

(define (surd-string d) (if (sqrt-rat-or d) (rat->string (sqrt-rat-or d)) (string-append "sqrt(" (rat->string d) ")")))

; For minpoly x^2 - d (alpha = sqrt(d)), render rep = (p q) as "p + q*sqrt(d)".
(define (alg->string a)
  (let ((minp (alg-min a)) (rep (alg-rep a)))
    (cond ((poly-const? rep) (rat->string (poly-coeff rep 0)))
          ((and (= (poly-deg minp) 2) (= (poly-coeff minp 1) 0) (= (poly-lead minp) 1))
           (alg-quadratic-string rep (- 0 (poly-coeff minp 0))))
          (else (string-append "(" (poly->string rep "r") ")  [r = RootOf(" (poly->string minp "x") ")]")))))

(define (surd-core d)
  (cond ((= d -1) "i")
        ((negative? d) (string-append "i*sqrt(" (rat->string (- 0 d)) ")"))
        (else (string-append "sqrt(" (rat->string d) ")"))))

(define (alg-quadratic-string rep d)         ; rep = p + q*alpha, alpha = sqrt(d)
  (let ((p (poly-coeff rep 0)) (q (poly-coeff rep 1)))
    (cond ((= q 0) (rat->string p))
          ((= p 0) (cond ((= q 1) (surd-core d))
                         ((= q -1) (string-append "-" (surd-core d)))
                         (else (string-append (rat->string q) "*" (surd-core d)))))
          (else (string-append (rat->string p)
                               (if (negative? q) " - " " + ")
                               (let ((aq (abs q))) (if (= aq 1) "" (string-append (rat->string aq) "*")))
                               (surd-core d))))))
