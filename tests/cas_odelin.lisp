(import "cas/odelin.lisp")
(display (odelin-fully-solvable? (list 2 -3 1))) (display " ") (display (odelin-fully-solvable? (list 1 -2 1))) (display " ") (display (odelin-fully-solvable? (list 2 -1 -2 1))) (newline)
(display (odelin-certify (list 1 -2 1) 2 1)) (display " ") (display (odelin-fully-solvable? (list 1 0 1))) (newline)
