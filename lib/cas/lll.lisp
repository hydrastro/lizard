; -*- lisp -*-
; lib/cas/lll.lisp -- Lenstra-Lenstra-Lovasz lattice basis reduction over the rationals.
;
; A lattice is the set of integer combinations of basis row-vectors b_0,...,b_{m-1}.  LLL
; rewrites the basis so the vectors are short and nearly orthogonal, while spanning the SAME
; lattice.  The algorithm interleaves size reduction (subtract integer multiples of earlier
; vectors so every Gram-Schmidt coefficient |mu_ij| <= 1/2) with swaps whenever the Lovasz
; condition |b*_k|^2 >= (delta - mu_{k,k-1}^2)|b*_{k-1}|^2 fails (delta = 3/4).  All arithmetic
; is exact rational, so the Gram-Schmidt process is exact.  A correct LLL output is fully
; characterized by its certificates: it is size-reduced, it satisfies the Lovasz condition,
; and it spans the same lattice -- the change of basis U with reduced = U * original is an
; integer matrix of determinant +-1 (equivalently the lattice determinant is preserved).
; Self-contained; no imports.

; ---------- rational vectors ----------
(define (ll-half) (/ 1 2))
(define (vdot u v) (if (null? u) 0 (+ (* (car u) (car v)) (vdot (cdr u) (cdr v)))))
(define (vsub u v) (if (null? u) '() (cons (- (car u) (car v)) (vsub (cdr u) (cdr v)))))
(define (vadd u v) (if (null? u) '() (cons (+ (car u) (car v)) (vadd (cdr u) (cdr v)))))
(define (vscale c v) (map (lambda (a) (* c a)) v))
(define (q-round x) (floor (+ x (ll-half))))      ; nearest integer (ties up)
(define (vnorm2 v) (vdot v v))

; ---------- matrices as lists of rational row-vectors ----------
(define (m-row M i) (if (= i 0) (car M) (m-row (cdr M) (- i 1))))
(define (m-nrows M) (if (null? M) 0 (+ 1 (m-nrows (cdr M)))))
(define (m-set-row M i r) (if (= i 0) (cons r (cdr M)) (cons (car M) (m-set-row (cdr M) (- i 1) r))))
(define (m-swap M i j) (m-set-row (m-set-row M i (m-row M j)) j (m-row M i)))

; ---------- Gram-Schmidt (exact) : returns the list of orthogonal b* vectors ----------
(define (gs-bstar B) (gsb B '()))
(define (gsb rows done)                              ; done = b* computed so far, in order
  (if (null? rows) done
      (gsb (cdr rows) (append done (list (gs-one (car rows) done))))))
(define (gs-one b done) (gs-sub b done))
(define (gs-sub b bstars)                            ; b - sum_j proj_{bstar_j}(b)
  (if (null? bstars) b
      (gs-sub (vsub b (vscale (/ (vdot b (car bstars)) (vnorm2 (car bstars))) (car bstars))) (cdr bstars))))
; mu_ij = <b_i, b*_j> / <b*_j, b*_j>
(define (gs-mu B bstars i j) (/ (vdot (m-row B i) (m-row bstars j)) (vnorm2 (m-row bstars j))))

; ---------- LLL ----------
(define (lll B) (lll-loop B (/ 3 4) 1 (m-nrows B)))
(define (lll-loop B delta k n)
  (if (>= k n) B
      (let ((B1 (size-reduce B k (- k 1))))
        (let ((bs (gs-bstar B1)))
          (if (>= (vnorm2 (m-row bs k))
                  (* (- delta (let ((m (gs-mu B1 bs k (- k 1)))) (* m m))) (vnorm2 (m-row bs (- k 1)))))
              (lll-loop B1 delta (+ k 1) n)
              (lll-loop (m-swap B1 k (- k 1)) delta (if (> (- k 1) 1) (- k 1) 1) n))))))
(define (size-reduce B k j)                          ; reduce b_k against b_j, b_{j-1}, ..., b_0
  (if (< j 0) B
      (size-reduce (reduce-one B k j) k (- j 1))))
(define (reduce-one B k j)
  (let ((q (q-round (gs-mu B (gs-bstar B) k j))))
    (if (= q 0) B (m-set-row B k (vsub (m-row B k) (vscale q (m-row B j)))))))

; ---------- exact rational determinant and inverse (small square matrices) ----------
(define (mat-det M) (md (m-copy M) (m-nrows M) 0 1))
(define (m-copy M) (map (lambda (r) (map (lambda (x) x) r)) M))
(define (md M n col acc)
  (if (>= col n) acc
      (let ((piv (find-piv M n col col)))
        (if (< piv 0) 0
            (let ((M1 (if (= piv col) M (m-swap M col piv))))
              (let ((sgn (if (= piv col) acc (- acc))))
                (let ((p (vget (m-row M1 col) col)))
                  (md (elim-below M1 n col p) n (+ col 1) (* sgn p)))))))))
(define (vget v i) (if (= i 0) (car v) (vget (cdr v) (- i 1))))
(define (find-piv M n col r) (cond ((>= r n) -1) ((not (= (vget (m-row M r) col) 0)) r) (else (find-piv M n col (+ r 1)))))
(define (elim-below M n col p) (eb M n col p (+ col 1)))
(define (eb M n col p r)
  (if (>= r n) M
      (let ((f (/ (vget (m-row M r) col) p)))
        (eb (m-set-row M r (vsub (m-row M r) (vscale f (m-row M col)))) n col p (+ r 1)))))

; integer matrix? (every entry an integer)
(define (int-entry? x) (= x (floor x)))
(define (int-matrix? M) (cond ((null? M) #t) ((all-int? (car M)) (int-matrix? (cdr M))) (else #f)))
(define (all-int? r) (cond ((null? r) #t) ((int-entry? (car r)) (all-int? (cdr r))) (else #f)))

; ---------- inverse via Gauss-Jordan on [M | I] (exact rationals) ----------
(define (ident n) (id-rows 0 n))
(define (id-rows i n) (if (>= i n) '() (cons (e-row i n) (id-rows (+ i 1) n))))
(define (e-row i n) (er 0 n i))
(define (er j n i) (if (>= j n) '() (cons (if (= j i) 1 0) (er (+ j 1) n i))))
(define (mat-inverse M)
  (let ((n (m-nrows M)))
    (strip-left (gj (augment M (ident n)) n 0) n)))
(define (augment M I) (if (null? M) '() (cons (append (car M) (car I)) (augment (cdr M) (cdr I)))))
(define (gj A n col)
  (if (>= col n) A
      (let ((piv (find-piv A n col col)))
        (let ((A1 (if (= piv col) A (m-swap A col piv))))
          (let ((p (vget (m-row A1 col) col)))
            (let ((A2 (m-set-row A1 col (vscale (/ 1 p) (m-row A1 col)))))
              (gj (clear-col A2 n col) n (+ col 1))))))))
(define (clear-col A n col) (cc A n col 0))
(define (cc A n col r)
  (if (>= r n) A
      (if (= r col) (cc A n col (+ r 1))
          (let ((f (vget (m-row A r) col)))
            (cc (m-set-row A r (vsub (m-row A r) (vscale f (m-row A col)))) n col (+ r 1))))))
(define (strip-left A n) (map (lambda (r) (drop-n r n)) A))
(define (drop-n r k) (if (= k 0) r (drop-n (cdr r) (- k 1))))
(define (mat-mul A B) (map (lambda (row) (mm-row row B)) A))
(define (mm-row row B) (mm-acc row B))
(define (mm-acc row B) (if (null? (cdr B)) (vscale (car row) (car B)) (vadd (vscale (car row) (car B)) (mm-acc (cdr row) (cdr B)))))
(define (zeros-like v) (map (lambda (x) 0) v))

; ---------- certificates ----------
(define (lll-size-reduced? B) (sr-check B (gs-bstar B) 1 (m-nrows B)))
(define (sr-check B bs i n)
  (if (>= i n) #t
      (and (sr-row B bs i (- i 1)) (sr-check B bs (+ i 1) n))))
(define (sr-row B bs i j)
  (if (< j 0) #t
      (and (<= (qabs (gs-mu B bs i j)) (ll-half)) (sr-row B bs i (- j 1)))))
(define (qabs x) (if (< x 0) (- x) x))
(define (lll-lovasz-ok? B) (lov B (gs-bstar B) (/ 3 4) 1 (m-nrows B)))
(define (lov B bs delta k n)
  (if (>= k n) #t
      (and (>= (vnorm2 (m-row bs k)) (* (- delta (let ((m (gs-mu B bs k (- k 1)))) (* m m))) (vnorm2 (m-row bs (- k 1)))))
           (lov B bs delta (+ k 1) n))))
; same lattice: U = reduced * original^{-1} is integer with |det| = 1
(define (lll-transform reduced original) (mat-mul reduced (mat-inverse original)))
(define (lll-same-lattice? reduced original)
  (let ((U (lll-transform reduced original)))
    (and (int-matrix? U) (= (qabs (mat-det U)) 1))))
(define (lll-det-preserved? reduced original) (= (qabs (mat-det reduced)) (qabs (mat-det original))))
