; -*- lisp -*-
; lib/cas/ellint.lisp -- the COMPLETE elliptic third-kind integration test and construction, correcting and
; completing Rungs 3b / the explicit elliptic logarithm (docs/TRAGER_ROADMAP.md).
;
; PRIOR STATE.  elltorsion.lisp decided INT dx/((x-s) sqrt(p)) elementary <=> the pole lifts to a torsion point
; P=(s,rho).  That condition is NECESSARY but NOT SUFFICIENT.  The correct criterion (Trager; Combot,
; arXiv:2103.04134, "the difference I - L is of first kind ... and then I is elementary if and only if I - L = 0")
; is TWO parts:
;   (1) P is torsion  -- this is exactly what makes the logarithmic part L = c log f EXIST; and
;   (2) the remainder I - L, which is then a holomorphic (first-kind) differential lambda*dx/y, must VANISH.
; A nonzero lambda means I = (elementary log) + lambda*(elliptic integral of the first kind), which is NOT
; elementary (dx/y is the canonical non-elementary elliptic integral).
;
; This module implements the complete test by actually CONSTRUCTING L and computing the remainder:
;   * build the Miller function g with div(g) = N[P] - N[O] (N = order of P) by INTERPOLATION -- solve for
;     g = A(x) + B(x) y, deg A = floor(N/2), deg B = floor((N-3)/2), vanishing to order N at P, using the local
;     y-power-series x = s+t, y = sqrt(p(s+t)); this is robust at 2-torsion places (unlike the naive Miller
;     line/vertical recurrence) and is verified by N(g) = g*conj(g) = (x-s)^N;
;   * f = g / conj(g) has div = N([P] - [-P]); the log-derivative c*f'/f with c = 1/(N rho) has exactly the
;     residues of omega = dx/((x-s)y) (namely +1/rho at P, -1/rho at -P);
;   * the remainder omega - c f'/f is lambda*dx/y; the integral is ELEMENTARY iff lambda = 0, and then the
;     antiderivative c log f is certified by the differentiation arbiter.
;
; Findings (all checked here): INT dx/(x sqrt(x^3+1)) IS elementary = (1/3) log((y-1)/(y+1)) (lambda = 0);
; but the torsion poles of orders 4, 5, 6 tested all have lambda != 0 and are therefore NON-elementary --
; correcting the earlier over-optimistic verdict.  Torsion is necessary, lambda = 0 is the rest.
;
; Builds on algfunc.lisp (the field K), elltorsion.lisp (group law + order), linalg.lisp (the linear solve).

(import "cas/algfunc.lisp")
(import "cas/elltorsion.lisp")
(import "cas/linalg.lisp")

(define (ei-nth l k) (if (= k 0) (car l) (ei-nth (cdr l) (- k 1))))
(define (ei-idx l i) (if (< i (length l)) (ei-nth l i) 0))

; ----- local y-power-series at P=(s,rho): x = s+t, y = sqrt(p(s+t)) = sum c_k t^k -----
(define (ei-pshift poly s) (ei-pshift-go (reverse poly) s (list 0)))
(define (ei-pshift-go cs s acc) (if (null? cs) acc (ei-pshift-go (cdr cs) s (poly-add (poly-mul acc (list s 1)) (list (car cs))))))
(define (ei-pscoeff P k) (if (< k (length P)) (ei-nth P k) 0))
(define (ei-yseries P rho M) (ei-yser-go P rho M 1 (list rho)))
(define (ei-yser-go P rho M k acc)
  (if (> k M) acc
      (let ((cv (ei-yconv acc k)))
        (let ((ck (/ (- (ei-pscoeff P k) cv) (* 2 rho))))
          (ei-yser-go P rho M (+ k 1) (append acc (list ck)))))))
(define (ei-yconv acc k) (ei-yconv-go acc k 1 0))
(define (ei-yconv-go acc k i s) (if (>= i k) s (ei-yconv-go acc k (+ i 1) (+ s (* (ei-nth acc i) (ei-nth acc (- k i)))))))

