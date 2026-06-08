; 219-lll-lattice-reduction.lisp -- LLL lattice basis reduction over the rationals.
;
; A lattice is all integer combinations of its basis vectors.  LLL rewrites a long, skewed
; basis into a short, nearly orthogonal one spanning the SAME lattice, by interleaving size
; reduction (|mu_ij| <= 1/2) with swaps whenever the Lovasz condition fails.  All Gram-Schmidt
; arithmetic is exact rational.  A correct output is characterized entirely by its certificates,
; which is what we check: the reduced basis is size-reduced, satisfies Lovasz, and spans the
; same lattice -- the change of basis U (reduced = U * original) is an integer matrix of
; determinant +-1.  `must` raises on failure.

(import "cas/lll.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'lll-check-failed)))

(display "LLL lattice basis reduction") (newline) (newline)

(display "1. a skewed 2D basis becomes short and orthogonal") (newline)
(define B (list (list 1 2) (list 3 4)))
(define R (lll B))
(display "    original = ") (display B) (display "   reduced = ") (display R) (newline)
(display "    change of basis U = ") (display (lll-transform R B)) (newline)
(must "reduced basis is size-reduced"           (lll-size-reduced? R))
(must "reduced basis satisfies the Lovasz condition" (lll-lovasz-ok? R))
(must "U is integer with |det U| = 1 (same lattice)"  (lll-same-lattice? R B))
(must "the lattice determinant is preserved"     (lll-det-preserved? R B))
(newline)

(display "2. a classic 3D lattice") (newline)
(define C (list (list 1 1 1) (list -1 0 2) (list 3 5 6)))
(define RC (lll C))
(display "    reduced = ") (display RC) (newline)
(must "size-reduced"            (lll-size-reduced? RC))
(must "Lovasz holds"            (lll-lovasz-ok? RC))
(must "same lattice"            (lll-same-lattice? RC C))
(must "first reduced vector is no longer than the first original" (<= (vnorm2 (m-row RC 0)) (vnorm2 (m-row C 0))))
(newline)

(display "3. a highly skewed basis is revealed to generate all of Z^3") (newline)
(define S (list (list 1 0 0) (list 10 1 0) (list 100 10 1)))
(define RS (lll S))
(display "    reduced = ") (display RS) (display "  (the standard basis)") (newline)
(must "reduces to the identity basis" (equal? RS (list (list 1 0 0) (list 0 1 0) (list 0 0 1))))
(must "same lattice as the skewed basis" (lll-same-lattice? RS S))
(newline)

(display "all LLL checks passed.") (newline)
