; 182-power-sums.lisp -- power sums of polynomial roots via Newton's identities.
;
; The power sums p_k = sum_i a_i^k of the roots a_i of a polynomial are fixed by the
; coefficients alone -- no root finding.  The elementary symmetric functions e_j are the
; signed coefficients, and Newton's identities turn them into p_1, p_2, ...  The result
; is checked by a round trip: reconstructing e_1..e_n from p_1..p_n via the inverse
; identity must return the original symmetric functions, and p_0 must equal the degree.
; The sums are exact even for irrational or complex roots.  `must` raises on failure.

(import "cas/powersums.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'powersums-check-failed)))

(display "Power sums of polynomial roots (Newton's identities)") (newline) (newline)

(display "1. a polynomial with known roots 1, 2, 3") (newline)
(display "    symmetric functions e = ") (display (list->string (e-list (list -6 11 -6 1)))) (newline)
(display "    power sums p_1..p_5  = ") (display (power-sums->string (list -6 11 -6 1) 5)) (newline)
(must "e = (6, 11, 6)"                  (equal? (e-list (list -6 11 -6 1)) (list 6 11 6)))
(must "p_2 = 1+4+9 = 14"                (= (power-sum (list -6 11 -6 1) 2) 14))
(must "p_3 = 1+8+27 = 36"               (= (power-sum (list -6 11 -6 1) 3) 36))
(must "p_1..p_5 = (6,14,36,98,276)"     (equal? (power-sums (list -6 11 -6 1) 5) (list 6 14 36 98 276)))
(must "round-trip certified"            (power-sums-ok? (list -6 11 -6 1)))
(newline)

(display "2. roots of x^2 - x - 1 give the Lucas numbers") (newline)
(display "    p_0..p_7 = 2, ") (display (power-sums->string (list -1 -1 1) 7)) (newline)
(must "p_0 = 2 (the degree)"            (= (power-sum (list -1 -1 1) 0) 2))
(must "Lucas L_1..L_7 = (1,3,4,7,11,18,29)" (equal? (power-sums (list -1 -1 1) 7) (list 1 3 4 7 11 18 29)))
(must "certified"                       (power-sums-ok? (list -1 -1 1)))
(newline)

(display "3. complex and irrational roots give exact real sums") (newline)
(display "    x^2+1 (roots +-i):    p_1..p_5 = ") (display (power-sums->string (list 1 0 1) 5)) (newline)
(display "    x^2-2 (roots +-sqrt2): p_1..p_6 = ") (display (power-sums->string (list -2 0 1) 6)) (newline)
(must "x^2+1 power sums = (0,-2,0,2,0)"   (equal? (power-sums (list 1 0 1) 5) (list 0 -2 0 2 0)))
(must "x^2-2 power sums = (0,4,0,8,0,16)" (equal? (power-sums (list -2 0 1) 6) (list 0 4 0 8 0 16)))
(must "x^2+1 certified"                   (power-sums-ok? (list 1 0 1)))
(newline)

(display "all power-sum checks passed.") (newline)
