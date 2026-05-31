; -*- lisp -*-
; examples/144-exact-rationals.lisp
;
; Exact rational arithmetic — lizard's numeric tower over the rationals (ℚ).
; Integers stay integers; division and any operation that produces a fraction
; yields an exact, fully-reduced rational (backed by GMP mpq).  Rationals that
; reduce to a whole number automatically demote back to an integer, so
; "integers are always integers" stays true.  Rational literals are written n/m.
;
; Self-checking: each `must` raises if its claim is false.

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'rational-regression)))

(newline)
(display "Exact division (no truncation):") (newline)
(must "7/2 is exact"        (equal? (/ 7 2) 7/2))
(must "1/3 is exact"        (equal? (/ 1 3) 1/3))
(must "10/2/5 = 1"          (equal? (/ 10 2 5) 1))
(must "literal 3/4 reads"   (equal? (/ 3 4) 3/4))

(display "Auto-reduction to lowest terms:") (newline)
(must "6/4 reduces to 3/2"      (equal? (/ 6 4) 3/2))
(must "2/3 * 3/4 = 1/2"         (equal? (* 2/3 3/4) 1/2))
(must "numerator of 6/8 is 3"   (equal? (numerator 6/8) 3))
(must "denominator of 6/8 is 4" (equal? (denominator 6/8) 4))

(display "Auto-demotion to integers:") (newline)
(must "6/3 demotes to 2"          (equal? (/ 6 3) 2))
(must "and 2 is an integer"       (integer? (/ 6 3)))
(must "1/2 + 1/2 = 1 (integer)"   (integer? (+ 1/2 1/2)))
(must "literal 4/2 reads as 2"    (equal? 4/2 2))
(must "4/2 is an integer"         (integer? 4/2))

(display "Mixed integer / rational arithmetic:") (newline)
(must "1 + 1/2 = 3/2"   (equal? (+ 1 1/2) 3/2))
(must "5 - 1/3 = 14/3"  (equal? (- 5 1/3) 14/3))
(must "3 * 1/6 = 1/2"   (equal? (* 3 1/6) 1/2))
(must "(1/2)^3 = 1/8"   (equal? (expt 1/2 3) 1/8))
(must "(2/3)^0 = 1"     (equal? (expt 2/3 0) 1))

(display "Comparisons across integers and rationals:") (newline)
(must "1/3 < 1/2"        (< 1/3 1/2))
(must "2/3 > 1/2"        (> 2/3 1/2))
(must "1/2 = 2/4"        (= 1/2 2/4))
(must "1/2 <= 1/2"       (<= 1/2 1/2))
(must "ordered chain"    (< 1/4 1/3 1/2 2/3 1))

(display "Predicates:") (newline)
(must "rational? 1/2"        (rational? 1/2))
(must "rational? 3 (ints too)" (rational? 3))
(must "integer? 1/2 is false"  (equal? (integer? 1/2) #f))
(must "exact? 1/2"             (exact? 1/2))
(must "number? 1/2"           (number? 1/2))
(must "negative? -1/2"        (negative? -1/2))
(must "positive? 1/2"        (positive? 1/2))
(must "zero? (1/2 - 1/2)"    (zero? (- 1/2 1/2)))

(display "Rationals as keys and set members (reduction merges duplicates):") (newline)
(define m (phash-map 1/2 'half 1/3 'third 2/4 'still-half))
(must "2/4 keys the same slot as 1/2" (equal? (phash-get m 1/2) 'still-half))
(must "so the map has 2 keys"         (equal? (phash-count m) 2))
(define s (pset 1/2 1/3 2/4 3/6))
(must "set dedups equal rationals"    (equal? (pset-count s) 2))
(must "membership by value"           (pset-contains? s 1/2))

(display "Exactness at scale (no floating-point error):") (newline)
(must "tiny * tiny is exact"  (equal? (* 1/1000000 1/1000000) 1/1000000000000))
(must "equal? sees reductions" (equal? 3/4 6/8))
; exact harmonic sum H_4 = 1 + 1/2 + 1/3 + 1/4 = 25/12
(define (harmonic n) (if (= n 0) 0 (+ (/ 1 n) (harmonic (- n 1)))))
(must "harmonic(4) = 25/12"   (equal? (harmonic 4) 25/12))
(must "harmonic(10) is exact" (equal? (harmonic 10) 7381/2520))

(newline)
(display "exact rational numeric tower: verified") (newline)
