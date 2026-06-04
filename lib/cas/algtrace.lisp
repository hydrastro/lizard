; -*- lisp -*-
; lib/cas/algtrace.lisp -- field trace and norm Tr,N : Q(alpha) -> Q, for the algebraic-residue RootSum.
;
; When the Rothstein-Trager residues are algebraic numbers alpha (roots of an irreducible factor R(z) of the
; resultant), the logarithmic part is sum_{R(alpha)=0} alpha log(v_alpha) and its derivative is the FIELD TRACE
; Tr(alpha (D v_alpha)/v_alpha), which lands back in Q(x).  To certify such an answer by differentiation we need
; the trace (and norm) of elements of Q(alpha) = Q[t]/(R), for R of ARBITRARY degree -- generalizing the
; explicit conjugate of the quadratic case in algres.lisp.
;
; Tr and N are computed from the REGULAR REPRESENTATION: multiplication by beta is a Q-linear map on the power
; basis 1, alpha, ..., alpha^{n-1}; its matrix M_beta has Tr(beta) = trace(M_beta) and N(beta) = det(M_beta).
; These are exact rational numbers.  Builds on algnum.lisp (which provides Q(alpha) arithmetic).

(import "cas/algnum.lisp")

; column j of the multiplication-by-beta matrix = coefficient vector of beta * alpha^j, reduced mod R.
(define (atr-col beta j)                                   ; -> length-n coefficient list (low->high), padded
  (atr-pad (alg-rep (alg-mul beta (alg-make (alg-min beta) (atr-monomial j)))) (atr-degR beta)))
(define (atr-monomial j) (if (= j 0) (list 1) (cons 0 (atr-monomial (- j 1)))))
(define (atr-degR a) (- (length (alg-min a)) 1))           ; n = deg of the minimal polynomial
(define (atr-pad v n) (if (= n 0) (quote ()) (cons (if (null? v) 0 (car v)) (atr-pad (if (null? v) (quote ()) (cdr v)) (- n 1)))))

; trace = sum of diagonal entries M[j][j] = (coeff of alpha^j in beta*alpha^j)
(define (atr-trace beta) (atr-trace-go beta 0 (atr-degR beta) 0))
(define (atr-trace-go beta j n acc)
  (if (= j n) acc (atr-trace-go beta (+ j 1) n (+ acc (atr-nth (atr-col beta j) j)))))
(define (atr-nth v i) (if (= i 0) (car v) (atr-nth (cdr v) (- i 1))))

; norm = determinant of the matrix whose columns are atr-col beta 0 .. n-1
(define (atr-norm beta)
  (atr-det (atr-matrix beta) (atr-degR beta)))
(define (atr-matrix beta) (atr-cols beta 0 (atr-degR beta)))
(define (atr-cols beta j n) (if (= j n) (quote ()) (cons (atr-col beta j) (atr-cols beta (+ j 1) n))))
; determinant by Gaussian elimination over Q (matrix given as list of COLUMNS; transpose-agnostic for det up to nothing)
(define (atr-det cols n)
  (if (= n 0) 1 (atr-det-rows (atr-transpose cols n) n)))
(define (atr-transpose cols n) (atr-trows cols 0 n))
(define (atr-trows cols i n) (if (= i n) (quote ()) (cons (atr-row cols i) (atr-trows cols (+ i 1) n))))
(define (atr-row cols i) (if (null? cols) (quote ()) (cons (atr-nth (car cols) i) (atr-row (cdr cols) i))))
; det of a list of ROWS (rationals) by fraction-free-ish Gaussian elimination with pivoting
(define (atr-det-rows rows n) (atr-gauss rows 1))
(define (atr-gauss rows sign)
  (if (null? rows) sign
      (let ((piv (atr-find-pivot rows)))
        (if (equal? piv (quote none)) 0
            (let ((prow (atr-nth-row rows piv)))
              (let ((pval (car prow)))
                (let ((rest (atr-eliminate (atr-without rows piv) prow)))
                  (* (if (= (remainder piv 2) 0) 1 -1) (* pval (atr-gauss (atr-drop-first-col rest) sign))))))))))
(define (atr-find-pivot rows) (atr-find-go rows 0))
(define (atr-find-go rows i) (if (null? rows) (quote none) (if (= (car (car rows)) 0) (atr-find-go (cdr rows) (+ i 1)) i)))
(define (atr-nth-row rows i) (if (= i 0) (car rows) (atr-nth-row (cdr rows) (- i 1))))
(define (atr-without rows i) (if (= i 0) (cdr rows) (cons (car rows) (atr-without (cdr rows) (- i 1)))))
(define (atr-eliminate rows prow) (if (null? rows) (quote ()) (cons (atr-elim-row (car rows) prow) (atr-eliminate (cdr rows) prow))))
(define (atr-elim-row row prow)
  (let ((f (/ (car row) (car prow))))
    (atr-vsub (cdr row) (atr-vscale f (cdr prow)))))
(define (atr-vsub a b) (cond ((null? a) (atr-vneg b)) ((null? b) a) (else (cons (- (car a) (car b)) (atr-vsub (cdr a) (cdr b))))))
(define (atr-vneg b) (if (null? b) (quote ()) (cons (- 0 (car b)) (atr-vneg (cdr b)))))
(define (atr-vscale f v) (if (null? v) (quote ()) (cons (* f (car v)) (atr-vscale f (cdr v)))))
(define (atr-drop-first-col rows) rows)                    ; elimination already dropped the pivot column
