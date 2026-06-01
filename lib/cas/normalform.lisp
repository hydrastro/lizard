; -*- lisp -*-
; lib/cas/normalform.lisp — kernel/rank/nullspace over Q and integer normal forms.
;
; Rounds out the linear-algebra pillar with the structural decompositions:
;   * reduced row echelon form, rank, and a nullspace (kernel) basis over Q;
;   * Hermite Normal Form over Z:  H = U A with U unimodular (det +/-1);
;   * Smith Normal Form over Z:    D = U A V with U,V unimodular, D diagonal and
;     d_1 | d_2 | ... (the structure theorem for finitely generated abelian groups).
;
; Everything is certified by re-multiplying the unimodular factors and checking
; det = +/-1 (so the transforms are genuine Z-automorphisms), plus A v = 0 for
; kernel vectors.  Integer work uses exact gcd/quotient/floor; builds on linalg.

(import "cas/linalg.lisp")

; ---------- shared row/element helpers ----------
(define (set-nth lst i v) (if (= i 0) (cons v (cdr lst)) (cons (car lst) (set-nth (cdr lst) (- i 1) v))))
(define (mref M i j) (nth (nth M i) j))
(define (row-scale c r) (map (lambda (x) (* c x)) r))
(define (row-axpy a src dst) (map2 (lambda (s d) (+ d (* a s))) src dst))   ; dst + a*src
(define (swap-rows M i j) (if (= i j) M (set-nth (set-nth M i (nth M j)) j (nth M i))))
(define (member-int x lst) (cond ((null? lst) #f) ((= x (car lst)) #t) (else (member-int x (cdr lst)))))
(define (idx-of lst x i) (cond ((null? lst) -1) ((= (car lst) x) i) (else (idx-of (cdr lst) x (+ i 1)))))

; ================= reduced row echelon form over Q =================
; returns (list rref-matrix pivot-columns)
(define (pivot-row-at M col prow nrows)
  (cond ((>= prow nrows) -1) ((not (= (mref M prow col) 0)) prow) (else (pivot-row-at M col (+ prow 1) nrows))))
(define (scale-row-to-one M r col) (set-nth M r (row-scale (/ 1 (mref M r col)) (nth M r))))
(define (elim-go M r col i nrows)
  (if (>= i nrows) M
    (if (= i r) (elim-go M r col (+ i 1) nrows)
      (let ((f (mref M i col)))
        (if (= f 0) (elim-go M r col (+ i 1) nrows)
            (elim-go (set-nth M i (row-axpy (- 0 f) (nth M r) (nth M i))) r col (+ i 1) nrows))))))
(define (rref-cols M col prow ncols nrows pivots)
  (if (or (>= col ncols) (>= prow nrows)) (list M (reverse pivots))
    (let ((p (pivot-row-at M col prow nrows)))
      (if (< p 0) (rref-cols M (+ col 1) prow ncols nrows pivots)
        (let ((M3 (elim-go (scale-row-to-one (swap-rows M prow p) prow col) prow col 0 nrows)))
          (rref-cols M3 (+ col 1) (+ prow 1) ncols nrows (cons col pivots)))))))
(define (mat-rref M) (rref-cols M 0 0 (mat-cols M) (mat-rows M) '()))
(define (mat-rank M) (length (car (cdr (mat-rref M)))))

; ---------- nullspace (kernel) basis over Q ----------
(define (free-cols piv nc) (filter (lambda (c) (not (member-int c piv))) (iota 0 (- nc 1))))
(define (null-entry R piv c f)
  (cond ((= c f) 1) ((member-int c piv) (- 0 (mref R (idx-of piv c 0) f))) (else 0)))
(define (null-vec R piv nc f) (map (lambda (c) (null-entry R piv c f)) (iota 0 (- nc 1))))
(define (mat-nullspace M)
  (let ((rr (mat-rref M)))
    (let ((R (car rr)) (piv (car (cdr rr))) (nc (mat-cols M)))
      (map (lambda (f) (null-vec R piv nc f)) (free-cols piv nc)))))
; certificate: A v = 0 for every basis vector v
(define (nullspace-ok? A) (all-zero-vecs (map (lambda (v) (mat-apply A v)) (mat-nullspace A))))
(define (all-zero-vecs vs) (cond ((null? vs) #t) ((zero-vec? (car vs)) (all-zero-vecs (cdr vs))) (else #f)))
(define (zero-vec? v) (cond ((null? v) #t) ((= (car v) 0) (zero-vec? (cdr v))) (else #f)))

; ================= Hermite Normal Form over Z (H = U A) =================
(define (augment-id A m) (map2 (lambda (row i) (append row i)) A (identity m)))
(define (left-cols M n) (map (lambda (r) (take-n r n)) M))
(define (right-cols M n) (map (lambda (r) (drop-n r n)) M))
(define (take-n lst n) (if (= n 0) '() (cons (car lst) (take-n (cdr lst) (- n 1)))))
(define (drop-n lst n) (if (= n 0) lst (drop-n (cdr lst) (- n 1))))

(define (nonzero-below M col prow nrows)
  (filter (lambda (i) (not (= (mref M i col) 0))) (iota prow (- nrows 1))))
(define (min-abs-row M col rows best)
  (cond ((null? rows) best)
        ((or (< best 0) (< (abs (mref M (car rows) col)) (abs (mref M best col)))) (min-abs-row M col (cdr rows) (car rows)))
        (else (min-abs-row M col (cdr rows) best))))
(define (reduce-against M col pr rows)
  (cond ((null? rows) M)
        ((= (car rows) pr) (reduce-against M col pr (cdr rows)))
        (else (let ((q (quotient (mref M (car rows) col) (mref M pr col))))
                (reduce-against (set-nth M (car rows) (row-axpy (- 0 q) (nth M pr) (nth M (car rows)))) col pr (cdr rows))))))
(define (positive-pivot M r col) (if (negative? (mref M r col)) (set-nth M r (row-scale -1 (nth M r))) M))
(define (hnf-reduce-col M col prow nrows)
  (let ((rows (nonzero-below M col prow nrows)))
    (cond ((null? rows) M)
          ((null? (cdr rows)) (positive-pivot (swap-rows M prow (car rows)) prow col))
          (else (hnf-reduce-col (reduce-against M col (min-abs-row M col rows -1) rows) col prow nrows)))))
(define (hnf-backreduce M col prow i)
  (if (>= i prow) M
    (let ((q (floor (/ (mref M i col) (mref M prow col)))))
      (hnf-backreduce (set-nth M i (row-axpy (- 0 q) (nth M prow) (nth M i))) col prow (+ i 1)))))
(define (hnf-cols M col prow n nrows)
  (if (or (>= col n) (>= prow nrows)) M
    (let ((M1 (hnf-reduce-col M col prow nrows)))
      (if (= (mref M1 prow col) 0) (hnf-cols M1 (+ col 1) prow n nrows)
        (hnf-cols (hnf-backreduce M1 col prow 0) (+ col 1) (+ prow 1) n nrows)))))
(define (mat-hnf A)
  (let ((m (mat-rows A)) (n (mat-cols A)))
    (let ((red (hnf-cols (augment-id A m) 0 0 n m)))
      (list (left-cols red n) (right-cols red n)))))      ; (list H U)
; certificate: U A = H, U integer, det U = +/-1
(define (all-int-mat M) (cond ((null? M) #t) ((all-int-row (car M)) (all-int-mat (cdr M))) (else #f)))
(define (all-int-row r) (cond ((null? r) #t) ((integer? (car r)) (all-int-row (cdr r))) (else #f)))
(define (unimodular? U) (and (all-int-mat U) (let ((d (matrix-det U))) (or (= d 1) (= d -1)))))
(define (hnf-ok? A) (let ((hu (mat-hnf A))) (and (equal? (mat-mul (car (cdr hu)) A) (car hu)) (unimodular? (car (cdr hu))))))

; ================= Smith Normal Form over Z (D = U A V) =================
; triple t = (A U V); row ops act on (A,U), column ops on (A,V).
(define (tA t) (car t))
(define (tU t) (car (cdr t)))
(define (tV t) (car (cdr (cdr t))))
(define (m-colaxpy M j c q) (let ((Mt (transpose M))) (transpose (set-nth Mt j (row-axpy q (nth Mt c) (nth Mt j))))))
(define (m-swapcol M i j) (if (= i j) M (transpose (swap-rows (transpose M) i j))))
(define (m-negrow M i) (set-nth M i (row-scale -1 (nth M i))))
(define (snf-rowop t i r q) (list (set-nth (tA t) i (row-axpy q (nth (tA t) r) (nth (tA t) i)))
                                  (set-nth (tU t) i (row-axpy q (nth (tU t) r) (nth (tU t) i))) (tV t)))
(define (snf-colop t j c q) (list (m-colaxpy (tA t) j c q) (tU t) (m-colaxpy (tV t) j c q)))
(define (snf-swaprow t i j) (list (swap-rows (tA t) i j) (swap-rows (tU t) i j) (tV t)))
(define (snf-swapcol t i j) (list (m-swapcol (tA t) i j) (tU t) (m-swapcol (tV t) i j)))
(define (snf-negrow t i) (list (m-negrow (tA t) i) (m-negrow (tU t) i) (tV t)))

; min |nonzero| entry position in submatrix rows/cols >= k, as (i . j), or #f
(define (mabs-scan A k m n i j best)
  (cond ((>= i m) best)
        ((>= j n) (mabs-scan A k m n (+ i 1) k best))
        ((= (mref A i j) 0) (mabs-scan A k m n i (+ j 1) best))
        ((or (not (pair? best)) (< (abs (mref A i j)) (abs (mref A (car best) (cdr best))))) (mabs-scan A k m n i (+ j 1) (cons i j)))
        (else (mabs-scan A k m n i (+ j 1) best))))
(define (min-abs-pos A k m n) (mabs-scan A k m n k k #f))

(define (snf-clear-row t k n j)         ; clear A[k][j], j>k, via column ops
  (if (>= j n) t
    (let ((q (quotient (mref (tA t) k j) (mref (tA t) k k))))
      (snf-clear-row (if (= q 0) t (snf-colop t j k (- 0 q))) k n (+ j 1)))))
(define (snf-clear-col t k m i)         ; clear A[i][k], i>k, via row ops
  (if (>= i m) t
    (let ((q (quotient (mref (tA t) i k) (mref (tA t) k k))))
      (snf-clear-col (if (= q 0) t (snf-rowop t i k (- 0 q))) k m (+ i 1)))))
(define (rowcol-clear? A k m n) (and (zrow A k (+ k 1) n) (zcol A k (+ k 1) m)))
(define (zrow A k j n) (cond ((>= j n) #t) ((= (mref A k j) 0) (zrow A k (+ j 1) n)) (else #f)))
(define (zcol A k i m) (cond ((>= i m) #t) ((= (mref A i k) 0) (zcol A k (+ i 1) m)) (else #f)))
; first row i>k carrying an entry (col>k) not divisible by pivot, else -1
(define (bad-row A k m n) (br-scan A k m n (+ k 1)))
(define (br-scan A k m n i) (cond ((>= i m) -1) ((row-has-nondiv A k i n (+ k 1)) i) (else (br-scan A k m n (+ i 1)))))
(define (row-has-nondiv A k i n j) (cond ((>= j n) #f) ((not (= (remainder (mref A i j) (mref A k k)) 0)) #t) (else (row-has-nondiv A k i n (+ j 1)))))

(define (snf-pivot t k m n)
  (let ((t1 (snf-clear-col (snf-clear-row t k n (+ k 1)) k m (+ k 1))))
    (cond ((not (rowcol-clear? (tA t1) k m n)) (snf-loop t1 k m n))
          ((>= (bad-row (tA t1) k m n) 0) (snf-loop (snf-rowop t1 k (bad-row (tA t1) k m n) 1) k m n))
          (else (snf-loop (if (negative? (mref (tA t1) k k)) (snf-negrow t1 k) t1) (+ k 1) m n)))))
(define (snf-loop t k m n)
  (if (or (>= k m) (>= k n)) t
    (let ((p (min-abs-pos (tA t) k m n)))
      (if (not (pair? p)) t
        (snf-pivot (snf-swapcol (snf-swaprow t k (car p)) k (cdr p)) k m n)))))
(define (mat-smith A)
  (let ((m (mat-rows A)) (n (mat-cols A)))
    (let ((f (snf-loop (list A (identity m) (identity n)) 0 m n)))
      (list (tA f) (tU f) (tV f)))))      ; (list D U V)

; ---------- certificates ----------
(define (diagonal? M i j m n)
  (cond ((>= i m) #t) ((>= j n) (diagonal? M (+ i 1) 0 m n))
        ((and (not (= i j)) (not (= (mref M i j) 0))) #f) (else (diagonal? M i (+ j 1) m n))))
(define (diag-list M i m n) (if (or (>= i m) (>= i n)) '() (cons (mref M i i) (diag-list M (+ i 1) m n))))
(define (divisibility? ds)
  (cond ((null? ds) #t) ((null? (cdr ds)) #t)
        ((= (car ds) 0) (all-zero-list (cdr ds)))
        ((= (remainder (car (cdr ds)) (car ds)) 0) (divisibility? (cdr ds)))
        (else #f)))
(define (all-zero-list ds) (cond ((null? ds) #t) ((= (car ds) 0) (all-zero-list (cdr ds))) (else #f)))
(define (smith-ok? A)
  (let ((duv (mat-smith A)))
    (let ((D (car duv)) (U (car (cdr duv))) (V (car (cdr (cdr duv)))) (m (mat-rows A)) (n (mat-cols A)))
      (and (equal? (mat-mul (mat-mul U A) V) D) (unimodular? U) (unimodular? V)
           (diagonal? D 0 0 m n) (divisibility? (filter (lambda (x) (not (= x 0))) (diag-list D 0 m n)))))))
(define (smith-invariants A) (diag-list (car (mat-smith A)) 0 (mat-rows A) (mat-cols A)))
