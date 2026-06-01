; -*- lisp -*-
; lib/cas/mgcd.lisp — greatest common divisor of bivariate polynomials over Q.
;
; A bivariate polynomial f(x,y) is represented as a list of Q[y] coefficients in x,
; low-to-high:  f = c_0(y) + c_1(y) x + ... + c_d(y) x^d, each c_i a Q[y] coefficient
; list (low-to-high in y, the poly.lisp univariate representation).  This is the ring
; Q[y][x].  The GCD is computed by the classic recipe
;
;     gcd(f,g) = gcd_y(cont(f), cont(g)) * pp( gcd over Q(y)[x] of f and g ),
;
; where cont(f) is the Q[y]-gcd of f's x-coefficients (so pp = f/cont is primitive),
; and the gcd over the field Q(y)[x] is found by the ordinary Euclidean algorithm
; using Q(y) = Q[y]/Q[y] (rational functions in y) as the coefficient field.  Working
; over a genuine field avoids pseudo-division and subresultant bookkeeping entirely;
; the field result is then cleared of y-denominators and made primitive over Q[y]
; (Gauss's lemma), recovering the gcd in Q[x,y].
;
; The result is checkable: a true gcd divides BOTH inputs exactly.  Divisibility of
; f by a primitive g is decided by polynomial division over Q(y)[x] -- the remainder
; vanishes iff g | f over Q[x,y].  The example asserts g | f1 and g | f2.
;
; Builds on poly.lisp (univariate Q[y] arithmetic).

(import "cas/poly.lisp")

; ---------- Q(y): rational functions in y, as (num . den), num,den in Q[y] ----------
(define (qf-norm n d)
  (if (poly-zero? n) (cons '() (list 1))
    (let ((g (poly-gcd n d)))
      (let ((n2 (car (poly-divmod n g))) (d2 (car (poly-divmod d g))))
        (let ((lc (poly-lead d2)))
          (cons (poly-scale (/ 1 lc) n2) (poly-scale (/ 1 lc) d2)))))))
(define (qf-from p) (qf-norm p (list 1)))
(define (qf-zero) (cons '() (list 1)))
(define (qf-zero? a) (poly-zero? (car a)))
(define (qf-neg a) (cons (poly-neg (car a)) (cdr a)))
(define (qf-add a b) (qf-norm (poly-add (poly-mul (car a) (cdr b)) (poly-mul (car b) (cdr a))) (poly-mul (cdr a) (cdr b))))
(define (qf-sub a b) (qf-add a (qf-neg b)))
(define (qf-mul a b) (qf-norm (poly-mul (car a) (car b)) (poly-mul (cdr a) (cdr b))))
(define (qf-div a b) (qf-norm (poly-mul (car a) (cdr b)) (poly-mul (cdr a) (car b))))

; ---------- Q(y)[x]: list of qf coefficients, low-to-high in x ----------
(define (xq-trim f) (reverse (drop-zeros (reverse f))))
(define (drop-zeros f) (cond ((null? f) '()) ((qf-zero? (car f)) (drop-zeros (cdr f))) (else f)))
(define (xq-deg f) (- (length (xq-trim f)) 1))
(define (xq-coeff f i) (if (< i (length f)) (nthq f i) (qf-zero)))
(define (nthq f i) (if (= i 0) (car f) (nthq (cdr f) (- i 1))))
(define (xq-lead f) (let ((t (xq-trim f))) (nthq t (- (length t) 1))))
(define (zeros-q k) (if (= k 0) '() (cons (qf-zero) (zeros-q (- k 1)))))
(define (xq-pad f n) (if (>= (length f) n) f (append f (zeros-q (- n (length f))))))
(define (xq-sub f g) (let ((n (max (length f) (length g)))) (xq-sub2 (xq-pad f n) (xq-pad g n))))
(define (xq-sub2 f g) (if (null? f) '() (cons (qf-sub (car f) (car g)) (xq-sub2 (cdr f) (cdr g)))))
; g shifted up by s and scaled by qf lc
(define (xq-scale-shift g lc s) (append (zeros-q s) (map (lambda (c) (qf-mul c lc)) g)))
; remainder of f divided by g over Q(y)[x]
(define (xq-rem f g)
  (let ((ft (xq-trim f)))
    (if (or (null? ft) (< (xq-deg ft) (xq-deg g))) ft
        (xq-rem (xq-trim (xq-sub ft (xq-scale-shift g (qf-div (xq-lead ft) (xq-lead g)) (- (xq-deg ft) (xq-deg g))))) g))))
(define (xq-monic f) (let ((lc (xq-lead f))) (map (lambda (c) (qf-div c lc)) (xq-trim f))))
(define (xq-gcd f g) (if (null? (xq-trim g)) (xq-monic f) (xq-gcd g (xq-rem f g))))

; ---------- conversions Q[y][x] <-> Q(y)[x] ----------
(define (embed f) (map qf-from f))                          ; Q[y] coeffs -> qf coeffs
(define (common-den f) (cd-go f (list 1)))
(define (cd-go f acc) (if (null? f) acc (cd-go (cdr f) (poly-mul acc (cdr (car f))))))
(define (clear-denoms f) (let ((D (common-den f))) (map (lambda (c) (car (poly-divmod (poly-mul (car c) D) (cdr c)))) f)))
(define (content-y f) (gcd-list (keep-nonzero f)))          ; Q[y]-gcd of x-coefficients
(define (keep-nonzero f) (cond ((null? f) '()) ((poly-zero? (car f)) (keep-nonzero (cdr f))) (else (cons (car f) (keep-nonzero (cdr f))))))
(define (gcd-list ps) (if (null? (cdr ps)) (car ps) (poly-gcd (car ps) (gcd-list (cdr ps)))))
(define (pp-y f) (let ((c (content-y f))) (map (lambda (p) (if (poly-zero? p) '() (car (poly-divmod p c)))) f)))
; normalize sign: make the leading x-coefficient's leading y-coefficient = 1
(define (xy-normalize f) (let ((lc (poly-lead (last-nonzero f)))) (map (lambda (p) (poly-scale (/ 1 lc) p)) f)))
(define (last-nonzero f) (let ((t (reverse f))) (first-nonzero t)))
(define (first-nonzero t) (cond ((null? t) (list 1)) ((poly-zero? (car t)) (first-nonzero (cdr t))) (else (car t))))

; ---------- the gcd ----------
(define (xy-allzero? f) (null? (keep-nonzero f)))
(define (mgcd f g)
  (cond ((xy-allzero? g) (xy-normalize f))
        ((xy-allzero? f) (xy-normalize g))
        (else
          (let ((cgcd (poly-gcd (content-y f) (content-y g))))
            (let ((pg (xy-normalize (pp-y (clear-denoms (xq-gcd (embed (pp-y f)) (embed (pp-y g))))))))
              (xy-normalize (map (lambda (p) (poly-mul cgcd p)) pg)))))))

; ---------- divisibility certificate: does g divide f over Q[x,y]? ----------
(define (divides? g f) (null? (xq-trim (xq-rem (embed f) (embed g)))))

; ---------- bivariate multiplication (for tests/building inputs) ----------
(define (xy-mul f g) (if (null? f) '() (xy-add (map (lambda (c) (poly-mul (car f) c)) g) (cons '() (xy-mul (cdr f) g)))))
(define (xy-add f g) (let ((n (max (length f) (length g)))) (xy-add2 (xy-pad f n) (xy-pad g n))))
(define (xy-pad f n) (if (>= (length f) n) f (append f (zeros-p (- n (length f))))))
(define (zeros-p k) (if (= k 0) '() (cons '() (zeros-p (- k 1)))))
(define (xy-add2 f g) (if (null? f) '() (cons (poly-add (car f) (car g)) (xy-add2 (cdr f) (cdr g)))))

; ---------- display ----------
(define (xy->string f) (let ((s (xyterms f 0 ""))) (if (equal? s "") "0" s)))
(define (xyterms f i acc)
  (if (null? f) acc
    (xyterms (cdr f) (+ i 1)
      (if (poly-zero? (car f)) acc
        (let ((t (xycoeff (car f) i))) (if (equal? acc "") t (string-append acc " + " t)))))))
(define (xycoeff c i)
  (let ((cs (poly->string c "y")))
    (cond ((= i 0) (string-append "(" cs ")"))
          ((= i 1) (string-append "(" cs ")*x"))
          (else (string-append "(" cs ")*x^" (number->string i))))))
