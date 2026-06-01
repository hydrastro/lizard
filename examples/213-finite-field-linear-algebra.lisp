; 213-finite-field-linear-algebra.lisp -- exact linear algebra over a prime field F_p.
;
; Over F_p every nonzero scalar is invertible, so Gaussian elimination yields exact
; determinants, ranks, inverses, solutions, and nullspace bases with no rounding.  This is
; certified structurally: A times its inverse is the identity, det(A B) = det(A) det(B),
; rank plus nullity equals the column count, the returned nullspace vectors lie in the
; kernel, and every solution satisfies its system.  `must` raises on failure.

(import "cas/linalgfp.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'linalgfp-check-failed)))
(define p 7)
(define B (list (list 2 0 1) (list 1 3 2) (list 0 1 1)))
(define S (list (list 1 2 3) (list 2 4 6) (list 1 1 1)))

(display "Linear algebra over F_7") (newline) (newline)

(display "1. determinant and inverse") (newline)
(display "    det [[1,2],[3,4]] = ") (display (fm-det (list (list 1 2) (list 3 4)) p)) (display "  (i.e. -2 mod 7)") (newline)
(display "    B = ((2 0 1)(1 3 2)(0 1 1)), det B = ") (display (fm-det B p)) (newline)
(display "    B^-1 = ") (display (fm-inverse B p)) (newline)
(must "det of [[1,2],[3,4]] is 5"  (= (fm-det (list (list 1 2) (list 3 4)) p) 5))
(must "B is invertible and B B^-1 = I" (fm-inverse-ok? B p))
(must "det(A B) = det(A) det(B)" (fm-detmul-ok? B (list (list 1 1 0) (list 0 2 1) (list 3 0 1)) p))
(newline)

(display "2. rank, nullspace, and rank-nullity") (newline)
(display "    S = ((1 2 3)(2 4 6)(1 1 1)) has rank ") (display (fm-rank S p)) (display ", nullspace ") (display (fm-nullspace S p)) (newline)
(must "rank of the singular matrix S is 2" (= (fm-rank S p) 2))
(must "rank + nullity = 3"                 (fm-ranknull-ok? S p))
(must "every nullspace vector is in the kernel" (fm-null-ok? S p))
(must "a full-rank 3x3 has empty nullspace" (= (length (fm-nullspace B p)) 0))
(newline)

(display "3. solving linear systems") (newline)
(display "    A x = (5 6 2) has solution x = ") (display (fm-solve (list (list 2 1 1) (list 1 3 2) (list 1 0 0)) (list 5 6 2) p)) (newline)
(must "the solution satisfies A x = b"  (fm-solve-ok? (list (list 2 1 1) (list 1 3 2) (list 1 0 0)) (list 5 6 2) p))
(must "another system solves correctly" (fm-solve-ok? (list (list 1 1 1) (list 0 1 4) (list 2 0 3)) (list 6 2 1) p))
(newline)

(display "all finite-field linear-algebra checks passed.") (newline)
