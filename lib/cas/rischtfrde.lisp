; -*- lisp -*-
; lib/cas/rischtfrde.lisp -- the TOWER-FIELD Risch differential equation solver: solves y' + f y = g where the
; coefficients live in a height-1 exponential tower K_1 = Q(x)(theta), theta = exp(b) with b in Q(x), and f is a
; base-field (Q(x)) coefficient.  This is the step that makes the recursive Risch procedure call ITSELF: it
; reduces the tower-field RDE to one base-field RDE per theta-degree, each solved by the rational-coefficient
; solver one level down (rischrde.lisp) -- closing the recursion at every exponential level
; (docs/TRAGER_ROADMAP.md, the summit).
;
; The decoupling.  For theta = exp(b) the derivation is DIAGONAL in theta-degree:
;     D(sum_k y_k theta^k) = sum_k (y_k' + k b' y_k) theta^k .
; So with a base-field coefficient f = phi(x) (theta-degree 0), the RDE y' + f y = g, written out per
; theta-degree k with y = sum y_k theta^k and g = sum g_k theta^k, becomes a family of INDEPENDENT scalar RDEs
; over Q(x):
;     y_k' + (k b' + phi) y_k = g_k         (one for each theta-degree k present).
; Each is exactly the rational-coefficient RDE solved by rischrde (rde-solve), so the tower-field RDE is solvable
; iff every per-degree base RDE is solvable; assembling the y_k gives y in K_1.  The differentiation certificate
; in K_1 (D y + f y = g, the diagonal derivation) is the final arbiter.
;
; This directly powers the exponential reduction: integrating sum_i a_i theta^i (theta = exp(b)) one level UP
; needs, per i != 0, the RDE c_i' + (i b') c_i = a_i in K below; when "below" is itself an exponential tower this
; module decides it by the same decoupling, the recursion bottoming out at Q(x) via rischrde.
;
; Representation.  A K_1 element is a list of Q(x)-rational coefficients (low-to-high in theta): (y_0 y_1 ...),
; each y_k a rational (num . den).  b is a Q(x) rational (its derivative b' = rat-deriv b); f = phi is a
; Q(x)-rational coefficient.
;
; Public:
;   tfr-bprime-deg b k       -> the base-field coefficient k b' + phi for theta-degree k (phi supplied separately)
;   tfr-solve b phi g        -> y (a K_1 element, list of rational coeffs) | 'no-tower-solution :
;                               solves y' + phi y = g over Q(x)(exp(b)) with phi a Q(x) coefficient
;   tfr-certify b phi g y    -> #t iff D y + phi y = g in K_1  (the diagonal-derivation certificate)
;   tfr-deriv b y            -> D y in K_1 (the diagonal exponential derivation), as a K_1 element
;
; Verified: INT e^x = e^x via y'+y=1 at degree 1; INT x e^x = (x-1) e^x via y'+y=x; INT e^x/x (Ei) detected non-
; solvable at degree 1; a two-degree right-hand side solved degree-by-degree; and the certificate in K_1.
;
; Builds on rischrde.lisp (the base-field rational RDE) and tower.lisp / poly.lisp.

(import "cas/rischrde.lisp")
(import "cas/tower.lisp")
(import "cas/poly.lisp")

(define (tfr-nth l k) (if (= k 0) (car l) (tfr-nth (cdr l) (- k 1))))
(define (tfr-len l) (if (null? l) 0 (+ 1 (tfr-len (cdr l)))))
(define (tfr-coeff g k) (if (< k (tfr-len g)) (tfr-nth g k) (rat-zero)))

; ----- the per-degree base-field coefficient  k b' + phi  (b' = rat-deriv b; phi a Q(x) rational) -----
(define (tfr-deg-coeff b phi k) (rat-add (rat-scale k (rat-deriv b)) phi))

; ----- the diagonal exponential derivation  D(sum y_k theta^k) = sum (y_k' + k b' y_k) theta^k ----
(define (tfr-deriv b y) (tfr-deriv-go b y 0))
(define (tfr-deriv-go b y k) (if (null? y) (quote ()) (cons (rat-add (rat-deriv (car y)) (rat-mul (rat-scale k (rat-deriv b)) (car y))) (tfr-deriv-go b (cdr y) (+ k 1)))))

; ----- solve the tower-field RDE by decoupling: one base RDE  y_k' + (k b' + phi) y_k = g_k  per degree -----
(define (tfr-solve b phi g) (tfr-solve-go b phi g 0 (tfr-len g) (quote ())))
(define (tfr-solve-go b phi g k m acc)
  (if (>= k m) (tfr-finish (tfr-reverse acc))
      (tfr-solve-step b phi g k m acc (rde-solve (tfr-deg-coeff b phi k) (tfr-coeff g k)))))
(define (tfr-solve-step b phi g k m acc yk)
  (if (equal? yk (quote no-rational-solution)) (quote no-tower-solution)
      (tfr-solve-go b phi g (+ k 1) m (cons yk acc))))
(define (tfr-finish y) (if (equal? y (quote no-tower-solution)) (quote no-tower-solution) y))
(define (tfr-reverse l) (tfr-rev l (quote ())))
(define (tfr-rev l acc) (if (null? l) acc (tfr-rev (cdr l) (cons (car l) acc))))

; ----- the certificate in K_1: D y + phi y = g (compare coefficient lists with rat-equal?) -----
(define (tfr-certify b phi g y) (if (equal? y (quote no-tower-solution)) #f (tfr-eq? (tfr-add (tfr-deriv b y) (tfr-scale phi y)) g)))
(define (tfr-add a b) (cond ((null? a) b) ((null? b) a) (else (cons (rat-add (car a) (car b)) (tfr-add (cdr a) (cdr b))))))
(define (tfr-scale phi y) (if (null? y) (quote ()) (cons (rat-mul phi (car y)) (tfr-scale phi (cdr y)))))
(define (tfr-eq? a b) (tfr-eq-go a b 0 (tfr-maxlen a b)))
(define (tfr-maxlen a b) (if (> (tfr-len a) (tfr-len b)) (tfr-len a) (tfr-len b)))
(define (tfr-eq-go a b k m) (if (>= k m) #t (if (rat-equal? (tfr-coeff a k) (tfr-coeff b k)) (tfr-eq-go a b (+ k 1) m) #f)))
