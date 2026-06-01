; 181-cyclotomic.lisp -- cyclotomic polynomials over Q.
;
; Phi_n is defined by prod_{d | n} Phi_d(x) = x^n - 1, giving the exact recurrence
; Phi_n = (x^n - 1) / prod_{d | n, d < n} Phi_d, computed by exact polynomial division
; with each Phi_d built only once via a memoized table over the divisors.  Two checks
; gate every result: the product over all divisors rebuilds x^n - 1 exactly, and the
; degree equals the Euler totient phi(n) counted independently.  Phi_105 is the smallest
; cyclotomic polynomial with a coefficient outside {-1, 0, 1}.  `must` raises on failure.

(import "cas/cyclotomic.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'cyclotomic-check-failed)))

(display "Cyclotomic polynomials over Q") (newline) (newline)

(display "1. small cases") (newline)
(display "    Phi_1  = ") (display (cyclotomic->string 1))  (newline)
(display "    Phi_4  = ") (display (cyclotomic->string 4))  (newline)
(display "    Phi_6  = ") (display (cyclotomic->string 6))  (newline)
(display "    Phi_12 = ") (display (cyclotomic->string 12)) (newline)
(must "Phi_1 = x - 1"          (equal? (cyclotomic 1) (list -1 1)))
(must "Phi_4 = x^2 + 1"        (equal? (cyclotomic 4) (list 1 0 1)))
(must "Phi_6 = x^2 - x + 1"    (equal? (cyclotomic 6) (list 1 -1 1)))
(must "Phi_12 = x^4 - x^2 + 1" (equal? (cyclotomic 12) (list 1 0 -1 0 1)))
(newline)

(display "2. degree equals the totient, product identity holds") (newline)
(must "deg Phi_5 = phi(5) = 4"     (= (poly-deg (cyclotomic 5)) 4))
(must "deg Phi_15 = phi(15) = 8"   (= (poly-deg (cyclotomic 15)) 8))
(must "Phi_7 certified"            (cyclotomic-ok? 7))
(must "Phi_15 certified"           (cyclotomic-ok? 15))
(must "Phi_36 certified"           (cyclotomic-ok? 36))
(newline)

(display "3. Phi_105: the famous coefficient outside {-1,0,1}") (newline)
(define t105 (cyclo-table 105))
(display "    degree ") (display (poly-deg (lookup 105 t105))) (display ", min coefficient ") (display (min-coeff-tab 105 t105)) (newline)
(must "deg Phi_105 = phi(105) = 48"  (= (poly-deg (lookup 105 t105)) 48))
(must "Phi_105 has a -2 coefficient" (= (min-coeff-tab 105 t105) -2))
(must "Phi_105 certified"            (cyclotomic-ok-tab? 105 t105))
(newline)

(display "all cyclotomic checks passed.") (newline)
