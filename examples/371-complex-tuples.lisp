; COMPLEX SOLUTION TUPLES over the Gaussian rationals Q(i): assemble complete complex solution POINTS of a
; triangular polynomial system, exactly, when the imaginary parts are rational (docs/CAS.md -- summit S3, complex
; coordinates and full varieties, beyond real algebraic tuples and the complex-root naming).
;
; A complex root (complex re im2) with im2 a perfect square is a Gaussian rational a + b i (a, b in Q), a genuine
; element of Q(i).  This carries such numbers as (gr a b), does exact Q(i) arithmetic, turns a perfect-square
; complex root into its two Gaussian roots, assembles a triangular system into complete complex points, and
; certifies a point by evaluating every generator to zero in Q(i).  A non-perfect-square imaginary part is
; reported 'not-gaussian rather than forced.
(import "cas/cplxtuples.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Complex solution tuples over Q(i): complete complex points, certified by exact Gaussian arithmetic.") (newline) (newline)

(display "Q(i) arithmetic is exact:") (newline)
(must "i * i = -1" (gr-equal? (gr-mul (gr-make 0 1) (gr-make 0 1)) (gr-make -1 0)))
(must "(1 + i)(1 - i) = 2" (gr-equal? (gr-mul (gr-make 1 1) (gr-make 1 -1)) (gr-make 2 0)))

(display "the system x^2 + 1 = 0, y = x has the complex point (i, i):") (newline)
(define g_x2 (list (cons 1 (list 2 0)) (cons 1 (list 0 0))))   ; x^2 + 1
(define g_yx (list (cons 1 (list 0 1)) (cons -1 (list 1 0))))  ; y - x
(define tup (cxt-tuple (list (quote complex) 0 1) (list (list 0 1))))
(display "  ") (display tup) (newline)
(must "the point is (i, i)" (equal? tup (list (gr-make 0 1) (gr-make 0 1))))
(must "x^2 + 1 vanishes at (i, i) in Q(i)" (gr-zero? (cxt-eval-gen g_x2 tup)))
(must "y - x vanishes at (i, i)" (gr-zero? (cxt-eval-gen g_yx tup)))
(must "the full certificate passes" (cxt-certify (list g_x2 g_yx) tup))

(display "the system x^2 + 1 = 0, y = x^2 has the point (i, -1), since i^2 = -1:") (newline)
(define g_yx2 (list (cons 1 (list 0 1)) (cons -1 (list 2 0))))  ; y - x^2
(define tup2 (cxt-tuple (list (quote complex) 0 1) (list (list 0 0 1))))
(must "the second coordinate is -1" (gr-equal? (car (cdr tup2)) (gr-make -1 0)))
(must "the point (i, -1) certifies" (cxt-certify (list g_x2 g_yx2) tup2))

(display "a Gaussian point with nonzero real part: x^2 - 2x + 2 = 0 has roots 1 +- i:") (newline)
(must "the roots are 1 + i and 1 - i" (equal? (cxt-roots-of (list (quote complex) 1 1)) (list (gr-make 1 1) (gr-make 1 -1))))
(must "1 + i satisfies x^2 - 2x + 2" (gr-zero? (cxt-eval-poly (list 2 -2 1) (gr-make 1 1))))

(display "soundness boundary: x^2 + x + 1 has imaginary part sqrt(3/4), not rational -- reported, not forced:") (newline)
(must "the root is not a Gaussian rational" (equal? (cxt-roots-of (list (quote complex) (/ -1 2) (/ 3 4))) (quote not-gaussian)))
(must "a wrong point (i, 1) fails the y = x certificate" (if (cxt-certify (list g_x2 g_yx) (list (gr-make 0 1) (gr-make 1 0))) #f #t))

(newline)
(display "Complex coordinates are now assembled into complete solution points over Q(i) and certified by exact") (newline)
(display "Gaussian arithmetic, for the case where the imaginary parts are rational.  Coordinates in Q(i, sqrt(d))") (newline)
(display "for non-square d, and full positive-dimensional varieties, remain ahead.") (newline)
