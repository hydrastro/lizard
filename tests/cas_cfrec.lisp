(import "cas/cfrec.lisp")
(display (cfrec-gf (list 1 1) (list 0 1))) (newline)
(display (cfrec-terms (list 1 1) (list 0 1) 12)) (newline)
(display (cfrec-gf-verify (list 1 1) (list 0 1) 40)) (display " ") (display (cfrec-gf-verify (list 2 1) (list 0 1) 40)) (display " ") (display (cfrec-gf-verify (list 1 1 1) (list 0 0 1) 40)) (newline)
