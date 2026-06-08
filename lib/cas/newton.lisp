; -*- lisp -*-
; lib/cas/newton.lisp -- the NEWTON POLYGON for a general plane algebraic curve F(x,y) = 0, continuing Rung 4
; (docs/TRAGER_ROADMAP.md).  puiseux.lisp handles the superelliptic case y^e = g(x) (one branch type); this
; module handles GENERAL F by computing the Newton polygon at x = 0, whose lower-left hull edges give the
; leading exponents (slopes) and leading coefficients of ALL the Puiseux branches -- the first, decisive step
; of Newton-Puiseux for arbitrary algebraic functions.
;
; F is given as a list of y-coefficients, each a polynomial in x (low-to-high):  F = (F0 F1 ... Fd), meaning
; F(x,y) = sum_j Fj(x) y^j.  The Newton polygon is the lower-left convex hull of the support points
; (ord_x(Fj), j) for the j with Fj != 0.  Each hull edge from (i1,j1) to (i2,j2) with j1 < j2 has slope
; -mu = (i1 - i2)/(j2 - j1) (so mu = (i2 - i1)/(j2 - j1) >= 0), and the branches lying on that edge have
; leading term  y ~ c x^mu  where c solves the EDGE POLYNOMIAL: the sum, over lattice points (i,j) on the edge,
; of (coeff of x^i in Fj) * c^j = 0 (a polynomial in c whose nonzero roots are the leading coefficients).
;
; This module returns, for each edge, (list mu-num mu-den edge-poly-in-c), from which the branch leading
; exponents mu = mu-num/mu-den and the leading coefficients (roots of the edge polynomial) are read.  It is the
; ramification-and-branching analyzer that the integral-basis construction of the rest of Rung 4 will consume.
;
; Builds on poly.lisp.

(import "cas/poly.lisp")

(define (nw-nth l k) (if (= k 0) (car l) (nw-nth (cdr l) (- k 1))))

; ord_x of a polynomial coefficient-list (lowest power with nonzero coeff); 'inf for the zero poly
(define (nw-ord g) (if (null? g) (quote inf) (nw-ord-go g 0)))
(define (nw-ord-go g k) (cond ((null? g) (quote inf)) ((not (= (car g) 0)) k) (else (nw-ord-go (cdr g) (+ k 1)))))

; support points: list of (i . j) with i = ord_x(Fj), for each j (0-based) with Fj nonzero
(define (nw-support F) (nw-support-go F 0))
(define (nw-support-go F j)
  (if (null? F) (quote ())
      (let ((o (nw-ord (car F))))
        (if (equal? o (quote inf))
            (nw-support-go (cdr F) (+ j 1))
            (cons (cons o j) (nw-support-go (cdr F) (+ j 1)))))))

; lower-left hull of the support, as a list of vertices ordered by increasing j.  We want the edges facing
; down-left (the Newton polygon for branches at x=0): for each j, keep the point of minimal i, then take the
; lower convex hull of those (i as a function of j).  Standard monotone-chain on (j, i) keeping lower hull.
(define (nw-min-i-per-j sup) (nw-mipj sup (quote ())))
(define (nw-mipj sup acc)
  (if (null? sup) (nw-sort-by-j acc)
      (let ((pt (car sup)))
        (nw-mipj (cdr sup) (nw-upsert acc (cdr pt) (car pt))))))   ; upsert (j -> min i)
(define (nw-upsert acc j i)
  (cond ((null? acc) (list (cons j i)))
        ((= (car (car acc)) j) (cons (cons j (if (< i (cdr (car acc))) i (cdr (car acc)))) (cdr acc)))
        (else (cons (car acc) (nw-upsert (cdr acc) j i)))))
(define (nw-sort-by-j l) (nw-isort l))
(define (nw-isort l) (if (null? l) (quote ()) (nw-insert (car l) (nw-isort (cdr l)))))
(define (nw-insert x s) (if (null? s) (list x) (if (<= (car x) (car (car s))) (cons x s) (cons (car s) (nw-insert x (cdr s))))))

; lower convex hull of points (j . i) sorted by j: keep vertices where the chain turns the right way.
; cross product test: for consecutive a,b,c, turn is "lower-convex" if (b-a)x(c-a) <= 0 removed appropriately.
(define (nw-lower-hull pts) (nw-hull-go pts (quote ())))
(define (nw-hull-go pts hull)
  (if (null? pts) (nw-reverse hull)
      (nw-hull-go (cdr pts) (nw-add-hull hull (car pts)))))
