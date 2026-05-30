; -*- lisp -*-
; ============================================================
;  EXAMPLE 70 — Standard library: sort, zip, partition, etc.
; ============================================================

(import "match.lisp")

(display "=== sort ===") (newline)
(display "  (sort '(5 3 1 4 2) <): ")
(display (sort '(5 3 1 4 2) <)) (newline)
(display "  (sort '(5 3 1 4 2) >): ")
(display (sort '(5 3 1 4 2) >)) (newline)
(newline)

(display "=== range ===") (newline)
(display "  (range 0 10): ") (display (range 0 10)) (newline)
(display "  (iota 5 1): ") (display (iota 5 1)) (newline)
(newline)

(display "=== zip ===") (newline)
(display "  (zip '(a b c) '(1 2 3)): ")
(display (zip '(a b c) '(1 2 3))) (newline)
(newline)

(display "=== enumerate ===") (newline)
(display "  (enumerate '(x y z)): ")
(display (enumerate '(x y z))) (newline)
(newline)

(display "=== partition ===") (newline)
(define (even? n) (= 0 (% n 2)))
(display "  (partition even? '(1 2 3 4 5 6)): ")
(display (partition even? '(1 2 3 4 5 6))) (newline)
(newline)

(display "=== take / drop ===") (newline)
(display "  (take 3 '(a b c d e)): ") (display (take 3 '(a b c d e))) (newline)
(display "  (drop 3 '(a b c d e)): ") (display (drop 3 '(a b c d e))) (newline)
(newline)

(display "=== any / every ===") (newline)
(display "  (any even? '(1 3 5 7)): ") (display (any even? '(1 3 5 7))) (newline)
(display "  (any even? '(1 3 4 7)): ") (display (any even? '(1 3 4 7))) (newline)
(display "  (every even? '(2 4 6)): ") (display (every even? '(2 4 6))) (newline)
(display "  (every even? '(2 3 6)): ") (display (every even? '(2 3 6))) (newline)
(newline)

(display "=== flatten ===") (newline)
(display "  (flatten '(1 (2 3) (4 (5 6)))): ")
(display (flatten '(1 (2 3) (4 (5 6))))) (newline)
(newline)

(display "=== alist helpers ===") (newline)
(define db '((name . "Alice") (age . 30) (city . "Monza")))
(display "  (alist-ref 'name db): ") (display (alist-ref 'name db)) (newline)
(display "  (alist-ref 'missing db 'default): ") (display (alist-ref 'missing db 'default)) (newline)
(define db2 (alist-set 'age 31 db))
(display "  (alist-ref 'age db2): ") (display (alist-ref 'age db2)) (newline)
(newline)

(display "=== threading ===") (newline)
(display "  (-> 5 (lambda (x) (* x 2)) (lambda (x) (+ x 1))): ")
(display (-> 5 (lambda (x) (* x 2)) (lambda (x) (+ x 1)))) (newline)
(newline)

(display "=== compose ===") (newline)
(define double (lambda (x) (* x 2)))
(define inc (lambda (x) (+ x 1)))
(define double-then-inc (compose inc double))
(display "  ((compose inc double) 5): ") (display (double-then-inc 5)) (newline)
(newline)

(display "=== End of stdlib example ===") (newline)
