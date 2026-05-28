; -*- lisp -*-
; ============================================================
;  EXAMPLE 72 — Atoms, exceptions, and strings
; ============================================================

(display "=== ATOMS (mutable reference cells) ===") (newline)
(define counter (atom 0))
(display "  Initial: ") (display (deref counter)) (newline)
(swap! counter (lambda (n) (+ n 1)))
(swap! counter (lambda (n) (+ n 1)))
(swap! counter (lambda (n) (+ n 1)))
(display "  After 3 swaps: ") (display (deref counter)) (newline)
(reset! counter 100)
(display "  After reset to 100: ") (display (deref counter)) (newline)
(display "  (atom? counter): ") (display (atom? counter)) (newline)
(display "  (atom? 42): ") (display (atom? 42)) (newline)
(newline)

(display "=== EXCEPTIONS ===") (newline)
(display "  Normal: ")
(display (guard (lambda (e) "caught!") '(+ 1 2)))
(newline)
(display "  Exception: ")
(display (guard (lambda (e) (list 'caught e)) '(raise 'oops)))
(newline)
(display "  Nested: ")
(display (guard
  (lambda (e) (list 'outer e))
  '(guard
    (lambda (e) (list 'inner e))
    '(raise 'deep))))
(newline)
(newline)

(display "=== STRING OPERATIONS ===") (newline)
(display "  (string-ref \"hello\" 1): ") (display (string-ref "hello" 1)) (newline)
(display "  (string-contains? \"hello world\" \"world\"): ")
(display (string-contains? "hello world" "world")) (newline)
(display "  (string-upcase \"hello\"): ") (display (string-upcase "hello")) (newline)
(display "  (string-downcase \"HELLO\"): ") (display (string-downcase "HELLO")) (newline)
(display "  (string-split \"a,b,c,d\" \",\"): ")
(display (string-split "a,b,c,d" ",")) (newline)
(display "  (string-join '(\"hello\" \"world\") \" \"): ")
(display (string-join '("hello" "world") " ")) (newline)
(newline)

(display "=== ATOMS AS ACCUMULATORS ===") (newline)
(define total (atom 0))
(for-each (lambda (x) (swap! total (lambda (n) (+ n x))))
          '(1 2 3 4 5 6 7 8 9 10))
(display "  Sum 1..10 via atom: ") (display (deref total)) (newline)
(newline)

(display "=== End ===") (newline)
