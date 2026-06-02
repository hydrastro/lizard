; Validating the n>=2 Sylvester-resultant determinant path over K1.  In the Rothstein-Trager logarithmic
; part the resultant Res_theta2(D*, A* - z D2(D*)) is, when D* has degree >= 3, a 5x5 or larger
; determinant over the field K1 = Q(x)(theta1).  That determinant is now computed by Gaussian elimination
; over K1 (the cofactor expansion that preceded it exhausted memory even at size 5x5).  With constant K1
; coefficients the height-two resultant must equal the trusted Sylvester resultant over Q computed by
; resultant.lisp, and that agreement -- on two degree-(3,2) pairs, each a 5x5 determinant -- is the
; certificate here, validating the determinant-path arithmetic against trusted machinery.
(import "cas/tower2rt.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define (kc n) (k1-from-int n))
(define f  (list (kc 2) (kc -3) (kc 0) (kc 1)))     ; theta2^3 - 3 theta2 + 2
(define g  (list (kc 1) (kc 1) (kc 2)))             ; 2 theta2^2 + theta2 + 1
(display "n>=2 Sylvester resultant over K1 via Gaussian elimination, validated against resultant over Q") (newline)
(must "deg f = 3 and deg g = 2 (a 5x5 Sylvester determinant)" (if (= (h2-deg f) 3) (= (h2-deg g) 2) #f))
(must "Gaussian determinant over K1 equals the trusted resultant over Q"
      (= (k1-to-rational (h2-resultant f g)) (resultant (list 2 -3 0 1) (list 1 1 2))))
(define f2 (list (kc 1) (kc 0) (kc -2) (kc 1)))     ; theta2^3 - 2 theta2 + 1
(define g2 (list (kc -1) (kc 3) (kc 1)))            ; theta2^2 + 3 theta2 - 1
(must "a second degree-(3,2) pair also matches the trusted resultant"
      (= (k1-to-rational (h2-resultant f2 g2)) (resultant (list 1 0 -2 1) (list -1 3 1))))
(newline) (display "n>=2 determinant-path resultant validated against trusted machinery.") (newline)
