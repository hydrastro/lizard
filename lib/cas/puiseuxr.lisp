; -*- lisp -*-
; lib/cas/puiseuxr.lisp -- RECURSIVE Newton-Puiseux: separating branches that share a leading term, the piece
; that completes the local-branch analysis for curves with tangent multiplicity (continuing Rung 4,
; docs/TRAGER_ROADMAP.md).
;
; newton.lisp gives the Newton-polygon edges, and puiseuxg.lisp expands a branch when its edge-polynomial root
; is SIMPLE.  When a root c has multiplicity m > 1, the m branches through the place all begin c*x^mu and only
; separate at higher order; the simple-root solver cannot tell them apart (F_y vanishes along the shared
; leading behaviour).  The classical fix is to SUBSTITUTE y = (c + y1) * x^mu, which yields a new polynomial
; equation G(x, y1) = 0; after dividing out the common x-power, G's own Newton polygon resolves the next term,
; and one recurses until the roots are simple (the branches have separated).
;
; This module implements that substitution and recursion.  pr-subst performs y -> (c + y1) x^mu on a curve
; given as a y-coefficient list (each coefficient a polynomial in x); pr-deflate divides out the common
; x-power; and pr-branch-leads returns, for each branch, the sequence of leading (exponent, coefficient) pairs
; that distinguishes it -- the data the integral-basis correction terms are built from.  Verified on
; F = (y - x^2)(y - x^2 - x^3): the shared leading term x^2 (double root c=1) is peeled, and the recursion
; separates the two branches y = x^2 and y = x^2 + x^3.
;
; Builds on newton.lisp (the polygon + edge polynomials) and poly.lisp.

(import "cas/newton.lisp")

(define (pr-nth l k) (if (= k 0) (car l) (pr-nth (cdr l) (- k 1))))