(define (nw-add-hull hull p)
  (if (if (null? hull) #t (null? (cdr hull))) (cons p hull)
      (if (nw-bad-turn (car (cdr hull)) (car hull) p)
          (nw-add-hull (cdr hull) p)
          (cons p hull))))
; points are (j . i); for lower hull we remove the middle point b if a->b->c is NOT a right turn downward,
; i.e. cross((b-a),(c-a)) >= 0 means b is above-or-on the line a-c, remove it.
(define (nw-bad-turn a b c)
  (let ((cr (- (* (- (car b) (car a)) (- (cdr c) (cdr a))) (* (- (cdr b) (cdr a)) (- (car c) (car a))))))
    (<= cr 0)))
(define (nw-reverse l) (nw-rev-go l (quote ())))
(define (nw-rev-go l acc) (if (null? l) acc (nw-rev-go (cdr l) (cons (car l) acc))))

; edges from the hull vertices (each consecutive pair), as ((j1.i1)(j2.i2)) with j1<j2
(define (nw-edges hull) (if (if (null? hull) #t (null? (cdr hull))) (quote ()) (cons (list (car hull) (car (cdr hull))) (nw-edges (cdr hull)))))

; for an edge from (j1.i1) to (j2.i2): slope mu = (i1 - i2)/(j2 - j1) (>=0).  Return (mu-num . mu-den) reduced.
(define (nw-edge-slope e)
  (let ((p1 (car e)) (p2 (car (cdr e))))
    (let ((dn (- (cdr p1) (cdr p2))) (dd (- (car p2) (car p1))))     ; (i1-i2)/(j2-j1)
      (let ((g (nw-gcd (if (< dn 0) (- 0 dn) dn) dd)))
        (if (= g 0) (cons 0 1) (cons (quotient dn g) (quotient dd g)))))))
(define (nw-gcd a b) (if (= b 0) a (nw-gcd b (remainder a b))))

; edge polynomial in c: sum over lattice points (i,j) ON the edge of [x^i] Fj * c^j.  The points on the edge
; are those (j, i) with j from j1..j2 stepping so (j, i) is collinear; i = i1 - mu*(j-j1) must be a nonneg int.
; We return the edge polynomial as a coefficient list in c (index = power of c), restricted to the edge points.
(define (nw-edge-poly F e)
  (let ((p1 (car e)) (p2 (car (cdr e))))
    (let ((j1 (car p1)) (i1 (cdr p1)) (j2 (car p2)) (i2 (cdr p2)))
      (let ((slope (nw-edge-slope e)))                                ; mu = num/den
        (nw-ep-go F j1 i1 j2 (car slope) (cdr slope) j1 (nw-zeros (+ j2 1)))))))
; iterate j from j1 to j2; the edge point has i = i1 - mu*(j - j1) = i1 - (num/den)*(j-j1); include only when
; that is a nonneg integer and within range; coefficient = [x^i] Fj.
(define (nw-ep-go F j1 i1 j2 mun mud j acc)
  (if (> j j2) acc
      (let ((di (* mun (- j j1))))
        (if (= (remainder di mud) 0)
            (let ((i (- i1 (quotient di mud))))
              (let ((coef (if (< j (length F)) (nw-coeff (nw-nth F j) i) 0)))
                (nw-ep-go F j1 i1 j2 mun mud (+ j 1) (nw-set acc j coef))))
            (nw-ep-go F j1 i1 j2 mun mud (+ j 1) acc)))))
(define (nw-coeff g i) (if (< i 0) 0 (if (< i (length g)) (nw-nth g i) 0)))
(define (nw-zeros k) (if (= k 0) (quote ()) (cons 0 (nw-zeros (- k 1)))))
(define (nw-set l idx v) (if (= idx 0) (cons v (if (null? l) (quote ()) (cdr l))) (cons (if (null? l) 0 (car l)) (nw-set (if (null? l) (quote ()) (cdr l)) (- idx 1) v))))

; ----- top-level: the Newton polygon analysis of F at x=0 -----
; returns a list of branches descriptors, one per hull edge:
;   (list (mu-num . mu-den) edge-poly-in-c)
; the branch leading exponents are mu = mu-num/mu-den; the leading coefficients are the nonzero roots of
; edge-poly-in-c.
(define (nw-newton-polygon F)
  (let ((sup (nw-support F)))
    (let ((mins (nw-min-i-per-j sup)))                               ; ((j . i) ...) sorted by j
      (let ((hull (nw-lower-hull mins)))
        (nw-describe F (nw-edges hull))))))
(define (nw-describe F edges)
  (if (null? edges) (quote ())
      (cons (list (nw-edge-slope (car edges)) (nw-edge-poly F (car edges)))
            (nw-describe F (cdr edges)))))

; convenience: list of leading exponents mu (as (num.den)) across edges
(define (nw-slopes F) (nw-slopes-go (nw-newton-polygon F)))
(define (nw-slopes-go bs) (if (null? bs) (quote ()) (cons (car (car bs)) (nw-slopes-go (cdr bs)))))