; ----- truncated power-series helpers (index = power of t) -----
(define (ei-zeros k) (if (= k 0) (quote ()) (cons 0 (ei-zeros (- k 1)))))
(define (ei-trunc l n) (if (= n 0) (quote ()) (if (null? l) (cons 0 (ei-trunc (quote ()) (- n 1))) (cons (car l) (ei-trunc (cdr l) (- n 1))))))
(define (ei-monshift j s M) (ei-trunc (ei-pshift (append (ei-zeros j) (list 1)) s) (+ M 1)))   ; (s+t)^j as t-series
(define (ei-smul a b M) (ei-smul-go a b M 0))
(define (ei-smul-go a b M k) (if (> k M) (quote ()) (cons (ei-sconv a b k) (ei-smul-go a b M (+ k 1)))))
(define (ei-sconv a b k) (ei-sconv-go a b k 0 0))
(define (ei-sconv-go a b k i s) (if (> i k) s (ei-sconv-go a b k (+ i 1) (+ s (* (ei-idx a i) (ei-idx b (- k i)))))))

; ----- build g = A(x) + B(x) y with div = N[P]-N[O], N = order(P), by interpolation (vanish order N at P) -----
; returns (cons A B) coefficient lists, or 'degenerate.
(define (ei-buildg cv s rho N)
  (let ((dA (quotient N 2)) (dB (quotient (- N 3) 2)))
    (let ((ys (ei-yseries (ei-pshift cv s) rho (+ N 6))))
      (let ((lhs (ei-mkl s ys dA dB N 0)) (rhs (ei-mkr s ys dA dB N 0)))
        (let ((sol (mat-solve lhs rhs)))
          (if (equal? sol (quote none)) (quote degenerate)
              (let ((full (append sol (list 1))))
                (cons (ei-firstn full (+ dA 1)) (ei-lastn full (+ dB 1))))))))))
; row k (k = 0..N-2): t^k coefficient from each unknown a_0..a_dA then b_0..b_dB; last unknown (b_dB) -> RHS.
(define (ei-arow s k dA j) (if (> j dA) (quote ()) (cons (ei-idx (ei-monshift j s (+ k 1)) k) (ei-arow s k dA (+ j 1)))))
(define (ei-brow s ys k dB j) (if (> j dB) (quote ()) (cons (ei-idx (ei-smul (ei-monshift j s (+ k 2)) ys (+ k 1)) k) (ei-brow s ys k dB (+ j 1)))))
(define (ei-frow s ys k dA dB) (append (ei-arow s k dA 0) (ei-brow s ys k dB 0)))
(define (ei-mkl s ys dA dB N k) (if (>= k (- N 1)) (quote ()) (cons (ei-trunc (ei-frow s ys k dA dB) (- N 1)) (ei-mkl s ys dA dB N (+ k 1)))))
(define (ei-mkr s ys dA dB N k) (if (>= k (- N 1)) (quote ()) (cons (- 0 (ei-idx (ei-frow s ys k dA dB) (- N 1))) (ei-mkr s ys dA dB N (+ k 1)))))
(define (ei-firstn l n) (if (= n 0) (quote ()) (cons (car l) (ei-firstn (cdr l) (- n 1)))))
(define (ei-dropn l k) (if (= k 0) l (ei-dropn (cdr l) (- k 1))))
(define (ei-lastn l n) (ei-dropn l (- (length l) n)))

