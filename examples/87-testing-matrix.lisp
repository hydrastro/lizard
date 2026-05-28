; -*- lisp -*-
; ============================================================
;  EXAMPLE 87 — Testing framework + matrix math
; ============================================================

(import "match.lisp")
(import "test.lisp")
(import "matrix.lisp")

; ═══════════════════════════════════════
; Use the test framework to verify matrix ops
; ═══════════════════════════════════════
(test-begin "matrix operations")

(define A '((1 2) (3 4)))
(define B '((5 6) (7 8)))
(define I (matrix-identity 2))

(check-equal? (matrix-rows A) 2 "A has 2 rows")
(check-equal? (matrix-cols A) 2 "A has 2 cols")
(check-equal? (matrix-ref A 0 1) 2 "A[0][1] = 2")

(check-equal? (matrix-add A B) '((6 8) (10 12)) "matrix addition")
(check-equal? (matrix-sub B A) '((4 4) (4 4)) "matrix subtraction")
(check-equal? (matrix-scale 2 A) '((2 4) (6 8)) "scalar multiply")
(check-equal? (matrix-transpose A) '((1 3) (2 4)) "transpose")
(check-equal? (matrix-identity 2) '((1 0) (0 1)) "identity 2x2")
(check-equal? (matrix-mul A I) A "A * I = A")
(check-equal? (matrix-mul A B) '((19 22) (43 50)) "matrix multiply")
(check-equal? (matrix-trace A) 5 "trace = 1+4")

(check-equal? (dot-product '(1 2 3) '(4 5 6)) 32 "dot product")
(check-equal? (vec-add '(1 2 3) '(4 5 6)) '(5 7 9) "vector add")
(check-equal? (vec-norm-sq '(3 4)) 25 "norm squared = 9+16")

(test-end)
(newline)

; ═══════════════════════════════════════
; Show a matrix computation
; ═══════════════════════════════════════
(display "=== 3×3 matrix multiplication ===") (newline)
(define M1 '((1 2 3) (4 5 6) (7 8 9)))
(define M2 '((9 8 7) (6 5 4) (3 2 1)))
(display "M1:") (newline)
(matrix-print M1)
(display "M2:") (newline)
(matrix-print M2)
(display "M1 × M2:") (newline)
(matrix-print (matrix-mul M1 M2))
(newline)

(display "=== Rotation matrix (90° integer approx) ===") (newline)
(define rot '((0 -1) (1 0)))
(display "Rotating vector (1, 0) by 90°: ")
(display (matrix-mul rot '((1) (0)))) (newline)
