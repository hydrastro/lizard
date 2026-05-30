; -*- lisp -*-
; ============================================================
;  EXAMPLE 86 — Classic algorithms
; ============================================================

(import "match.lisp")       ; for take, drop, range
(import "algorithms.lisp")

(display "=== SORTING ===") (newline)
(display "  quicksort '(5 2 8 1 9 3 7 4 6):") (newline)
(display "    ") (display (quicksort '(5 2 8 1 9 3 7 4 6))) (newline)
(display "  mergesort '(5 2 8 1 9 3 7 4 6):") (newline)
(display "    ") (display (mergesort '(5 2 8 1 9 3 7 4 6))) (newline)
(newline)

(display "=== SEARCHING ===") (newline)
(display "  linear-search '(a b c d e) 'd: ")
(display (linear-search '(a b c d e) 'd)) (newline)
(define sorted-vec (pvec 1 3 5 7 9 11 13))
(display "  binary-search [1,3,5,7,9,11,13] 9: ")
(display (binary-search sorted-vec 9)) (newline)
(display "  binary-search [1,3,5,7,9,11,13] 8: ")
(display (binary-search sorted-vec 8)) (newline)
(newline)

(display "=== NUMBER THEORY ===") (newline)
(display "  factorial 10: ") (display (factorial 10)) (newline)
(display "  factorial 20: ") (display (factorial 20)) (newline)
(display "  fib 30: ") (display (fib 30)) (newline)
(display "  fib 50: ") (display (fib 50)) (newline)
(display "  gcd 48 36: ") (display (my-gcd 48 36)) (newline)
(display "  prime? 17: ") (display (prime? 17)) (newline)
(display "  prime? 18: ") (display (prime? 18)) (newline)
(display "  primes up to 30: ") (newline)
(display "    ") (display (primes-up-to 30)) (newline)
(display "  collatz-length 27: ") (display (collatz-length 27)) (newline)
(newline)

(display "=== STRING ALGORITHMS ===") (newline)
(display "  palindrome? \"racecar\": ") (display (palindrome? "racecar")) (newline)
(display "  palindrome? \"hello\": ") (display (palindrome? "hello")) (newline)
(display "  char-count \"mississippi\" \"s\": ")
(display (char-count "mississippi" "s")) (newline)
(newline)

(display "=== BIG COMPUTATION ===") (newline)
(display "  factorial 50: ") (newline)
(display "    ") (display (factorial 50)) (newline)
(display "  2^100 via fib-style: ") (display (fib 100)) (newline)
(newline)

(display "=== End of algorithms ===") (newline)
