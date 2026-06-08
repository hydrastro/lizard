; 161-linear-algebra.lisp — exact symbolic linear algebra over Q.
;
; Matrices are lists of rational rows.  This module ties the CAS together:
; determinants reuse the resultant machinery, linear solving reuses the exact
; Gauss-Jordan solver from Gosper, the characteristic polynomial comes from
; Faddeev-LeVerrier (only Q arithmetic), and EIGENVALUES are the exact roots of
; that polynomial via the equation solver -- rationals, surds, or i -- so the
; Fibonacci matrix yields the golden ratio and a rotation yields +/- i, exactly.
;
; Certificates: Cayley-Hamilton p(A)=0, A A^{-1}=I, det cross-checked against the
; Sylvester-style determinant, eigenvalues verified by back-substitution, and
; rational eigenvalues additionally by det(A - lambda I)=0.  `must` raises on fail.

(import "cas/linalg.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'linalg-check-failed)))

(define D (list (list 2 0 0) (list 0 3 0) (list 0 0 5)))
(define M (list (list 1 2) (list 3 4)))
(define F (list (list 0 1) (list 1 1)))       ; companion of x^2 - x - 1
(define R (list (list 0 -1) (list 1 0)))      ; rotation by 90 degrees
(define K (list (list 2 1 0) (list 1 2 1) (list 0 1 2)))  ; tridiagonal

(display "exact symbolic linear algebra") (newline) (newline)

(display "1. characteristic polynomial") (newline)
(must "charpoly diag(2,3,5) = (x-2)(x-3)(x-5)" (equal? (poly-norm (mat-charpoly D)) (list -30 31 -10 1)))
(must "charpoly [[1,2],[3,4]] = x^2-5x-2"      (equal? (poly-norm (mat-charpoly M)) (list -2 -5 1)))
(must "charpoly Fibonacci = x^2-x-1"           (equal? (poly-norm (mat-charpoly F)) (list -1 -1 1)))
(must "charpoly rotation = x^2+1"              (equal? (poly-norm (mat-charpoly R)) (list 1 0 1)))
(newline)

(display "2. determinant cross-checked (Sylvester det vs charpoly)") (newline)
(must "det diag(2,3,5) = 30" (and (= (mat-det D) 30) (det-consistent? D)))
(must "det [[1,2],[3,4]] = -2" (and (= (mat-det M) -2) (det-consistent? M)))
(must "det tridiagonal = 4"    (and (= (mat-det K) 4) (det-consistent? K)))
(newline)

(display "3. Cayley-Hamilton: p(A) = 0") (newline)
(must "diag(2,3,5)"  (cayley-hamilton? D))
(must "[[1,2],[3,4]]" (cayley-hamilton? M))
(must "Fibonacci"    (cayley-hamilton? F))
(must "rotation"     (cayley-hamilton? R))
(must "tridiagonal"  (cayley-hamilton? K))
(newline)

(display "4. inverse with A A^{-1} = I") (newline)
(must "[[1,2],[3,4]]^{-1} = [[-2,1],[3/2,-1/2]]"
      (equal? (mat-inverse M) (list (list -2 1) (list (/ 3 2) (/ -1 2)))))
(must "A A^{-1} = I  ([[1,2],[3,4]])" (inverse-ok? M (mat-inverse M)))
(must "A A^{-1} = I  (tridiagonal)"   (inverse-ok? K (mat-inverse K)))
(newline)

(display "5. eigenvalues, exact, verified by back-substitution") (newline)
(must "diag(2,3,5) eigenvalues verified" (solutions-verify (mat-charpoly D) (mat-eigenvalues D)))
(must "rational eigenvalue 2 of diag: det(A-2I)=0"     (rational-eig-ok? D 2))
(must "non-eigenvalue 4 of diag: det(A-4I) nonzero"    (not (rational-eig-ok? D 4)))
(must "[[1,2],[3,4]] eigenvalues (5+/-sqrt33)/2 verified" (solutions-verify (mat-charpoly M) (mat-eigenvalues M)))
(must "Fibonacci eigenvalues (golden ratio) verified"     (solutions-verify (mat-charpoly F) (mat-eigenvalues F)))
(must "rotation eigenvalues +/- i verified"               (solutions-verify (mat-charpoly R) (mat-eigenvalues R)))
(newline)

(display "6. linear system A x = b with A x = b checked") (newline)
(must "[[2,1],[1,3]] x = [3,5] -> (4/5, 7/5)"
      (let ((x (mat-solve (list (list 2 1) (list 1 3)) (list 3 5))))
        (and (equal? x (list (/ 4 5) (/ 7 5))) (solve-ok? (list (list 2 1) (list 1 3)) (list 3 5) x))))
(newline)

(display "all linear-algebra checks passed.") (newline)
