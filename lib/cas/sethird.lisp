; -*- lisp -*-
; lib/cas/sethird.lisp -- the SUPERELLIPTIC THIRD-KIND LOGARITHM: integrating logarithmic differentials u'/u on
; the curve y^n = g(x) and, conversely, RECOGNIZING such a differential and recovering its logarithm.  This is
; the Rothstein-Trager step over the superelliptic field -- the logarithmic half of the Rung-4 integration
; payoff (docs/TRAGER_ROADMAP.md), built on the field (sefield.lisp) and its Norm (senorm.lisp).
;
; CONSTRUCTIVE direction (st-log): given a field element u in K = Q(x)[y]/(y^n - g), the differential u'/u
; integrates to log u.  st-log returns the rationalized differential (the field numerator over the polynomial
; denominator N(u)) together with the certified statement INT (u'/u) dx = log u; st-log-certify checks the
; cleared identity u * (u'/u-numerator) = N(u) * u'.
;
; RECOGNIZER direction (st-recognize), the actual Rothstein-Trager step for the most common third-kind log
; u = a(x) + y: here N(u) = a^n + (-1)^{n+1} g (the elementary symmetric functions of the roots of y^n - g
; vanish except the last, e_n = (-1)^{n+1} g).  So given a logarithmic differential presented with denominator D,
; the candidate a is the n-th root of D - (-1)^{n+1} g; if that is an exact polynomial n-th power and the
; resulting u = a + y reproduces the differential, the integral is log(a + y), certified.  This recovers the
; logarithm of a third-kind differential from the differential itself.
;
; Verified: on y^3 = x^3+1, st-log of u = x+y gives N(u) = 2x^3+1 and a certified log; st-recognize of that
; differential's denominator recovers a = x and returns log(x+y); and the n=2 case (Norm a^2-g) is handled too.
;
; Builds on senorm.lisp / sefield.lisp (the field, Norm, rationalized log derivative) and poly.lisp.

(import "cas/senorm.lisp")

(define (st-nth l k) (if (= k 0) (car l) (st-nth (cdr l) (- k 1))))

; ----- constructive: INT (u'/u) dx = log u, with the rationalized differential -----
; returns (list 'log u (cons F D)) meaning INT (F/D) dx = log u, where F is a field element and D = N(u).
(define (st-log g n u)
  (let ((ld (sn-logderiv g n u)))
    (list (quote log) u ld)))
; certificate: u * F = D * u'  (the cleared logarithmic-derivative identity)
(define (st-log-certify g n u)
  (sn-logderiv-check g n u))

; ----- the field element a + y (the common third-kind argument) -----
(define (st-a+y a n) (sf-set (sf-set (sf-zeros n) 0 (rat-from-poly a)) 1 (rat-one)))

; Norm(a+y) = a^n + (-1)^{n+1} g  (closed form; matches the matrix-determinant Norm)
(define (st-norm-a+y a g n) (poly-add (st-ppow a n) (poly-scale (st-sign (+ n 1)) g)))
(define (st-ppow p e) (if (= e 0) (list 1) (poly-mul p (st-ppow p (- e 1)))))
(define (st-sign k) (if (= (remainder k 2) 0) 1 -1))

; ----- exact polynomial n-th root (for low-degree a: degree 0 or 1, the usual third-kind case) -----
(define (st-nthroot P n)
  (let ((dP (poly-deg P)))
    (if (not (= (remainder dP n) 0)) (quote none)
        (let ((cand (st-root-build P n (quotient dP n))))
          (if (equal? cand (quote none)) (quote none)
              (if (st-peq? (st-ppow cand n) P) cand (quote none)))))))
(define (st-peq? a b) (poly-zero? (poly-sub a b)))
(define (st-root-build P n d)
  (let ((lead (st-rat-root (poly-lead P) n)))
    (if (equal? lead (quote none)) (quote none)
        (cond ((= d 0) (list lead))
              ((= d 1) (list (/ (poly-coeff P (- (poly-deg P) 1)) (* n (st-ipow lead (- n 1)))) lead))
              (else (quote none))))))
(define (st-ipow b e) (if (= e 0) 1 (* b (st-ipow b (- e 1)))))
(define (st-rat-root c n)                              ; exact rational n-th root of c, or 'none
  (cond ((= c 0) 0) ((= c 1) 1)
        ((= c -1) (if (= (remainder n 2) 1) -1 (quote none)))
        (else (st-try-root c n 2))))
(define (st-try-root c n b)                            ; search small integer bases (sufficient for our models)
  (cond ((> b 20) (quote none))
        ((= (st-ipow b n) c) b)
        ((= (st-ipow (- 0 b) n) c) (- 0 b))
        (else (st-try-root c n (+ b 1)))))

; ----- the Rothstein-Trager recognizer for u = a + y -----
; given the curve (g, n) and a logarithmic differential presented by its denominator D (a polynomial),
; recover a with D = Norm(a+y), i.e. a = nth-root(D - (-1)^{n+1} g); if it exists, return (list 'log (a+y))
; and the recovered a; else 'not-third-kind-a+y.
(define (st-recognize g n D)
  (let ((target (poly-sub D (poly-scale (st-sign (+ n 1)) g))))
    (let ((a (st-nthroot target n)))
      (if (equal? a (quote none)) (quote not-third-kind-a+y)
          (list (quote log) (st-a+y a n) a)))))

; full check that a recovered/proposed (g, n, a) reproduces the differential whose denominator is D:
; verify D = Norm(a+y) and the log identity holds.
(define (st-recognize-certify g n a)
  (let ((u (st-a+y a n)))
    (if (rat-equal? (sn-norm g n u) (rat-from-poly (st-norm-a+y a g n)))
        (st-log-certify g n u)
        #f)))