; binomial coefficient C(n,k), iterative (no deep recursion)
(define (pr-binom n k) (if (if (< k 0) #t (> k n)) 0 (pr-binom-go n k 1 0)))
(define (pr-binom-go n k acc i) (if (>= i k) acc (pr-binom-go n k (quotient (* acc (- n i)) (+ i 1)) (+ i 1))))
(define (pr-ipow b e) (if (= e 0) 1 (* b (pr-ipow b (- e 1)))))
(define (pr-zeros k) (if (= k 0) (quote ()) (cons 0 (pr-zeros (- k 1)))))
(define (pr-xmul P e) (append (pr-zeros e) P))                 ; P(x) * x^e

; substitute y -> (c + y1) * x^mu in F (y-coeff list); return the new y1-coeff list.
; F = sum_j Fj(x) y^j  ->  sum_j Fj(x) x^{mu j} (c+y1)^j ; coefficient of y1^i is
;   sum_{j>=i} Fj(x) x^{mu j} C(j,i) c^{j-i}.
(define (pr-subst F c mu)
  (let ((d (- (length F) 1)))
    (pr-subst-go F c mu d 0)))
(define (pr-subst-go F c mu d i)
  (if (> i d) (quote ())
      (cons (pr-coeff F c mu d i i (quote ())) (pr-subst-go F c mu d (+ i 1)))))
(define (pr-coeff F c mu d i j acc)
  (if (> j d) acc
      (let ((Fj (pr-nth F j)))
        (let ((term (poly-scale (* (pr-binom j i) (pr-ipow c (- j i))) (pr-xmul Fj (* mu j)))))
          (pr-coeff F c mu d i (+ j 1) (poly-add acc term))))))

; common x-order across all coefficient polynomials (the x-power to divide out), and the deflation
(define (pr-min-xord F) (pr-mxo F (quote none)))
(define (pr-mxo F best)
  (if (null? F) (if (equal? best (quote none)) 0 best)
      (let ((o (pr-xord (car F))))
        (if (equal? o (quote inf)) (pr-mxo (cdr F) best)
            (pr-mxo (cdr F) (if (equal? best (quote none)) o (if (< o best) o best)))))))
(define (pr-xord p) (if (null? p) (quote inf) (pr-xord-go p 0)))
(define (pr-xord-go p k) (cond ((null? p) (quote inf)) ((not (= (car p) 0)) k) (else (pr-xord-go (cdr p) (+ k 1)))))
(define (pr-deflate F) (pr-deflate-by F (pr-min-xord F)))
(define (pr-deflate-by F v) (if (null? F) (quote ()) (cons (pr-dropk (car F) v) (pr-deflate-by (cdr F) v))))
(define (pr-dropk p v) (if (= v 0) p (if (null? p) (quote ()) (pr-dropk (cdr p) (- v 1)))))

; rational nonzero roots of a poly-in-c WITH multiplicities: returns list of (root . mult).
(define (pr-roots-mult P D) (pr-rm-collect P (pr-rat-roots P D)))
(define (pr-rm-collect P roots) (if (null? roots) (quote ()) (cons (cons (car roots) (pr-mult P (car roots))) (pr-rm-collect P (cdr roots)))))
(define (pr-mult P r) (pr-mult-go P r 0))
(define (pr-mult-go P r acc) (if (pr-divides-root? P r) (pr-mult-go (pr-deflate-root P r) r (+ acc 1)) acc))
(define (pr-divides-root? P r) (= (poly-eval P r) 0))
(define (pr-deflate-root P r) (car (poly-divmod P (list (- 0 r) 1))))   ; P / (x - r)
(define (pr-rat-roots P D) (pr-rr-pos P D 1 1 (quote ())))
(define (pr-rr-pos P D p q acc)
  (cond ((> p D) (pr-rr-neg P D 1 1 acc))
        ((> q D) (pr-rr-pos P D (+ p 1) 1 acc))
        (else (let ((r (/ p q))) (if (if (= (poly-eval P r) 0) (not (pr-memq r acc)) #f) (pr-rr-pos P D p (+ q 1) (cons r acc)) (pr-rr-pos P D p (+ q 1) acc))))))
(define (pr-rr-neg P D p q acc)
  (cond ((> p D) acc)
        ((> q D) (pr-rr-neg P D (+ p 1) 1 acc))
        (else (let ((r (/ (- 0 p) q))) (if (if (= (poly-eval P r) 0) (not (pr-memq r acc)) #f) (pr-rr-neg P D p (+ q 1) (cons r acc)) (pr-rr-neg P D p (+ q 1) acc))))))
(define (pr-memq x l) (if (null? l) #f (if (= x (car l)) #t (pr-memq x (cdr l)))))

; ----- branch leading-term sequences via the recursion (integer-slope edges; the common case at a place) -----
; pr-branch-leads F depth: returns a list of branches, each a list of (mu . c) leading-term pairs accumulated
; along the recursion (y = c0 x^{mu0} + c1 x^{mu0+mu1} + ...).  depth bounds the recursion (number of terms).
(define (pr-branch-leads F depth) (pr-bl F depth (quote ())))
(define (pr-bl F depth prefix)
  (if (<= depth 0) (list (pr-reverse prefix))
      ; if the constant-in-y coefficient F0 is zero, y | F: y = 0 (i.e. the current y1 = 0) is an EXACT branch
      ; here -- emit it as a completed branch -- and continue separating the remaining branches of F / y.
      (if (pr-zero-poly? (car F))
          (pr-append (list (pr-reverse prefix)) (pr-bl-edges (pr-divy F) (nw-newton-polygon (pr-divy F)) depth prefix))
          (pr-bl-edges F (nw-newton-polygon F) depth prefix))))
(define (pr-zero-poly? p) (cond ((null? p) #t) ((not (= (car p) 0)) #f) (else (pr-zero-poly? (cdr p)))))
(define (pr-divy F) (cdr F))                                   ; divide a y-coeff list by y (drop F0, shift down)
(define (pr-bl-edges F edges depth prefix)
  (if (null? edges) (quote ())
      (pr-append (pr-bl-edge F (car edges) depth prefix) (pr-bl-edges F (cdr edges) depth prefix))))
(define (pr-bl-edge F edge depth prefix)
  (let ((slope (car edge)) (ep (car (cdr edge))))
    (if (not (= (cdr slope) 1)) (list (pr-reverse (cons (cons slope (quote ramified)) prefix)))   ; fractional slope: report, stop
        (let ((mu (car slope)))
          (pr-bl-roots F mu (pr-roots-mult ep 8) depth prefix)))))
(define (pr-bl-roots F mu roots depth prefix)
  (if (null? roots) (quote ())
      (let ((c (car (car roots))) (m (cdr (car roots))))
        (let ((here (cons (cons mu c) prefix)))
          (if (= m 1)
              (pr-append (list (pr-reverse here)) (pr-bl-roots F mu (cdr roots) depth prefix))
              ; multiple root: substitute and recurse to separate
              (pr-append (pr-bl (pr-deflate (pr-subst F c mu)) (- depth 1) here)
                         (pr-bl-roots F mu (cdr roots) depth prefix)))))))
(define (pr-reverse l) (pr-rev l (quote ())))
(define (pr-rev l acc) (if (null? l) acc (pr-rev (cdr l) (cons (car l) acc))))
(define (pr-append a b) (if (null? a) b (cons (car a) (pr-append (cdr a) b))))
