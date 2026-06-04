; -*- lisp -*-
; lib/cas/mixedexp.lisp -- RUNG 5 (the open summit): MIXED TRANSCENDENTAL-OVER-ALGEBRAIC integration, the first
; case INT B * exp(h) dx where the coefficient B is an ALGEBRAIC function (a field element of the algebraic field
; K = Q(x)[y]/(y^2 - p)) and h is a rational function.  This couples the transcendental Risch layer (the exp
; monomial on top) with the algebraic layer below (docs/TRAGER_ROADMAP.md): the exponential sits over an
; algebraic coefficient field -- the genuine mixed-tower situation, FriCAS territory.
;
; The Risch exponential case: INT B exp(h) dx is elementary with the same exponential iff there is a field
; element A in K with  A' + h' A = B  (and then INT B exp(h) dx = A exp(h)).  This is the Risch differential
; equation (RDE) with coefficients in the algebraic field K rather than in Q(x).  Writing A = u + v y and
; B = Bu + Bv y on y^2 = p (so y' = p' y/(2 p)), the RDE decouples by sector:
;     u' + h' u                = Bu        (the rational sector)
;     v' + (p'/(2 p) + h') v   = Bv        (the y sector, carrying the algebraic weight p'/(2 p))
; The answer A exp(h) is always certified by differentiating in the field and checking A' + h' A = B exactly --
; the differentiation certificate is the arbiter, as everywhere in this system.
;
; Public:
;   mx-rhs p hp A           -> the field element B = A' + h' A   (constructive direction; builds integrands)
;   mx-certify p hp A B     -> #t iff A' + h' A = B in K, i.e. INT B exp(h) dx = A exp(h)
;   mx-solve-sqrt hp Bu Bv du dv -> A | 'none : for the canonical field y^2 = x (y = sqrt x) with h' a
;                              constant, solve the RDE by undetermined coefficients (u of degree du, v of degree
;                              dv), returning the field element A; the result is certificate-checked.
;   mx-integrate-sqrt hp B degbound -> (list 'elementary A) | 'no-elementary-exp-part
;
; Verified: INT ((1 + 2x)/(2 sqrt x)) exp(x) dx = sqrt(x) exp(x) on y^2 = x; constructed cases on y^2 = x^2 + 1;
; and the honest 'none when no field element of the search degree solves the RDE.
;
; Builds on algfunc.lisp (the field K and its derivation) and poly.lisp / tower.lisp.

(import "cas/algfunc.lisp")

; ----- constructive direction: B = A' + h' A  (hp = h', a rational function) -----
(define (mx-rhs p hp A) (af-add (af-deriv p A) (af-mul p (af-from-rat hp) A)))

; ----- the certificate: A' + h' A = B in K (so INT B exp(h) dx = A exp(h)) -----
(define (mx-certify p hp A B) (af-equal? (mx-rhs p hp A) B))

; ===== solver for the canonical field y^2 = x (y = sqrt x), h' = c a constant =====
; Unknown A = u(x) + v(x) y, u of degree du, v of degree dv.  We solve A' + c A = B by undetermined coefficients
; on the (du+1)+(dv+1) rational unknowns, using a tiny exact linear solve, then certify.

; p = x for this field
(define (mx-px) (rat-from-poly (list 0 1)))

; build A from a coefficient vector cs (length du+1+dv+1): first du+1 are u's coeffs (low->high), rest are v's
(define (mx-A du dv cs) (af-make (rat-from-poly (mx-take cs (+ du 1))) (rat-from-poly (mx-take (mx-drop cs (+ du 1)) (+ dv 1)))))
(define (mx-take l n) (if (= n 0) (quote ()) (if (null? l) (cons 0 (mx-take (quote ()) (- n 1))) (cons (car l) (mx-take (cdr l) (- n 1))))))
(define (mx-drop l n) (if (= n 0) l (if (null? l) (quote ()) (mx-drop (cdr l) (- n 1)))))

; residual vector of A'+cA-B as concatenated numerator-coefficient lists of the two sectors (each cleared and
; padded to a fixed width so the vectors line up across perturbations)
(define (mx-resid hp Bu Bv du dv cs)
  (let ((A (mx-A du dv cs)))
    (let ((R (af-add (mx-rhs (mx-px) hp A) (af-make (rat-neg Bu) (rat-neg Bv)))))
      (append (mx-cl (af-u R)) (mx-cl (af-v R))))))
(define (mx-cl r) (mx-pad (poly-norm (rat-num r)) 10))
(define (mx-pad l n) (if (= n 0) (quote ()) (cons (if (null? l) 0 (car l)) (mx-pad (if (null? l) (quote ()) (cdr l)) (- n 1)))))

; the linear system: residual is affine in cs.  b = -resid(0); column j = resid(e_j) - resid(0).
(define (mx-zeros m) (if (= m 0) (quote ()) (cons 0 (mx-zeros (- m 1)))))
(define (mx-unit m j) (mx-unit-go m j 0))
(define (mx-unit-go m j i) (if (= i m) (quote ()) (cons (if (= i j) 1 0) (mx-unit-go m j (+ i 1)))))
(define (mx-vsub a b) (if (null? a) (quote ()) (cons (- (car a) (car b)) (mx-vsub (cdr a) (cdr b)))))
(define (mx-vneg a) (if (null? a) (quote ()) (cons (- 0 (car a)) (mx-vneg (cdr a)))))

(define (mx-solve-sqrt hp Bu Bv du dv)
  (let ((m (+ (+ du 1) (+ dv 1))))
    (let ((r0 (mx-resid hp Bu Bv du dv (mx-zeros m))))
      (let ((cols (mx-cols hp Bu Bv du dv m 0 (quote ()))))
        (let ((sol (mx-lin-solve cols (mx-vneg r0) (length r0) m)))
          (if (equal? sol (quote none)) (quote none)
              (let ((A (mx-A du dv sol)))
                (if (mx-certify (mx-px) hp A (af-make Bu Bv)) A (quote none))))))))) ; certify recovered A
(define (mx-cols hp Bu Bv du dv m j acc)
  (if (= j m) (reverse acc)
      (mx-cols hp Bu Bv du dv m (+ j 1) (cons (mx-vsub (mx-resid hp Bu Bv du dv (mx-unit m j)) (mx-resid hp Bu Bv du dv (mx-zeros m))) acc))))

; ----- exact linear solver M c = b (M given as columns), rows >= m, unique solution expected; else 'none -----
; transpose to rows, drop all-zero rows, then reduce: repeatedly pick a row with a nonzero leading pivot in an
; unused column, scale it to 1, eliminate that column from ALL other working rows, and set it aside.  Finally
; read each variable from its pivot row.  Processing sets pivot rows aside (no value-equality test).
(define (mx-lin-solve cols b rows m)
  (let ((rowsM (mx-aug (mx-rows-from-cols cols rows m) b)))
    (let ((work (mx-drop-zero-rows rowsM m)))
      (mx-reduce work m 0 (quote ())))))
(define (mx-rows-from-cols cols rows m) (mx-rfc cols rows 0))
(define (mx-rfc cols rows i) (if (= i rows) (quote ()) (cons (mx-rowi cols i) (mx-rfc cols rows (+ i 1)))))
(define (mx-rowi cols i) (if (null? cols) (quote ()) (cons (mx-vnth (car cols) i) (mx-rowi (cdr cols) i))))
(define (mx-vnth v i) (if (= i 0) (car v) (mx-vnth (cdr v) (- i 1))))
(define (mx-aug rows b) (if (null? rows) (quote ()) (cons (append (car rows) (list (car b))) (mx-aug (cdr rows) (cdr b)))))
; drop rows whose first m entries are all zero (the augmented column ignored); if such a row has nonzero rhs -> inconsistent
(define (mx-drop-zero-rows rows m) (if (null? rows) (quote ()) (if (mx-row-zero? (car rows) m 0) (mx-drop-zero-rows (cdr rows) m) (cons (car rows) (mx-drop-zero-rows (cdr rows) m)))))
(define (mx-row-zero? row m i) (cond ((= i m) #t) ((= (mx-vnth row i) 0) (mx-row-zero? row m (+ i 1))) (else #f)))

; reduce: pivots accumulated in 'piv' as (col . row). work = remaining rows.  Full Gauss-Jordan: when a new
; pivot for column c is chosen, eliminate column c from BOTH the working rows and the already-collected pivot
; rows, so every pivot row ends unit in its own column and zero in all other pivot columns.
(define (mx-reduce work m c piv)
  (if (= c m) (mx-read piv m 0 (quote ()))
      (let ((pr (mx-first-with-col work c)))
        (if (equal? pr (quote none)) (mx-reduce work m (+ c 1) piv)   ; free column (no pivot) -> value 0
            (let ((prn (mx-scale-row pr (/ 1 (mx-vnth pr c)))))
              (mx-reduce (mx-elim-others (mx-remove-row work pr) prn c) m (+ c 1)
                         (cons (cons c prn) (mx-elim-piv piv prn c))))))))
; eliminate column c from each accumulated pivot row (keeping their (col . row) tags)
(define (mx-elim-piv piv prn c) (if (null? piv) (quote ()) (cons (cons (car (car piv)) (mx-axpy (cdr (car piv)) prn (- 0 (mx-vnth (cdr (car piv)) c)))) (mx-elim-piv (cdr piv) prn c))))
(define (mx-first-with-col work c) (cond ((null? work) (quote none)) ((not (= (mx-vnth (car work) c) 0)) (car work)) (else (mx-first-with-col (cdr work) c))))
(define (mx-remove-row work pr) (mx-rr-go work pr #f))
(define (mx-rr-go work pr removed) (cond ((null? work) (quote ())) ((if (not removed) (mx-eq-row? (car work) pr) #f) (mx-rr-go (cdr work) pr #t)) (else (cons (car work) (mx-rr-go (cdr work) pr removed)))))
(define (mx-eq-row? a b) (if (null? a) (if (null? b) #t #f) (if (= (car a) (car b)) (mx-eq-row? (cdr a) (cdr b)) #f)))
(define (mx-scale-row row s) (if (null? row) (quote ()) (cons (* s (car row)) (mx-scale-row (cdr row) s))))
; subtract (row_c) * prn from every working row to zero its column c
(define (mx-elim-others work prn c) (if (null? work) (quote ()) (cons (mx-axpy (car work) prn (- 0 (mx-vnth (car work) c))) (mx-elim-others (cdr work) prn c))))
(define (mx-axpy row prn f) (if (null? row) (quote ()) (cons (+ (car row) (* f (car prn))) (mx-axpy (cdr row) (cdr prn) f))))
; also eliminate column c from previously-collected pivot rows so they stay reduced (full Gauss-Jordan)
; read solution: for each variable j (0..m-1), if there is a pivot row for column j, its value is that row's
; rhs minus contributions from other columns (but after full elimination pivot rows are unit, so rhs is the value)
(define (mx-read piv m j acc)
  (if (= j m) (reverse acc)
      (let ((pr (mx-piv-for piv j)))
        (mx-read piv m (+ j 1) (cons (if (equal? pr (quote none)) 0 (mx-solve-var pr piv m)) acc)))))
(define (mx-piv-for piv j) (cond ((null? piv) (quote none)) ((= (car (car piv)) j) (cdr (car piv))) (else (mx-piv-for (cdr piv) j))))
; value of variable j from its pivot row: rhs - sum_{k != j, k pivot or free} coeff_k * value_k.  Since other
; pivot columns were eliminated from this row during reduction they are zero here; free columns are 0-valued.
; So the value is simply the rhs entry.
(define (mx-solve-var pr piv m) (mx-vnth pr m))

; ----- top-level for the sqrt field: search bounded degrees for a solving A -----
(define (mx-integrate-sqrt hp B degbound)
  (let ((A (mx-iter hp (af-u B) (af-v B) 0 degbound)))
    (if (equal? A (quote none)) (quote no-elementary-exp-part) (list (quote elementary) A))))
(define (mx-iter hp Bu Bv d degbound)
  (if (> d degbound) (quote none)
      (let ((A (mx-solve-sqrt hp Bu Bv d d)))
        (if (equal? A (quote none)) (mx-iter hp Bu Bv (+ d 1) degbound) A))))
