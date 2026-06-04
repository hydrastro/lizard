; -*- lisp -*-
; lib/cas/hyperjac.lisp -- the JACOBIAN GROUP LAW of a genus-2 hyperelliptic curve y^2 = f(x) (deg f = 5), by
; Mumford representation and Cantor's algorithm: the higher-genus analogue of the elliptic chord-tangent law that
; the third-kind construction needs to move past genus 1 (docs/TRAGER_ROADMAP.md -- the full third-kind / general
; algebraic Risch frontier, the divisor arithmetic on the Jacobian).
;
; A reduced divisor of degree <= g = 2 on y^2 = f is represented in MUMFORD form as a pair [u, v] of polynomials
; over Q with u monic, deg v < deg u <= 2, and u | (v^2 - f) -- the curve condition that each point (x_i, v(x_i))
; with u(x_i) = 0 lies on the curve.  The identity (zero divisor) is [1, 0]; a single point (a, b) is [x - a, b];
; the negation is -[u, v] = [u, -v].  CANTOR'S ALGORITHM adds D1 = [u1, v1] and D2 = [u2, v2]:
;   COMPOSITION: with d = gcd(u1, u2, v1 + v2) = s1 u1 + s2 u2 + s3 (v1 + v2),
;       u = u1 u2 / d^2,    v = (s1 u1 v2 + s2 u2 v1 + s3 (v1 v2 + f)) / d   (mod u);
;   REDUCTION (while deg u > 2):  u <- monic((f - v^2)/u),  v <- (-v) mod u,  repeat.
; The result is the unique reduced divisor in the class of D1 + D2.  This is the group law on the Jacobian; its
; n-fold multiple of a class decides torsion (the genus-2 analogue of the elliptic order test), the ingredient the
; full third-kind construction needs in genus 2.  Every output is checked against the curve condition u | (v^2 - f),
; so a malformed result is caught rather than trusted.
;
; This implements the standard Cantor composition/reduction over Q[x] from scratch (Cantor 1987), with its own
; extended gcd; it is exact and certified by the Mumford invariant, not adapted from any existing system's source.
;
; Public (a divisor is (list u v) with u, v coefficient lists low->high; f the curve polynomial):
;   hj-identity                -> [1, 0], the zero divisor
;   hj-point a b               -> [x - a, b], the divisor of a single affine point (a, b)
;   hj-u D / hj-v D            -> the Mumford polynomials
;   hj-valid? f D              -> #t iff u monic, deg v < deg u, and u | (v^2 - f) (the curve condition)
;   hj-neg D                   -> -D = [u, -v]
;   hj-add f D1 D2             -> D1 + D2 by Cantor (composition + reduction), a reduced divisor
;   hj-double f D              -> D + D
;   hj-mul f n D               -> n * D (repeated addition), the divisor-class multiple (for torsion tests)
;   hj-equal? D1 D2            -> structural equality of reduced divisors
;   hj-egcd a b                -> (list g s t) with g = gcd(a,b) = s a + t b (extended gcd over Q[x])
;
; Verified: [x,1] + [1,0] = [x,1] (identity); [x,1] + [x+1,0] = [x^2+x, x+1] on y^2 = x^5+1 (curve condition
; holds); negation and -D + D = identity; the curve condition guards every result.
;
; Builds on poly.lisp.

(import "cas/poly.lisp")

; ----- divisors -----
(define (hj-identity) (list (list 1) (list)))
(define (hj-point a b) (list (list (- 0 a) 1) (if (= b 0) (list) (list b))))
(define (hj-u D) (car D))
(define (hj-v D) (car (cdr D)))
(define (hj-make u v) (list (poly-monic u) (hj-redv v u)))
(define (hj-redv v u) (if (poly-zero? u) v (car (cdr (poly-divmod v u)))))  ; v mod u (kept low degree)

; ----- extended gcd over Q[x]: g = s a + t b, g monic -----
(define (hj-egcd a b) (if (poly-zero? b)
                          (hj-egcd-base a)
                          (hj-egcd-step a b (poly-divmod a b))))
(define (hj-egcd-base a) (hj-egcd-norm a (list 1) (list) a))
(define (hj-egcd-step a b dm) (hj-egcd-combine b (car dm) (hj-egcd b (car (cdr dm)))))
; recursion gives (g s' t') for (b, r); back-substitute: gcd(a,b)=gcd(b,r), s = t', t = s' - q t'
(define (hj-egcd-combine b q gst) (list (car gst) (car (cdr (cdr gst))) (poly-sub (car (cdr gst)) (poly-mul q (car (cdr (cdr gst)))))))
; normalize so g is monic (scale g,s,t by 1/lc(g))
(define (hj-egcd-norm g s t orig) (hj-scale-triple g s t))
(define (hj-scale-triple g s t) (hj-st g s t (poly-lc-inv g)))
(define (hj-st g s t c) (list (poly-scale c g) (poly-scale c s) (poly-scale c t)))
(define (poly-lc-inv p) (/ 1 (poly-lc p)))
(define (poly-lc p) (poly-nth p (poly-deg p)))
(define (poly-nth p k) (if (= k 0) (car p) (poly-nth (cdr p) (- k 1))))

