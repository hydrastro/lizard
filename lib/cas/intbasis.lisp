; -*- lisp -*-
; lib/cas/intbasis.lisp -- the INTEGRAL BASIS of an algebraic function field, the construction that lifts
; algebraic integration past the hyperelliptic restriction to general curves (docs/TRAGER_ROADMAP.md, Rung 4).
;
; For F(x,y) = 0 defining the function field K(x)[y]/(F), the integral closure of K[x] in that field is a free
; K[x]-module of rank n = deg_y F.  The naive powers 1, y, ..., y^{n-1} span only an ORDER; at the singular
; places of the curve the integral closure is strictly larger, and Trager's integration algorithm needs an
; actual basis of the closure (the regular differentials w_i dx/F_y) -- that is the INTEGRAL BASIS.
;
; This module provides two things:
;
;  (1) the Puiseux-based LOCAL INTEGRALITY ENGINE.  An element w (a polynomial in y with poly-in-x coefficients)
;      possibly divided by a denominator d(x) is INTEGRAL at x = a iff it has no pole on ANY branch of the curve
;      over x = a.  Using the branch expansions y = Y(t), t = (x-a)^(1/q) from puiseuxg.lisp, the valuation of w
;      on a branch is ord_t of the substituted t-series, and w/d is integral at a iff ord_t(w) >= q*ord_x(d) on
;      every branch.  ib-valuation and ib-integral-at? implement exactly this -- the measurement at the heart of
;      every integral-basis algorithm (van Hoeij's method is built on these branch valuations).
;
;  (2) the explicit, certified INTEGRAL BASIS for the hyperelliptic/quadratic case y^2 = D(x).  Writing the
;      square-free factorization D = c * prod p_i^{e_i}, the integral closure is K[x] + K[x]*(y/g) with
;      g = prod p_i^{floor(e_i/2)}: indeed (y/g)^2 = D/g^2 is a polynomial, so y/g is integral, and it removes
;      exactly the part of D that is a square.  ib-quadratic returns (list 1 (cons numerator g)) describing the
;      basis {1, y/g}, and ib-quadratic-certify checks (y/g)^2 = D/g^2 in K[x] (the integrality witness).
;      This is the n = 2 instance of the general construction, fully computable and certified here; the general
;      n case is assembled from the same branch-valuation engine in (1).
;
; Builds on puiseuxg.lisp (branch expansions + valuations) and poly.lisp (square-free factorization).

(import "cas/puiseuxg.lisp")

(define (ib-nth l k) (if (= k 0) (car l) (ib-nth (cdr l) (- k 1))))
(define (ib-idx l i) (if (< i (length l)) (ib-nth l i) 0))

; ----- valuation of an element on a branch -----
; element N is a y-coefficient list (N0 N1 ... ) meaning sum_j Nj(x) y^j (each Nj a poly in x).
; branch is (puiseux q p coeffs) from puiseuxg: y = sum_i coeffs[i] x^((p+i)/q), i.e. in t=x^(1/q) (a=0),
; y(t) = sum_i coeffs[i] t^(p+i).  Returns ord_t of N(t^q, y(t)) (an integer), or 'inf if identically zero.
(define (ib-valuation N branch M)
  (let ((q (ib-nth branch 1)) (p (ib-nth branch 2)) (coeffs (ib-nth branch 3)))
    (let ((yt (pg-shiftup coeffs p)))                          ; y as a t-series
      (let ((val (pg-feval N yt q (+ (* M q) (* 4 q)))))        ; N(t^q, y(t)) as t-series
        (ib-ord val)))))
; precision-BOUNDED valuation: compute the t-series only to `prec` terms; if no nonzero coefficient appears by
; then return prec (i.e. "ord >= prec"), which is all an integrality comparison needs.  Caps cost regardless of
; how many series terms a full expansion would carry.
(define (ib-valuation-bound N branch prec)
  (let ((q (ib-nth branch 1)) (p (ib-nth branch 2)) (coeffs (ib-nth branch 3)))
    (let ((yt (pg-shiftup coeffs p)))
      (let ((val (pg-feval N yt q prec)))
        (ib-ord-bound val prec)))))
(define (ib-ord-bound s prec) (ib-ordb-go s 0 prec))
(define (ib-ordb-go s k prec) (cond ((>= k prec) prec) ((null? s) prec) ((not (= (car s) 0)) k) (else (ib-ordb-go (cdr s) (+ k 1) prec))))
(define (ib-ord s) (ib-ord-go s 0))
(define (ib-ord-go s k) (cond ((> k 400) (quote inf)) ((null? s) (quote inf)) ((not (= (car s) 0)) k) (else (ib-ord-go (cdr s) (+ k 1)))))

; is the element N/(x^k) integral at x = 0 (regular on all branches over 0)?  Need ord_t(N) >= q*k on each
; branch over 0.  branches = (pg-branches F M).  Returns #t iff every branch satisfies the bound.  SOUNDNESS:
; if any branch over the place is needs-radical (irrational tangent -- the branch lives over an extension of Q),
; the rational engine cannot certify integrality there, so this returns #f (not a silent pass): we never claim
; an integral element we cannot witness over Q.  ib-branches-rational? reports whether all branches are rational.
(define (ib-integral-at0? N k branches M)
  (cond ((null? branches) #t)
        ((not (equal? (car (car branches)) (quote puiseux))) #f)   ; needs-radical branch: cannot certify over Q
        (else (let ((q (ib-nth (car branches) 1)))
                (let ((v (ib-valuation-bound N (car branches) (+ (* q k) q))))
                  (if (>= v (* q k))
                      (ib-integral-at0? N k (cdr branches) M)
                      #f))))))
; are all branches over the place rational (genuine puiseux tuples, not needs-radical)?
(define (ib-branches-rational? branches)
  (cond ((null? branches) #t)
        ((not (equal? (car (car branches)) (quote puiseux))) #f)
        (else (ib-branches-rational? (cdr branches)))))

; ----- explicit certified integral basis for y^2 = D(x) -----
; returns (list (quote quadratic-integral-basis) g) where the basis is {1, y/g}, g = prod p_i^{floor(e_i/2)}.
(define (ib-quadratic D)
  (let ((sf (square-free (poly-monic D))))                     ; ((mult factor) ...), factors monic
    (list (quote quadratic-integral-basis) (ib-build-g sf))))
(define (ib-build-g sf)
  (if (null? sf) (list 1)
      (let ((e (car (car sf))) (fac (car (cdr (car sf)))))
        (poly-mul (ib-pow fac (quotient e 2)) (ib-build-g (cdr sf))))))
(define (ib-pow b n) (if (= n 0) (list 1) (poly-mul b (ib-pow b (- n 1)))))

; certify: (y/g)^2 = D/g^2 must be a polynomial (no remainder), i.e. g^2 | D.  Returns the quotient D/g^2 if so,
; else 'not-integral.  This is the integrality witness for y/g.
(define (ib-quadratic-certify D)
  (let ((g (ib-build-g (square-free (poly-monic D))))
        (lead (poly-lead D)))
    (let ((g2 (poly-mul g g)))
      (let ((qr (poly-divmod D g2)))
        (if (ib-zero-poly? (car (cdr qr))) (car qr) (quote not-integral))))))
(define (ib-zero-poly? p) (cond ((null? p) #t) ((not (= (car p) 0)) #f) (else (ib-zero-poly? (cdr p)))))

; convenience: does y/g properly extend the naive order (i.e. is g non-constant)?  When g is constant the
; integral basis is the naive {1, y}; when g is non-constant the closure is strictly larger.
(define (ib-quadratic-extends? D) (> (poly-deg (ib-build-g (square-free (poly-monic D)))) 0))

; ----- local integral basis at x = 0 (pure-power triangular form) -----
; For each power y^j (j = 0 .. n-1, n = deg_y F), find the largest exponent k (0 .. maxk) such that y^j/x^k is
; integral at 0 (regular on every branch over 0).  Returns the list of (j . k_j): the basis {y^j / x^{k_j}}.
; This is the local integral basis whenever the van-Hoeij correction terms vanish (true for superelliptic and
; many other curves); each basis element's integrality is witnessed by the branch valuations (ib-integral-at0?).
(define (ib-local-basis-at0 F maxk M)
  (let ((n (- (length F) 1)))                 ; n = deg_y F; basis powers y^0..y^{n-1}
    (let ((br (pg-branches F M)))
      (ib-lb-go F br maxk M 0 (- n 1)))))
(define (ib-lb-go F br maxk M j n)
  (if (> j n) (quote ())
      (cons (cons j (ib-max-k F br (ib-ypow j) maxk M))
            (ib-lb-go F br maxk M (+ j 1) n))))
; element y^j as a y-coefficient list (j zeros then 1)
(define (ib-ypow j) (append (ib-zeros-list j) (list (list 1))))
(define (ib-zeros-list j) (if (= j 0) (quote ()) (cons (list) (ib-zeros-list (- j 1)))))
; largest k in 0..maxk with N/x^k integral at 0
(define (ib-max-k F br N maxk M) (ib-mk-go F br N maxk M 0 0))
(define (ib-mk-go F br N maxk M k best)
  (if (> k maxk) best
      (if (ib-integral-at0? N k br M) (ib-mk-go F br N maxk M (+ k 1) k) best)))

; the sum of the denominator exponents sum_j k_j is the "delta" -- the x=0 contribution to the difference
; between the integral closure and the naive order (a local measure of the singularity).
(define (ib-delta-at0 basis) (ib-delta-go basis))
(define (ib-delta-go b) (if (null? b) 0 (+ (cdr (car b)) (ib-delta-go (cdr b)))))

; ----- GLOBAL integral basis: combine local bases across all singular places -----
; The singular x-values are where the curve degenerates.  For the superelliptic case y^n = g(x) they are the
; repeated roots of g (equivalently the roots of the repeated part of its square-free factorization); we take
; the squarefree factors of multiplicity >= 2 of g and use each factor's rational roots as the places.  At each
; place x=a the local basis (computed on the shifted curve F(x+a,y)) gives exponents k_j(a); the GLOBAL basis is
; w_j = y^j / d_j(x) with d_j(x) = prod_a (x-a)^{k_j(a)} (pure-power triangular form, corrections vanishing for
; superelliptic curves).  Returns the list of denominators d_0 .. d_{n-1} (each a poly in x), so the basis is
; { y^j / d_j }.

; shift F(x,y) -> F(x+a, y): shift each y-coefficient polynomial by a
(define (ib-pshift1 p a) (if (= a 0) p (ib-pshift1-go (reverse p) a (list 0))))
(define (ib-pshift1-go cs a acc) (if (null? cs) acc (ib-pshift1-go (cdr cs) a (poly-add (poly-mul acc (list a 1)) (list (car cs))))))
(define (ib-Fshift F a) (if (null? F) (quote ()) (cons (ib-pshift1 (car F) a) (ib-Fshift (cdr F) a))))

; rational roots of a polynomial (small rational-root scan; sufficient for the low-degree models we handle)
(define (ib-rat-roots P D) (ib-rr-pos P D 0 1 (quote ())))
(define (ib-rr-pos P D p q acc)
  (cond ((> p D) (ib-rr-neg P D 1 1 acc))
        ((> q D) (ib-rr-pos P D (+ p 1) 1 acc))
        (else (let ((r (/ p q))) (if (if (= (poly-eval P r) 0) (not (ib-memq r acc)) #f) (ib-rr-pos P D p (+ q 1) (cons r acc)) (ib-rr-pos P D p (+ q 1) acc))))))
(define (ib-rr-neg P D p q acc)
  (cond ((> p D) acc)
        ((> q D) (ib-rr-neg P D (+ p 1) 1 acc))
        (else (let ((r (/ (- 0 p) q))) (if (if (= (poly-eval P r) 0) (not (ib-memq r acc)) #f) (ib-rr-neg P D p (+ q 1) (cons r acc)) (ib-rr-neg P D p (+ q 1) acc))))))
(define (ib-memq x l) (if (null? l) #f (if (= x (car l)) #t (ib-memq x (cdr l)))))

; singular places for a superelliptic curve y^n = g(x): rational roots of the squarefree factors of g of
; multiplicity >= 2 (the places where the cover ramifies enough to enlarge the order).
(define (ib-sing-places-superelliptic g D)
  (ib-sps-go (square-free (poly-monic g)) D))
(define (ib-sps-go sf D)
  (if (null? sf) (quote ())
      (let ((mult (car (car sf))) (fac (car (cdr (car sf)))))
        (if (>= mult 2)
            (ib-append-uniq (ib-rat-roots fac D) (ib-sps-go (cdr sf) D))
            (ib-sps-go (cdr sf) D)))))
(define (ib-append-uniq a b) (if (null? a) b (if (ib-memq (car a) b) (ib-append-uniq (cdr a) b) (cons (car a) (ib-append-uniq (cdr a) b)))))

; global denominators for y^n = g(x): for each power j, d_j = prod over singular places a of (x-a)^{k_j(a)}.
; F is the curve list ((-g) 0 ... 0 1) of length n+1.  maxk, M as in the local routine.  SOUNDNESS: if any
; singular place has an irrational tangent (its branches are needs-radical), the basis is not certifiable over
; Q and the routine returns 'needs-extension rather than a wrong answer.
(define (ib-global-basis-superelliptic F g maxk M)
  (let ((places (ib-sing-places-superelliptic g 6)))
    (if (ib-any-place-irrational? F places M) (quote needs-extension)
        (let ((n (- (length F) 1)))
          (ib-gb-cols F places maxk M 0 (- n 1))))))
(define (ib-any-place-irrational? F places M)
  (cond ((null? places) #f)
        ((not (ib-branches-rational? (pg-branches (ib-Fshift F (car places)) M))) #t)
        (else (ib-any-place-irrational? F (cdr places) M))))
(define (ib-gb-cols F places maxk M j n)
  (if (> j n) (quote ())
      (cons (ib-gb-denom F places maxk M j) (ib-gb-cols F places maxk M (+ j 1) n))))
; denominator for power j: product over places of (x-a)^{k_j(a)}, k_j(a) from the local basis at a
(define (ib-gb-denom F places maxk M j)
  (if (null? places) (list 1)
      (let ((a (car places)))
        (let ((kj (ib-place-kj F a maxk M j)))
          (poly-mul (ib-pow (list (- 0 a) 1) kj) (ib-gb-denom F (cdr places) maxk M j))))))
; k_j at place a: the exponent for power j in the local basis of the shifted curve
(define (ib-place-kj F a maxk M j)
  (let ((lb (ib-local-basis-at0 (ib-Fshift F a) maxk M)))
    (cdr (ib-assoc-j lb j))))
(define (ib-assoc-j lb j) (if (null? lb) (cons j 0) (if (= (car (car lb)) j) (car lb) (ib-assoc-j (cdr lb) j))))
