; -*- lisp -*-
; lib/cas/puiseuxg.lisp -- term-by-term PUISEUX EXPANSION of the branches of a general plane algebraic curve
; F(x,y) = 0, continuing Rung 4 (docs/TRAGER_ROADMAP.md).  newton.lisp gives the leading exponents and
; coefficients of every branch (the Newton-polygon edges); this module ITERATES from each leading term to the
; FULL Puiseux series of the branch -- the local analytic description of an arbitrary algebraic function that
; the integral basis (the remainder of Rung 4) is built on.
;
; F is a list of y-coefficients, each a polynomial in x (low-to-high): F = (F0 F1 ... Fd) means
; sum_j Fj(x) y^j.  For a branch with Newton slope mu = p/q (reduced) and a nonzero edge-polynomial root c, set
; the uniformizer t = x^(1/q); the branch is y = sum_{k >= p} a_k t^k with a_p = c.  Working in t (so x = t^q
; makes F a t-/y-polynomial), each successive coefficient is determined LINEARLY:
;     a_k = - [t^{k+L}] F(t^q, y_partial) / [t^L] F_y(t^q, y_partial),     L = ord_t F_y(t^q, y_partial),
; which is exactly Newton-Puiseux for a branch that is smooth in the uniformizer (F_y a unit there -- the case
; for a reduced curve away from singular leading behaviour).  The result is returned as (puiseux q p coeffs),
; meaning y = sum_i coeffs[i] x^((p+i)/q), and every branch is power-checked by substituting back into F.
;
; Reuses newton.lisp (the polygon, slopes, edge polynomials) and poly.lisp.  Verified: y^2-(1+x) recovers
; sqrt(1+x); the cusp y^2-x^3 recovers y = x^(3/2); y = x + y^2 recovers the Catalan series x + x^2 + 2x^3 +
; 5x^4 + ...; and the node y^2-x^2-x^3 yields its two branches y = +-x + ... .

(import "cas/newton.lisp")

(define (pg-nth l k) (if (= k 0) (car l) (pg-nth (cdr l) (- k 1))))
(define (pg-idx l i) (if (< i (length l)) (pg-nth l i) 0))

; ----- truncated t-series arithmetic (index = power of t) -----
(define (pg-zeros k) (if (= k 0) (quote ()) (cons 0 (pg-zeros (- k 1)))))
(define (pg-sset l idx v) (if (= idx 0) (cons v (if (null? l) (quote ()) (cdr l))) (cons (if (null? l) 0 (car l)) (pg-sset (if (null? l) (quote ()) (cdr l)) (- idx 1) v))))
(define (pg-sadd a b) (if (null? a) b (if (null? b) a (cons (+ (car a) (car b)) (pg-sadd (cdr a) (cdr b))))))
(define (pg-smul a b M) (pg-smul-go a b M 0))
(define (pg-smul-go a b M k) (if (> k M) (quote ()) (cons (pg-sconv a b k) (pg-smul-go a b M (+ k 1)))))
(define (pg-sconv a b k) (pg-sconv-go a b k 0 0))
(define (pg-sconv-go a b k i s) (if (> i k) s (pg-sconv-go a b k (+ i 1) (+ s (* (pg-idx a i) (pg-idx b (- k i)))))))

; inflate a poly-in-x P(x) to P(t^q) as a t-series (x^i -> t^(i q))
(define (pg-inflate P q M) (pg-inflate-go P q M 0 (pg-zeros (+ M 1))))
(define (pg-inflate-go P q M i acc) (if (>= i (length P)) acc (pg-inflate-go P q M (+ i 1) (pg-sset acc (* i q) (pg-nth P i)))))

; F(t^q, Y) for a t-series Y: sum_j inflate(Fj) * Y^j, truncated to M
(define (pg-feval F Y q M) (pg-feval-go F Y q M 0 (pg-zeros (+ M 1)) (cons 1 (pg-zeros M))))
(define (pg-feval-go F Y q M j acc Yj)
  (if (>= j (length F)) acc
      (pg-feval-go F Y q M (+ j 1)
                   (pg-sadd acc (pg-smul (pg-inflate (pg-nth F j) q M) Yj M))
                   (pg-smul Yj Y M))))

; F_y as a y-coefficient list: (F1, 2 F2, 3 F3, ...)
(define (pg-fderiv-y F) (pg-fd-go (cdr F) 1))
(define (pg-fd-go F j) (if (null? F) (quote ()) (cons (poly-scale j (car F)) (pg-fd-go (cdr F) (+ j 1)))))

