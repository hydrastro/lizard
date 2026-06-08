; -*- lisp -*-
; lib/cas/groebnerf4.lisp -- the LINEAR-ALGEBRA reduction at the heart of the F4 algorithm: simultaneous reduction
; of a set of polynomials by Gaussian elimination on a Macaulay matrix, instead of one-at-a-time polynomial
; division (docs/CAS.md -- summit S4, an F4-class engine direction atop the Buchberger engines).
;
; F4 replaces the sequential polynomial reductions of Buchberger with a single linear-algebra step: the
; polynomials to be reduced are laid out as the rows of a matrix whose columns are indexed by every monomial that
; occurs, sorted in descending monomial order, and the matrix is row-reduced.  The reduced nonzero rows, read back
; as polynomials, are the reductions; their leading monomials (the pivot columns) are the leading terms of the
; reduced set.  This module builds that Macaulay matrix from a set of multivariate polynomials (in the groebner.lisp
; term representation, term = (coeff . exponent-vector), descending lex), row-reduces it to reduced row-echelon
; form over Q, and reads the polynomials back.  The row space is preserved exactly, so the reduced set spans the
; same Q-vector space of polynomials as the input -- a fact CERTIFIED here by checking that every original
; polynomial is a Q-linear combination of the reduced rows (each original reduces to zero against the echelon
; rows).  This is the exact linear-algebra core; turning it into a full Groebner engine (generating the S-pairs and
; the monomial multiples that make the row space closed under the ideal, the symbolic-preprocessing loop) is the
; remaining work, but the reduction step itself -- the part that makes F4 fast -- is here and is exact.
;
; Public (polys in groebner.lisp's representation; a "matrix" is a list of rational row-lists):
;   f4-monomials polys         -> all distinct monomials (exponent vectors) in the set, sorted descending
;   f4-matrix polys            -> (cols . rows): the column monomial list and the coefficient matrix
;   f4-rref M                  -> the reduced row-echelon form of a rational matrix M
;   f4-reduce polys            -> the reduced polynomials read back from the RREF of the Macaulay matrix
;   f4-spans-same? polys       -> #t iff every input polynomial is in the row space of the reduced set (certificate)
;   f4-leading-monomials polys -> the leading (pivot) monomials of the reduced set
;
; Verified: <x+y, x-y> reduces to the rows x and y (the row space <x, y>); a set with a redundant row (x+y, 2x+2y)
; reduces to the single row x+y; <x^2+y^2, x^2-y^2> reduces to x^2 and y^2; the span-preservation certificate holds
; in each case.
;
; Builds on groebner.lisp.

(import "cas/groebner.lisp")

(define (f4-len l) (if (null? l) 0 (+ 1 (f4-len (cdr l)))))
(define (f4-app a b) (if (null? a) b (cons (car a) (f4-app (cdr a) b))))
(define (f4-map fn l) (if (null? l) (quote ()) (cons (fn (car l)) (f4-map fn (cdr l)))))
(define (f4-nth l k) (if (= k 0) (car l) (f4-nth (cdr l) (- k 1))))

; ----- collect the distinct monomials, sorted descending in lex -----
(define (f4-monomials polys) (f4-sort-desc (f4-dedupe (f4-all-monos polys))))
(define (f4-all-monos polys) (if (null? polys) (quote ()) (f4-app (f4-map (lambda (t) (cdr t)) (car polys)) (f4-all-monos (cdr polys)))))
(define (f4-dedupe l) (f4-dd l (quote ())))
(define (f4-dd l seen) (cond ((null? l) (f4-rev seen)) ((f4-member? (car l) seen) (f4-dd (cdr l) seen)) (else (f4-dd (cdr l) (cons (car l) seen)))))
(define (f4-member? x l) (cond ((null? l) #f) ((equal? x (car l)) #t) (else (f4-member? x (cdr l)))))
(define (f4-rev l) (f4-rev-go l (quote ())))
(define (f4-rev-go l a) (if (null? l) a (f4-rev-go (cdr l) (cons (car l) a))))
; insertion sort by descending lex on exponent vectors (reuse groebner's lex comparison via mono-cmp surrogate)
(define (f4-sort-desc l) (if (null? l) (quote ()) (f4-insert (car l) (f4-sort-desc (cdr l)))))
(define (f4-insert x s) (cond ((null? s) (list x)) ((f4-gt? x (car s)) (cons x s)) (else (cons (car s) (f4-insert x (cdr s))))))
; lex greater: first differing coordinate is larger
(define (f4-gt? a b) (cond ((null? a) #f) ((> (car a) (car b)) #t) ((< (car a) (car b)) #f) (else (f4-gt? (cdr a) (cdr b)))))

; ----- coefficient of a monomial m in a polynomial p -----
(define (f4-coeff p m) (cond ((null? p) 0) ((equal? (cdr (car p)) m) (car (car p))) (else (f4-coeff (cdr p) m))))

; ----- build the Macaulay matrix: (cols . rows) -----
(define (f4-matrix polys) (cons (f4-monomials polys) (f4-rows polys (f4-monomials polys))))
(define (f4-rows polys cols) (f4-map (lambda (p) (f4-map (lambda (m) (f4-coeff p m)) cols)) polys))

; ----- reduced row-echelon form over Q -----
(define (f4-rref M) (f4-rref-go M 0 0))
(define (f4-rref-go M row col)
  (if (if (>= row (f4-len M)) #t (>= col (f4-ncols M))) M
      (f4-rref-dispatch M row col (f4-pivot-row M row col))))
(define (f4-ncols M) (if (null? M) 0 (f4-len (car M))))
; find a row >= `row` with nonzero entry in `col`
(define (f4-pivot-row M row col) (f4-find-piv M row col row))
(define (f4-find-piv M r col cur) (cond ((>= cur (f4-len M)) -1) ((not (= (f4-nth (f4-nth M cur) col) 0)) cur) (else (f4-find-piv M r col (+ cur 1)))))
(define (f4-rref-dispatch M row col piv)
  (if (= piv -1) (f4-rref-go M row (+ col 1))
      (f4-rref-go (f4-eliminate (f4-swap-norm M row piv col) row col) (+ row 1) (+ col 1))))
; swap pivot row into position `row` and normalize it so the pivot is 1
(define (f4-swap-norm M row piv col) (f4-set-row (f4-swap M row piv) row (f4-scale-row (f4-nth (f4-swap M row piv) row) (/ 1 (f4-nth (f4-nth (f4-swap M row piv) row) col)))))
(define (f4-swap M i j) (f4-map (lambda (k) (cond ((= k i) (f4-nth M j)) ((= k j) (f4-nth M i)) (else (f4-nth M k)))) (f4-range 0 (f4-len M))))
(define (f4-range a b) (if (>= a b) (quote ()) (cons a (f4-range (+ a 1) b))))
(define (f4-scale-row r s) (f4-map (lambda (x) (* x s)) r))
(define (f4-set-row M row newr) (f4-map (lambda (k) (if (= k row) newr (f4-nth M k))) (f4-range 0 (f4-len M))))
; eliminate `col` from every other row using the (normalized) pivot row
(define (f4-eliminate M row col) (f4-map (lambda (k) (if (= k row) (f4-nth M k) (f4-axpy (f4-nth M k) (f4-nth M row) (f4-nth (f4-nth M k) col)))) (f4-range 0 (f4-len M))))
(define (f4-axpy r piv factor) (f4-zipsub r (f4-scale-row piv factor)))
(define (f4-zipsub a b) (if (null? a) (quote ()) (cons (- (car a) (car b)) (f4-zipsub (cdr a) (cdr b)))))

; ----- read polynomials back from the reduced matrix -----
(define (f4-reduce polys) (f4-rows-to-polys (f4-rref (cdr (f4-matrix polys))) (car (f4-matrix polys))))
(define (f4-rows-to-polys rows cols) (f4-filter-nonempty (f4-map (lambda (r) (f4-row-to-poly r cols)) rows)))
(define (f4-row-to-poly r cols) (f4-rtp r cols))
(define (f4-rtp r cols) (cond ((null? r) (quote ())) ((= (car r) 0) (f4-rtp (cdr r) (cdr cols))) (else (cons (cons (car r) (car cols)) (f4-rtp (cdr r) (cdr cols))))))
(define (f4-filter-nonempty l) (cond ((null? l) (quote ())) ((null? (car l)) (f4-filter-nonempty (cdr l))) (else (cons (car l) (f4-filter-nonempty (cdr l))))))

; ----- leading (pivot) monomials of the reduced set -----
(define (f4-leading-monomials polys) (f4-map (lambda (p) (cdr (car p))) (f4-reduce polys)))

; ----- certificate: every input polynomial reduces to zero against the reduced echelon rows -----
(define (f4-spans-same? polys) (f4-all-reduce-zero polys (f4-reduce polys)))
(define (f4-all-reduce-zero polys basis) (cond ((null? polys) #t) ((f4-reduces-zero? (car polys) basis) (f4-all-reduce-zero (cdr polys) basis)) (else #f)))
; reduce p by subtracting basis rows that match its leading monomial, until empty or stuck
(define (f4-reduces-zero? p basis) (f4-rz (f4-poly-sort p) basis))
(define (f4-rz p basis) (cond ((null? p) #t) (else (f4-rz-step p basis (f4-find-basis basis (cdr (car p)))))))
(define (f4-rz-step p basis b) (if (equal? b (quote none)) #f (f4-rz (f4-poly-sub p (f4-poly-scale (car (car p)) b)) basis)))
(define (f4-find-basis basis m) (cond ((null? basis) (quote none)) ((equal? (cdr (car (car basis))) m) (car basis)) (else (f4-find-basis (cdr basis) m))))
; polynomial helpers in the descending-lex term representation
(define (f4-poly-sort p) p)                            ; rows come from RREF already pivot-leading; inputs sorted by groebner
(define (f4-poly-scale c p) (f4-map (lambda (t) (cons (* c (car t)) (cdr t))) p))
(define (f4-poly-sub a b) (f4-ps-clean (f4-ps a b)))
(define (f4-ps a b)                                     ; a - b, both descending-lex term lists
  (cond ((null? a) (f4-poly-scale -1 b))
        ((null? b) a)
        ((equal? (cdr (car a)) (cdr (car b))) (cons (cons (- (car (car a)) (car (car b))) (cdr (car a))) (f4-ps (cdr a) (cdr b))))
        ((f4-gt? (cdr (car a)) (cdr (car b))) (cons (car a) (f4-ps (cdr a) b)))
        (else (cons (cons (- 0 (car (car b))) (cdr (car b))) (f4-ps a (cdr b))))))
(define (f4-ps-clean p) (cond ((null? p) (quote ())) ((= (car (car p)) 0) (f4-ps-clean (cdr p))) (else (cons (car p) (f4-ps-clean (cdr p))))))
