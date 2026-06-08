(import "cas/wz.lisp")
; discovered certificate for SUM C(n,k) = 2^n
(display (wz-certificate->string
  (wz-search (list (list (/ 1 2) (/ 1 2))) (list (list 1 1) (list -1))
             (list (list 0 1) (list -1)) (list (list 1) (list 1))))) (newline)
; discovered certificate for SUM k C(n,k) = n 2^(n-1)
(display (wz-certificate->string
  (wz-search (list (list 0 (/ 1 2))) (list (list 1 1) (list -1))
             (list (list 0 1) (list -1)) (list (list) (list 1))))) (newline)
; bivariate identity (k+1)^2
(display (bp-mul (list (list 1) (list 1)) (list (list 1) (list 1)))) (newline)
