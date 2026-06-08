; -*- lisp -*-
; lib/cas/linalgfp.lisp -- linear algebra over a prime field F_p.
;
; Matrices are lists of rows.  Over F_p every nonzero element is invertible, so Gaussian
; elimination gives exact determinants, ranks, inverses, solutions, and nullspace bases with
; no rounding.  The determinant is read from a triangularization (tracking the sign of row
; swaps and the product of pivots); rank, inverse, solving, and nullspace come from reduced
; row echelon form, the inverse and solver running on an augmented matrix.
;
; The implementation is checked by structural identities rather than trust: A * A^{-1} = I
; for invertible A, the product rule det(A B) = det(A) det(B), the rank-nullity theorem
; rank + nullity = n, and A x = b for every solution returned.  Function names carry an fm-
; prefix to avoid colliding with the rational-arithmetic linalg module.  Builds on
; numbertheory.lisp.

(import "cas/numbertheory.lisp")

(define (nthc l i) (if (= i 0) (car l) (nthc (cdr l) (- i 1))))
(define (set-nth l i v) (if (= i 0) (cons v (cdr l)) (cons (car l) (set-nth (cdr l) (- i 1) v))))
(define (map2 f a b) (if (or (null? a) (null? b)) '() (cons (f (car a) (car b)) (map2 f (cdr a) (cdr b)))))
(define (fm-rows M) (length M))
(define (fm-cols M) (length (car M)))
(define (fm-get M i j) (nthc (nthc M i) j))
(define (fm-zeros k) (if (= k 0) '() (cons 0 (fm-zeros (- k 1)))))
(define (fm-id n p) (fm-id-go n 0))
(define (fm-id-go n i) (if (>= i n) '() (cons (set-nth (fm-zeros n) i 1) (fm-id-go n (+ i 1)))))

; ---------- multiplication ----------
(define (fm-transpose M) (if (null? (car M)) '() (cons (map car M) (fm-transpose (map cdr M)))))
(define (dotp a b p) (dp a b p 0))
(define (dp a b p acc) (if (null? a) (imod acc p) (dp (cdr a) (cdr b) p (+ acc (* (car a) (car b))))))
(define (fm-mul A B p) (let ((BT (fm-transpose B))) (map (lambda (row) (map (lambda (col) (dotp row col p)) BT)) A)))

; ---------- row operations ----------
(define (row-scale r c p) (map (lambda (x) (imod (* x c) p)) r))
(define (row-axpy dst src f p) (map2 (lambda (x y) (imod (- x (* f y)) p)) dst src))   ; dst - f*src
(define (get-row M i) (nthc M i))
(define (set-row M i v) (set-nth M i v))
(define (swap-rows M i j) (set-row (set-row M i (get-row M j)) j (get-row M i)))
(define (find-nz M r c p n) (cond ((>= r n) -1) ((= (imod (fm-get M r c) p) 0) (find-nz M (+ r 1) c p n)) (else r)))
(define (elim-all M r c p) (ea M r c p 0 (length M)))
(define (ea M r c p k n) (cond ((>= k n) M) ((= k r) (ea M r c p (+ k 1) n)) (else (ea (set-row M k (row-axpy (get-row M k) (get-row M r) (fm-get M k c) p)) r c p (+ k 1) n))))
(define (rref M r c ncols p)
  (if (or (>= c ncols) (>= r (length M))) M
      (let ((piv (find-nz M r c p (length M))))
        (if (< piv 0) (rref M r (+ c 1) ncols p)
            (let ((M1 (swap-rows M r piv)))
              (let ((M2 (set-row M1 r (row-scale (get-row M1 r) (mod-inverse (fm-get M1 r c) p) p))))
                (rref (elim-all M2 r c p) (+ r 1) (+ c 1) ncols p)))))))

; ---------- determinant (triangularize, no normalization) ----------
(define (elim-below M r c p) (eb M r c p (+ r 1) (length M)))
(define (eb M r c p k n) (if (>= k n) M (eb (set-row M k (row-axpy (get-row M k) (get-row M r) (imod (* (fm-get M k c) (mod-inverse (fm-get M r c) p)) p) p)) r c p (+ k 1) n)))
(define (det-go M i n sign p)
  (if (>= i n) (imod (* sign (diag-prod M 0 n p)) p)
      (let ((piv (find-nz M i i p n)))
        (if (< piv 0) 0
            (if (= piv i) (det-go (elim-below M i i p) (+ i 1) n sign p)
                (det-go (elim-below (swap-rows M i piv) i i p) (+ i 1) n (imod (- 0 sign) p) p))))))
(define (diag-prod M i n p) (if (>= i n) 1 (imod (* (fm-get M i i) (diag-prod M (+ i 1) n p)) p)))
(define (fm-det M p) (det-go M 0 (fm-rows M) 1 p))

; ---------- rank ----------
(define (nonzero-row? r p) (cond ((null? r) #f) ((= (imod (car r) p) 0) (nonzero-row? (cdr r) p)) (else #t)))
(define (count-nz rows p) (cond ((null? rows) 0) ((nonzero-row? (car rows) p) (+ 1 (count-nz (cdr rows) p))) (else (count-nz (cdr rows) p))))
(define (fm-rank M p) (count-nz (rref M 0 0 (fm-cols M) p) p))

; ---------- inverse via [A | I] ----------
(define (augment A B) (map2 (lambda (ra rb) (append ra rb)) A B))
(define (take-n l n) (if (or (= n 0) (null? l)) '() (cons (car l) (take-n (cdr l) (- n 1)))))
(define (drop-n l n) (if (or (= n 0) (null? l)) l (drop-n (cdr l) (- n 1))))
(define (left-half M n) (map (lambda (r) (take-n r n)) M))
(define (right-half M n) (map (lambda (r) (drop-n r n)) M))
(define (fm-inverse M p)
  (let ((n (fm-rows M)))
    (let ((R (rref (augment M (fm-id n p)) 0 0 (* 2 n) p)))
      (if (equal? (left-half R n) (fm-id n p)) (right-half R n) 'singular))))

; ---------- linear solve A x = b ----------
(define (leading-col row ncols) (lc row 0 ncols))
(define (lc row j ncols) (cond ((>= j ncols) -1) ((= (car row) 0) (lc (cdr row) (+ j 1) ncols)) (else j)))
(define (build-sol rows ncols sol p)
  (cond ((null? rows) sol) ((equal? sol 'none) 'none)
        (else (let ((lcv (leading-col (car rows) ncols)))
                (if (< lcv 0) (if (= (imod (nthc (car rows) ncols) p) 0) (build-sol (cdr rows) ncols sol p) 'none)
                    (build-sol (cdr rows) ncols (set-nth sol lcv (imod (nthc (car rows) ncols) p)) p))))))
(define (fm-solve A b p)
  (let ((aug (map2 (lambda (r v) (append r (list v))) A b)) (n (fm-cols A)))
    (build-sol (rref aug 0 0 n p) n (fm-zeros n) p)))

; ---------- nullspace basis ----------
(define (pivot-cols rows ncols) (pc rows ncols 0 '()))
(define (pc rows ncols ri acc) (if (null? rows) (reverse acc) (let ((l (leading-col (car rows) ncols))) (if (< l 0) (pc (cdr rows) ncols (+ ri 1) acc) (pc (cdr rows) ncols (+ ri 1) (cons l acc))))))
(define (member? x l) (cond ((null? l) #f) ((= x (car l)) #t) (else (member? x (cdr l)))))
(define (rref-row-for-col rows col ncols) (cond ((null? rows) '()) ((= (leading-col (car rows) ncols) col) (car rows)) (else (rref-row-for-col (cdr rows) col ncols))))
(define (null-vec rows pivs free ncols p) (nv rows pivs free ncols p 0))
(define (nv rows pivs free ncols p j)
  (if (>= j ncols) '()
      (cons (cond ((= j free) 1) ((member? j pivs) (imod (- 0 (nthc (rref-row-for-col rows j ncols) free)) p)) (else 0))
            (nv rows pivs free ncols p (+ j 1)))))
(define (fm-nullspace M p)
  (let ((R (rref M 0 0 (fm-cols M) p)) (ncols (fm-cols M)))
    (let ((pivs (pivot-cols R ncols)))
      (map (lambda (free) (null-vec R pivs free ncols p)) (free-cols pivs ncols 0 '())))))
(define (free-cols pivs ncols j acc) (if (>= j ncols) (reverse acc) (free-cols pivs ncols (+ j 1) (if (member? j pivs) acc (cons j acc)))))

; ---------- certificates ----------
(define (fm-inverse-ok? M p) (let ((Mi (fm-inverse M p))) (and (not (equal? Mi 'singular)) (equal? (fm-mul M Mi p) (fm-id (fm-rows M) p)))))
(define (fm-detmul-ok? A B p) (= (imod (* (fm-det A p) (fm-det B p)) p) (fm-det (fm-mul A B p) p)))
(define (fm-ranknull-ok? M p) (= (+ (fm-rank M p) (length (fm-nullspace M p))) (fm-cols M)))
(define (fm-solve-ok? A b p) (let ((x (fm-solve A b p))) (and (not (equal? x 'none)) (equal? (fm-mul A (map list x) p) (map list b)))))
(define (fm-null-ok? M p) (all-zero (map (lambda (v) (fm-mul M (map list v) p)) (fm-nullspace M p)) p))
(define (all-zero mats p) (cond ((null? mats) #t) ((zero-mat? (car mats) p) (all-zero (cdr mats) p)) (else #f)))
(define (zero-mat? M p) (cond ((null? M) #t) ((= (imod (car (car M)) p) 0) (zero-mat? (cdr M) p)) (else #f)))
