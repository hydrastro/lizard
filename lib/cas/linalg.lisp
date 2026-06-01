; -*- lisp -*-
; lib/cas/linalg.lisp — exact symbolic linear algebra over Q.
;
; Matrices are lists of rows; rows are lists of exact rationals (bignums included).
; This ties the whole CAS together: determinants reuse resultant.lisp, linear
; solving reuses the exact Gauss-Jordan solver from gosper.lisp, and EIGENVALUES
; are the exact roots of the characteristic polynomial via solve.lisp, returned
; as rationals, surds, or RootOf -- so e.g. the Fibonacci matrix yields the golden
; ratio (1+sqrt 5)/2 and a rotation yields +/- i, exactly.
;
; Characteristic polynomial via Faddeev-LeVerrier (only Q matrix arithmetic, no
; polynomial-entry determinant), which also hands us det and a built-in
; Cayley-Hamilton identity.  Certificates: p(A)=0 (Cayley-Hamilton), A A^{-1}=I,
; det cross-checked against the Sylvester-style matrix-det, and each eigenvalue
; verified by solve.lisp plus (for rationals) det(A - lambda I) = 0.
;
; Top-level helpers only; builds on resultant.lisp, gosper.lisp, solve.lisp.

(import "cas/resultant.lisp")
(import "cas/gosper.lisp")
(import "cas/solve.lisp")

; ---------- basic matrix algebra ----------
(define (mat-rows A) (length A))
(define (mat-cols A) (if (null? A) 0 (length (car A))))
(define (dotv u v) (if (null? u) 0 (+ (* (car u) (car v)) (dotv (cdr u) (cdr v)))))
(define (transpose M) (if (null? (car M)) '() (cons (map car M) (transpose (map cdr M)))))
(define (mat-mul A B) (let ((Bt (transpose B))) (map (lambda (row) (map (lambda (col) (dotv row col)) Bt)) A)))
(define (map2 f a b) (if (null? a) '() (cons (f (car a) (car b)) (map2 f (cdr a) (cdr b)))))
(define (mat-add A B) (map2 (lambda (r s) (map2 + r s)) A B))
(define (mat-scale c M) (map (lambda (row) (map (lambda (x) (* c x)) row)) M))
(define (mat-sub A B) (mat-add A (mat-scale -1 B)))
(define (unit-vec n i) (map (lambda (j) (if (= j i) 1 0)) (iota 0 (- n 1))))
(define (identity n) (map (lambda (i) (unit-vec n i)) (iota 0 (- n 1))))
(define (zero-mat n) (map (lambda (i) (map (lambda (j) 0) (iota 0 (- n 1)))) (iota 0 (- n 1))))
(define (trace M) (tr-diag M 0))
(define (tr-diag M i) (if (null? M) 0 (+ (nth (car M) i) (tr-diag (cdr M) (+ i 1)))))
(define (mat-equal? A B) (equal? A B))
(define (augment-col A b) (map2 (lambda (row bi) (append row (list bi))) A b))

; ---------- characteristic polynomial (Faddeev-LeVerrier) ----------
; returns charpoly coefficients low-to-high: (c_n c_{n-1} ... c_1 1),
; where det(lambda I - A) = lambda^n + c_1 lambda^{n-1} + ... + c_n.
(define (fl-cs A M cs k n)
  (if (> k n) cs
    (let ((Mk (mat-mul A (mat-add M (mat-scale (car cs) (identity n))))))
      (fl-cs A Mk (cons (/ (- 0 (trace Mk)) k) cs) (+ k 1) n))))
(define (mat-charpoly A)
  (let ((n (mat-rows A)))
    (append (fl-cs A A (list (- 0 (trace A))) 2 n) (list 1))))

; ---------- determinant ----------
; det A = (-1)^n * constant term of charpoly; cross-checked against matrix-det.
(define (mat-det A) (matrix-det A))
(define (mat-det-via-charpoly A) (* (expt -1 (mat-rows A)) (poly-coeff (mat-charpoly A) 0)))

; ---------- matrix polynomial evaluation (Horner) ----------
(define (mat-poly-eval p A) (mpe (reverse (poly-norm p)) A (mat-rows A) (zero-mat (mat-rows A))))
(define (mpe rc A n acc) (if (null? rc) acc (mpe (cdr rc) A n (mat-add (mat-mul acc A) (mat-scale (car rc) (identity n))))))

; ---------- inverse (solve A X = I column by column) ----------
(define (any-none xs) (cond ((null? xs) #f) ((equal? (car xs) 'none) #t) (else (any-none (cdr xs)))))
(define (mat-inverse A)
  (let ((n (mat-rows A)))
    (let ((cols (map (lambda (i) (lin-solve (augment-col A (unit-vec n i)) n)) (iota 0 (- n 1)))))
      (if (any-none cols) 'singular (transpose cols)))))

; ---------- linear system A x = b ----------
(define (mat-solve A b) (lin-solve (augment-col A b) (mat-cols A)))   ; -> x | 'none
(define (mat-apply A x) (map (lambda (row) (dotv row x)) A))

; ---------- eigenvalues (exact, via the equation solver) ----------
(define (mat-eigenvalues A) (solve-poly (mat-charpoly A)))            ; list of (descriptor mult)

; ---------- certificates ----------
(define (cayley-hamilton? A) (equal? (mat-poly-eval (mat-charpoly A) A) (zero-mat (mat-rows A))))
(define (inverse-ok? A Ainv) (and (not (equal? Ainv 'singular)) (equal? (mat-mul A Ainv) (identity (mat-rows A)))))
(define (det-consistent? A) (= (mat-det A) (mat-det-via-charpoly A)))
(define (solve-ok? A b x) (and (not (equal? x 'none)) (equal? (mat-apply A x) b)))
; rational eigenvalue check: det(A - lambda I) = 0
(define (rational-eig-ok? A lam) (= (matrix-det (mat-sub A (mat-scale lam (identity (mat-rows A))))) 0))

; ---------- display ----------
(define (mat-charpoly->string A) (poly->string (mat-charpoly A) "x"))
(define (mat-eigenvalues->string A) (solutions->string (mat-eigenvalues A)))
