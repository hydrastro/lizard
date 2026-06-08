; -*- lisp -*-
; lib/cas/mfactor.lisp — factorization of bivariate polynomials over Q into
; irreducibles, by evaluation + y-adic Hensel lifting + recombination.
;
; Strategy (for f(x,y) squarefree, primitive, and MONIC in x):
;   1. pick a shift s so f(x,s) keeps full x-degree and is squarefree over Q;
;   2. factor the univariate f(x,s) over Q into monic irreducibles u_1..u_r;
;   3. Hensel-lift  f = prod u_i  from mod (y-s) to mod (y-s)^N, N > deg_y f;
;   4. mf-recombine: the true factors are products of subsets of the lifted factors,
;      detected by exact bivariate division (mgcd/divides?).
; The whole result is gated by reconstruction: the product of the returned factors
; must equal f, so a wrong factorization cannot be reported.
;
; Two coefficient layouts are used.  The bivariate polynomial keeps the mgcd "xy"
; layout (a list of Q[y] coefficients in x).  Hensel lifting is more natural in the
; transposed "ys" layout: a list of Q[x] coefficients in y (a polynomial in y whose
; coefficients are ordinary poly.lisp polynomials in x), truncated at y^N.
;
; Builds on msqfree.lisp (bivariate gcd/squarefree/division) and factor.lisp
; (univariate factorization over Q).

(import "cas/msqfree.lisp")
(import "cas/factor.lisp")

