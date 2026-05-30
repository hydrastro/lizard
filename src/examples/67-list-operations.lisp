; -*- lisp -*-
; ============================================================
;  EXAMPLE 67 — List operations
; ============================================================

(display "=== length ===") (newline)
(display "  (length '()): ") (display (length '())) (newline)
(display "  (length '(a b c)): ") (display (length '(a b c))) (newline)
(display "  (length (list 1 2 3 4 5)): ") (display (length (list 1 2 3 4 5))) (newline)
(newline)

(display "=== append ===") (newline)
(display "  (append '(1 2) '(3 4)): ") (display (append '(1 2) '(3 4))) (newline)
(display "  (append '() '(x y)): ") (display (append '() '(x y))) (newline)
(newline)

(display "=== reverse ===") (newline)
(display "  (reverse '(1 2 3)): ") (display (reverse '(1 2 3))) (newline)
(display "  (reverse '()): ") (display (reverse '())) (newline)
(newline)

(display "=== list? ===") (newline)
(display "  (list? '(1 2 3)): ") (display (list? '(1 2 3))) (newline)
(display "  (list? '()): ") (display (list? '())) (newline)
(display "  (list? 42): ") (display (list? 42)) (newline)
(display "  (list? (cons 1 2)): ") (display (list? (cons 1 2))) (newline)
(newline)

(display "=== map ===") (newline)
(define (square x) (* x x))
(display "  (map square '(1 2 3 4 5)): ")
(display (map square '(1 2 3 4 5))) (newline)
(newline)

(display "=== filter ===") (newline)
(define (even? n) (= 0 (% n 2)))
(display "  (filter even? '(1 2 3 4 5 6)): ")
(display (filter even? '(1 2 3 4 5 6))) (newline)
(newline)

(display "=== apply ===") (newline)
(display "  (apply + '(1 2)): ") (display (apply + '(1 2))) (newline)
(display "  (apply list '(a b c)): ") (display (apply list '(a b c))) (newline)
(newline)

(display "=== for-each ===") (newline)
(display "  ") (for-each (lambda (x) (display x) (display " ")) '(hello world foo))
(newline)
(newline)

(display "=== Combined ===") (newline)
(display "  Sum of squares of even numbers 1..10:") (newline)
(define nums (list 1 2 3 4 5 6 7 8 9 10))
(display "  ")
(display (apply + (map square (filter even? nums))))
(newline)
(display "  ↑ should be 220 (4+16+36+64+100)") (newline)
(newline)

(display "=== End of list operations ===") (newline)