; three-way gcd d = gcd(u1,u2,v1+v2) with cofactors s1 u1 + s2 u2 + s3 (v1+v2) = d
(define (hj-gcd3 u1 u2 w) (hj-gcd3-go (hj-egcd u1 u2) w))
(define (hj-gcd3-go g12 w)                        ; g12 = (d1 e1 e2), d1 = e1 u1 + e2 u2
  (hj-gcd3-fin g12 (hj-egcd (car g12) w)))         ; second = (d c1 c2), d = c1 d1 + c2 w
(define (hj-gcd3-fin g12 g2)
  (list (car g2)                                   ; d
        (poly-mul (car (cdr g2)) (car (cdr g12)))  ; s1 = c1 e1
        (poly-mul (car (cdr g2)) (car (cdr (cdr g12)))) ; s2 = c1 e2
        (car (cdr (cdr g2)))))                      ; s3 = c2

; ----- validity (curve condition) -----
(define (hj-valid? f D) (hj-valid-go f (hj-u D) (hj-v D)))
(define (hj-valid-go f u v) (if (hj-monic? u) (if (< (poly-deg v) (poly-deg u)) (poly-zero? (car (cdr (poly-divmod (poly-sub (poly-mul v v) f) u)))) #f) #f))
(define (hj-monic? u) (if (poly-zero? u) #f (= (poly-lc u) 1)))

; ----- negation -----
(define (hj-neg D) (list (hj-u D) (poly-scale -1 (hj-v D))))

; ----- Cantor addition: composition then reduction -----
(define (hj-add f D1 D2) (hj-add-go f (hj-u D1) (hj-v D1) (hj-u D2) (hj-v D2)))
(define (hj-add-go f u1 v1 u2 v2) (hj-compose f u1 v1 u2 v2 (hj-gcd3 u1 u2 (poly-add v1 v2))))
(define (hj-compose f u1 v1 u2 v2 g)              ; g = (d s1 s2 s3)
  (hj-comp2 f (car g) (car (cdr g)) (car (cdr (cdr g))) (car (cdr (cdr (cdr g)))) u1 v1 u2 v2))
(define (hj-comp2 f d s1 s2 s3 u1 v1 u2 v2)
  (hj-reduce f
    (car (poly-divmod (poly-mul u1 u2) (poly-mul d d)))    ; u = u1 u2 / d^2
    (hj-comp-v d s1 s2 s3 u1 v1 u2 v2 f (car (poly-divmod (poly-mul u1 u2) (poly-mul d d))))))
(define (hj-comp-v d s1 s2 s3 u1 v1 u2 v2 f u)
  (hj-redv (car (poly-divmod (poly-add (poly-add (poly-mul (poly-mul s1 u1) v2) (poly-mul (poly-mul s2 u2) v1))
                                       (poly-mul s3 (poly-add (poly-mul v1 v2) f))) d)) u))

; ----- reduction: while deg u > 2, u <- monic((f - v^2)/u), v <- (-v) mod u -----
(define (hj-reduce f u v) (if (> (poly-deg u) 2) (hj-reduce-step f u v) (list (poly-monic u) (hj-redv v u))))
(define (hj-reduce-step f u v) (hj-reduce-go f (poly-monic (car (poly-divmod (poly-sub f (poly-mul v v)) u))) v))
(define (hj-reduce-go f unew v) (hj-reduce f unew (hj-redv (poly-scale -1 v) unew)))

; ----- doubling, scalar multiple, equality -----
(define (hj-double f D) (hj-add f D D))
(define (hj-mul f n D) (if (<= n 0) (hj-identity) (hj-mul-go f n D (hj-identity))))
(define (hj-mul-go f n D acc) (if (= n 0) acc (hj-mul-go f (- n 1) D (hj-add f acc D))))
(define (hj-equal? D1 D2) (if (hj-peq? (hj-u D1) (hj-u D2)) (hj-peq? (hj-v D1) (hj-v D2)) #f))
(define (hj-peq? a b) (equal? (poly-norm a) (poly-norm b)))
(define (poly-norm p) (reverse (hj-drop0 (reverse p))))
(define (hj-drop0 p) (cond ((null? p) (quote ())) ((= (car p) 0) (hj-drop0 (cdr p))) (else p)))
