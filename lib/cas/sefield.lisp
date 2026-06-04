; -*- lisp -*-
; lib/cas/sefield.lisp -- the SUPERELLIPTIC FUNCTION FIELD K = Q(x)[y]/(y^n - g(x)) for arbitrary n, with its
; derivation and a certified logarithm constructor.  This generalizes algfunc.lisp (which is fixed at n = 2)
; to any degree, the algebraic foundation the rest of the Rung-4 superelliptic integration is built on
; (docs/TRAGER_ROADMAP.md).
;
; An element is a length-n list (a_0 a_1 ... a_{n-1}) of rational functions (each a (num den) pair from
; ratfun/tower), representing a_0 + a_1 y + ... + a_{n-1} y^{n-1}.  The defining relation is y^n = g, so
; multiplication reduces any y^{>=n} via y^{i+j} = g^{floor((i+j)/n)} y^{(i+j) mod n}.
;
; Derivation: y^n = g gives y' = g' y/(n g), so
;     d/dx (sum_j a_j y^j) = sum_j [ a_j' + a_j (j/n) g'/g ] y^j,
; which stays within y^0..y^{n-1} (the y^{j-1} y' multiplies one factor of y back), so the derivative needs no
; reduction -- a clean closed form per sector.
;
; The logarithm: for a field element u, d/dx log u = u'/u.  Rather than invert u, a candidate identity
; INT f dx = c log u is certified by clearing the denominator: f * u = c * u' as an identity in the field
; (needing only multiply and derive, both exact).  sf-log-certify f u c checks exactly this.  This constructs
; and verifies genuinely algebraic logarithms such as log(P(x) + y) on y^n = g for any n.
;
; Public: sf-zero/sf-one/sf-make n; sf-add/sf-sub/sf-scale (by a rational); sf-mul g n; sf-deriv g n; sf-equal?;
; sf-y (the element y); sf-logderiv-times-u g n u (returns u' = d/dx u, used with the cleared-denominator
; certificate); sf-log-certify g n f u c.  Verified at n=2 against algfunc, and at n=3 on log(y) and log(x+y).
;
; Builds on tower.lisp (rational-function arithmetic rat-*) and poly.lisp.

(import "cas/tower.lisp")

(define (sf-nth l k) (if (= k 0) (car l) (sf-nth (cdr l) (- k 1))))

; ----- constructors -----
(define (sf-zeros n) (if (= n 0) (quote ()) (cons (rat-zero) (sf-zeros (- n 1)))))
(define (sf-zero n) (sf-zeros n))
(define (sf-one n) (cons (rat-one) (sf-zeros (- n 1))))
(define (sf-set l k v) (if (= k 0) (cons v (cdr l)) (cons (car l) (sf-set (cdr l) (- k 1) v))))
(define (sf-y n) (sf-set (sf-zeros n) 1 (rat-one)))             ; the element y (needs n >= 2)
; build from a rational coefficient list (already length n)
(define (sf-from-coeffs cs) cs)

; ----- additive structure -----
(define (sf-add a b) (if (null? a) (quote ()) (cons (rat-add (car a) (car b)) (sf-add (cdr a) (cdr b)))))
(define (sf-scale r a) (if (null? a) (quote ()) (cons (rat-mul r (car a)) (sf-scale r (cdr a)))))
(define (sf-neg a) (sf-scale (rat-from-poly (list -1)) a))
(define (sf-sub a b) (sf-add a (sf-neg b)))

; ----- multiplication: reduce y^{i+j} = g^{floor((i+j)/n)} y^{(i+j) mod n} -----
; explicit product: for each i,j multiply a_i b_j into sector (i+j) mod n with factor g^{floor((i+j)/n)}
(define (sf-product g n a b) (sf-prod-i g n a b 0 (sf-zeros n)))
(define (sf-prod-i g n a b i acc)
  (if (>= i (length a)) acc
      (sf-prod-i g n a b (+ i 1) (sf-prod-j g n a b i 0 acc))))
(define (sf-prod-j g n a b i j acc)
  (if (>= j (length b)) acc
      (let ((s (+ i j)))
        (let ((sector (remainder s n)) (gp (quotient s n)))
          (let ((coeff (rat-mul (sf-nth a i) (rat-mul (sf-nth b j) (sf-gpow g gp)))))
            (sf-prod-j g n a b i (+ j 1) (sf-set acc sector (rat-add (sf-nth acc sector) coeff))))))))
(define (sf-gpow g e) (if (= e 0) (rat-one) (rat-mul (rat-from-poly g) (sf-gpow g (- e 1)))))

; ----- derivation: d/dx (sum a_j y^j) = sum [a_j' + a_j (j/n) g'/g] y^j -----
(define (sf-deriv g n a) (sf-deriv-go g n a 0))
(define (sf-deriv-go g n a j)
  (if (null? a) (quote ())
      (cons (rat-add (rat-deriv (car a))
                     (rat-mul (car a) (rat-mul (sf-jn j n) (sf-gprime-over-g g))))
            (sf-deriv-go g n (cdr a) (+ j 1)))))
(define (sf-jn j n) (rat-make (list j) (list n)))                ; the constant j/n as a rational function
(define (sf-gprime-over-g g) (rat-make (poly-deriv g) g))        ; g'/g

; ----- equality -----
(define (sf-equal? a b) (sf-eq-go a b))
(define (sf-eq-go a b) (cond ((null? a) (if (null? b) #t #f)) ((rat-equal? (car a) (car b)) (sf-eq-go (cdr a) (cdr b))) (else #f)))

; ----- logarithm certificate: INT f dx = c log u  <=>  f * u = c * u'  in the field -----
; f, u are field elements; c is a rational constant (as a rational function).  Returns #t iff the identity holds.
(define (sf-log-certify g n f u c)
  (sf-equal? (sf-product g n f u) (sf-scale c (sf-deriv g n u))))

; the field element u' = d/dx u (exposed for building/inspecting log examples)
(define (sf-du g n u) (sf-deriv g n u))