; ----- the complete decision + construction -----
; INT dx/((x-s) sqrt(p)), p a squarefree cubic (a rat, denom 1), s a rational pole (p(s) != 0).
; returns:
;   (list 'elementary 'log c f)            -- elementary; c log f certified, f in K
;   (list 'non-elementary 'first-kind-remainder)  -- pole torsion but lambda != 0 (log + nonzero elliptic int)
;   (list 'non-elementary 'infinite-order)        -- pole not torsion
;   (list 'needs-extension) | (list 'branch-pole) | (list 'not-genus1) | (list 'degenerate)
(define (ei-integrate p s)
  (let ((pp (rat-num p)))
    (if (not (= (poly-deg pp) 3)) (list (quote not-genus1))
        (let ((ps (poly-eval pp s)))
          (if (= ps 0) (list (quote branch-pole))
              (let ((rho (elt-sqrt-q ps)))
                (if (equal? rho (quote no)) (list (quote needs-extension))
                    (let ((a2 (poly-coeff pp 2)) (a1 (poly-coeff pp 1)))
                      (let ((ord (ei-torsion-order (cons s rho) a2 a1)))
                        (if (equal? ord (quote infinite)) (list (quote non-elementary) (quote infinite-order))
                            (ei-build-and-test p pp s rho a2 a1 ord)))))))))))

; torsion order that also works for non-integral models (no Nagell-Lutz early exit; bound by 12 then 'infinite)
(define (ei-torsion-order P a2 a1) (ei-ord-go P P a2 a1 1))
(define (ei-ord-go cur P a2 a1 n)
  (cond ((equal? cur (quote O)) n)
        ((> n 14) (quote infinite))
        (else (ei-ord-go (elt-add cur P a2 a1) P a2 a1 (+ n 1)))))

(define (ei-build-and-test p pp s rho a2 a1 N)
  (let ((gAB (ei-buildg pp s rho N)))
    (if (equal? gAB (quote degenerate)) (list (quote degenerate))
        (let ((g (af-make (rat-from-poly (car gAB)) (rat-from-poly (cdr gAB)))))
          (let ((gc (af-make (af-u g) (rat-neg (af-v g)))))
            (let ((f (af-div p g gc)))
              (let ((cc (rat-make (list 1) (list (* N rho)))))                 ; c = 1/(N rho)
                (let ((integ (af-make (rat-zero) (rat-make (list 1) (poly-mul (list (- 0 s) 1) pp)))))
                  (let ((rem (af-sub integ (ei-scale p cc (af-div p (af-deriv p f) f)))))
                    (if (ei-zero-elt? rem)
                        (if (af-certify p (af-zero) cc f integ)
                            (list (quote elementary) (quote log) cc f)
                            (list (quote non-elementary) (quote certificate-failed)))
                        (list (quote non-elementary) (quote first-kind-remainder))))))))))))

(define (ei-scale p c e) (af-mul p (af-make c (rat-zero)) e))           ; multiply element by rational constant c
(define (ei-zero-elt? e) (if (rat-zero? (af-u e)) (rat-zero? (af-v e)) #f))

(define (ei-elementary? p s) (equal? (car (ei-integrate p s)) (quote elementary)))
; expose lambda (the holomorphic remainder coefficient) for inspection: returns the remainder element's v-part
; over (1/p) -- i.e. the coefficient of dx/y -- or 0 when elementary.
(define (ei-remainder-lambda p s)
  (let ((pp (rat-num p)))
    (let ((ps (poly-eval pp s)))
      (let ((rho (elt-sqrt-q ps)))
        (if (equal? rho (quote no)) (quote needs-extension)
            (let ((a2 (poly-coeff pp 2)) (a1 (poly-coeff pp 1)))
              (let ((N (ei-torsion-order (cons s rho) a2 a1)))
                (if (equal? N (quote infinite)) (quote infinite)
                    (let ((gAB (ei-buildg pp s rho N)))
                      (let ((g (af-make (rat-from-poly (car gAB)) (rat-from-poly (cdr gAB)))))
                        (let ((gc (af-make (af-u g) (rat-neg (af-v g)))))
                          (let ((f (af-div p g gc)))
                            (let ((cc (rat-make (list 1) (list (* N rho)))))
                              (let ((integ (af-make (rat-zero) (rat-make (list 1) (poly-mul (list (- 0 s) 1) pp)))))
                                (let ((rem (af-sub integ (ei-scale p cc (af-div p (af-deriv p f) f)))))
                                  (rat-mul (af-v rem) (rat-from-poly pp)))))))))))))))))   ; (v-part)*p = lambda
