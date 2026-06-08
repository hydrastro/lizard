(import "cas/algfuncint.lisp")
(define R1 (list 1 0 1)) (define R2 (list 5 2 1))
(display (afi-build (list 1 0 1) R1)) (newline)
(display (int-poly-sqrt-certify (list 0 0 0 1) R1)) (display " ") (display (int-poly-sqrt-certify (list 1 0 -2 0 3) R1)) (display " ") (display (int-poly-sqrt-certify (list 0 0 0 0 0 1) R2)) (newline)