(define (pg-loword s) (pg-low-go s 0))
(define (pg-low-go s k) (cond ((> k 200) 0) ((null? s) 0) ((not (= (car s) 0)) k) (else (pg-low-go (cdr s) (+ k 1)))))

; ----- term-by-term refinement from a known leading term a_p = c (uniformizer power q) to order M in t -----
(define (pg-refine F Fy y q p M k)
  (if (> k M) y
      (let ((Fval (pg-feval F y q (+ M (* 3 q)))) (Fyval (pg-feval Fy y q (+ M (* 3 q)))))
        (let ((L (pg-loword Fyval)))
          (let ((denom (pg-idx Fyval L)))
            (if (= denom 0) y                                  ; F_y identically zero along branch: stop (singular)
                (let ((ak (/ (- 0 (pg-idx Fval (+ k L))) denom)))
                  (pg-refine F Fy (pg-sset y k ak) q p M (+ k 1)))))))))

; ----- rational nonzero roots of the edge polynomial (rational root scan; sufficient for low-degree curves) -----
(define (pg-edge-roots P D) (pg-er-go P D 1 1 (quote ())))
(define (pg-er-go P D p q acc)
  (cond ((> p D) (pg-add-negs P D 1 1 acc))                   ; positives done; now negatives
        ((> q D) (pg-er-go P D (+ p 1) 1 acc))
        (else (let ((r (/ p q)))
                (if (if (= (poly-eval P r) 0) (not (pg-mem r acc)) #f)
                    (pg-er-go P D p (+ q 1) (cons r acc))
                    (pg-er-go P D p (+ q 1) acc))))))
(define (pg-add-negs P D p q acc)
  (cond ((> p D) acc)
        ((> q D) (pg-add-negs P D (+ p 1) 1 acc))
        (else (let ((r (/ (- 0 p) q)))
                (if (if (= (poly-eval P r) 0) (not (pg-mem r acc)) #f)
                    (pg-add-negs P D p (+ q 1) (cons r acc))
                    (pg-add-negs P D p (+ q 1) acc))))))
(define (pg-mem x l) (if (null? l) #f (if (= x (car l)) #t (pg-mem x (cdr l)))))

; ----- expand ONE branch given an edge descriptor ((mu-num . mu-den) edge-poly) and a chosen root c -----
; returns (list 'puiseux q p coeffs) with y = sum_i coeffs[i] x^((p+i)/q)
(define (pg-branch F edge c M)
  (let ((slope (car edge)))
    (let ((p (car slope)) (q (cdr slope)))
      (let ((Fy (pg-fderiv-y F)))
        (let ((y0 (pg-sset (pg-zeros (+ (* (+ M 1) q) (* 4 q))) p c)))
          (let ((full (pg-refine F Fy y0 q p (+ p M) (+ p 1))))
            (list (quote puiseux) q p (pg-extract full p (+ p M)))))))))
(define (pg-extract y from to) (if (> from to) (quote ()) (cons (pg-idx y from) (pg-extract y (+ from 1) to))))

; ----- top level: all branches of F (one Puiseux series per edge per rational edge-root) to M terms -----
; returns a list of (list 'puiseux q p coeffs); branches whose edge polynomial has no rational nonzero root
; are reported as (list 'needs-radical edge-poly).
(define (pg-branches F M) (pg-br-go F (nw-newton-polygon F) M))
(define (pg-br-go F edges M)
  (if (null? edges) (quote ())
      (append (pg-edge-branches F (car edges) M) (pg-br-go F (cdr edges) M))))
(define (pg-edge-branches F edge M)
  (let ((roots (pg-edge-roots (car (cdr edge)) 8)))
    (if (null? roots) (list (list (quote needs-radical) (car (cdr edge))))
        (pg-mk-branches F edge roots M))))
(define (pg-mk-branches F edge roots M)
  (if (null? roots) (quote ())
      (cons (pg-branch F edge (car roots) M) (pg-mk-branches F edge (cdr roots) M))))

; ----- verification: substitute a branch back into F, return the t-series of F (should be ~0) -----
(define (pg-verify F branch M)
  (let ((q (pg-nth branch 1)) (p (pg-nth branch 2)) (coeffs (pg-nth branch 3)))
    (let ((y (pg-shiftup coeffs p)))
      (pg-feval F y q M))))
(define (pg-shiftup l k) (if (= k 0) l (cons 0 (pg-shiftup l (- k 1)))))
(define (pg-zero-series? s upto) (pg-zs-go s 0 upto))
(define (pg-zs-go s k upto) (if (> k upto) #t (if (= (pg-idx s k) 0) (pg-zs-go s (+ k 1) upto) #f)))