(define (irange a b) (if (> a b) '() (cons a (irange (+ a 1) b))))
(define (take a n) (if (or (= n 0) (null? a)) '() (cons (car a) (take (cdr a) (- n 1)))))

; ---------- xy  <->  ys  transpose ----------
(define (idx c j) (if (< j (length c)) (nth c j) 0))           ; j-th coeff of a number-list, else 0
(define (ymaxdeg f) (fold-md f 0))
(define (fold-md f acc) (if (null? f) acc (fold-md (cdr f) (max acc (- (length (car f)) 1)))))
(define (xy->ys f) (if (null? f) '() (map (lambda (j) (poly-norm (map (lambda (c) (idx c j)) f))) (irange 0 (ymaxdeg f)))))
(define (ys->xy g) (if (null? g) '() (map (lambda (i) (poly-norm (map (lambda (d) (idx d i)) g))) (irange 0 (ymaxdeg g)))))

; ---------- ys arithmetic (coefficients are Q[x] polys; index = power of y) ----------
(define (ys-c a j) (if (< j (length a)) (nth a j) '()))
(define (ys-trim g) (reverse (ys-dropz (reverse g))))
(define (ys-dropz g) (cond ((null? g) '()) ((poly-zero? (car g)) (ys-dropz (cdr g))) (else g)))
(define (ys-add a b) (let ((n (max (length a) (length b)))) (map (lambda (j) (poly-add (ys-c a j) (ys-c b j))) (irange 0 (- n 1)))))
(define (ys-neg a) (map poly-neg a))
(define (ys-sub a b) (ys-add a (ys-neg b)))
(define (ys-conv a b k i acc) (if (> i k) acc (ys-conv a b k (+ i 1) (poly-add acc (poly-mul (ys-c a i) (ys-c b (- k i)))))))
(define (ys-mul a b) (if (or (null? a) (null? b)) '() (map (lambda (k) (ys-conv a b k 0 '())) (irange 0 (+ (- (length a) 1) (- (length b) 1))))))
(define (ys-mul-trunc a b n) (take (ys-mul a b) n))
(define (ys-scale-x p g) (map (lambda (d) (poly-mul p d)) g))   ; multiply every y-coeff by a Q[x] poly

; ---------- extended Euclid over Q[x]; Bezout for coprime a,b: s*a + t*b = 1 ----------
(define (prod-x ps) (if (null? ps) (list 1) (poly-mul (car ps) (prod-x (cdr ps)))))
(define (exeucl r0 r1 s0 s1 t0 t1)
  (if (poly-zero? r1) (list r0 s0 t0)
      (let ((qr (poly-divmod r0 r1)))
        (exeucl r1 (car (cdr qr)) s1 (poly-sub s0 (poly-mul (car qr) s1)) t1 (poly-sub t0 (poly-mul (car qr) t1))))))
(define (bezout-x a b)
  (let ((g (exeucl a b (list 1) '() '() (list 1))))
    (let ((c (poly-lead (car g))))
      (list (poly-scale (/ 1 c) (car (cdr g))) (poly-scale (/ 1 c) (car (cdr (cdr g))))))))

; ---------- y-adic Hensel pair lift ----------
; given f (ys, mod y^n) with f0 = g0*h0 over Q[x] and s*g0+t*h0=1, lift to g,h (ys)
; with f == g*h (mod y^n),  g == g0, h == h0 (mod y),  deg_x g = deg_x g0.
(define (zeros-poly k) (if (= k 0) '() (cons '() (zeros-poly (- k 1)))))
(define (ys-add-term g k r) (ys-add g (append (zeros-poly k) (list r))))
(define (mf-hensel-pair f g0 h0 s t n) (hp-loop f (list g0) (list h0) g0 h0 s t 1 n))
(define (hp-loop f g h g0 h0 s t k n)
  (if (>= k n) (cons g h)
      (let ((e (ys-c (ys-sub f (ys-mul-trunc g h n)) k)))
        (if (poly-zero? e) (hp-loop f g h g0 h0 s t (+ k 1) n)
            (let ((qr (poly-divmod (poly-mul t e) g0)))
              (hp-loop f (ys-add-term g k (car (cdr qr)))
                         (ys-add-term h k (poly-add (poly-mul s e) (poly-mul (car qr) h0)))
                         g0 h0 s t (+ k 1) n))))))

; ---------- multifactor lift: us = monic Q[x] factors of f at y=0 ----------
(define (hensel-factors f us n)
  (if (null? (cdr us)) (list (take f n))
      (let ((g0 (car us)) (h0 (prod-x (cdr us))))
        (let ((st (bezout-x g0 h0)))
          (let ((gh (mf-hensel-pair f g0 h0 (car st) (car (cdr st)) n)))
            (cons (car gh) (hensel-factors (cdr gh) (cdr us) n)))))))

; ---------- y-shift (substitute y -> y+s) and evaluation at y=s ----------
(define (yshift-poly c s) (if (poly-zero? c) '() (cons (poly-eval c s) (yshift-poly (car (poly-divmod c (list (- 0 s) 1))) s))))
(define (yshift f s) (map (lambda (c) (yshift-poly c s)) f))
(define (eval-y f s) (poly-norm (map (lambda (c) (poly-eval c s)) f)))
(define (find-shift f s) (if (> s 16) 0 (let ((gs (eval-y f s))) (if (= (poly-deg (poly-gcd gs (poly-deriv gs))) 0) s (find-shift f (+ s 1))))))

; ---------- recombination ----------
(define (member-eq x lst) (cond ((null? lst) #f) ((equal? x (car lst)) #t) (else (member-eq x (cdr lst)))))
(define (combinations lst k)
  (cond ((= k 0) (list '()))
        ((null? lst) '())
        (else (append (map (lambda (c) (cons (car lst) c)) (combinations (cdr lst) (- k 1))) (combinations (cdr lst) k)))))
(define (prod-ys lst n) (if (null? (cdr lst)) (take (car lst) n) (ys-mul-trunc (car lst) (prod-ys (cdr lst) n) n)))
(define (scan-combos f combos n)
  (cond ((null? combos) #f)
        (else (let ((P (xy-normalize (ys->xy (prod-ys (car combos) n)))))
                (if (divides? P f) (cons P (car combos)) (scan-combos f (cdr combos) n))))))
(define (find-factor f pool size n)
  (if (> size (- (length pool) 1)) #f
      (let ((hit (scan-combos f (combinations pool size) n))) (if hit hit (find-factor f pool (+ size 1) n)))))
(define (remove-subset pool sub) (filter (lambda (x) (not (member-eq x sub))) pool))
(define (mf-recombine f pool n)
  (if (or (null? pool) (null? (cdr pool))) (list (xy-normalize f))
      (let ((hit (find-factor f pool 1 n)))
        (if hit (cons (car hit) (mf-recombine (xy-quotient f (car hit)) (remove-subset pool (cdr hit)) n))
            (list (xy-normalize f))))))

; ---------- factor a squarefree, primitive, monic-in-x bivariate poly ----------
(define (factor-sqfree g)
  (let ((s (find-shift g 0)))
    (let ((gs (yshift g s)))
      (let ((us (map (lambda (mf) (car (cdr mf))) (car (cdr (factor-Q (eval-y gs 0)))))))
        (if (null? (cdr us)) (list g)
            (map (lambda (fac) (yshift fac (- 0 s)))
                 (mf-recombine gs (hensel-factors (xy->ys gs) us (+ 1 (ymaxdeg gs))) (+ 1 (ymaxdeg gs)))))))))

; ---------- top level: full factorization into (irreducible . multiplicity) ----------
(define (factor-bivariate f) (sqfree->irr (sqfree f)))
(define (sqfree->irr sqf)
  (if (null? sqf) '()
      (append (map (lambda (g) (cons g (cdr (car sqf)))) (factor-sqfree (car (car sqf)))) (sqfree->irr (cdr sqf)))))

; ---------- reconstruction certificate (product of factor^mult == f) ----------
(define (factor-ok? f facts) (equal? (xy-normalize (xy-trim (reconstruct facts))) (xy-normalize (xy-trim f))))

; ---------- display ----------
(define (factor-bivariate->string facts) (if (null? facts) "1" (fb-go facts "")))
(define (fb-go facts acc)
  (if (null? facts) acc
    (let ((piece (if (= (cdr (car facts)) 1) (string-append "(" (xy->string (car (car facts))) ")")
                     (string-append "(" (xy->string (car (car facts))) ")^" (number->string (cdr (car facts)))))))
      (fb-go (cdr facts) (if (equal? acc "") piece (string-append acc " * " piece))))))
