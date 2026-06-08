; 188-stirling.lisp -- Stirling numbers (both kinds) and Bell numbers.
;
; S(n,k) counts partitions of n elements into k nonempty blocks; the unsigned c(n,k)
; counts permutations of n with k cycles; B(n) is the total number of partitions.  All
; are built row by row.  The numbers are certified by exact polynomial identities:
;   x^n = sum_k S(n,k) x^{falling k};  x(x+1)...(x+n-1) = sum_k c(n,k) x^k;
;   sum_k c(n,k) = n!;  and B(n) summed from the S-row equals B(n) from the Bell
; recurrence.  `must` raises on failure.

(import "cas/stirling.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'stirling-check-failed)))

(display "Stirling and Bell numbers") (newline) (newline)

(display "1. known values") (newline)
(display "    S(5,.) = ") (display (stirling2-row 5)) (newline)
(display "    c(5,.) = ") (display (stirling1-row 5)) (newline)
(display "    Bell B_0..B_8 = ") (display (bell-list-str 8)) (newline)
(must "S(5,.) = (0 1 15 25 10 1)"   (equal? (stirling2-row 5) (list 0 1 15 25 10 1)))
(must "S(4,2) = 7"                  (= (stirling2 4 2) 7))
(must "c(5,.) = (0 24 50 35 10 1)"  (equal? (stirling1-row 5) (list 0 24 50 35 10 1)))
(must "c(4,2) = 11"                 (= (stirling1 4 2) 11))
(must "Bell = (1 1 2 5 15 52 203 877 4140)" (equal? (bell-list-str 8) (list 1 1 2 5 15 52 203 877 4140)))
(newline)

(display "2. second kind: monomials in the falling-factorial basis") (newline)
(must "x^3 identity"  (stirling2-ok? 3))
(must "x^5 identity"  (stirling2-ok? 5))
(must "x^7 identity"  (stirling2-ok? 7))
(newline)

(display "3. first kind: coefficients of the rising factorial; rows sum to n!") (newline)
(must "rising_4 = sum c(4,k) x^k"  (stirling1-ok? 4))
(must "rising_6 = sum c(6,k) x^k"  (stirling1-ok? 6))
(must "sum_k c(5,k) = 5! = 120"    (stirling1-sum-ok? 5))
(must "sum_k c(7,k) = 7! = 5040"   (stirling1-sum-ok? 7))
(newline)

(display "4. Bell numbers computed two independent ways agree") (newline)
(must "B_5 two ways"  (bell-ok? 5))
(must "B_8 two ways"  (bell-ok? 8))
(must "B_10 two ways" (bell-ok? 10))
(newline)

(display "all Stirling/Bell checks passed.") (newline)
