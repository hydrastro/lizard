; -*- lisp -*-
; ============================================================
;  EXAMPLE 104 — Combinatorics
; ============================================================

(import "match.lisp")
(import "combinatorics.lisp")

(display "=== COUNTS ===") (newline)
(display "  5! = ") (display (comb-factorial 5)) (newline)
(display "  C(5,2) = ") (display (choose 5 2)) (newline)
(display "  C(10,3) = ") (display (choose 10 3)) (newline)
(display "  P(5,2) = ") (display (perm-count 5 2)) (newline)
(newline)

(display "=== PERMUTATIONS ===") (newline)
(display "  permutations of (a b c):") (newline)
(display "    ") (display (permutations '(a b c))) (newline)
(display "  count of perms of (1 2 3 4): ")
(display (length (permutations '(1 2 3 4)))) (display " (= 4! = 24)") (newline)
(newline)

(display "=== COMBINATIONS ===") (newline)
(display "  2-subsets of (a b c d):") (newline)
(display "    ") (display (combinations '(a b c d) 2)) (newline)
(display "  3-subsets of (1 2 3 4 5): ")
(display (length (combinations '(1 2 3 4 5) 3))) (display " (= C(5,3) = 10)") (newline)
(newline)

(display "=== POWER SET ===") (newline)
(display "  all subsets of (x y z):") (newline)
(display "    ") (display (power-set '(x y z))) (newline)
(display "  count of subsets of (1 2 3 4): ")
(display (length (power-set '(1 2 3 4)))) (display " (= 2^4 = 16)") (newline)
(newline)

(display "=== CARTESIAN PRODUCT ===") (newline)
(display "  (1 2) × (a b): ")
(display (cartesian '(1 2) '(a b))) (newline)
(newline)

(display "=== INTEGER PARTITIONS ===") (newline)
(display "  partitions of 4:") (newline)
(display "    ") (display (partitions 4)) (newline)
(display "  partitions of 5:") (newline)
(display "    ") (display (partitions 5)) (newline)
(display "  count of partitions of 6: ")
(display (length (partitions 6))) (display " (should be 11)") (newline)
(newline)

(display "=== DERANGEMENTS ===") (newline)
(display "  derangements of 4 items: ") (display (derangements 4)) (newline)
(display "  derangements of 5 items: ") (display (derangements 5)) (newline)
(newline)

(display "=== End of combinatorics ===") (newline)
