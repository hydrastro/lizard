; The LINEAR-ALGEBRA reduction at the heart of the F4 algorithm: simultaneous reduction of a set of polynomials by
; Gaussian elimination on a Macaulay matrix, instead of one-at-a-time polynomial division (docs/CAS.md -- summit
; S4, an F4-class engine direction atop the Buchberger engines).
;
; The polynomials are laid out as rows of a matrix whose columns are the monomials that occur, sorted descending;
; the matrix is row-reduced to reduced row-echelon form over Q; the nonzero rows read back as polynomials are the
; reductions, and the pivot columns are their leading monomials.  The row space is preserved exactly -- certified
; by checking every original polynomial reduces to zero against the echelon rows.
(import "cas/groebnerf4.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "F4's linear-algebra core: reduce a polynomial set by Gaussian elimination on a Macaulay matrix.") (newline) (newline)

(display "reduced row-echelon form over Q is exact:") (newline)
(must "RREF of [[1,1],[1,-1]] is the identity" (equal? (f4-rref (list (list 1 1) (list 1 -1))) (list (list 1 0) (list 0 1))))

(define f1 (list (cons 1 (list 1 0)) (cons 1 (list 0 1))))   ; x + y
(define f2 (list (cons 1 (list 1 0)) (cons -1 (list 0 1))))  ; x - y

(display "the set {x + y, x - y} reduces to the rows x and y (its row space is <x, y>):") (newline)
(display "  monomials (sorted descending): ") (display (f4-monomials (list f1 f2))) (newline)
(display "  reduced set: ") (display (f4-reduce (list f1 f2))) (newline)
(must "it reduces to {x, y}" (equal? (f4-reduce (list f1 f2)) (list (list (cons 1 (list 1 0))) (list (cons 1 (list 0 1))))))
(must "the row space is preserved (every input reduces to zero against the result)" (f4-spans-same? (list f1 f2)))

(display "a redundant set {x + y, 2x + 2y} collapses to a single row:") (newline)
(define f3 (list (cons 2 (list 1 0)) (cons 2 (list 0 1))))   ; 2x + 2y
(must "only one independent row remains" (= (length (f4-reduce (list f1 f3))) 1))
(must "and it is x + y" (equal? (car (f4-reduce (list f1 f3))) (list (cons 1 (list 1 0)) (cons 1 (list 0 1)))))
(must "the span is preserved" (f4-spans-same? (list f1 f3)))

(display "the set {x^2 + y^2, x^2 - y^2} reduces to {x^2, y^2}:") (newline)
(define g1 (list (cons 1 (list 2 0)) (cons 1 (list 0 2))))
(define g2 (list (cons 1 (list 2 0)) (cons -1 (list 0 2))))
(must "it reduces to {x^2, y^2}" (equal? (f4-reduce (list g1 g2)) (list (list (cons 1 (list 2 0))) (list (cons 1 (list 0 2))))))
(must "the leading monomials are x^2 and y^2" (equal? (f4-leading-monomials (list g1 g2)) (list (list 2 0) (list 0 2))))
(must "the span is preserved" (f4-spans-same? (list g1 g2)))

(newline)
(display "The linear-algebra reduction that makes F4 fast -- batch Gaussian elimination on a Macaulay matrix --") (newline)
(display "is now exact over Q, with row-space preservation certified.  Generating the S-pairs and monomial multiples") (newline)
(display "that close the row space under the ideal (the symbolic-preprocessing loop) is the remaining work.") (newline)
