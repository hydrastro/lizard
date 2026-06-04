; ASSEMBLY OF COMPLETE ALGEBRAIC SOLUTION TUPLES for a triangular system whose leading coordinate is irrational:
; given the minimal polynomial of x_0 = alpha and the later coordinates as polynomials in x_0, build the full
; solution point (alpha, h_1(alpha), ...) with every coordinate an exact element of Q(alpha), certified by
; evaluating each generator to zero in Q(alpha) (docs/CAS.md -- summit S3, the tuple assembly polysolve3 left open).
;
; polysolve3 names each coordinate separately; this pairs them into a complete point.  For m(x_0) = 0 with
; x_j = h_j(x_0), each root alpha gives (alpha, h_1(alpha), ...), every later coordinate computed EXACTLY in the
; number field Q(alpha), and the point certified by exact algebraic arithmetic -- no floating point.
(import "cas/algtuples.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Complete algebraic solution tuples over Q(alpha): coordinates paired into points of the variety.") (newline) (newline)

(define minp (list -2 0 1))   ; x^2 - 2, so alpha = sqrt(2)
(define g_minp (list (cons 1 (list 2 0)) (cons -2 (list 0 0))))   ; x^2 - 2
(define g_yx   (list (cons 1 (list 0 1)) (cons -1 (list 1 0))))   ; y - x

(display "the system x^2 - 2 = 0, y = x: the solution point (sqrt(2), sqrt(2)) over Q(sqrt(2)):") (newline)
(define tup1 (at-tuple minp (list (list 0 1))))
(display "  ") (display tup1) (newline)
(chk "the tuple has two coordinates" (= (at-len tup1) 2))
(chk "x^2 - 2 vanishes at the point, evaluated in Q(sqrt 2)" (alg-zero? (at-eval-gen minp g_minp tup1)))
(chk "y - x vanishes at the point" (alg-zero? (at-eval-gen minp g_yx tup1)))
(chk "the full certificate passes: both generators vanish" (at-certify minp (list g_minp g_yx) tup1))

(display "the system x^2 - 2 = 0, y = x^2: the second coordinate is the rational 2, living inside Q(sqrt 2):") (newline)
(define tup2 (at-tuple minp (list (list 0 0 1))))
(define g_yx2 (list (cons 1 (list 0 1)) (cons -1 (list 2 0))))    ; y - x^2
(chk "the point (sqrt 2, 2) certifies" (at-certify minp (list g_minp g_yx2) tup2))

(display "a cubic field: x^3 - 2 = 0, y = x^2, the point (2^(1/3), 2^(2/3)) over Q(2^(1/3)):") (newline)
(define minp3 (list -2 0 0 1))
(define tup3 (at-tuple minp3 (list (list 0 0 1))))
(define g3  (list (cons 1 (list 3 0)) (cons -2 (list 0 0))))      ; x^3 - 2
(define g3y (list (cons 1 (list 0 1)) (cons -1 (list 2 0))))      ; y - x^2
(chk "the cube-root point certifies in Q(2^(1/3))" (at-certify minp3 (list g3 g3y) tup3))

(display "soundness: a wrong point (sqrt 2, sqrt 2 + 1) fails the y = x certificate:") (newline)
(define wrong (list (at-root minp) (alg-add (at-root minp) (alg-one minp))))
(chk "the wrong point is rejected" (if (at-certify minp (list g_minp g_yx) wrong) #f #t))

(newline)
(display "Named coordinates are now assembled into complete solution points over a number field Q(alpha), with") (newline)
(display "every later coordinate computed exactly by algebraic arithmetic and the whole point certified to vanish on") (newline)
(display "every generator.  Naming complex solutions and handling several independent algebraic coordinates remain.") (newline)
